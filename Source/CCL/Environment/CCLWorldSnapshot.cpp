#include "CCLWorldSnapshot.h"

#include "Agents/CCLAgentBiology.h"
#include "Agents/CCLLifeSimulation.h"
#include "CCLSurfaceSimulation.h"
#include "CCLEcologySimulation.h"
#include "CCLWorldParticipant.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr uint32 Magic = 0x574C4343; // CCLW
	constexpr int32 MaxBytes = 40 * 1024 * 1024;
	constexpr int32 MaxSections = 32;
	constexpr int32 MaxSectionBytes = 16 * 1024 * 1024;
	const FName LegacyInventorySection(TEXT("Inventories"));
	const FName LegacyPlayerSection(TEXT("Players"));

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
		if (Ar.IsError() || Tag != Magic || (S.Schema < 1 || S.Schema > 7))
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

		if (S.Schema >= 2)
		{
			TArray<uint8> EnvironmentBytes;
			FString Error;
			if (Ar.IsSaving() && !FCCLEnvironmentInputsCodec::Encode(S.Environment, EnvironmentBytes, Error))
			{
				return false;
			}

			int32 EnvironmentCount = EnvironmentBytes.Num();
			Ar << EnvironmentCount;
			if (Ar.IsError() || EnvironmentCount < 64 || EnvironmentCount > 4 * 1024 * 1024
				|| (Ar.IsLoading() && EnvironmentCount > Ar.TotalSize() - Ar.Tell()))
			{
				return false;
			}

			EnvironmentBytes.SetNumUninitialized(EnvironmentCount);
			Ar.Serialize(EnvironmentBytes.GetData(), EnvironmentCount);
			if (Ar.IsError() || (Ar.IsLoading() && !FCCLEnvironmentInputsCodec::Decode(EnvironmentBytes, S.Environment, Error)))
			{
				return false;
			}
		}

		if (S.Schema >= 3)
		{
			Count = S.Surface.Num();
			Ar << Count;
			if (Ar.IsError() || Count < 56 || Count > FCCLSurfaceSimulation::MaximumBytes
				|| (Ar.IsLoading() && Count > Ar.TotalSize() - Ar.Tell()))
			{
				return false;
			}
			S.Surface.SetNumUninitialized(Count);
			Ar.Serialize(S.Surface.GetData(), Count);
		}

		if (S.Schema >= 4)
		{
			Count = S.Ecology.Num();
			Ar << Count;
			if (Ar.IsError() || Count < 60 || Count > FCCLEcologySimulation::MaximumBytes
				|| (Ar.IsLoading() && Count > Ar.TotalSize() - Ar.Tell()))
			{
				return false;
			}

			S.Ecology.SetNumUninitialized(Count);
			Ar.Serialize(S.Ecology.GetData(), Count);
		}

		if (S.Schema == 5 || S.Schema == 6)
		{
			// Schemas 5 and 6 stored the inventory and player blobs at fixed positions. They become named sections
			// without being decoded here; their owners validate them like any other section.
			const FName LegacyNames[] = {LegacyInventorySection, LegacyPlayerSection};
			if (!Ar.IsLoading())
			{
				return false;
			}

			for (int32 Index = 0; Index < static_cast<int32>(S.Schema) - 4; ++Index)
			{
				int32 SectionCount = 0;
				Ar << SectionCount;
				if (Ar.IsError() || SectionCount < 10 || SectionCount > MaxSectionBytes || SectionCount > Ar.TotalSize() - Ar.Tell())
				{
					return false;
				}

				auto& Section = S.Sections.AddDefaulted_GetRef();
				Section.Name = LegacyNames[Index];
				Section.Bytes.SetNumUninitialized(SectionCount);
				Ar.Serialize(Section.Bytes.GetData(), SectionCount);
			}
		}
		else if (S.Schema >= 7)
		{
			int32 SectionCount = S.Sections.Num();
			Ar << SectionCount;
			if (Ar.IsError() || SectionCount < 0 || SectionCount > MaxSections)
			{
				return false;
			}

			S.Sections.SetNum(SectionCount);
			for (auto& Section : S.Sections)
			{
				FString Name = Section.Name.ToString();
				Ar << Name;
				int32 Length = Section.Bytes.Num();
				Ar << Length;
				if (Ar.IsError() || Name.IsEmpty() || Name.Len() > 64 || Length < 0 || Length > MaxSectionBytes ||
					(Ar.IsLoading() && Length > Ar.TotalSize() - Ar.Tell()))
				{
					return false;
				}

				Section.Name = FName(*Name);
				Section.Bytes.SetNumUninitialized(Length);
				Ar.Serialize(Section.Bytes.GetData(), Length);
			}
		}

		if (Ar.IsError())
		{
			return false;
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
	const FCCLLifeSimulation& Life, FCCLWorldSnapshot& Snapshot, FString& Error, const FCCLEnvironmentInputs* Environment, const FCCLSurfaceSimulation* Surface,
	const FCCLEcologySimulation* Ecology)
{
	Error.Reset();
	FCCLWorldSnapshot Candidate;
	Candidate.Identity = Identity;
	if (Environment)
	{
		Candidate.Environment = *Environment;
	}

	if (!Clock.Capture(Candidate.Clock, Error) || Clock.GetWorldSeconds() != Life.GetTime() || !Life.Save(Candidate.Life))
	{
		return Reject(Error, TEXT("Cannot capture inconsistent or busy clock/life state."));
	}

	FCCLSurfaceSimulation EmptySurface;
	if (!Surface)
	{
		if (!EmptySurface.Initialize(Identity.WorldId, Clock.GetGameSeconds(), Clock.GetWorldSeconds(), Clock.GetCompletedStepId(), Error))
		{
			return false;
		}
		Surface = &EmptySurface;
	}
	if (!Surface->Capture(Candidate.Surface, Error))
	{
		return false;
	}

	FCCLEcologySimulation EmptyEcology;
	if (!Ecology)
	{
		if (!EmptyEcology.Initialize(Identity.WorldId, Clock.GetWorldSeconds(), Error))
		{
			return false;
		}

		Ecology = &EmptyEcology;
	}

	if (!Ecology->Capture(Candidate.Ecology, Error))
	{
		return false;
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

	// Schema 1 had no environment. Its constructor-supplied default is the explicit migration.
	Candidate.bEnvironmentMigrated = Candidate.Schema == 1;
	Candidate.bSurfaceMigrated = Candidate.Schema < 3;
	if (Candidate.Schema < 3)
	{
		FCCLSurfaceSimulation EmptySurface;
		if (!EmptySurface.Initialize(Candidate.Identity.WorldId, Candidate.Clock.GameSeconds,
			Candidate.Clock.WorldSeconds, Candidate.Clock.CompletedStepId, Error) || !EmptySurface.Capture(Candidate.Surface, Error))
		{
			return false;
		}
	}
	Candidate.bEcologyMigrated = Candidate.Schema < 4;
	if (Candidate.bEcologyMigrated)
	{
		FCCLEcologySimulation EmptyEcology;
		if (!EmptyEcology.Initialize(Candidate.Identity.WorldId, Candidate.Clock.WorldSeconds, Error)
			|| !EmptyEcology.Capture(Candidate.Ecology, Error))
		{
			return false;
		}
	}

	Candidate.Sections.Sort([](const FCCLWorldSection& A, const FCCLWorldSection& B)
	{
		return A.Name.LexicalLess(B.Name);
	});
	Candidate.Schema = 7;
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
	FCCLWorldClock& Clock, FCCLLifeSimulation& Life, FString& Error, FCCLSurfaceSimulation* Surface,
	FCCLEcologySimulation* Ecology)
{
	if (Snapshot.Identity.Domain != ExpectedDomain)
	{
		return Reject(Error, TEXT("World snapshot belongs to a different play domain."));
	}

	FCCLWorldClock CandidateClock;
	FCCLSurfaceSimulation CandidateSurface;
	FCCLEcologySimulation CandidateEcology;
	if (!Validate(Snapshot, Error) || !CandidateClock.Restore(Snapshot.Clock, Error)
		|| !CandidateSurface.Restore(Snapshot.Surface, Error) || !CandidateEcology.Restore(Snapshot.Ecology, Error)
		|| !Life.Load(Snapshot.Life, Error))
	{
		return false;
	}

	// Life.Load validates before replacing its store; publication below cannot fail.
	if (Surface)
	{
		*Surface = MoveTemp(CandidateSurface);
	}
	if (Ecology)
	{
		*Ecology = MoveTemp(CandidateEcology);
	}

	Clock = MoveTemp(CandidateClock);
	return true;
}

bool FCCLWorldSnapshotCodec::Validate(const FCCLWorldSnapshot& Snapshot, FString& Error)
{
	Error.Reset();
	if (Snapshot.Schema != 7 || Snapshot.Identity.BaseWorldVersion != 1 || !Snapshot.Identity.WorldId.IsValid() ||
		Snapshot.Identity.Generation == 0 || static_cast<uint8>(Snapshot.Identity.Domain) > static_cast<uint8>(ECCLWorldDomain::Scenario) ||
		Snapshot.Life.Num() < 32 || Snapshot.Life.Num() > MaxBytes - 128)
	{
		return Reject(Error, TEXT("Unsupported world identity, version or life payload."));
	}

	FCCLSurfaceSimulation Surface;
	if (!Surface.Restore(Snapshot.Surface, Error) || Surface.GetWorldId() != Snapshot.Identity.WorldId
		|| Surface.GetGameSeconds() != Snapshot.Clock.GameSeconds || Surface.GetWorldSeconds() != Snapshot.Clock.WorldSeconds
		|| Surface.GetStepId() != Snapshot.Clock.CompletedStepId)
	{
		return Reject(Error, TEXT("World clock and surface snapshot do not agree."));
	}

	FCCLEcologySimulation Ecology;
	if (!Ecology.Restore(Snapshot.Ecology, Error) || Ecology.GetWorldId() != Snapshot.Identity.WorldId
		|| Ecology.GetWorldSeconds() != Snapshot.Clock.WorldSeconds)
	{
		return Reject(Error, TEXT("World clock and ecology snapshot do not agree."));
	}

	FCCLCelestialSystem Celestial;
	FCCLSurfaceScene Surfaces;
	FCCLCelestialObservation Observation;
	if (!FCCLEnvironmentInputsCodec::Prepare(Snapshot.Environment, Snapshot.Clock.WorldSeconds,
		Celestial, Surfaces, Observation, Error) || !Surface.ValidateAirContext(Snapshot.Environment.Openings, Error))
	{
		return false;
	}

	FCCLWorldClock Clock;
	FCCLLifeSimulation Life;
	if (!Clock.Restore(Snapshot.Clock, Error) || !Life.Load(Snapshot.Life, Error) || Clock.GetWorldSeconds() != Life.GetTime())
	{
		return Reject(Error, TEXT("World clock and life snapshot do not agree."));
	}

	for (const auto& Pair : Ecology.GetIndividuals())
	{
		const auto* Agent = Life.Find(Pair.Key);
		if (Agent && CCLAgentBiology::Find(*Agent))
		{
			return Reject(Error, TEXT("An individual cannot have both ecology and Agent biology ownership."));
		}
	}

	if (Snapshot.Sections.Num() > MaxSections)
	{
		return Reject(Error, TEXT("World snapshot has too many sections."));
	}

	for (int32 Index = 0; Index < Snapshot.Sections.Num(); ++Index)
	{
		const auto& Section = Snapshot.Sections[Index];
		if (Section.Name.IsNone() || Section.Bytes.Num() > MaxSectionBytes || !FCCLWorldSectionRegistry::Find(Section.Name) ||
			(Index > 0 && !Snapshot.Sections[Index - 1].Name.LexicalLess(Section.Name)))
		{
			return Reject(Error, TEXT("World snapshot sections must be unique, ordered and registered."));
		}
	}

	// Sections validate in registry order and share item claims, so one item cannot live in two owners.
	TSet<FGuid> ClaimedItems;
	FCCLWorldSectionContext Context;
	Context.WorldId = Snapshot.Identity.WorldId;
	Context.WorldSeconds = Snapshot.Clock.WorldSeconds;
	Context.Surface = &Surface;
	Context.Life = &Life;
	Context.ClaimedItems = &ClaimedItems;
	for (const auto* Entry : FCCLWorldSectionRegistry::GetOrdered())
	{
		if (const auto* Bytes = Snapshot.FindSection(Entry->Name); Bytes && !Entry->Validator(Context, *Bytes, Error))
		{
			return Error.IsEmpty() ? Reject(Error, TEXT("Invalid world section.")) : false;
		}
	}

	return true;
}

// FCCLWorldSnapshot

const TArray<uint8>* FCCLWorldSnapshot::FindSection(FName Name) const
{
	const auto* Section = Sections.FindByPredicate([Name](const FCCLWorldSection& Value) { return Value.Name == Name; });
	return Section ? &Section->Bytes : nullptr;
}

void FCCLWorldSnapshot::SetSection(FName Name, TArray<uint8> Bytes)
{
	if (auto* Section = Sections.FindByPredicate([Name](const FCCLWorldSection& Value) { return Value.Name == Name; }))
	{
		Section->Bytes = MoveTemp(Bytes);
		return;
	}

	Sections.Add({Name, MoveTemp(Bytes)});
	Sections.Sort([](const FCCLWorldSection& A, const FCCLWorldSection& B)
	{
		return A.Name.LexicalLess(B.Name);
	});
}

void FCCLWorldSnapshot::RemoveSection(FName Name)
{
	Sections.RemoveAll([Name](const FCCLWorldSection& Value) { return Value.Name == Name; });
}
