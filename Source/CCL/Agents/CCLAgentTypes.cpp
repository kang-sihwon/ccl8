#include "CCLAgentTypes.h"

bool FCCLFeatureRegistry::Register(FCCLFeatureRegistration Registration)
{
	if (!Registration.Tag.IsValid() || !Registration.Type || Registration.Version < 1 ||
		!Registration.Validate || Entries.Contains(Registration.Tag))
	{
		return false;
	}

	Entries.Add(Registration.Tag, MoveTemp(Registration));
	return true;
}

bool FCCLFeatureRegistry::UpgradeAndValidate(FCCLAgentRecord& Record, FString& Error) const
{
	if (!Record.Id.IsValid() || !Record.DefinitionId.IsValid() ||
		!FMath::IsFinite(Record.LastSimulatedTime) || Record.LastSimulatedTime < 0 ||
		Record.Location.Position.ContainsNaN() || Record.Features.Num() > 64 ||
		!FMath::IsFinite(Record.Intent.StartedTime) || !FMath::IsFinite(Record.Intent.ExpireTime) ||
		Record.Intent.StartedTime < 0 || Record.Intent.ExpireTime < Record.Intent.StartedTime)
	{
		Error = TEXT("Invalid agent identity, time, location or feature count.");
		return false;
	}

	FCCLAgentRecord Candidate = Record;
	for (auto& Pair : Candidate.Features)
	{
		const auto* Entry = Entries.Find(Pair.Key);
		auto& State = Pair.Value;
		if (!Entry || State.Version < 1 || State.Version > Entry->Version || !State.Data.IsValid())
		{
			Error = TEXT("Unknown feature or unsupported feature version.");
			return false;
		}

		for (const auto Dependency : Entry->Dependencies)
		{
			if (!Candidate.Features.Contains(Dependency))
			{
				Error = TEXT("Missing required feature.");
				return false;
			}
		}

		if (State.Version < Entry->Version)
		{
			if (!Entry->Migrate || !Entry->Migrate(State.Version, State.Data))
			{
				Error = TEXT("Feature migration failed.");
				return false;
			}

			State.Version = Entry->Version;
		}

		if (State.Data.GetScriptStruct() != Entry->Type || !Entry->Validate(State.Data))
		{
			Error = TEXT("Feature type or contents failed validation.");
			return false;
		}
	}

	Record = MoveTemp(Candidate);
	Error.Reset();
	return true;
}

FCCLAgentHandle FCCLAgentStore::Add(FCCLAgentRecord Record, const FCCLFeatureRegistry& Registry, FString& Error)
{
	check(IsInGameThread());
	if (Records.Contains(Record.Id) || !Registry.UpgradeAndValidate(Record, Error))
	{
		return {};
	}

	const FGuid Id = Record.Id;
	FEntry Entry;
	Entry.Record = MoveTemp(Record);
	Entry.Generation = NextGeneration++;
	Records.Add(Id, MoveTemp(Entry));
	return GetHandle(Id);
}

FCCLAgentLease FCCLAgentStore::Acquire(FCCLAgentHandle Handle, FGuid Writer)
{
	check(IsInGameThread());
	auto* Entry = Records.Find(Handle.Id);
	if (!Entry || Entry->Generation != Handle.Generation || Entry->Writer.IsValid() || !Writer.IsValid())
	{
		return {};
	}

	Entry->Writer = Writer;
	Entry->Epoch = NextEpoch++;
	return {Handle, Writer, Entry->Epoch};
}

bool FCCLAgentStore::Commit(const FCCLAgentLease& Lease, FCCLAgentRecord Record,
	const FCCLFeatureRegistry& Registry, FString& Error)
{
	check(IsInGameThread());
	auto* Entry = Records.Find(Lease.Handle.Id);
	if (!Entry || !Lease.Writer.IsValid() || Entry->Generation != Lease.Handle.Generation ||
		Entry->Writer != Lease.Writer || Entry->Epoch != Lease.Epoch || Record.Id != Lease.Handle.Id ||
		Record.LastSimulatedTime < Entry->Record.LastSimulatedTime || !Registry.UpgradeAndValidate(Record, Error))
	{
		return false;
	}

	Entry->Record = MoveTemp(Record);
	return true;
}

bool FCCLAgentStore::Release(const FCCLAgentLease& Lease)
{
	check(IsInGameThread());
	auto* Entry = Records.Find(Lease.Handle.Id);
	if (!Entry || !Lease.Writer.IsValid() || Entry->Generation != Lease.Handle.Generation ||
		Entry->Writer != Lease.Writer || Entry->Epoch != Lease.Epoch)
	{
		return false;
	}

	Entry->Writer.Invalidate();
	Entry->Epoch = NextEpoch++;
	return true;
}

bool FCCLAgentStore::Replace(TArray<FCCLAgentRecord> InRecords, const FCCLFeatureRegistry& Registry, FString& Error)
{
	check(IsInGameThread());
	for (const auto& Pair : Records)
	{
		if (Pair.Value.Writer.IsValid())
		{
			Error = TEXT("Quiesce writers before replacing agent state.");
			return false;
		}
	}

	TSet<FGuid> Ids;
	for (auto& Record : InRecords)
	{
		if (Ids.Contains(Record.Id) || !Registry.UpgradeAndValidate(Record, Error))
		{
			return false;
		}

		Ids.Add(Record.Id);
	}

	Records.Reset();
	for (auto& Record : InRecords)
	{
		Add(MoveTemp(Record), Registry, Error);
	}

	return true;
}

bool FCCLAgentStore::Snapshot(TArray<FCCLAgentRecord>& OutRecords) const
{
	check(IsInGameThread());
	TArray<FCCLAgentRecord> Result;
	for (const auto& Pair : Records)
	{
		if (Pair.Value.Writer.IsValid())
		{
			return false;
		}

		Result.Add(Pair.Value.Record);
	}

	Result.Sort([](const FCCLAgentRecord& A, const FCCLAgentRecord& B) { return A.Id < B.Id; });
	OutRecords = MoveTemp(Result);
	return true;
}

const FCCLAgentRecord* FCCLAgentStore::Find(FCCLAgentHandle Handle) const
{
	const auto* Entry = Records.Find(Handle.Id);
	return Entry && Entry->Generation == Handle.Generation ? &Entry->Record : nullptr;
}

FCCLAgentHandle FCCLAgentStore::GetHandle(FGuid Id) const
{
	const auto* Entry = Records.Find(Id);
	return Entry ? FCCLAgentHandle{Id, Entry->Generation} : FCCLAgentHandle{};
}
