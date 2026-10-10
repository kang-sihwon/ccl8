#include "CCLFoodSmokeSubsystem.h"

#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Session/CCLSessionRecord.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Base64.h"
#include "Misc/Crc.h"

bool UCCLFoodSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLFoodSmoke"));
#endif
}

void UCCLFoodSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}

	if (Now - Started > 180.)
	{
		Finish(false, TEXT("timeout"));
		return;
	}

	auto* World = GetWorld();
	auto* Runtime = World ? World->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	auto* Agents = World ? World->GetSubsystem<UCCLAgentWorldSubsystem>() : nullptr;
	const auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	if (!Runtime || !Runtime->IsRunning() || !Agents || !Agents->IsRunning() || (Director && !Director->IsReady()) || Now < Next)
	{
		return;
	}

	FString Error, Directory;
	FParse::Value(FCommandLine::Get(), TEXT("CCLFoodSmokeDir="), Directory);
	auto Check = [&](bool bOK, const TCHAR* Message)
	{
		if (!bOK)
		{
			Finish(false, FString::Printf(TEXT("%s %s"), Message, *Error));
		}

		return bOK;
	};
	if (Step == 0)
	{
		if (!Check(Runtime->ChangeTimeScale(0., Error), TEXT("pause")))
		{
			return;
		}

		Step = 1;
		Next = Now + 2.;
		return;
	}

	auto* Definition = FCCLSessionCodec::Item(TEXT("DA_Bread"));
	if (!Check(Definition && Definition->FindFragment<FCCLItemFragment_Perishable>(), TEXT("authored bread")))
	{
		return;
	}

	auto MakeInventory = [&]()
	{
		auto* Owner = World->SpawnActor<AActor>();
		auto* Inventory = NewObject<UCCLInventoryComponent>(Owner);
		Owner->AddInstanceComponent(Inventory);
		Inventory->RegisterComponent();
		return Inventory;
	};
	auto* Inventory = MakeInventory();
	FGuid Id;
	const bool bResume = FParse::Param(FCommandLine::Get(), TEXT("CCLFoodResume"));
    const FGuid ContainerId(851, 852, 853, 854);
    if (!bResume && !Check(Inventory->BindPersistence(ContainerId, Error), TEXT("bind persistent container")))
    {
        return;
    }
	auto Advance = [&](double Seconds)
	{
		const double Before = Runtime->GetClock().GetWorldSeconds();
		if (!Runtime->ChangeTimeScale(Seconds, Error) || !Runtime->QueueGameTime(1., Error))
		{
			return false;
		}

		for (int32 I = 0; I < FMath::CeilToInt(Seconds) + 32 && Runtime->GetClock().HasPendingTime(); ++I)
		{
			if (!Runtime->AdvancePending(1., Seconds, Error))
			{
				return false;
			}
		}

		return !Runtime->GetClock().HasPendingTime() && Runtime->ChangeTimeScale(0., Error)
			&& FMath::IsNearlyEqual(Runtime->GetClock().GetWorldSeconds() - Before, Seconds, 0.000001);
	};
	if (bResume)
	{
		TArray<uint8> Saved;
		TArray<FString> Lines;
		FCCLInventoryEntry Entry;
		Entry.Definition = Definition;
		Entry.Quantity = 4;
		Entry.Slot = 0;
		if (!Check(FFileHelper::LoadFileToArray(Saved, *(Directory / TEXT("food-world.bin")))
			&& Agents->Restore(Saved, Error)
			&& FFileHelper::LoadFileToStringArray(Lines, *(Directory / TEXT("food-item.txt"))) && Lines.Num() == 2
			&& FGuid::Parse(Lines[0], Entry.Id) && CCLFood::Decode(Lines[1], Entry.Food, Error)
			&& Inventory->BindPersistence(ContainerId, Error) && Inventory->Find(Entry.Id)
            && Inventory->Find(Entry.Id)->Quantity == 4, TEXT("restore inventory from world by persistent identity")))
		{
			return;
		}

		Id = Entry.Id;
		FString Actual;
		if (!Check(CCLFood::Encode(Inventory->Find(Id)->Food, Actual, Error) && Actual == Lines[1], TEXT("exact resumed state")))
		{
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_FOOD RESTART_RESTORED world=%.6f crc=%u"), Runtime->GetClock().GetWorldSeconds(), FCrc::StrCrc32(*Actual));
		if (Runtime->GetIdentity().Domain == ECCLWorldDomain::Campaign)
		{
			FCCLSessionRecord Record;
			if (!Check(FFileHelper::LoadFileToArray(Saved, *(Directory / TEXT("food-session.bin")))
				&& FCCLSessionCodec::Decode(Saved, Record) && Record.Items.Num() == 1
				&& Record.Items[0].Food == Actual && Record.Items[0].Id == Id, TEXT("session new-process roundtrip")))
			{
				return;
			}
		}

		FCCLFoodQuality Before, After;
		if (!Check(Inventory->GetFoodQuality(Id, Before) && Advance(10.) && Inventory->GetFoodQuality(Id, After)
			&& After.Freshness < Before.Freshness && After.OldestAgeSeconds > Before.OldestAgeSeconds,
			TEXT("continued food aging")))
		{
			return;
		}
	}
	else
	{
		Id = Inventory->Add(Definition, 3);
		FCCLFoodStorage Humid;
		Humid.RelativeHumidity = 1.;
		const double Birth = Runtime->GetClock().GetWorldSeconds();
		if (!Check(Id.IsValid() && Inventory->ChangeFoodStorage(Id, Humid) && Advance(100.)
			&& Inventory->Add(Definition, 2) == Id && Inventory->Find(Id)->Food.Lots.Num() == 2,
			TEXT("real clock and new lot merge")))
		{
			return;
		}

		const FGuid Split = Inventory->SplitFood(Id, 1, 1);
		if (!Check(Split.IsValid() && Inventory->Find(Split)->Food.Lots[0].CreatedWorldSeconds == Birth
			&& Inventory->MergeFood(Split, Id), TEXT("split merge preserves birth")))
		{
			return;
		}

		auto* Target = MakeInventory();
		Target->Capacity = 1;
        if (!Check(Target->BindPersistence(FGuid(861, 862, 863, 864), Error), TEXT("bind transfer destination")))
        {
            return;
        }
		const FGuid Full = Target->Add(Definition, 20);
		FString Before, After;
		CCLFood::Encode(Inventory->Find(Id)->Food, Before, Error);
		if (!Check(Full.IsValid() && !Inventory->TransferFood(Id, 2, Target)
			&& CCLFood::Encode(Inventory->Find(Id)->Food, After, Error) && Before == After
			&& Target->Remove(Full, 20), TEXT("full destination preserves source")))
		{
			return;
		}

		bool bObserversSawFinal = true;
		const FDelegateHandle Observer = Inventory->OnChanged.AddLambda([&]()
		{
			int32 Total = 0;
			for (const auto& E : Inventory->GetEntries())
			{
				Total += E.Quantity;
			}
			for (const auto& E : Target->GetEntries())
			{
				Total += E.Quantity;
			}
			bObserversSawFinal &= Total == 5;
		});
		const bool bTransferred = Inventory->TransferFood(Id, 2, Target);
		Inventory->OnChanged.Remove(Observer);
		if (!Check(bTransferred && bObserversSawFinal && Inventory->Find(Id)->Quantity == 3
			&& Target->GetEntries().Num() == 1 && Target->GetEntries()[0].Quantity == 2
			&& Target->GetEntries()[0].Food.Lots[0].CreatedWorldSeconds == Birth, TEXT("atomic container transfer")))
		{
			return;
		}

		const FGuid TargetId = Target->GetEntries()[0].Id;
		if (!Check(Target->Remove(TargetId, 1) && Target->TransferFood(TargetId, 1, Inventory)
			&& Inventory->Find(Id)->Quantity == 4 && Target->GetEntries().IsEmpty()
			&& Inventory->MoveToSlot(Id, 3) && Inventory->MoveToSlot(Id, 0), TEXT("consume and move preserve remaining lots")))
		{
			return;
		}

		if (!Check(Inventory->TransferFood(Id, 4, Target) && Target->Find(Id)
			&& Inventory->GetEntries().IsEmpty() && Target->TransferFood(Id, 4, Inventory)
			&& Inventory->Find(Id), TEXT("whole stack transfer preserves item identity")))
		{
			return;
		}

		FCCLFoodQuality Quality;
		if (!Check(Inventory->GetFoodQuality(Id, Quality) && Quality.OldestAgeSeconds >= 100.
			&& Quality.Freshness < 1. && Quality.Freshness > 0. && Inventory->Find(Id)->Food.Lots.Num() == 2,
			TEXT("remaining quality")))
		{
			return;
		}

		TArray<uint8> Saved;
		FString Food;
		if (!Check(Runtime->Save(Agents->GetSimulation(), Saved, Error)
			&& CCLFood::Encode(Inventory->Find(Id)->Food, Food, Error)
			&& FFileHelper::SaveArrayToFile(Saved, *(Directory / TEXT("food-world.bin")))
			&& FFileHelper::SaveStringToFile(Id.ToString() + TEXT("\n") + Food, *(Directory / TEXT("food-item.txt"))), TEXT("save generation")))
		{
			return;
		}

        FCCLWorldSnapshot CapturedWorld;
        if (!Check(FCCLWorldSnapshotCodec::Decode(Saved, CapturedWorld, Error) && CapturedWorld.Inventories.FilterByPredicate([&](const auto& R) { return R.ContainerId == ContainerId || R.ContainerId == FGuid(861, 862, 863, 864); }).Num() == 2,
            TEXT("both containers captured in world envelope"))) { return; }
        const TArray<uint8> WorldSaved = Saved;
        const double SavedTime = Runtime->GetClock().GetWorldSeconds();
        if (!Check(Advance(10.) && Inventory->Remove(Id, 1) && Target->Add(Definition, 1).IsValid(), TEXT("mutate after checkpoint")))
        {
            return;
        }
        const double FutureTime = Runtime->GetClock().GetWorldSeconds();
        if (!Check(Inventory->BeginTransaction(), TEXT("open inventory transaction"))) { return; }
        const bool bRejectedBusy = !Agents->Restore(WorldSaved, Error);
        Inventory->EndTransaction(false);
        if (!Check(bRejectedBusy && Runtime->GetClock().GetWorldSeconds() == FutureTime
            && Inventory->Find(Id)->Quantity == 3 && Target->GetEntries().Num() == 1, TEXT("busy restore preserves all owners")))
        {
            return;
        }
        FCCLSimulationSnapshot Life;
        if (!Check(Agents->GetSimulation().Capture(Life) && !Life.Agents.IsEmpty(), TEXT("life snapshot for lock"))) { return; }
        auto& AgentStore = Agents->GetSimulation().GetAgents();
        const auto Lease = AgentStore.Acquire(AgentStore.GetHandle(Life.Agents[0].Id), FGuid::NewGuid());
        const bool bRejectedLife = Lease.Writer.IsValid() && !Agents->Restore(WorldSaved, Error);
        const bool bReleasedLife = AgentStore.Release(Lease);
        if (!Check(bRejectedLife && bReleasedLife && !Inventory->IsTransactionOpen() && !Target->IsTransactionOpen()
            && Inventory->Find(Id)->Quantity == 3 && Runtime->GetClock().GetWorldSeconds() == FutureTime,
            TEXT("life failure cancels prepared inventory restore"))) { return; }
        auto Corrupt = WorldSaved;
        Corrupt.Last() ^= 1;
        if (!Check(!Agents->Restore(Corrupt, Error) && Inventory->Find(Id)->Quantity == 3
            && Runtime->GetClock().GetWorldSeconds() == FutureTime, TEXT("corrupt world preserves current state"))) { return; }
        auto* Unbound = MakeInventory();
        const FGuid UnboundFood = Unbound->Add(Definition, 1);
        TArray<uint8> Unchanged = {1, 2, 3};
        if (!Check(UnboundFood.IsValid() && !Runtime->Save(Agents->GetSimulation(), Unchanged, Error)
            && Unchanged == TArray<uint8>({1, 2, 3}) && !Agents->Restore(WorldSaved, Error), TEXT("unbound food cannot escape checkpoint"))) { return; }
        Unbound->GetOwner()->Destroy();
        auto* Duplicate = MakeInventory();
        if (!Check(!Duplicate->BindPersistence(ContainerId, Error), TEXT("duplicate container owner rejected"))) { return; }
        Duplicate->GetOwner()->Destroy();
        bool bObservedFinal = false;
        const auto RestoreObserver = Inventory->OnChanged.AddLambda([&]()
        {
            bObservedFinal = Runtime->GetClock().GetWorldSeconds() == SavedTime
                && Inventory->Find(Id) && Inventory->Find(Id)->Quantity == 4 && Target->GetEntries().IsEmpty();
        });
        const bool bRestored = Agents->Restore(WorldSaved, Error);
        Inventory->OnChanged.Remove(RestoreObserver);
        if (!Check(bRestored && bObservedFinal, TEXT("clock and both containers published together"))) { return; }
        FString RestoredFood;
        if (!Check(CCLFood::Encode(Inventory->Find(Id)->Food, RestoredFood, Error) && RestoredFood == Food,
            TEXT("world restores exact food history"))) { return; }
        Inventory->GetOwner()->Destroy();
        TArray<uint8> UnloadedWorld;
        if (!Check(Runtime->Save(Agents->GetSimulation(), UnloadedWorld, Error)
            && FFileHelper::SaveArrayToFile(UnloadedWorld, *(Directory / TEXT("food-world.bin"))), TEXT("save unloaded container"))) { return; }
        auto* WrongCapacity = MakeInventory();
        WrongCapacity->Capacity = 1;
        if (!Check(!WrongCapacity->BindPersistence(ContainerId, Error), TEXT("capacity mismatch rejected"))) { return; }
        WrongCapacity->GetOwner()->Destroy();
        Inventory = MakeInventory();
        if (!Check(Inventory->BindPersistence(ContainerId, Error) && Inventory->Find(Id)
            && Inventory->Find(Id)->Quantity == 4, TEXT("unload rebind preserves contents"))) { return; }
        Saved = UnloadedWorld;

		if (Runtime->GetIdentity().Domain == ECCLWorldDomain::Campaign)
		{
			FCCLSessionRecord Record;
			Record.AgentSimulation = FBase64::Encode(Saved);
			FCCLSavedItem Item;
			Item.Id = Id;
			Item.Definition = Definition->GetFName();
			Item.Quantity = 4;
			Item.Slot = 0;
			Item.Food = Food;
            Record.Items.Add(Item);
            if (!Check(!FCCLSessionCodec::Validate(Record), TEXT("session and world reject duplicate item ownership"))) { return; }
            FCCLWorldSnapshot SessionWorld;
            TArray<uint8> SessionWorldBytes;
            if (!Check(FCCLWorldSnapshotCodec::Decode(Saved, SessionWorld, Error), TEXT("session owner separation"))) { return; }
            SessionWorld.Inventories.RemoveAll([&](const auto& R) { return R.ContainerId == ContainerId; });
            if (!Check(FCCLWorldSnapshotCodec::Encode(SessionWorld, SessionWorldBytes, Error), TEXT("legacy session-owned food envelope"))) { return; }
            Record.AgentSimulation = FBase64::Encode(SessionWorldBytes);
            FCCLSessionRecord Decoded;
			if (!Check(FCCLSessionCodec::Encode(Record, Saved) && FCCLSessionCodec::Decode(Saved, Decoded)
				&& Decoded.Items[0].Food == Food && FFileHelper::SaveArrayToFile(Saved, *(Directory / TEXT("food-session.bin"))), TEXT("session food persistence")))
			{
				return;
			}

			auto Future = Inventory->Find(Id)->Food;
			CCLFood::Advance(Future, Runtime->GetClock().GetWorldSeconds() + 1., Error);
			CCLFood::Encode(Future, Record.Items[0].Food, Error);
			if (!Check(!FCCLSessionCodec::Validate(Record), TEXT("future food rejected by session")))
			{
				return;
			}
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_FOOD SAVED world=%.6f crc=%u"), Runtime->GetClock().GetWorldSeconds(), FCrc::StrCrc32(*Food));
		Target->GetOwner()->Destroy();
	}

	Inventory->GetOwner()->Destroy();
	Finish(true, bResume ? TEXT("new process food preserved and continued") : TEXT("food merge split transfer consume and save verified"));
}

void UCCLFoodSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_FOOD %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLFoodSmoke"));
}

TStatId UCCLFoodSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLFoodSmokeSubsystem, STATGROUP_Tickables);
}
