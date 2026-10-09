#include "CCLWorldGenerationStore.h"

#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr uint32 MarkerMagic = 0x43434c47;
	constexpr int64 MaximumBundleBytes = 256ll * 1024 * 1024;
	constexpr int32 MaximumRegions = 64;

	bool ReadBounded(const FString& Path, int64 Limit, TArray<uint8>& Bytes)
	{
		const int64 Size = IFileManager::Get().FileSize(*Path);
		return Size > 0 && Size <= Limit && FFileHelper::LoadFileToArray(Bytes, *Path) && Bytes.Num() == Size;
	}

	bool WriteFlushed(const FString& Path, const TArray<uint8>& Bytes)
	{
		TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*Path, FILEWRITE_NoReplaceExisting));
		if (!Writer)
		{
			return false;
		}

		Writer->Serialize(const_cast<uint8*>(Bytes.GetData()), Bytes.Num());
		Writer->Flush();
		return !Writer->IsError() && Writer->Close();
	}

	uint32 CRC(const TArray<uint8>& Bytes)
	{
		return FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
	}
}

FCCLTerrainSaveContext FCCLWorldGenerationStore::ContextFor(const FCCLWorldSnapshot& World)
{
	FCCLTerrainSaveContext Context;
	Context.WorldId = World.Identity.WorldId;
	Context.BaseWorldVersion = World.Identity.BaseWorldVersion;
	Context.WorldGeneration = World.Identity.Generation;
	Context.GameSeconds = World.Clock.GameSeconds;
	Context.WorldSeconds = World.Clock.WorldSeconds;
	return Context;
}

bool FCCLWorldGenerationStore::Write(const FString& Root, const TArray<uint8>& World,
	const TMap<FGuid, TArray<uint8>>& Terrain, FString& OutDirectory, FString& Error)
{
	FCCLWorldSnapshot WorldState;
	if (Root.IsEmpty() || Terrain.Num() > MaximumRegions || !FCCLWorldSnapshotCodec::Decode(World, WorldState, Error))
	{
		Error = TEXT("Invalid world generation bundle.");
		return false;
	}

	const auto Context = ContextFor(WorldState);
	int64 Total = World.Num();
	for (const auto& Pair : Terrain)
	{
		FCCLTerrainSnapshot Region;
		FCCLTerrainSaveContext RegionContext;
		Total += Pair.Value.Num();
		if (Total > MaximumBundleBytes || !FCCLTerrainCodec::Decode(Pair.Value, Region, RegionContext, Error)
			|| Region.Definition.RegionId != Pair.Key || !(RegionContext == Context))
		{
			Error = TEXT("Terrain and world generation contexts differ or exceed the bundle budget.");
			return false;
		}
	}

	const FString Directory = Root / FString::Printf(TEXT("g-%020llu-%s"), Context.WorldGeneration, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	if (!IFileManager::Get().MakeDirectory(*Directory, true))
	{
		Error = TEXT("Cannot create a world generation directory.");
		return false;
	}

	TArray<uint8> Marker;
	FMemoryWriter Writer(Marker, true);
	uint32 Magic = MarkerMagic;
	uint32 Version = 1;
	uint32 WorldCRC = CRC(World);
	int32 Count = Terrain.Num();
	Writer << Magic << Version << WorldCRC << Count;
	if (!WriteFlushed(Directory / TEXT("world.bin"), World))
	{
		Error = TEXT("Cannot write world generation payload; previous generation retained.");
		return false;
	}

	TArray<FGuid> Ids;
	Terrain.GenerateKeyArray(Ids);
	Ids.Sort();
	for (FGuid Id : Ids)
	{
		const auto& Bytes = Terrain.FindChecked(Id);
		uint32 Checksum = CRC(Bytes);
		Writer << Id << Checksum;
		if (!WriteFlushed(Directory / (Id.ToString(EGuidFormats::Digits) + TEXT(".bin")), Bytes))
		{
			Error = TEXT("Cannot write terrain payload; previous generation retained.");
			return false;
		}
	}

	uint32 MarkerCRC = CRC(Marker);
	Writer << MarkerCRC;
	Writer.Close();
	if (!WriteFlushed(Directory / TEXT("complete.tmp"), Marker)
		|| !IFileManager::Get().Move(*(Directory / TEXT("complete.bin")), *(Directory / TEXT("complete.tmp")), false, false, false, true))
	{
		Error = TEXT("Cannot publish the completion marker; previous generation retained.");
		return false;
	}

	OutDirectory = Directory;
	Error.Reset();
	return true;
}

bool FCCLWorldGenerationStore::ReadDirectory(const FString& Directory, ECCLWorldDomain Domain,
	FCCLWorldGenerationBundle& OutBundle, FString& Error)
{
	TArray<uint8> Marker;
	if (!ReadBounded(Directory / TEXT("complete.bin"), 20 + MaximumRegions * 20, Marker) || Marker.Num() < 20)
	{
		return false;
	}

	uint32 ExpectedCRC = 0;
	FMemory::Memcpy(&ExpectedCRC, Marker.GetData() + Marker.Num() - 4, 4);
	if (FCrc::MemCrc32(Marker.GetData(), Marker.Num() - 4) != ExpectedCRC)
	{
		return false;
	}

	FMemoryReader Reader(Marker, true);
	uint32 Magic = 0;
	uint32 Version = 0;
	uint32 WorldCRC = 0;
	int32 Count = 0;
	Reader << Magic << Version << WorldCRC << Count;
	if (Magic != MarkerMagic || Version != 1 || Count < 0 || Count > MaximumRegions || Marker.Num() != 20 + Count * 20)
	{
		return false;
	}

	FCCLWorldGenerationBundle Bundle;
	FCCLWorldSnapshot WorldState;
	if (!ReadBounded(Directory / TEXT("world.bin"), 64ll * 1024 * 1024, Bundle.World)
		|| CRC(Bundle.World) != WorldCRC || !FCCLWorldSnapshotCodec::Decode(Bundle.World, WorldState, Error)
		|| WorldState.Identity.Domain != Domain)
	{
		return false;
	}

	Bundle.Context = ContextFor(WorldState);
	Bundle.Directory = Directory;
	int64 Total = Bundle.World.Num();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FGuid Id;
		uint32 Checksum = 0;
		Reader << Id << Checksum;
		if (!Id.IsValid() || Bundle.Terrain.Contains(Id))
		{
			return false;
		}

		TArray<uint8> Bytes;
		FCCLTerrainSnapshot Region;
		FCCLTerrainSaveContext Context;
		if (!ReadBounded(Directory / (Id.ToString(EGuidFormats::Digits) + TEXT(".bin")), FCCLTerrainCodec::MaximumBytes, Bytes)
			|| CRC(Bytes) != Checksum || !FCCLTerrainCodec::Decode(Bytes, Region, Context, Error)
			|| Region.Definition.RegionId != Id || !(Context == Bundle.Context))
		{
			return false;
		}

		Total += Bytes.Num();
		if (Total > MaximumBundleBytes)
		{
			return false;
		}

		Bundle.Terrain.Add(Id, MoveTemp(Bytes));
	}

	OutBundle = MoveTemp(Bundle);
	return true;
}

bool FCCLWorldGenerationStore::Recover(const FString& Root, ECCLWorldDomain Domain,
	FCCLWorldGenerationBundle& OutBundle, FString& Error)
{
	TArray<FString> Directories;
	IFileManager::Get().FindFiles(Directories, *(Root / TEXT("g-*")), false, true);
	Directories.Sort([](const FString& A, const FString& B) { return A > B; });
	FCCLWorldGenerationBundle Best;
	bool bFound = false;
	for (const FString& Name : Directories)
	{
		FCCLWorldGenerationBundle Candidate;
		if (ReadDirectory(Root / Name, Domain, Candidate, Error)
			&& (!bFound || Candidate.Context.WorldGeneration > Best.Context.WorldGeneration))
		{
			Best = MoveTemp(Candidate);
			bFound = true;
		}
	}

	if (!bFound)
	{
		Error = TEXT("No complete, valid world generation exists.");
		return false;
	}

	OutBundle = MoveTemp(Best);
	Error.Reset();
	return true;
}
