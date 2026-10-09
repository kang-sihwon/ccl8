#include "CCLWorldSnapshot.h"

#include "Agents/CCLLifeSimulation.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr uint32 Magic = 0x574C4343; // CCLW
	constexpr int32 MaxBytes = 36 * 1024 * 1024;

	bool Reject(FString& Error, const TCHAR* Reason)
	{
		Error = Reason;
		return false;
	}

	bool Serialize(FArchive& Ar, FCCLWorldSnapshot& S)
	{
		uint32 Tag = Magic;
		uint8 Domain = static_cast<uint8>(S.Identity.Domain);
		Ar << Tag << S.Schema << S.Identity.WorldId << S.Identity.BaseWorldVersion << S.Identity.Generation << Domain;
		S.Identity.Domain = static_cast<ECCLWorldDomain>(Domain);
		Ar << S.Clock.Version << S.Clock.CompletedStepId << S.Clock.GameSeconds << S.Clock.WorldSeconds;
		Ar << S.Clock.AcceptedGameSeconds << S.Clock.RequestedWorldSeconds << S.Clock.TimeScale;
		if (Ar.IsError() || Tag != Magic)
		{
			return false;
		}

		int32 Count = S.Clock.Pending.Num();
		Ar << Count;
		if (Ar.IsError() || Count < 0 || Count > 256 ||
			(Ar.IsLoading() && int64(Count) * 32 > Ar.TotalSize() - Ar.Tell()))
		{
			return false;
		}

		S.Clock.Pending.SetNum(Count);
		for (auto& P : S.Clock.Pending)
		{
			Ar << P.GameSeconds << P.GameToSeconds << P.WorldToSeconds << P.Scale;
		}

		Count = S.Clock.Rates.Num();
		Ar << Count;
		if (Ar.IsError() || Count < 1 || Count > 4096 ||
			(Ar.IsLoading() && int64(Count) * 24 > Ar.TotalSize() - Ar.Tell()))
		{
			return false;
		}

		S.Clock.Rates.SetNum(Count);
		for (auto& R : S.Clock.Rates)
		{
			Ar << R.GameSeconds << R.WorldSeconds << R.Scale;
		}

		Count = S.Life.Num();
		Ar << Count;
		if (Ar.IsError() || Count < 32 || Count > MaxBytes - 128 ||
			(Ar.IsLoading() && Count != Ar.TotalSize() - Ar.Tell()))
		{
			return false;
		}

		S.Life.SetNumUninitialized(Count);
		Ar.Serialize(S.Life.GetData(), Count);
		return !Ar.IsError();
	}
}

bool FCCLWorldSnapshotCodec::Capture(const FCCLWorldIdentity& Identity, const FCCLWorldClock& Clock,
	const FCCLLifeSimulation& Life, FCCLWorldSnapshot& Snapshot, FString& Error)
{
	Error.Reset();
	FCCLWorldSnapshot Candidate;
	Candidate.Identity = Identity;
	if (!Clock.Capture(Candidate.Clock, Error) || Clock.GetWorldSeconds() != Life.GetTime() || !Life.Save(Candidate.Life))
	{
		return Reject(Error, TEXT("Cannot capture inconsistent or busy clock/life state."));
	}

	if (!Validate(Candidate, Error))
	{
		return false;
	}

	Snapshot = MoveTemp(Candidate);
	return true;
}

bool FCCLWorldSnapshotCodec::Encode(const FCCLWorldSnapshot& Snapshot, TArray<uint8>& Bytes, FString& Error)
{
	if (!Validate(Snapshot, Error))
	{
		return false;
	}

	FCCLWorldSnapshot Copy = Snapshot;
	TArray<uint8> Candidate;
	FMemoryWriter Writer(Candidate, true);
	if (!Serialize(Writer, Copy) || Candidate.Num() > MaxBytes - 4)
	{
		return Reject(Error, TEXT("World snapshot exceeds its storage budget."));
	}

	const uint32 CRC = FCrc::MemCrc32(Candidate.GetData(), Candidate.Num());
	Candidate.Append(reinterpret_cast<const uint8*>(&CRC), sizeof(CRC));
	Bytes = MoveTemp(Candidate);
	return true;
}

bool FCCLWorldSnapshotCodec::Decode(const TArray<uint8>& Bytes, FCCLWorldSnapshot& Snapshot, FString& Error)
{
	Error.Reset();
	if (Bytes.Num() < 128 || Bytes.Num() > MaxBytes)
	{
		return Reject(Error, TEXT("Invalid world snapshot length."));
	}

	const int32 Size = Bytes.Num() - 4;
	uint32 CRC;
	FMemory::Memcpy(&CRC, Bytes.GetData() + Size, sizeof(CRC));
	if (CRC != FCrc::MemCrc32(Bytes.GetData(), Size))
	{
		return Reject(Error, TEXT("World snapshot checksum mismatch."));
	}

	TArray<uint8> Payload;
	Payload.Append(Bytes.GetData(), Size);
	FMemoryReader Reader(Payload, true);
	FCCLWorldSnapshot Candidate;
	if (!Serialize(Reader, Candidate) || Reader.Tell() != Size)
	{
		return Reject(Error, TEXT("Malformed world snapshot payload."));
	}

	if (!Validate(Candidate, Error))
	{
		return false;
	}

	Snapshot = MoveTemp(Candidate);
	return true;
}

bool FCCLWorldSnapshotCodec::MigrateLegacy(const TArray<uint8>& LifeBytes, ECCLWorldDomain Domain,
	FCCLWorldSnapshot& Snapshot, FString& Error)
{
	FCCLLifeSimulation Life;
	FCCLWorldClock Clock;
	if (!Life.Load(LifeBytes, Error) || !Clock.Reset(Life.GetTime(), 60, Error))
	{
		return Reject(Error, TEXT("Legacy life snapshot could not be migrated."));
	}

	FCCLWorldIdentity Identity;
	Identity.WorldId = FGuid::NewGuid();
	Identity.Domain = Domain;
	return Capture(Identity, Clock, Life, Snapshot, Error);
}

bool FCCLWorldSnapshotCodec::Restore(const FCCLWorldSnapshot& Snapshot, ECCLWorldDomain ExpectedDomain,
	FCCLWorldClock& Clock, FCCLLifeSimulation& Life, FString& Error)
{
	if (Snapshot.Identity.Domain != ExpectedDomain)
	{
		return Reject(Error, TEXT("World snapshot belongs to a different play domain."));
	}

	FCCLWorldClock CandidateClock;
	if (!Validate(Snapshot, Error) || !CandidateClock.Restore(Snapshot.Clock, Error) || !Life.Load(Snapshot.Life, Error))
	{
		return false;
	}

	// Life.Load validates before replacing its store; publication below cannot fail.
	Clock = MoveTemp(CandidateClock);
	return true;
}

bool FCCLWorldSnapshotCodec::Validate(const FCCLWorldSnapshot& Snapshot, FString& Error)
{
	Error.Reset();
	if (Snapshot.Schema != 1 || Snapshot.Identity.BaseWorldVersion != 1 || !Snapshot.Identity.WorldId.IsValid() ||
		Snapshot.Identity.Generation == 0 || static_cast<uint8>(Snapshot.Identity.Domain) > static_cast<uint8>(ECCLWorldDomain::Scenario) ||
		Snapshot.Life.Num() < 32 || Snapshot.Life.Num() > MaxBytes - 128)
	{
		return Reject(Error, TEXT("Unsupported world identity, version or life payload."));
	}

	FCCLWorldClock Clock;
	FCCLLifeSimulation Life;
	if (!Clock.Restore(Snapshot.Clock, Error) || !Life.Load(Snapshot.Life, Error) || Clock.GetWorldSeconds() != Life.GetTime())
	{
		return Reject(Error, TEXT("World clock and life snapshot do not agree."));
	}

	return true;
}
