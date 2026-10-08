#include "CCLCampaignDirector.h"

#include "Items/CCLWorldPickup.h"
#include "Agents/CCLAgentComponent.h"
#include "Items/CCLItemDefinition.h"

#include "CCLCampaignState.h"
#include "CCLCharacter.h"
#include "Combat/CCLEnemyCharacter.h"
#include "Combat/CCLFighterComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

ACCLCampaignDirector::ACCLCampaignDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	GuardLocations = {FVector(-100.f, -220.f, 96.f), FVector(550.f, 230.f, 96.f)};
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

	const FName Supplies[] = {TEXT("DA_Pistol"), TEXT("DA_Rifle"), TEXT("DA_Bullets")};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FTransform Transform(FRotator::ZeroRotator, FVector(-1550, -350 + Index * 130, 45));
		auto* Pickup = GetWorld()->SpawnActorDeferred<ACCLWorldPickup>(ACCLWorldPickup::StaticClass(), Transform, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Pickup)
		{
			Pickup->Definition = LoadObject<UCCLItemDefinition>(nullptr,
				*FString::Printf(TEXT("/Game/Progression/%s.%s"), *Supplies[Index].ToString(), *Supplies[Index].ToString()));
			Pickup->Quantity = Index == 2 ? 60 : 1;
			Pickup->FinishSpawning(Transform);
		}
	}

	State->SetProgress(ECCLCampaignPhase::Village, GuardLocations.Num());
	for (const FVector& Location : GuardLocations)
	{
		if (auto* Enemy = SpawnEnemy(Location, false))
		{
			Guards.Add(Enemy);
		}
		else
		{
			State->SetProgress(ECCLCampaignPhase::Error, GuardLocations.Num());
			return;
		}
	}

	if (Guards.IsEmpty())
	{
		State->SetProgress(ECCLCampaignPhase::Error, 0);
	}
}

void ACCLCampaignDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const auto& Guard : Guards)
	{
		if (Guard.IsValid())
		{
			Guard->OnDefeated.RemoveAll(this);
		}
	}

	if (Boss.IsValid())
	{
		Boss->OnDefeated.RemoveAll(this);
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

void ACCLCampaignDirector::NotifyEnemyDefeated(ACCLEnemyCharacter* Enemy)
{
	if (!HasAuthority() || !State.IsValid() || !IsValid(Enemy) || !Enemy->IsDead() || Defeated.Contains(Enemy) ||
		State->GetPhase() == ECCLCampaignPhase::Error || State->GetPhase() == ECCLCampaignPhase::Victory)
	{
		return;
	}

	if (Enemy == Boss.Get() && State->GetPhase() == ECCLCampaignPhase::Boss)
	{
		Defeated.Add(Enemy);
		State->SetProgress(ECCLCampaignPhase::Victory, 0);
		UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN victory"));
		return;
	}

	if (!Guards.Contains(Enemy))
	{
		return;
	}

	Defeated.Add(Enemy);
	const int32 Remaining = Guards.Num() - Defeated.Num();
	State->SetProgress(ECCLCampaignPhase::Road, Remaining);
	if (Remaining == 0)
	{
		Boss = SpawnEnemy(BossLocation, true);
		State->SetProgress(Boss.IsValid() ? ECCLCampaignPhase::Boss : ECCLCampaignPhase::Error, 0);
	}
}

ACCLEnemyCharacter* ACCLCampaignDirector::SpawnEnemy(FVector Location, bool bBoss)
{
	const FTransform Transform(FRotator(0.f, 180.f, 0.f), Location);
	auto* Enemy = GetWorld()->SpawnActorDeferred<ACCLEnemyCharacter>(ACCLEnemyCharacter::StaticClass(), Transform,
		this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (!Enemy)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_CAMPAIGN enemy spawn failed"));
		return nullptr;
	}

	Enemy->FindComponentByClass<UCCLAgentComponent>()->AgentId = FGuid(0xCC190000, 0, 1, bBoss ? 3 : Guards.Num() + 1);
	Enemy->bRespawnEnabled = 0;
	Enemy->Archetype = bBoss ? 2 : (Guards.IsEmpty() ? 0 : 1);
	Enemy->DisplayName = bBoss ? TEXT("Gate Warden") : (Enemy->Archetype == 1 ? TEXT("Road Raider") : TEXT("Gate Guard"));
	Enemy->DetectionRadius = 650.f;
	Enemy->LeashRadius = 900.f;
	Enemy->FindComponentByClass<UCCLFighterComponent>()->InitialHealth = bBoss ? 240.f : 80.f;
	Enemy->GetCharacterMovement()->MaxWalkSpeed = bBoss ? 210.f : (Enemy->Archetype == 1 ? 330.f : 260.f);
	Enemy->OnDefeated.AddUObject(this, &ThisClass::NotifyEnemyDefeated);
	Enemy->FinishSpawning(Transform);
	return IsValid(Enemy) ? Enemy : nullptr;
}

uint8 ACCLCampaignDirector::GetDefeatedMask() const
{
	uint8 Mask = 0;
	for (int32 Index = 0; Index < Guards.Num() && Index < 2; ++Index)
	{
		if (!Guards[Index].IsValid() || Guards[Index]->IsDead()) { Mask |= 1 << Index; }
	}
	return Mask;
}
bool ACCLCampaignDirector::RestoreCheckpoint(uint8 Mask, bool bVictory)
{
	if (!HasAuthority() || !State.IsValid() || Mask > 3 || (bVictory && Mask != 3) || Guards.Num() != 2) { return false; }
	for (int32 Index = 0; Index < Guards.Num(); ++Index)
	{
		if ((Mask & (1 << Index)) != 0 && Guards[Index].IsValid())
		{
			Guards[Index]->OnDefeated.RemoveAll(this);
			Defeated.Add(Guards[Index]);
			Guards[Index]->Destroy();
		}
	}
	if (bVictory)
	{
		if (Boss.IsValid()) { Boss->OnDefeated.RemoveAll(this); Boss->Destroy(); }
		State->SetProgress(ECCLCampaignPhase::Victory, 0);
	}
	else if (Mask == 3)
	{
		if (!Boss.IsValid()) { Boss = SpawnEnemy(BossLocation, true); }
		State->SetProgress(Boss.IsValid() ? ECCLCampaignPhase::Boss : ECCLCampaignPhase::Error, 0);
		if (!Boss.IsValid()) { return false; }
	}
	else { State->SetProgress(Mask ? ECCLCampaignPhase::Road : ECCLCampaignPhase::Village, 2 - ((Mask & 1) != 0) - ((Mask & 2) != 0)); }
	return true;
}
