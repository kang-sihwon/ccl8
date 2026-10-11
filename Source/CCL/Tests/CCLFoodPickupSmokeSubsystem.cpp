#include "CCLFoodPickupSmokeSubsystem.h"
#include "Items/CCLInventorySnapshot.h"

#include "Items/CCLWorldPickup.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLInventoryPersistenceSubsystem.h"
#include "Items/CCLItemDefinition.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLExperimentDirector.h"
#include "Session/CCLSessionRecord.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool UCCLFoodPickupSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLFoodPickupSmoke"));
#endif
}

void UCCLFoodPickupSmokeSubsystem::Tick(float DeltaTime)
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
	auto* W = GetWorld();
	auto* D = W ? ACCLExperimentDirector::Find(W) : nullptr;
	auto* R = W ? W->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	if (!D || !D->IsReady() || !R || Now < Next)
	{
		return;
	}
	FString Directory, Error;
	FParse::Value(FCommandLine::Get(), TEXT("CCLFoodPickupSmokeDir="), Directory);
	auto Check = [&](bool OK, const TCHAR* Message)
	{
		if (!OK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s %s"), Step, Message, *Error));
		}
		return OK;
	};
	auto FindPickup = [&]() -> ACCLWorldPickup*
	{
		for (TActorIterator<ACCLWorldPickup> It(W); It; ++It)
		{
			if (FMath::Abs(It->GetActorLocation().X + 5000.f) < 10.f)
			{
				return *It;
			}
		}
		return nullptr;
	};
	if (W->GetNetMode() == NM_Client)
	{
		auto* Observed = FindPickup();
		const bool bLate = FParse::Param(FCommandLine::Get(), TEXT("CCLFoodPickupLate"));
		if (FPaths::FileExists(Directory / TEXT("collected.txt")))
		{
			if (!Observed && Step >= 2)
			{
				Finish(true, TEXT("public food history and collection destruction replicated"));
			}
			return;
		}
		if (!Observed || !Observed->GetFoodView().bReady)
		{
			return;
		}
		if (!Check(
				Observed->GetFoodInventory()->GetEntries().IsEmpty() &&
					!Observed->TryCollect(const_cast<UCCLInventoryComponent*>(Observed->GetFoodInventory()), Observed),
				TEXT("private history withheld and client collect denied")))
		{
			return;
		}
		if (!Step)
		{
			UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_PICKUP BASE_OBSERVED"));
			Step = 1;
		}
		const bool bRestored = FPaths::FileExists(Directory / TEXT("restored.txt"));
		FString Oracle;
		if (!FFileHelper::LoadFileToString(Oracle,
										   *(Directory / (bRestored ? TEXT("restored.txt") : TEXT("ready.txt")))))
		{
			return;
		}
		TArray<FString> Fields;
		Oracle.ParseIntoArray(Fields, TEXT(","));
		if (!Check(Fields.Num() == 7, TEXT("oracle")))
		{
			return;
		}
		const auto& V = Observed->GetFoodView();
		if (V.Epoch.ToString() != Fields[0] || Observed->PersistentId.ToString() != Fields[6] ||
			!Observed->GetActorLocation().Equals(FVector(-5000, -5000, 10000), .01)
			|| !Observed->GetActorRotation().Equals(FRotator(10, 20, 30), .01)
			|| !Observed->GetActorScale3D().Equals(FVector(.8, 1.2, 1.6), .001) ||
			Observed->Quantity != FCString::Atoi(*Fields[5]) ||
			!FMath::IsNearlyEqual(V.WorldSeconds, FCString::Atod(*Fields[1]), 1.e-8) ||
			!FMath::IsNearlyEqual(V.AgeSeconds, FCString::Atod(*Fields[2]), 1.e-8) ||
			!FMath::IsNearlyEqual(V.Freshness, FCString::Atod(*Fields[3]), 1.e-8) ||
			!FMath::IsNearlyEqual(V.Mold, FCString::Atod(*Fields[4]), 1.e-8))
		{
			return;
		}
		Step = bRestored ? 3 : 2;
		const TCHAR* Marker = bRestored ? (bLate ? TEXT("restored-late.txt") : TEXT("restored-first.txt"))
										: (bLate ? TEXT("late-observed.txt") : TEXT("first-observed.txt"));
		FFileHelper::SaveStringToFile(TEXT("observed"), *(Directory / Marker));
		return;
	}
	if (!R->IsRunning())
	{
		return;
	}

	auto MakeInventory = [&]()
	{
		auto* Owner = W->SpawnActor<AActor>();
		auto* Root = NewObject<USceneComponent>(Owner);
		Owner->SetRootComponent(Root);
		Root->RegisterComponent();
		Owner->SetActorLocation(FVector(-5000, -5000, 10000));
		auto* Inventory = NewObject<UCCLInventoryComponent>(Owner);
		Owner->AddInstanceComponent(Inventory);
		Inventory->RegisterComponent();
		return Inventory;
	};
	auto* Agents = W->GetSubsystem<UCCLAgentWorldSubsystem>();
	const bool bRead = FParse::Param(FCommandLine::Get(), TEXT("CCLFoodPickupRead"));
	if (bRead && Step == 0)
	{
		R->SetTickableTickType(ETickableTickType::Never);
		if (!Check(FFileHelper::LoadFileToArray(GroundSave, *(Directory / TEXT("ground.bin"))) &&
					   FFileHelper::LoadFileToArray(CollectedSave, *(Directory / TEXT("collected.bin"))) &&
					   R->Restore(Agents->GetSimulation(), GroundSave, Error),
				   TEXT("reader restores ground snapshot")))
		{
			return;
		}
		Pickup = FindPickup();
		FString Encoded, Expected;
		if (!Check(Pickup && Pickup->GetFoodInventory()->FindSlot(0) &&
					   CCLFood::Encode(Pickup->GetFoodInventory()->FindSlot(0)->Food, Encoded, Error) &&
					   FFileHelper::LoadFileToString(Expected, *(Directory / TEXT("ground-food.txt"))) &&
					   Encoded == Expected,
				   TEXT("new process pickup exact food history")))
		{
			return;
		}
		FCCLWorldSnapshot Ground;
		TArray<FCCLInventorySnapshot> GroundRecords;
		if (!Check(FCCLWorldSnapshotCodec::Decode(GroundSave, Ground, Error) && CCLInventorySnapshot::ReadWorld(Ground, GroundRecords, Error),
				   TEXT("reader transform oracle")))
		{
			return;
		}

		const auto* Record = GroundRecords.FindByPredicate([&](const auto& Value) { return Value.ContainerId == Pickup->PersistentId; });
		if (!Check(Record && Pickup->GetActorTransform().Equals(Record->Transform, 1.e-6), TEXT("reader exact pickup transform")))
		{
			return;
		}

		OriginalId = Pickup->GetFoodInventory()->FindSlot(0)->Id;
		Target = MakeInventory();
		if (!Check(Target->BindPersistence(FGuid(950, 951, 952, 953), Error) && Target->GetEntries().IsEmpty(),
				   TEXT("reader empty target")))
		{
			return;
		}
		ResumeUntil = R->GetClock().GetWorldSeconds() + 10.;
		if (!Check(R->ChangeTimeScale(4., Error), TEXT("reader resume scale")))
		{
			return;
		}
		Step = 50;
		return;
	}
	if (bRead && Step == 50)
	{
		if (R->GetClock().GetWorldSeconds() < ResumeUntil)
		{
			if (!R->GetClock().HasPendingTime() && !Check(R->QueueGameTime(.25, Error), TEXT("reader queue")))
			{
				return;
			}
			if (!R->AdvancePending(.25, 1., Error) && !Check(Error.Contains(TEXT("awaits")), TEXT("reader advance")))
			{
				return;
			}
			return;
		}
		FCCLFoodQuality Quality;
		if (!Check(Pickup->GetFoodInventory()->GetFoodQuality(OriginalId, Quality) && Quality.OldestAgeSeconds >= 30. &&
					   R->Restore(Agents->GetSimulation(), CollectedSave, Error) && !FindPickup() &&
					   Target->Find(OriginalId) && Target->Find(OriginalId)->Quantity == 6,
				   TEXT("reader continues age then restores collection tombstone")))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_PICKUP CONTINUATION_MATCH"));
		Finish(true, TEXT("fresh process recreated pickup history and preserved collected absence"));
		return;
	}
	const FVector Position(-5000, -5000, 10000);
	auto Spawn = [&](UCCLItemDefinition* Item, int32 Count)
	{
		const FTransform Transform(FRotator(10, 20, 30), Position, FVector(2, 3, 4));
		auto* Actor = W->SpawnActorDeferred<ACCLWorldPickup>(ACCLWorldPickup::StaticClass(), Transform, nullptr,
															 nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Actor->Definition = Item;
		Actor->Quantity = Count;
		Actor->bAlwaysRelevant = true;
		Actor->FinishSpawning(Transform);
		return Actor;
	};
	auto* Bread = FCCLSessionCodec::Item(TEXT("DA_Bread"));
	if (!Check(Bread != nullptr, TEXT("bread asset")))
	{
		return;
	}
	if (!Step)
	{
		R->SetTickableTickType(ETickableTickType::Never);
		if (!Check(R->ChangeTimeScale(4., Error), TEXT("set clock")))
		{
			return;
		}
		Pickup = Spawn(Bread, 4);
		const auto* Entry = Pickup->GetFoodInventory()->FindSlot(0);
		if (!Check(Entry && Entry->Food.Lots.Num() == 1, TEXT("food born at spawn")))
		{
			return;
		}
		Created = Entry->Food.Lots[0].CreatedWorldSeconds;
		OriginalId = Entry->Id;
		OriginalOrigin = Entry->Food.Lots[0].OriginId;
		Target = MakeInventory();
		SavedPickupId = Pickup->PersistentId;
		SavedPickupTransform = Pickup->GetActorTransform();
		if (!Check(SavedPickupId.IsValid() && Target->BindPersistence(FGuid(950, 951, 952, 953), Error),
				   TEXT("persistent pickup and target")))
		{
			return;
		}
		Step = 1;
		UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_PICKUP BASE_READY"));
		return;
	}
	if (Step == 1)
	{
		if (W->GetNetMode() != NM_Standalone && !FPaths::FileExists(Directory / TEXT("base-observed.txt")))
		{
			return;
		}
		if (R->GetClock().GetWorldSeconds() < Created + 20.)
		{
			if (!R->GetClock().HasPendingTime() && !Check(R->QueueGameTime(.25, Error), TEXT("queue")))
			{
				return;
			}
			if (!R->AdvancePending(.25, 1., Error) && !Check(Error.Contains(TEXT("awaits")), TEXT("advance")))
			{
				return;
			}
			return;
		}
		if (!Check(R->ChangeTimeScale(0., Error), TEXT("pause")))
		{
			return;
		}
		Step = 2;
		Next = Now + .5;
		return;
	}
	if (Step == 2)
	{
		const auto& V = Pickup->GetFoodView();
		if (!Check(V.bReady && V.AgeSeconds >= 20. && V.Freshness < 1., TEXT("age before collection")))
		{
			return;
		}
		auto* Full = MakeInventory();
		Full->Capacity = 1;
		const auto* Before = Pickup->GetFoodInventory()->FindSlot(0);
		FString BeforeFood, AfterFood;
		CCLFood::Encode(Before->Food, BeforeFood, Error);
		if (!Check(Full->Add(FCCLSessionCodec::Item(TEXT("DA_Bullets")), 1).IsValid() &&
					   !Pickup->TryCollect(Full, Full->GetOwner()),
				   TEXT("full inventory refused")))
		{
			return;
		}
		Full->GetOwner()->Destroy();
		Target->GetOwner()->SetActorLocation(Position + FVector(500, 0, 0));
		if (!Check(!Pickup->TryCollect(Target, Target->GetOwner()), TEXT("distant collect refused")))
		{
			return;
		}
		Target->GetOwner()->SetActorLocation(Position - FVector(200, 0, 0));
		auto* Wall = W->SpawnActor<AActor>();
		auto* Box = NewObject<UBoxComponent>(Wall);
		Wall->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(20, 100, 100));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Ignore);
		Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Box->RegisterComponent();
		Wall->SetActorLocation(Position - FVector(100, 0, 0));
		if (!Check(!Pickup->TryCollect(Target, Target->GetOwner()), TEXT("occluded collection refused")))
		{
			return;
		}
		Wall->Destroy();
		Target->GetOwner()->SetActorLocation(Position);
		if (!Check(!Pickup->InitializeFood(Bread, Before->Food, OriginalId, Error),
				   TEXT("repeat initialization refused")))
		{
			return;
		}
		CCLFood::Encode(Pickup->GetFoodInventory()->FindSlot(0)->Food, AfterFood, Error);
		if (!Check(BeforeFood == AfterFood, TEXT("failed collection preserves history")))
		{
			return;
		}
		TArray<FCCLInventorySnapshot> Records;
		if (!Check(W->GetSubsystem<UCCLInventoryPersistenceSubsystem>()->Capture(
					   R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), Records, Error) &&
					   R->Save(Agents->GetSimulation(), GroundSave, Error) &&
					   FFileHelper::SaveArrayToFile(GroundSave, *(Directory / TEXT("ground.bin"))) &&
					   FFileHelper::SaveStringToFile(BeforeFood, *(Directory / TEXT("ground-food.txt"))),
				   TEXT("registered ground pickup saves with world")))
		{
			return;
		}
		const FString Oracle =
			FString::Printf(TEXT("%s,%.12f,%.12f,%.12f,%.12f,%d,%s"), *V.Epoch.ToString(), V.WorldSeconds, V.AgeSeconds,
							V.Freshness, V.Mold, Pickup->Quantity, *Pickup->PersistentId.ToString());
		if (!Check(FFileHelper::SaveStringToFile(Oracle, *(Directory / TEXT("ready.txt"))), TEXT("save oracle")))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_PICKUP READY"));
		Step = 3;
		return;
	}
	if (Step == 3)
	{
		if (W->GetNetMode() != NM_Standalone && (!FPaths::FileExists(Directory / TEXT("first-observed.txt")) ||
												 !FPaths::FileExists(Directory / TEXT("late-observed.txt"))))
		{
			return;
		}
		bool bReentered = false;
		const auto Handle =
			Target->OnChanged.AddLambda([&]() { bReentered |= Pickup->TryCollect(Target, Target->GetOwner()); });
		const bool bCollected = Pickup->TryCollect(Target, Target->GetOwner());
		Target->OnChanged.Remove(Handle);
		const auto* Entry = Target->Find(OriginalId);
		if (!Check(bCollected && !bReentered && Entry && Entry->Food.Lots.Num() == 1 &&
					   Entry->Food.Lots[0].OriginId == OriginalOrigin &&
					   Entry->Food.Lots[0].CreatedWorldSeconds == Created && Entry->Food.Lots[0].SpoilageSeconds > 0. &&
					   !Pickup->TryCollect(Target, Target->GetOwner()),
				   TEXT("collect retains item and lot history exactly once")))
		{
			return;
		}
		auto Future = Entry->Food;
		Future.UpdatedWorldSeconds = R->GetClock().GetWorldSeconds() + 1.;
		const FTransform Transform(Position);
		auto* Invalid = W->SpawnActorDeferred<ACCLWorldPickup>(ACCLWorldPickup::StaticClass(), Transform);
		if (!Check(!Invalid->InitializeFood(Bread, Future, FGuid::NewGuid(), Error), TEXT("future history refused")))
		{
			return;
		}
		Invalid->Destroy();
		if (!Check(Target->Add(Bread, 2).IsValid(), TEXT("new batch merges without losing old batch")))
		{
			return;
		}
		const double OldExposure = Target->Find(OriginalId)->Food.Lots[0].SpoilageSeconds;
		auto* Partial = ACCLWorldPickup::DropFood(Target, OriginalId, 3, Transform, Error);
		if (!Check(Partial && Target->Find(OriginalId)->Quantity == 3, TEXT("partial drop")))
		{
			return;
		}
		const auto* Dropped = Partial->GetFoodInventory()->FindSlot(0);
		if (!Check(Dropped->Id != OriginalId && Dropped->Food.Lots.Num() == 1 &&
					   Dropped->Food.Lots[0].OriginId == OriginalOrigin &&
					   Dropped->Food.Lots[0].SpoilageSeconds == OldExposure &&
					   !ACCLWorldPickup::DropFood(Target, OriginalId, 0, Transform, Error) &&
					   Partial->TryCollect(Target, Target->GetOwner()) && Target->Find(OriginalId)->Quantity == 6,
				   TEXT("split origin and exposure survive drop and merge")))
		{
			return;
		}
		FCCLFoodStorage Cold;
		Cold.TemperatureCelsius = -5.;
		if (!Check(Target->ChangeFoodStorage(OriginalId, Cold), TEXT("cold storage before drop")))
		{
			return;
		}
		auto* Whole = ACCLWorldPickup::DropFood(Target, OriginalId, 6, Transform, Error);
		if (!Check(Whole && Target->GetEntries().IsEmpty() &&
					   Whole->GetFoodInventory()->FindSlot(0)->Id == OriginalId &&
					   Whole->GetFoodInventory()->FindSlot(0)->Food.Storage == FCCLFoodStorage() &&
					   Whole->GetFoodInventory()->FindSlot(0)->Food.Lots[0].SpoilageSeconds == OldExposure &&
					   Whole->TryCollect(Target, Target->GetOwner()) && Target->Find(OriginalId)->Food.Lots.Num() == 2,
				   TEXT("whole drop retains item identity and mixed lots")))
		{
			return;
		}
		auto* Generic = Spawn(FCCLSessionCodec::Item(TEXT("DA_Bullets")), 2);
		if (!Check(Generic->TryCollect(Target, Target->GetOwner()), TEXT("ordinary pickup regression")))
		{
			return;
		}
		// The same target owner participates in both ground and collected world snapshots.
		if (!Check(Target->BindPersistence(FGuid(950, 951, 952, 953), Error), TEXT("bind collected history")))
		{
			return;
		}
		Step = 4;
		Next = Now + .5;
		return;
	}
	if (Step == 4)
	{
		TArray<FCCLInventorySnapshot> Records, Decoded;
		TArray<uint8> Bytes;
		auto* Store = W->GetSubsystem<UCCLInventoryPersistenceSubsystem>();
		if (!Check(Store->Capture(R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), Records, Error) &&
					   CCLInventorySnapshot::Encode(Records, R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(),
													Bytes, Error) &&
					   CCLInventorySnapshot::Decode(Bytes, R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(),
													Decoded, Error),
				   TEXT("collected history persists in container snapshot")))
		{
			return;
		}
		const auto* Record =
			Decoded.FindByPredicate([](const auto& Value) { return Value.ContainerId == FGuid(950, 951, 952, 953); });
		FCCLFoodState Food;
		const auto* Item =
			Record ? Record->Items.FindByPredicate([&](const auto& Value) { return Value.Id == OriginalId; }) : nullptr;
		if (!Check(Item && CCLFood::Decode(Item->Food, Food, Error) && Food.Lots.Num() == 2 &&
					   Food.Lots[0].OriginId == OriginalOrigin && Food.Lots[0].CreatedWorldSeconds == Created,
				   TEXT("snapshot preserved mixed pickup history")))
		{
			return;
		}
		if (!Check(R->Save(Agents->GetSimulation(), CollectedSave, Error) &&
					   FFileHelper::SaveArrayToFile(CollectedSave, *(Directory / TEXT("collected.bin"))),
				   TEXT("save collection tombstones")))
		{
			return;
		}
		FCCLSimulationSnapshot Life;
		if (!Check(Agents->GetSimulation().Capture(Life) && !Life.Agents.IsEmpty(), TEXT("lease fixture")))
		{
			return;
		}
		auto& AgentStore = Agents->GetSimulation().GetAgents();
		const auto Lease = AgentStore.Acquire(AgentStore.GetHandle(Life.Agents[0].Id), FGuid::NewGuid());
		const bool Rejected = Lease.Writer.IsValid() && !R->Restore(Agents->GetSimulation(), GroundSave, Error);
		const bool Released = AgentStore.Release(Lease);
		if (!Check(Rejected && Released && !FindPickup() && !Target->IsTransactionOpen() &&
					   Target->Find(OriginalId)->Quantity == 6,
				   TEXT("life failure discards candidate pickup and preserves inventory")))
		{
			return;
		}
		if (!Check(R->Restore(Agents->GetSimulation(), GroundSave, Error), TEXT("restore ground generation")))
		{
			return;
		}
		Pickup = FindPickup();
		if (!Check(Pickup && Pickup->PersistentId == SavedPickupId && Target->GetEntries().IsEmpty() &&
					   Pickup->GetFoodInventory()->Find(OriginalId) && Pickup->GetActorTransform().Equals(SavedPickupTransform, 1.e-6),
				   TEXT("pickup identity position and empty target restored atomically")))
		{
			return;
		}
		Pickup->bAlwaysRelevant = true;
		Step = 5;
		Next = Now + .5;
		return;
	}
	if (Step == 5)
	{
		const auto& V = Pickup->GetFoodView();
		const FString Oracle =
			FString::Printf(TEXT("%s,%.12f,%.12f,%.12f,%.12f,%d,%s"), *V.Epoch.ToString(), V.WorldSeconds, V.AgeSeconds,
							V.Freshness, V.Mold, Pickup->Quantity, *Pickup->PersistentId.ToString());
		if (!FPaths::FileExists(Directory / TEXT("restored.txt")) &&
			!Check(FFileHelper::SaveStringToFile(Oracle, *(Directory / TEXT("restored.txt"))), TEXT("restored oracle")))
		{
			return;
		}
		if (W->GetNetMode() != NM_Standalone && (!FPaths::FileExists(Directory / TEXT("restored-first.txt")) ||
												 !FPaths::FileExists(Directory / TEXT("restored-late.txt"))))
		{
			return;
		}
		FCCLSimulationSnapshot Life;
		if (!Check(Agents->GetSimulation().Capture(Life) && !Life.Agents.IsEmpty(), TEXT("live pickup lease fixture")))
		{
			return;
		}
		auto& AgentStore = Agents->GetSimulation().GetAgents();
		const auto Lease = AgentStore.Acquire(AgentStore.GetHandle(Life.Agents[0].Id), FGuid::NewGuid());
		const bool Rejected = Lease.Writer.IsValid() && !R->Restore(Agents->GetSimulation(), CollectedSave, Error);
		const bool Released = AgentStore.Release(Lease);
		if (!Check(Rejected && Released && FindPickup() == Pickup && Target->GetEntries().IsEmpty() &&
					   !Pickup->GetFoodInventory()->IsTransactionOpen(),
				   TEXT("failed restore preserves original live pickup")))
		{
			return;
		}
		FCCLWorldSnapshot WrongOwner;
		TArray<uint8> WrongBytes;
		TArray<FCCLInventorySnapshot> WrongRecords;
		if (!Check(FCCLWorldSnapshotCodec::Decode(GroundSave, WrongOwner, Error) && CCLInventorySnapshot::ReadWorld(WrongOwner, WrongRecords, Error),
				   TEXT("owner fixture decode")))
		{
			return;
		}
		auto* Wrong = WrongRecords.FindByPredicate([&](const auto& Value)
															 { return Value.ContainerId == SavedPickupId; });
		if (!Check(Wrong != nullptr, TEXT("pickup owner fixture")))
		{
			return;
		}
		Wrong->Owner = ECCLInventoryOwner::Container;
		Wrong->Transform = FTransform::Identity;
		if (!Check(CCLInventorySnapshot::WriteWorld(WrongOwner, WrongRecords, Error) &&
					   FCCLWorldSnapshotCodec::Encode(WrongOwner, WrongBytes, Error) &&
					   !R->Restore(Agents->GetSimulation(), WrongBytes, Error) && FindPickup() == Pickup,
				   TEXT("owner kind mismatch preserves current actor")))
		{
			return;
		}
		Pickup->Destroy();
		const FTransform DifferentPose(Position + FVector(300, 0, 0));
		auto* Streamed = W->SpawnActorDeferred<ACCLWorldPickup>(ACCLWorldPickup::StaticClass(), DifferentPose);
		Streamed->PersistentId = SavedPickupId;
		Streamed->Definition = Bread;
		Streamed->Quantity = 1;
		Streamed->FinishSpawning(DifferentPose);
		if (!Check(Streamed->GetActorTransform().Equals(SavedPickupTransform, 1.e-6) && Streamed->Quantity == 4
			&& Streamed->GetFoodInventory()->Find(OriginalId), TEXT("same persistent owner reload restores stored pose and contents")))
		{
			return;
		}

		Streamed->Destroy();
		TArray<uint8> Unloaded;
		if (!Check(!FindPickup() && R->Save(Agents->GetSimulation(), Unloaded, Error) &&
					   FFileHelper::SaveArrayToFile(Unloaded, *(Directory / TEXT("unloaded.bin"))) &&
					   R->Restore(Agents->GetSimulation(), Unloaded, Error),
				   TEXT("unloaded pickup remains in world snapshot")))
		{
			return;
		}
		Pickup = FindPickup();
		if (!Check(Pickup && Pickup->PersistentId == SavedPickupId && Pickup->GetFoodInventory()->Find(OriginalId)
			&& Pickup->GetActorTransform().Equals(SavedPickupTransform, 1.e-6),
				   TEXT("unloaded pickup recreated with original identity")))
		{
			return;
		}
		if (!Check(R->Restore(Agents->GetSimulation(), CollectedSave, Error) && !FindPickup() &&
					   Target->Find(OriginalId) && Target->Find(OriginalId)->Quantity == 6,
				   TEXT("collected snapshot removes restored ground actor without duplication")))
		{
			return;
		}
		auto* Seed = W->SpawnActorDeferred<ACCLWorldPickup>(ACCLWorldPickup::StaticClass(), FTransform(Position));
		Seed->PersistentId = SavedPickupId;
		Seed->Definition = Bread;
		Seed->Quantity = 4;
		Seed->FinishSpawning(FTransform(Position));
		if (!Check(Seed->IsActorBeingDestroyed() && !FindPickup(),
				   TEXT("collected persistent seed cannot respawn food")))
		{
			return;
		}
		FFileHelper::SaveStringToFile(TEXT("collected"), *(Directory / TEXT("collected.txt")));
		Finish(true, TEXT("pickup world save restore rollback collection and network verified"));
	}
}

void UCCLFoodPickupSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_PICKUP %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!bSuccess || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLFoodPickupSmoke"));
	}
}

TStatId UCCLFoodPickupSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLFoodPickupSmokeSubsystem, STATGROUP_Tickables);
}
