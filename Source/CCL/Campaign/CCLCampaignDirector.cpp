#include "CCLCampaignDirector.h"

#include "CCLCampaignState.h"
#include "Session/CCLSessionRecord.h"
#include "CCLCharacter.h"
#include "Agents/CCLAgentComponent.h"
#include "Combat/CCLEnemyCharacter.h"
#include "Combat/CCLEnemyDefinition.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLWorldPickup.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	FCCLEncounterSpawn Encounter(const FVector& Location, const TCHAR* Definition)
	{
		FCCLEncounterSpawn Spawn;
		Spawn.Location = Location;
		Spawn.Definition = TSoftObjectPtr<UCCLEnemyDefinition>(FSoftObjectPath(Definition));
		return Spawn;
	}

	FCCLSupplySpawn Supply(const FVector& Location, const TCHAR* Item, int32 Quantity, uint32 Id)
	{
		FCCLSupplySpawn Spawn;
		Spawn.PlacementId = FGuid(0xCC190000, 0, 2, Id);
		Spawn.Location = Location;
		Spawn.Item = TSoftObjectPtr<UCCLItemDefinition>(FSoftObjectPath(Item));
		Spawn.Quantity = Quantity;
		return Spawn;
	}
}

// 생성자

ACCLCampaignDirector::ACCLCampaignDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	Guards = {
		Encounter(FVector(-100.f, -220.f, 96.f), TEXT("/Game/Progression/DA_GateGuard.DA_GateGuard")),
		Encounter(FVector(550.f, 230.f, 96.f), TEXT("/Game/Progression/DA_RoadRaider.DA_RoadRaider"))};
	Boss = Encounter(FVector(1400.f, 0.f, 96.f), TEXT("/Game/Progression/DA_GateWarden.DA_GateWarden"));
	Supplies = {
		Supply(FVector(-1550.f, -350.f, 45.f), TEXT("/Game/Progression/DA_Pistol.DA_Pistol"), 1, 1),
		Supply(FVector(-1550.f, -220.f, 45.f), TEXT("/Game/Progression/DA_Rifle.DA_Rifle"), 1, 2),
		Supply(FVector(-1550.f, -90.f, 45.f), TEXT("/Game/Progression/DA_Bullets.DA_Bullets"), 60, 3),
		Supply(FVector(-1300.f, 0.f, 55.f), TEXT("/Game/Progression/DA_IronGauntlets.DA_IronGauntlets"), 1, 4),
		Supply(FVector(-1300.f, 200.f, 55.f), TEXT("/Game/Progression/DA_RecoveryPotion.DA_RecoveryPotion"), 3, 5)};
}

// 부모 인터페이스 함수

void ACCLCampaignDirector::PostLoad()
{
	Super::PostLoad();
	// Old maps have no placement field. Only the known default layout is migratable.
	const auto* Defaults = GetDefault<ACCLCampaignDirector>();
	for (auto& Spawn : Supplies)
	{
		if (Spawn.PlacementId.IsValid())
		{
			continue;
		}

		for (const auto& Default : Defaults->Supplies)
		{
			if (Spawn.Item == Default.Item && Spawn.Location == Default.Location && Spawn.Quantity == Default.Quantity)
			{
				Spawn.PlacementId = Default.PlacementId;
				break;
			}
		}
	}
}

void ACCLCampaignDirector::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(HasAuthority());

	if (!HasAuthority())
	{
		return;
	}

	State = GetWorld()->GetGameState<ACCLCampaignState>();
	if (!State.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_CAMPAIGN missing campaign GameState"));
		return;
	}

	FString SupplyError;
	if (!ValidateSupplies(SupplyError))
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_CAMPAIGN %s"), *SupplyError);
		State->SetProgress(ECCLCampaignPhase::Error, 0);
		return;
	}

	for (const auto& Spawn : Supplies)
	{
		const FTransform Transform(FRotator::ZeroRotator, Spawn.Location);
		auto* Pickup = GetWorld()->SpawnActorDeferred<ACCLWorldPickup>(ACCLWorldPickup::StaticClass(), Transform, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Pickup)
		{
			Pickup->Definition = Spawn.Item.LoadSynchronous();
			Pickup->Quantity = Spawn.Quantity;
			Pickup->FinishSpawning(Transform);
			SupplyActors.Add(Spawn.PlacementId, Pickup);
		}
	}

	if (Guards.IsEmpty() || Guards.Num() > MaxGuards)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_CAMPAIGN road needs 1-%d guards, found %d"), MaxGuards, Guards.Num());
		State->SetProgress(ECCLCampaignPhase::Error, 0);
		return;
	}

	State->SetProgress(ECCLCampaignPhase::Village, Guards.Num());
	for (int32 Index = 0; Index < Guards.Num(); ++Index)
	{
		auto* Enemy = SpawnEnemy(Guards[Index], MakeGuardAgentId(Index));
		if (!Enemy)
		{
			State->SetProgress(ECCLCampaignPhase::Error, Guards.Num());
			return;
		}

		GuardEnemies.Add(Enemy);
	}
}

void ACCLCampaignDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const auto& Guard : GuardEnemies)
	{
		if (Guard.IsValid())
		{
			Guard->OnDefeated.RemoveAll(this);
		}
	}

	if (BossEnemy.IsValid())
	{
		BossEnemy->OnDefeated.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ACCLCampaignDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || !State.IsValid() || State->GetPhase() != ECCLCampaignPhase::Village)
	{
		return;
	}

	for (TActorIterator<ACCLCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead() && It->GetController() && It->GetActorLocation().X > RoadStartX)
		{
			State->SetProgress(ECCLCampaignPhase::Road, State->GetRemainingGuards());
			break;
		}
	}
}

// 내 클래스 함수

bool ACCLCampaignDirector::RestoreCheckpoint(uint8 Mask, bool bVictory)
{
	const uint8 AllGuards = GetAllGuardsMask();
	if (!HasAuthority() || !State.IsValid() || GuardEnemies.Num() != Guards.Num() || (Mask & ~AllGuards) != 0 ||
		(bVictory && Mask != AllGuards))
	{
		return false;
	}

	int32 Remaining = 0;
	for (int32 Index = 0; Index < GuardEnemies.Num(); ++Index)
	{
		if ((Mask & (1 << Index)) == 0)
		{
			++Remaining;
			continue;
		}

		if (GuardEnemies[Index].IsValid())
		{
			GuardEnemies[Index]->OnDefeated.RemoveAll(this);
			Defeated.Add(GuardEnemies[Index]);
			GuardEnemies[Index]->Destroy();
		}
	}

	if (bVictory)
	{
		if (BossEnemy.IsValid())
		{
			BossEnemy->OnDefeated.RemoveAll(this);
			BossEnemy->Destroy();
		}

		State->SetProgress(ECCLCampaignPhase::Victory, 0);
	}
	else if (Mask == AllGuards)
	{
		if (!BossEnemy.IsValid())
		{
			BossEnemy = SpawnEnemy(Boss, BossAgentId());
		}

		State->SetProgress(BossEnemy.IsValid() ? ECCLCampaignPhase::Boss : ECCLCampaignPhase::Error, 0);
		if (!BossEnemy.IsValid())
		{
			return false;
		}
	}
	else
	{
		State->SetProgress(Mask ? ECCLCampaignPhase::Road : ECCLCampaignPhase::Village, Remaining);
	}

	return true;
}

void ACCLCampaignDirector::NotifyEnemyDefeated(ACCLEnemyCharacter* Enemy)
{
	if (!HasAuthority() || !State.IsValid() || !IsValid(Enemy) || !Enemy->IsDead() || Defeated.Contains(Enemy) ||
		State->GetPhase() == ECCLCampaignPhase::Error || State->GetPhase() == ECCLCampaignPhase::Victory)
	{
		return;
	}

	if (Enemy == BossEnemy.Get() && State->GetPhase() == ECCLCampaignPhase::Boss)
	{
		Defeated.Add(Enemy);
		State->SetProgress(ECCLCampaignPhase::Victory, 0);
		UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN victory"));
		return;
	}

	if (!GuardEnemies.Contains(Enemy))
	{
		return;
	}

	Defeated.Add(Enemy);
	const int32 Remaining = GuardEnemies.Num() - Defeated.Num();
	State->SetProgress(ECCLCampaignPhase::Road, Remaining);
	if (Remaining == 0)
	{
		BossEnemy = SpawnEnemy(Boss, BossAgentId());
		State->SetProgress(BossEnemy.IsValid() ? ECCLCampaignPhase::Boss : ECCLCampaignPhase::Error, 0);
	}
}

ACCLEnemyCharacter* ACCLCampaignDirector::SpawnEnemy(const FCCLEncounterSpawn& Spawn, FGuid AgentId)
{
	auto* Definition = Spawn.Definition.LoadSynchronous();
	if (!Definition)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_CAMPAIGN enemy definition is missing: %s"), *Spawn.Definition.ToString());
		return nullptr;
	}

	const FTransform Transform(FRotator(0.f, 180.f, 0.f), Spawn.Location);
	auto* Enemy = GetWorld()->SpawnActorDeferred<ACCLEnemyCharacter>(ACCLEnemyCharacter::StaticClass(), Transform,
		this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (!Enemy)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_CAMPAIGN enemy spawn failed"));
		return nullptr;
	}

	// Stable identities keep each encounter's Agent record across checkpoints.
	Enemy->FindComponentByClass<UCCLAgentComponent>()->AgentId = AgentId;
	Enemy->ApplyDefinition(Definition);
	Enemy->bRespawnEnabled = 0;
	Enemy->OnDefeated.AddUObject(this, &ThisClass::NotifyEnemyDefeated);
	Enemy->FinishSpawning(Transform);
	return IsValid(Enemy) ? Enemy : nullptr;
}

// Get·Set·단순 검사 중 헤더에서 정의하지 않은 함수

uint8 ACCLCampaignDirector::GetDefeatedMask() const
{
	uint8 Mask = 0;
	for (int32 Index = 0; Index < GuardEnemies.Num() && Index < MaxGuards; ++Index)
	{
		if (!GuardEnemies[Index].IsValid() || GuardEnemies[Index]->IsDead())
		{
			Mask |= 1 << Index;
		}
	}

	return Mask;
}

uint8 ACCLCampaignDirector::GetAllGuardsMask() const
{
	return static_cast<uint8>((1 << FMath::Clamp(Guards.Num(), 0, MaxGuards)) - 1);
}

FGuid ACCLCampaignDirector::MakeGuardAgentId(int32 GuardIndex)
{
	if (GuardIndex < 0 || GuardIndex >= MaxGuards)
	{
		return FGuid();
	}

	return FGuid(0xCC190000, 0, 1, GuardIndex < 2 ? GuardIndex + 1 : GuardIndex + 2);
}

bool ACCLCampaignDirector::ValidateSupplies(FString& Error) const
{
	TSet<FGuid> Ids;
	if (Supplies.Num() > 64)
	{
		Error = TEXT("Too many campaign supplies.");
		return false;
	}

	for (const auto& Spawn : Supplies)
	{
		auto* Item = Spawn.Item.LoadSynchronous();
		if (!Spawn.PlacementId.IsValid() || Ids.Contains(Spawn.PlacementId) || !Item || Spawn.Quantity < 1 ||
			Spawn.Quantity > 1000000 || Item->FindFragment<FCCLItemFragment_Perishable>())
		{
			Error = TEXT("Supply needs a unique placement ID, valid quantity and non-perishable item. Food uses world persistence.");
			return false;
		}

		Ids.Add(Spawn.PlacementId);
	}

	return true;
}

bool ACCLCampaignDirector::CaptureSupplies(TArray<FCCLSavedSupply>& OutSupplies, FString& Error) const
{
	if (!HasAuthority() || !ValidateSupplies(Error) || SupplyActors.Num() != Supplies.Num())
	{
		Error = TEXT("Campaign supply initialization is incomplete.");
		return false;
	}

	TArray<FCCLSavedSupply> Candidate;
	for (const auto& Spawn : Supplies)
	{
		const auto* Ref = SupplyActors.Find(Spawn.PlacementId);
		const auto* Pickup = Ref ? Ref->Get() : nullptr;
		if (!Pickup || Pickup->IsActorBeingDestroyed())
		{
			continue;
		}

		if (Pickup->PersistentId.IsValid() || Pickup->Definition != Spawn.Item.Get() ||
			Pickup->Quantity < 1 || Pickup->Quantity > Spawn.Quantity)
		{
			Error = TEXT("Campaign supply changed ownership, definition or quantity.");
			return false;
		}

		Candidate.Add({Spawn.PlacementId, Pickup->Definition->GetFName(), Pickup->Quantity});
	}

	OutSupplies = MoveTemp(Candidate);
	return true;
}

bool ACCLCampaignDirector::PrepareSupplyRestore(const FCCLSessionRecord& Record,
	TArray<FCCLSavedSupply>& OutSupplies, FString& Error) const
{
	if (!HasAuthority() || !ValidateSupplies(Error) || Guards.IsEmpty() || Guards.Num() > MaxGuards ||
		(Record.DefeatedGuards & ~GetAllGuardsMask()) != 0 ||
		(Record.Victory && Record.DefeatedGuards != GetAllGuardsMask()))
	{
		Error = TEXT("Campaign layout or checkpoint is invalid.");
		return false;
	}

	TArray<FCCLSavedSupply> Candidate = Record.SupplyPlacements;
	if (Record.Version < 8)
	{
		const auto* Defaults = GetDefault<ACCLCampaignDirector>();
		if (Guards.Num() > 2 || Supplies.Num() != Defaults->Supplies.Num())
		{
			Error = TEXT("Legacy checkpoint cannot identify this encounter or supply layout.");
			return false;
		}

		for (const auto& Spawn : Supplies)
		{
			const auto* Default = Defaults->Supplies.FindByPredicate([&](const FCCLSupplySpawn& Value)
			{
				return Value.PlacementId == Spawn.PlacementId;
			});
			if (!Default || Default->Item != Spawn.Item || Default->Quantity != Spawn.Quantity || Default->Location != Spawn.Location)
			{
				Error = TEXT("Legacy supply placement or quantity is ambiguous.");
				return false;
			}
		}

		for (FName Name : Record.RemainingSupplies)
		{
			const auto* Spawn = Supplies.FindByPredicate([&](const FCCLSupplySpawn& Value)
			{
				return Value.Item.Get()->GetFName() == Name;
			});
			if (!Spawn)
			{
				Error = TEXT("Legacy supply definition is not in the default layout.");
				return false;
			}

			Candidate.Add({Spawn->PlacementId, Name, Spawn->Quantity});
		}
	}

	TSet<FGuid> Ids;
	for (const auto& Saved : Candidate)
	{
		const auto* Spawn = Supplies.FindByPredicate([&](const FCCLSupplySpawn& Value)
		{
			return Value.PlacementId == Saved.PlacementId;
		});
		if (!Spawn || Ids.Contains(Saved.PlacementId) || Saved.Definition != Spawn->Item.Get()->GetFName() ||
			Saved.Quantity < 1 || Saved.Quantity > Spawn->Quantity)
		{
			Error = TEXT("Saved supply placement, definition or quantity does not match this map.");
			return false;
		}

		const auto* Actor = SupplyActors.Find(Saved.PlacementId);
		if (!Actor || !Actor->IsValid() || Actor->Get()->IsActorBeingDestroyed() || Actor->Get()->PersistentId.IsValid())
		{
			Error = TEXT("Supply restore requires a fresh initialized campaign.");
			return false;
		}

		Ids.Add(Saved.PlacementId);
	}

	OutSupplies = MoveTemp(Candidate);
	return true;
}

void ACCLCampaignDirector::ApplySupplies(const TArray<FCCLSavedSupply>& Saved)
{
	for (const auto& Pair : SupplyActors)
	{
		auto* Pickup = Pair.Value.Get();
		if (!Pickup)
		{
			continue;
		}

		const auto* Value = Saved.FindByPredicate([&](const FCCLSavedSupply& Entry)
		{
			return Entry.PlacementId == Pair.Key;
		});
		if (Value)
		{
			Pickup->Quantity = Value->Quantity;
			Pickup->ForceNetUpdate();
		}
		else
		{
			Pickup->Destroy();
		}
	}
}

#if WITH_EDITOR
void ACCLCampaignDirector::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	TSet<FGuid> Ids;
	for (auto& Spawn : Supplies)
	{
		if (!Spawn.PlacementId.IsValid() || Ids.Contains(Spawn.PlacementId))
		{
			Spawn.PlacementId = FGuid::NewGuid();
		}

		Ids.Add(Spawn.PlacementId);
	}

	Super::PostEditChangeProperty(Event);
}
#endif
