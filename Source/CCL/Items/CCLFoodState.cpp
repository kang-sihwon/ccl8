#include "CCLFoodState.h"

#include "Misc/Base64.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr double MaxTime = 1.e15;
	constexpr double MaxPolicyTime = 1.e12;

	bool InRange(double Value, double Min, double Max)
	{
		return FMath::IsFinite(Value) && Value >= Min && Value <= Max;
	}

	bool SameLot(const FCCLFoodLot& A, const FCCLFoodLot& B)
	{
		return A.OriginId == B.OriginId && A.CreatedWorldSeconds == B.CreatedWorldSeconds
			&& A.SpoilageSeconds == B.SpoilageSeconds && A.MoldSeconds == B.MoldSeconds;
	}

	void Canonicalize(FCCLFoodState& State)
	{
		State.Lots.Sort([](const FCCLFoodLot& A, const FCCLFoodLot& B)
		{
			if (A.CreatedWorldSeconds != B.CreatedWorldSeconds)
			{
				return A.CreatedWorldSeconds < B.CreatedWorldSeconds;
			}

			if (A.OriginId != B.OriginId)
			{
				return A.OriginId < B.OriginId;
			}

			if (A.SpoilageSeconds != B.SpoilageSeconds)
			{
				return A.SpoilageSeconds > B.SpoilageSeconds;
			}

			return A.MoldSeconds > B.MoldSeconds;
		});
		for (int32 I = State.Lots.Num() - 1; I > 0; --I)
		{
			if (SameLot(State.Lots[I - 1], State.Lots[I]))
			{
				State.Lots[I - 1].Quantity += State.Lots[I].Quantity;
				State.Lots.RemoveAt(I);
			}
		}
	}

	void SerializeHeader(FArchive& Ar, FCCLFoodState& State)
	{
		Ar << State.WorldId << State.UpdatedWorldSeconds;
		Ar << State.Policy.ShelfLifeSeconds << State.Policy.MoldDelaySeconds << State.Policy.MoldGrowthSeconds;
		Ar << State.Storage.TemperatureCelsius << State.Storage.RelativeHumidity;
		Ar << State.Storage.SpoilageMultiplier << State.Storage.MoldExposure;
	}

	void SerializeLot(FArchive& Ar, FCCLFoodLot& Lot)
	{
		Ar << Lot.OriginId << Lot.Quantity << Lot.CreatedWorldSeconds << Lot.SpoilageSeconds << Lot.MoldSeconds;
	}
}

bool FCCLFoodPolicy::operator==(const FCCLFoodPolicy& Other) const
{
	return ShelfLifeSeconds == Other.ShelfLifeSeconds && MoldDelaySeconds == Other.MoldDelaySeconds
		&& MoldGrowthSeconds == Other.MoldGrowthSeconds;
}

bool FCCLFoodStorage::operator==(const FCCLFoodStorage& Other) const
{
	return TemperatureCelsius == Other.TemperatureCelsius && RelativeHumidity == Other.RelativeHumidity
		&& SpoilageMultiplier == Other.SpoilageMultiplier && MoldExposure == Other.MoldExposure;
}

bool CCLFood::ValidatePolicy(const FCCLFoodPolicy& Policy)
{
	return InRange(Policy.ShelfLifeSeconds, 0.001, MaxPolicyTime)
		&& InRange(Policy.MoldDelaySeconds, 0., MaxPolicyTime)
		&& InRange(Policy.MoldGrowthSeconds, 0.001, MaxPolicyTime);
}

bool CCLFood::ValidateStorage(const FCCLFoodStorage& Storage)
{
	return InRange(Storage.TemperatureCelsius, -80., 80.) && InRange(Storage.RelativeHumidity, 0., 1.)
		&& InRange(Storage.SpoilageMultiplier, 0., 1.) && InRange(Storage.MoldExposure, 0., 1.);
}

int32 CCLFood::Quantity(const FCCLFoodState& State)
{
	int64 Sum = 0;
	for (const auto& Lot : State.Lots)
	{
		Sum += Lot.Quantity;
	}

	return Sum >= 0 && Sum <= 1000 ? int32(Sum) : INDEX_NONE;
}

bool CCLFood::Validate(const FCCLFoodState& State, int32 ExpectedQuantity, FString& Error)
{
	Error.Reset();
	if (!State.IsPresent())
	{
		if (!State.WorldId.IsValid() && State.UpdatedWorldSeconds == 0. && State.Policy == FCCLFoodPolicy()
			&& State.Storage == FCCLFoodStorage() && ExpectedQuantity <= 0)
		{
			return true;
		}

		Error = TEXT("Absent food must be an empty canonical state.");
		return false;
	}

	if (!State.WorldId.IsValid() || !InRange(State.UpdatedWorldSeconds, 0., MaxTime)
		|| !ValidatePolicy(State.Policy) || !ValidateStorage(State.Storage) || State.Lots.Num() > MaxLots
		|| Quantity(State) < 1 || (ExpectedQuantity >= 0 && Quantity(State) != ExpectedQuantity))
	{
		Error = TEXT("Invalid food world, time, policy, storage or quantity.");
		return false;
	}

	TMap<FGuid, double> Origins;
	for (const auto& Lot : State.Lots)
	{
		if (!Lot.OriginId.IsValid() || Lot.Quantity < 1 || Lot.Quantity > 1000
			|| !InRange(Lot.CreatedWorldSeconds, 0., State.UpdatedWorldSeconds)
			|| !InRange(Lot.SpoilageSeconds, 0., State.Policy.ShelfLifeSeconds)
			|| !InRange(Lot.MoldSeconds, 0., State.Policy.MoldDelaySeconds + State.Policy.MoldGrowthSeconds))
		{
			Error = TEXT("Invalid food lot or exposure.");
			return false;
		}

		if (const auto* Birth = Origins.Find(Lot.OriginId); Birth && *Birth != Lot.CreatedWorldSeconds)
		{
			Error = TEXT("One food origin cannot have multiple creation times.");
			return false;
		}

		Origins.Add(Lot.OriginId, Lot.CreatedWorldSeconds);
	}

	return true;
}

bool CCLFood::Create(FGuid WorldId, FGuid OriginId, int32 Count, double WorldSeconds,
	const FCCLFoodPolicy& Policy, const FCCLFoodStorage& Storage, FCCLFoodState& Out, FString& Error)
{
	FCCLFoodState Candidate;
	Candidate.WorldId = WorldId;
	Candidate.UpdatedWorldSeconds = WorldSeconds;
	Candidate.Policy = Policy;
	Candidate.Storage = Storage;
	Candidate.Lots.Add({OriginId, Count, WorldSeconds, 0., 0.});
	if (!Validate(Candidate, Count, Error))
	{
		return false;
	}

	Out = MoveTemp(Candidate);
	return true;
}

bool CCLFood::Advance(FCCLFoodState& State, double WorldSeconds, FString& Error)
{
	if (!Validate(State, INDEX_NONE, Error) || !State.IsPresent()
		|| !InRange(WorldSeconds, State.UpdatedWorldSeconds, MaxTime))
	{
		Error = TEXT("Food advance requires valid food and nondecreasing committed world time.");
		return false;
	}

	const double Delta = WorldSeconds - State.UpdatedWorldSeconds;
	const auto& Storage = State.Storage;
	const double Temperature = Storage.TemperatureCelsius <= 0. ? 0. : FMath::Pow(2., (Storage.TemperatureCelsius - 20.) / 10.);
	const double Spoilage = Delta * Temperature * (0.25 + 0.75 * Storage.RelativeHumidity) * Storage.SpoilageMultiplier;
	const double Mold = Storage.TemperatureCelsius > 45. ? 0. : Delta * Temperature
		* FMath::Clamp((Storage.RelativeHumidity - 0.4) / 0.6, 0., 1.) * Storage.MoldExposure;
	FCCLFoodState Candidate = State;
	for (auto& Lot : Candidate.Lots)
	{
		Lot.SpoilageSeconds = FMath::Min(Candidate.Policy.ShelfLifeSeconds, Lot.SpoilageSeconds + Spoilage);
		Lot.MoldSeconds = FMath::Min(Candidate.Policy.MoldDelaySeconds + Candidate.Policy.MoldGrowthSeconds, Lot.MoldSeconds + Mold);
	}

	Candidate.UpdatedWorldSeconds = WorldSeconds;
	Canonicalize(Candidate);
	if (!Validate(Candidate, INDEX_NONE, Error))
	{
		return false;
	}

	State = MoveTemp(Candidate);
	return true;
}

bool CCLFood::ChangeStorage(FCCLFoodState& State, const FCCLFoodStorage& Storage, double WorldSeconds, FString& Error)
{
	if (!ValidateStorage(Storage))
	{
		Error = TEXT("Invalid food storage condition.");
		return false;
	}

	FCCLFoodState Candidate = State;
	if (!Advance(Candidate, WorldSeconds, Error))
	{
		return false;
	}

	Candidate.Storage = Storage;
	State = MoveTemp(Candidate);
	return true;
}

bool CCLFood::Merge(FCCLFoodState& Destination, const FCCLFoodState& Source, double WorldSeconds, FString& Error)
{
	FCCLFoodState Candidate = Destination;
	FCCLFoodState Incoming = Source;
	if (&Destination == &Source || !Advance(Candidate, WorldSeconds, Error) || !Advance(Incoming, WorldSeconds, Error)
		|| Candidate.WorldId != Incoming.WorldId || !(Candidate.Policy == Incoming.Policy)
		|| Quantity(Candidate) + Quantity(Incoming) > 1000)
	{
		Error = TEXT("Food merge requires distinct compatible stacks in the same world.");
		return false;
	}

	// Incoming history is settled under its old storage. Future exposure uses destination storage.
	Candidate.Lots.Append(Incoming.Lots);
	Canonicalize(Candidate);
	if (!Validate(Candidate, INDEX_NONE, Error))
	{
		return false;
	}

	Destination = MoveTemp(Candidate);
	return true;
}

bool CCLFood::Split(FCCLFoodState& Source, int32 Count, double WorldSeconds, FCCLFoodState& Removed, FString& Error)
{
	FCCLFoodState Candidate = Source;
	if (&Source == &Removed || Count <= 0 || !Advance(Candidate, WorldSeconds, Error) || Count > Quantity(Candidate))
	{
		Error = TEXT("Invalid food split quantity or output alias.");
		return false;
	}

	FCCLFoodState Taken = Candidate;
	Taken.Lots.Reset();
	int32 Remaining = Count;
	for (auto& Lot : Candidate.Lots)
	{
		if (Remaining == 0)
		{
			break;
		}

		FCCLFoodLot Part = Lot;
		Part.Quantity = FMath::Min(Lot.Quantity, Remaining);
		Taken.Lots.Add(Part);
		Lot.Quantity -= Part.Quantity;
		Remaining -= Part.Quantity;
	}

	Candidate.Lots.RemoveAll([](const FCCLFoodLot& Lot) { return Lot.Quantity == 0; });
	if (Candidate.Lots.IsEmpty())
	{
		Candidate = {};
	}

	Source = MoveTemp(Candidate);
	Removed = MoveTemp(Taken);
	return true;
}

bool CCLFood::Evaluate(const FCCLFoodState& State, double WorldSeconds, FCCLFoodQuality& Quality, FString& Error)
{
	FCCLFoodState Candidate = State;
	if (!Advance(Candidate, WorldSeconds, Error))
	{
		return false;
	}

	FCCLFoodQuality Result;
	Result.Freshness = 0.;
	const double Total = Quantity(Candidate);
	for (const auto& Lot : Candidate.Lots)
	{
		const double Freshness = FMath::Clamp(1. - Lot.SpoilageSeconds / Candidate.Policy.ShelfLifeSeconds, 0., 1.);
		const double Mold = FMath::Clamp((Lot.MoldSeconds - Candidate.Policy.MoldDelaySeconds) / Candidate.Policy.MoldGrowthSeconds, 0., 1.);
		Result.Freshness += Freshness * Lot.Quantity / Total;
		Result.Mold += Mold * Lot.Quantity / Total;
		Result.OldestAgeSeconds = FMath::Max(Result.OldestAgeSeconds, WorldSeconds - Lot.CreatedWorldSeconds);
		Result.SpoiledQuantity += Freshness == 0. ? Lot.Quantity : 0;
		Result.MoldyQuantity += Mold > 0. ? Lot.Quantity : 0;
	}

	Quality = Result;
	return true;
}

bool CCLFood::Encode(const FCCLFoodState& State, FString& Encoded, FString& Error)
{
	if (!Validate(State, INDEX_NONE, Error))
	{
		return false;
	}

	if (!State.IsPresent())
	{
		Encoded.Reset();
		return true;
	}

	FCCLFoodState Copy = State;
	Canonicalize(Copy);
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	uint32 Magic = 0x46444C43;
	uint16 Version = 1;
	int32 Count = Copy.Lots.Num();
	Writer << Magic << Version;
	SerializeHeader(Writer, Copy);
	Writer << Count;
	for (auto& Lot : Copy.Lots)
	{
		SerializeLot(Writer, Lot);
	}

	uint32 Checksum = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
	Writer << Checksum;
	Encoded = FBase64::Encode(Bytes);
	return true;
}

bool CCLFood::Decode(const FString& Encoded, FCCLFoodState& State, FString& Error)
{
	if (Encoded.IsEmpty())
	{
		State = {};
		Error.Reset();
		return true;
	}

	TArray<uint8> Bytes;
	if (Encoded.Len() > 20000 || !FBase64::Decode(Encoded, Bytes) || Bytes.Num() < 98 || Bytes.Num() > 11400)
	{
		Error = TEXT("Food payload size or encoding invalid.");
		return false;
	}

	uint32 Checksum = 0;
	FMemory::Memcpy(&Checksum, Bytes.GetData() + Bytes.Num() - 4, 4);
	if (Checksum != FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4))
	{
		Error = TEXT("Food payload checksum mismatch.");
		return false;
	}

	FCCLFoodState Candidate;
	FMemoryReader Reader(Bytes);
	uint32 Magic = 0;
	uint16 Version = 0;
	int32 Count = 0;
	Reader << Magic << Version;
	SerializeHeader(Reader, Candidate);
	Reader << Count;
	if (Magic != 0x46444C43 || Version != 1 || Count < 1 || Count > MaxLots || Reader.TotalSize() - Reader.Tell() != Count * 44 + 4)
	{
		Error = TEXT("Food payload schema or lot count invalid.");
		return false;
	}

	Candidate.Lots.SetNum(Count);
	for (auto& Lot : Candidate.Lots)
	{
		SerializeLot(Reader, Lot);
	}

	if (Reader.IsError() || !Validate(Candidate, INDEX_NONE, Error))
	{
		return false;
	}

	Canonicalize(Candidate);
	State = MoveTemp(Candidate);
	return true;
}
