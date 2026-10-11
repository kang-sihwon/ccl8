#include "CCLCampaignDirector.h"

#include "CCLCampaignState.h"
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

	FCCLSupplySpawn Supply(const FVector& Location, const TCHAR* Item, int32 Quantity)
	{
		FCCLSupplySpawn Spawn;
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
		Supply(FVector(-1550.f, -350.f, 45.f), TEXT("/Game/Progression/DA_Pistol.DA_Pistol"), 1),
		Supply(FVector(-1550.f, -220.f, 45.f), TEXT("/Game/Progression/DA_Rifle.DA_Rifle"), 1),
		Supply(FVector(-1550.f, -90.f, 45.f), TEXT("/Game/Progression/DA_Bullets.DA_Bullets"), 60)};
}

// 부모 인터페이스 함수

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
		}
	}

	if (Guards.IsEmpty() || Guards.Num() > MaxGuards)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_CAMPAIGN road needs 1-%d guards, found %d"), MaxGuards, Guards.Num());
		State->SetProgress(ECCLCampaignPhase::Error, 0);
		return;
	}

	State->SetProgress(ECCLCampaignPhase::Village, Guards.Num());
	for (const auto& Spawn : Guards)
	{
		auto* Enemy = SpawnEnemy(Spawn, false);
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
			BossEnemy = SpawnEnemy(Boss, true);
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
		BossEnemy = SpawnEnemy(Boss, true);
		State->SetProgress(BossEnemy.IsValid() ? ECCLCampaignPhase::Boss : ECCLCampaignPhase::Error, 0);
	}
}

ACCLEnemyCharacter* ACCLCampaignDirector::SpawnEnemy(const FCCLEncounterSpawn& Spawn, bool bBoss)
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
	Enemy->FindComponentByClass<UCCLAgentComponent>()->AgentId = FGuid(0xCC190000, 0, 1, bBoss ? 3 : GuardEnemies.Num() + 1);
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
