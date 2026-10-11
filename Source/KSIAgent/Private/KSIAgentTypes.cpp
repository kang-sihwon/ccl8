#include "KSIAgentTypes.h"

bool FKSIFeatureRegistry::Register(FKSIFeatureRegistration Registration)
{
	if (!Registration.Tag.IsValid() || !Registration.Type || Registration.Version < 1 ||
		!Registration.Validate || Entries.Contains(Registration.Tag))
	{
		return false;
	}

	Entries.Add(Registration.Tag, MoveTemp(Registration));
	return true;
}

bool FKSIFeatureRegistry::UpgradeAndValidate(FKSIAgentRecord& Record, FString& Error) const
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

	FKSIAgentRecord Candidate = Record;
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

FKSIAgentHandle FKSIAgentStore::Add(FKSIAgentRecord Record, const FKSIFeatureRegistry& Registry, FString& Error)
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

FKSIAgentLease FKSIAgentStore::Acquire(FKSIAgentHandle Handle, FGuid Writer)
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

bool FKSIAgentStore::Commit(const FKSIAgentLease& Lease, FKSIAgentRecord Record,
	const FKSIFeatureRegistry& Registry, FString& Error)
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

bool FKSIAgentStore::CommitBatch(TArray<TPair<FKSIAgentLease, FKSIAgentRecord>> Updates,
	const FKSIFeatureRegistry& Registry, FString& Error)
{
	check(IsInGameThread());
	TSet<FGuid> Seen;
	for (auto& Update : Updates)
	{
		const auto& Lease = Update.Key;
		auto& Record = Update.Value;
		const auto* Entry = Records.Find(Lease.Handle.Id);
		if (!Entry || Seen.Contains(Record.Id) || !Lease.Writer.IsValid() || Entry->Generation != Lease.Handle.Generation ||
			Entry->Writer != Lease.Writer || Entry->Epoch != Lease.Epoch || Record.Id != Lease.Handle.Id ||
			Record.LastSimulatedTime < Entry->Record.LastSimulatedTime || !Registry.UpgradeAndValidate(Record, Error))
		{
			return false;
		}

		Seen.Add(Record.Id);
	}

	for (auto& Update : Updates)
	{
		Records[Update.Key.Handle.Id].Record = MoveTemp(Update.Value);
	}

	return true;
}

bool FKSIAgentStore::Release(const FKSIAgentLease& Lease)
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

bool FKSIAgentStore::Replace(TArray<FKSIAgentRecord> InRecords, const FKSIFeatureRegistry& Registry, FString& Error)
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

bool FKSIAgentStore::Snapshot(TArray<FKSIAgentRecord>& OutRecords) const
{
	check(IsInGameThread());
	TArray<FKSIAgentRecord> Result;
	for (const auto& Pair : Records)
	{
		if (Pair.Value.Writer.IsValid())
		{
			return false;
		}

		Result.Add(Pair.Value.Record);
	}

	Result.Sort([](const FKSIAgentRecord& A, const FKSIAgentRecord& B) { return A.Id < B.Id; });
	OutRecords = MoveTemp(Result);
	return true;
}

const FKSIAgentRecord* FKSIAgentStore::Find(FKSIAgentHandle Handle) const
{
	const auto* Entry = Records.Find(Handle.Id);
	return Entry && Entry->Generation == Handle.Generation ? &Entry->Record : nullptr;
}

FKSIAgentHandle FKSIAgentStore::GetHandle(FGuid Id) const
{
	const auto* Entry = Records.Find(Id);
	return Entry ? FKSIAgentHandle{Id, Entry->Generation} : FKSIAgentHandle{};
}
