#include "CCLInventorySnapshot.h"

#include "Environment/CCLWorldParticipant.h"
#include "Environment/CCLWorldSnapshot.h"

#include "CCLFoodState.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
bool Reject(FString& Error)
{
	Error = TEXT("Invalid, oversized or inconsistent persistent inventory snapshot.");
	return false;
}

bool String(FArchive& Ar, FString& Text, int32 Limit)
{
	FTCHARToUTF8 Utf8(*Text);
	int32 Size = Ar.IsSaving() ? Utf8.Length() : 0;
	Ar << Size;
	if (Ar.IsError() || Size < 0 || Size > Limit || (Ar.IsLoading() && Size > Ar.TotalSize() - Ar.Tell()))
	{
		return false;
	}
	if (Ar.IsSaving())
	{
		Ar.Serialize(const_cast<ANSICHAR*>(Utf8.Get()), Size);
	}
	else
	{
		TArray<ANSICHAR> Buffer;
		Buffer.SetNumZeroed(Size + 1);
		Ar.Serialize(Buffer.GetData(), Size);
		const FUTF8ToTCHAR Converted(Buffer.GetData(), Size);
		Text = FString(Converted.Length(), Converted.Get());
	}
	return !Ar.IsError();
}

bool Serialize(FArchive& Ar, TArray<FCCLInventorySnapshot>& Records)
{
	uint32 Magic = 0x494C4343;
	uint16 Version = 3;
	int32 Count = Records.Num();
	Ar << Magic << Version << Count;
	if (Ar.IsError() || Magic != 0x494C4343 || (Version < 1 || Version > 3) || Count < 0 || Count > 1024 ||
		(Ar.IsLoading() && int64(Count) * 24 > Ar.TotalSize() - Ar.Tell()))
	{
		return false;
	}
	Records.SetNum(Count);
	int32 Total = 0;
	for (auto& Record : Records)
	{
		int32 Items = Record.Items.Num();
		Ar << Record.ContainerId << Record.Capacity << Items;
		if (Ar.IsError() || Items < 0 || Items > 256 || (Total += Items) > 4096 ||
			(Ar.IsLoading() && int64(Items) * 36 > Ar.TotalSize() - Ar.Tell()))
		{
			return false;
		}
		Record.Items.SetNum(Items);
		for (auto& Item : Record.Items)
		{
			Ar << Item.Id << Item.Quantity << Item.Slot << Item.LoadedAmmo;
			if (!String(Ar, Item.Definition, 512) || !String(Ar, Item.Food, 20000) ||
				Ar.Tell() > CCLInventorySnapshot::MaximumBytes)
			{
				return false;
			}
		}
		if (Version >= 2)
		{
			auto& B = Record.Environment;
			uint8 Source = uint8(B.Source);
			Ar << Source << B.SourceId << B.SampleWorldSeconds << B.Conditions.TemperatureCelsius
			   << B.Conditions.RelativeHumidity << B.Conditions.SpoilageMultiplier << B.Conditions.MoldExposure;
			B.Source = ECCLFoodEnvironment(Source);
		}
		if (Version >= 3)
		{
			uint8 Owner = uint8(Record.Owner);
			FVector Location = Record.Transform.GetLocation(), Scale = Record.Transform.GetScale3D();
			FQuat Rotation = Record.Transform.GetRotation();
			Ar << Owner << Location.X << Location.Y << Location.Z << Rotation.X << Rotation.Y << Rotation.Z
			   << Rotation.W << Scale.X << Scale.Y << Scale.Z;
			if (Ar.IsError() || Location.ContainsNaN() || Scale.ContainsNaN() || Rotation.ContainsNaN() ||
				!Rotation.IsNormalized() || Scale.GetMin() <= 0. || Scale.GetMax() > 10000. ||
				Location.GetAbsMax() > 1.e12)
			{
				return false;
			}
			Record.Owner = ECCLInventoryOwner(Owner);
			Record.Transform = FTransform(Rotation, Location, Scale);
		}
	}
	return !Ar.IsError();
}
} // namespace

bool CCLInventorySnapshot::Validate(const TArray<FCCLInventorySnapshot>& Records, FGuid WorldId, double WorldSeconds,
									FString& Error)
{
	Error.Reset();
	if (!WorldId.IsValid() || !FMath::IsFinite(WorldSeconds) || WorldSeconds < 0. || Records.Num() > 1024)
	{
		return Reject(Error);
	}
	TSet<FGuid> Containers, Items;
	int32 Total = 0;
	int64 Size = 10;
	for (const auto& Record : Records)
	{
		if (!Record.ContainerId.IsValid() || Containers.Contains(Record.ContainerId) || Record.Capacity < 1 ||
			Record.Capacity > 128 || Record.Items.Num() > Record.Capacity + 128 ||
			(Total += Record.Items.Num()) > 4096 ||
			!CCLFoodEnvironment::Validate(Record.Environment, WorldSeconds, Error))
		{
			return Reject(Error);
		}
		const auto Scale = Record.Transform.GetScale3D();
		if (uint8(Record.Owner) > uint8(ECCLInventoryOwner::FoodPickup) || Record.Transform.ContainsNaN() ||
			!Record.Transform.GetRotation().IsNormalized() || Record.Transform.GetLocation().GetAbsMax() > 1.e12 ||
			Scale.GetMin() <= 0. || Scale.GetMax() > 10000. ||
			(Record.Owner == ECCLInventoryOwner::Container && !Record.Transform.Equals(FTransform::Identity, 0.)) ||
			(Record.Owner == ECCLInventoryOwner::FoodPickup && (Record.Capacity != 1 || Record.Items.Num() > 1)))
		{
			return Reject(Error);
		}
		Containers.Add(Record.ContainerId);
		Size += 162;
		TSet<int32> Slots;
		for (const auto& Item : Record.Items)
		{
			FCCLFoodState Food;
			if (!Item.Id.IsValid() || Items.Contains(Item.Id) || !Item.Definition.StartsWith(TEXT("/Game/")) ||
				Item.Definition.Len() > 512 || Item.Definition.Contains(TEXT("..")) || Item.Quantity < 1 ||
				Item.Quantity > 1000 || Item.Slot < INDEX_NONE || Item.Slot >= Record.Capacity ||
				(Item.Slot >= 0 && Slots.Contains(Item.Slot)) || Item.LoadedAmmo < 0 || Item.LoadedAmmo > 1000000 ||
				Item.Food.Len() > 20000 || !CCLFood::Decode(Item.Food, Food, Error) ||
				!CCLFood::Validate(Food, Food.IsPresent() ? Item.Quantity : 0, Error) ||
				(Food.IsPresent() && (Food.WorldId != WorldId || Food.UpdatedWorldSeconds > WorldSeconds ||
									  (Record.Environment.IsDynamic() &&
									   (Food.UpdatedWorldSeconds < Record.Environment.SampleWorldSeconds ||
										!(Food.Storage == Record.Environment.Conditions))))))
			{
				return Reject(Error);
			}
			if (Record.Owner == ECCLInventoryOwner::FoodPickup &&
				(!Food.IsPresent() || Item.Slot != 0 || Item.LoadedAmmo != 0))
			{
				return Reject(Error);
			}
			Items.Add(Item.Id);
			Slots.Add(Item.Slot);
			Size += 36 + FTCHARToUTF8(*Item.Definition).Length() + FTCHARToUTF8(*Item.Food).Length();
			if (Size > MaximumBytes)
			{
				return Reject(Error);
			}
		}
	}
	return true;
}

bool CCLInventorySnapshot::Encode(const TArray<FCCLInventorySnapshot>& Records, FGuid WorldId, double WorldSeconds,
								  TArray<uint8>& Bytes, FString& Error)
{
	if (!Validate(Records, WorldId, WorldSeconds, Error))
	{
		return false;
	}
	auto Candidate = Records;
	TArray<uint8> Output;
	FMemoryWriter Writer(Output);
	if (!Serialize(Writer, Candidate) || Output.Num() > MaximumBytes)
	{
		return Reject(Error);
	}
	Bytes = MoveTemp(Output);
	return true;
}

bool CCLInventorySnapshot::Decode(const TArray<uint8>& Bytes, FGuid WorldId, double WorldSeconds,
								  TArray<FCCLInventorySnapshot>& Records, FString& Error)
{
	if (Bytes.Num() < 10 || Bytes.Num() > MaximumBytes)
	{
		return Reject(Error);
	}
	TArray<FCCLInventorySnapshot> Candidate;
	FMemoryReader Reader(Bytes);
	if (!Serialize(Reader, Candidate) || Reader.Tell() != Reader.TotalSize() ||
		!Validate(Candidate, WorldId, WorldSeconds, Error))
	{
		return Reject(Error);
	}
	Records = MoveTemp(Candidate);
	return true;
}

FName CCLInventorySnapshot::WorldSection()
{
	static const FName Name(TEXT("Inventories"));
	return Name;
}

bool CCLInventorySnapshot::ReadWorld(const FCCLWorldSnapshot& World, TArray<FCCLInventorySnapshot>& Records, FString& Error)
{
	const auto* Bytes = World.FindSection(WorldSection());
	if (!Bytes)
	{
		Records.Reset();
		return true;
	}

	return Decode(*Bytes, World.Identity.WorldId, World.Clock.WorldSeconds, Records, Error);
}

bool CCLInventorySnapshot::WriteWorld(FCCLWorldSnapshot& World, const TArray<FCCLInventorySnapshot>& Records, FString& Error)
{
	TArray<uint8> Bytes;
	if (!Encode(Records, World.Identity.WorldId, World.Clock.WorldSeconds, Bytes, Error))
	{
		return false;
	}

	World.SetSection(WorldSection(), MoveTemp(Bytes));
	return true;
}

namespace
{
	// World containers validate before player profiles so each item has exactly one owner.
	const FCCLWorldSectionRegistrar InventorySection(CCLInventorySnapshot::WorldSection(), 1,
		[](const FCCLWorldSectionContext& Context, const TArray<uint8>& Bytes, FString& Error)
		{
			TArray<FCCLInventorySnapshot> Records;
			if (!CCLInventorySnapshot::Decode(Bytes, Context.WorldId, Context.WorldSeconds, Records, Error))
			{
				return false;
			}

			for (const auto& Record : Records)
			{
				auto Binding = Record.Environment;
				if (!Context.Surface || !CCLFoodEnvironment::Sample(Binding, *Context.Surface, Error))
				{
					return false;
				}

				for (const auto& Item : Record.Items)
				{
					bool bClaimed = false;
					Context.ClaimedItems->Add(Item.Id, &bClaimed);
					if (bClaimed)
					{
						Error = TEXT("An item cannot belong to two world inventory owners.");
						return false;
					}
				}
			}

			return true;
		});
}
