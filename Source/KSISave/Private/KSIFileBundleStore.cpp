#include "KSIFileBundleStore.h"

#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr int64 MaximumRootBytes = 64ll * 1024 * 1024;
	constexpr int64 MaximumPartBytes = 32ll * 1024 * 1024;
	constexpr int64 MaximumBundleBytes = 256ll * 1024 * 1024;
	constexpr int32 MaximumParts = 64;

    bool ValidFormat(const FKSIFileBundleFormat& Format)
    {
        const FString& Name = Format.RootFileName;
        FGuid PartId;
        return Format.MarkerMagic != 0 && !Name.IsEmpty() && Name.Len() <= 128
            && FPaths::GetCleanFilename(Name) == Name && !Name.Contains(TEXT(":"))
            && !Name.EndsWith(TEXT(".")) && !Name.EndsWith(TEXT(" "))
            && !Name.Contains(TEXT("\\")) && !Name.Contains(TEXT("/"))
            && Name != TEXT(".") && Name != TEXT("..")
            && !Name.Equals(TEXT("complete.bin"), ESearchCase::IgnoreCase)
            && !Name.Equals(TEXT("complete.tmp"), ESearchCase::IgnoreCase)
            && !(Name.EndsWith(TEXT(".bin"), ESearchCase::IgnoreCase)
                && FGuid::ParseExact(FPaths::GetBaseFilename(Name), EGuidFormats::Digits, PartId));
    }

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

bool FKSIFileBundleStore::Write(const FString& Root, const TArray<uint8>& Payload,
    const TMap<FGuid, TArray<uint8>>& Parts, const FKSIFileBundleFormat& Format,
    FKSIFileBundleValidator Validate, FString& OutDirectory, FString& Error)
{
    if (Root.IsEmpty() || !ValidFormat(Format) || Payload.IsEmpty() || Payload.Num() > MaximumRootBytes || Parts.Num() > MaximumParts)
    {
        Error = TEXT("Invalid file bundle limits or format.");
        return false;
    }
    int64 Total = Payload.Num();
    for (const auto& Pair : Parts)
    {
        Total += Pair.Value.Num();
        if (!Pair.Key.IsValid() || Pair.Value.IsEmpty() || Pair.Value.Num() > MaximumPartBytes || Total > MaximumBundleBytes)
        {
            Error = TEXT("Invalid file bundle part or size budget.");
            return false;
        }
    }
    uint64 Generation = 0;
    if (!Validate(Payload, Parts, Generation, Error))
    {
        return false;
    }

	const FString Directory = Root / FString::Printf(TEXT("g-%020llu-%s"), Generation, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	if (!IFileManager::Get().MakeDirectory(*Directory, true))
	{
		Error = TEXT("Cannot create a file bundle directory.");
		return false;
	}

	TArray<uint8> Marker;
	FMemoryWriter Writer(Marker, true);
	uint32 Magic = Format.MarkerMagic;
	uint32 Version = 1;
	uint32 RootCRC = CRC(Payload);
	int32 Count = Parts.Num();
	Writer << Magic << Version << RootCRC << Count;
	if (!WriteFlushed(Directory / Format.RootFileName, Payload))
	{
		Error = TEXT("Cannot write file bundle payload; previous generation retained.");
		return false;
	}

	TArray<FGuid> Ids;
	Parts.GenerateKeyArray(Ids);
	Ids.Sort();
	for (FGuid Id : Ids)
	{
		const auto& Bytes = Parts.FindChecked(Id);
		uint32 Checksum = CRC(Bytes);
		Writer << Id << Checksum;
		if (!WriteFlushed(Directory / (Id.ToString(EGuidFormats::Digits) + TEXT(".bin")), Bytes))
		{
			Error = TEXT("Cannot write part payload; previous generation retained.");
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

bool FKSIFileBundleStore::ReadDirectory(const FString& Directory, const FKSIFileBundleFormat& Format,
    FKSIFileBundleValidator Validate, FKSIFileBundle& OutBundle, FString& Error)
{
	TArray<uint8> Marker;
	if (!ReadBounded(Directory / TEXT("complete.bin"), 20 + MaximumParts * 20, Marker) || Marker.Num() < 20)
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
	uint32 RootCRC = 0;
	int32 Count = 0;
	Reader << Magic << Version << RootCRC << Count;
	if (Magic != Format.MarkerMagic || Version != 1 || Count < 0 || Count > MaximumParts || Marker.Num() != 20 + Count * 20)
	{
		return false;
	}

	FKSIFileBundle Bundle;
	if (!ReadBounded(Directory / Format.RootFileName, MaximumRootBytes, Bundle.Root)
		|| CRC(Bundle.Root) != RootCRC)
	{
		return false;
	}

	Bundle.Directory = Directory;
	int64 Total = Bundle.Root.Num();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FGuid Id;
		uint32 Checksum = 0;
		Reader << Id << Checksum;
		if (!Id.IsValid() || Bundle.Parts.Contains(Id))
		{
			return false;
		}

		TArray<uint8> Bytes;
		if (!ReadBounded(Directory / (Id.ToString(EGuidFormats::Digits) + TEXT(".bin")), MaximumPartBytes, Bytes)
			|| CRC(Bytes) != Checksum)
		{
			return false;
		}

		Total += Bytes.Num();
		if (Total > MaximumBundleBytes)
		{
			return false;
		}

		Bundle.Parts.Add(Id, MoveTemp(Bytes));
	}

	if (!Validate(Bundle.Root, Bundle.Parts, Bundle.Generation, Error))
	{
		return false;
	}
	OutBundle = MoveTemp(Bundle);
	return true;
}

bool FKSIFileBundleStore::Recover(const FString& Root, const FKSIFileBundleFormat& Format,
    FKSIFileBundleValidator Validate, FKSIFileBundle& OutBundle, FString& Error)
{
	if (Root.IsEmpty() || !ValidFormat(Format))
	{
		Error = TEXT("Invalid file bundle root or format.");
		return false;
	}
	TArray<FString> Directories;
	IFileManager::Get().FindFiles(Directories, *(Root / TEXT("g-*")), false, true);
	Directories.Sort([](const FString& A, const FString& B) { return A > B; });
	FKSIFileBundle Best;
	bool bFound = false;
	for (const FString& Name : Directories)
	{
		FKSIFileBundle Candidate;
		if (ReadDirectory(Root / Name, Format, Validate, Candidate, Error)
			&& (!bFound || Candidate.Generation > Best.Generation))
		{
			Best = MoveTemp(Candidate);
			bFound = true;
		}
	}

	if (!bFound)
	{
		Error = TEXT("No complete, valid file bundle exists.");
		return false;
	}

	OutBundle = MoveTemp(Best);
	Error.Reset();
	return true;
}
