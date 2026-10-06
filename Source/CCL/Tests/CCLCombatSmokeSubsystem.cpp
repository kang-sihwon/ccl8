#include "CCLCombatSmokeSubsystem.h"

#include "CCLCharacter.h"
#include "CCLPlayerController.h"
#include "CCLGameModeBase.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLStaminaSet.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLEffects.h"
#include "Combat/CCLEnemyCharacter.h"
#include "Combat/CCLFighterComponent.h"
#include "Combat/CCLCombatComponent.h"
#include "Combat/CCLHealthTarget.h"
#include "Combat/CCLCombatDefinition.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	UCCLAbilitySystemComponent* ASC(AActor* Actor)
	{
		const auto* Interface = Cast<IAbilitySystemInterface>(Actor);
		return Interface ? Cast<UCCLAbilitySystemComponent>(Interface->GetAbilitySystemComponent()) : nullptr;
	}

	float Health(AActor* Actor)
	{
		return ASC(Actor)->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
	}

	float Stamina(AActor* Actor)
	{
		return ASC(Actor)->GetNumericAttribute(UCCLStaminaSet::GetStaminaAttribute());
	}

	FGameplayTag Hit(AActor* Source, AActor* Target)
	{
		auto* Combat = Source->FindComponentByClass<UCCLCombatComponent>();
		auto* Fighter = Source->FindComponentByClass<UCCLFighterComponent>();
		const uint32 Id = Combat->BeginAttack(Fighter->GetAttack(), Source->GetActorForwardVector());
		FHitResult Result(Target, nullptr, Target->GetActorLocation(), FVector::UpVector);
		Result.ImpactPoint = Target->GetActorLocation();
		const auto Outcome = Combat->ResolveHit(Target, Result, Id);
		Combat->EndAttack();
		return Outcome;
	}
}

bool UCCLCombatSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	FString Role;
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CCLCombatSmoke="), Role);
#endif
}

TStatId UCCLCombatSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLCombatSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLCombatSmokeSubsystem::RegisterDriver(ACCLPlayerController* Controller)
{
	if (GetWorld()->GetNetMode() == NM_Client || Driver.IsValid())
	{
		return;
	}

	Driver = Controller;
	StepStarted = GetWorld()->GetTimeSeconds();
	NextStepAt = StepStarted + 1.0;
}

void UCCLCombatSmokeSubsystem::ExecuteClientStep(int32 InStep)
{
	auto* Controller = Cast<ACCLPlayerController>(GetWorld()->GetFirstPlayerController());
	auto* Component = Controller ? ASC(Controller->GetPawn()) : nullptr;

	if (!Component)
	{
		return;
	}

	Controller->SetControlRotation(FRotator::ZeroRotator);

	if (InStep < 0)
	{
		Controller->FlushPressedKeys();
		return;
	}

	FGameplayTag Input;

	if (InStep == 0 || InStep == 6)
	{
		Input = CCLTags::Input_Attack;
	}

	if (InStep >= 1 && InStep <= 3)
	{
		Input = CCLTags::Input_Guard;
	}

	if (InStep == 4)
	{
		Input = CCLTags::Input_Parry;
	}

	if (InStep == 5)
	{
		Input = CCLTags::Input_Dodge;
	}

	if (Input.IsValid())
	{
		Component->AbilityInputTagPressed(Input);

		if (Input != CCLTags::Input_Guard)
		{
			Component->AbilityInputTagReleased(Input);
		}
	}

	if (InStep == 7)
	{
		Controller->CCLRetry();
	}

	UE_LOG(LogTemp, Display, TEXT("CCL_COMBAT INPUT step=%d stamina=%.1f"), InStep, Stamina(Controller->GetPawn()));
}

void UCCLCombatSmokeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bFailed || bComplete)
	{
		return;
	}

	if (!Started)
	{
		Started = FPlatformTime::Seconds();
	}

	if (FPlatformTime::Seconds() - Started > 150.)
	{
		Check(false, TEXT("watchdog"));
		return;
	}

	FString Role;
	FParse::Value(FCommandLine::Get(), TEXT("CCLCombatSmoke="), Role);
	auto* LocalController = Cast<ACCLPlayerController>(GetWorld()->GetFirstPlayerController());

	if (Role == TEXT("driver") && !bRegistered && LocalController && LocalController->IsLocalController())
	{
		auto* LocalASC = ASC(LocalController->GetPawn());

		if (LocalASC && LocalASC->GetActivatableAbilities().Num() >= 4)
		{
			LocalController->ServerCombatTestReady();
			bRegistered = 1;
		}
	}

	if (GetWorld()->GetNetMode() == NM_Client || !Driver.IsValid())
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	if (NextStepAt > 0.)
	{
		if (Now < NextStepAt)
		{
			return;
		}

		NextStepAt = 0.;
		PrepareStep();
		return;
	}

	if (Now - StepStarted > 12.)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_COMBAT stage=%d sub=%d health=%.1f stamina=%.1f enemy=%.1f"), Step, Substep,
		    Driver->GetPawn() ? Health(Driver->GetPawn()) : -1.f, Driver->GetPawn() ? Stamina(Driver->GetPawn()) : -1.f, Enemy.IsValid() ? Health(Enemy.Get()) : -1.f);
		Check(false, TEXT("step timeout"));
		return;
	}

	auto* Pawn = Cast<ACCLCharacter>(Driver->GetPawn());

	if (!Pawn)
	{
		return;
	}

	auto* Component = ASC(Pawn);

	if (Step == 0 && Health(Enemy.Get()) == 60.f && !Component->HasMatchingGameplayTag(CCLTags::State_Busy))
	{
		Check(FMath::IsNearlyEqual(Stamina(Pawn), 85.f), TEXT("predicted attack charged exactly once"));
		Advance();
	}
	else if (Step == 1)
	{
		if (!Substep && Component->HasMatchingGameplayTag(CCLTags::State_Guard))
		{
			Check(Hit(Enemy.Get(), Pawn) == CCLTags::Outcome_Guarded && Health(Pawn) == 100.f && Stamina(Pawn) == 70.f, TEXT("front guard"));
			Driver->ClientCombatTestStep(-1);
			Substep = 1;
		}
		else if (Substep && !Component->HasMatchingGameplayTag(CCLTags::State_Guard))
		{
			Advance();
		}
	}
	else if (Step == 2 && Component->HasMatchingGameplayTag(CCLTags::State_Guard))
	{
		Check(Hit(Enemy.Get(), Pawn) == CCLTags::Outcome_GuardBroken && Health(Pawn) == 100.f && Stamina(Pawn) == 0.f &&
		          Component->HasMatchingGameplayTag(CCLTags::State_Stagger) && !Component->HasMatchingGameplayTag(CCLTags::State_Guard),
		    TEXT("guard break blocks triggering hit"));
		Check(Hit(Enemy.Get(), Pawn) == CCLTags::Outcome_Damage && Health(Pawn) == 80.f, TEXT("followup damages staggered target"));
		Advance();
	}
	else if (Step == 3 && Component->HasMatchingGameplayTag(CCLTags::State_Guard))
	{
		Enemy->SetActorLocation(Pawn->GetActorLocation() - FVector(100.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		Check(Hit(Enemy.Get(), Pawn) == CCLTags::Outcome_Damage && Health(Pawn) == 80.f, TEXT("rear guard does not block"));
		Advance();
	}
	else if (Step == 4 && Component->HasMatchingGameplayTag(CCLTags::State_Parry))
	{
		Check(Hit(Enemy.Get(), Pawn) == CCLTags::Outcome_Parried && Health(Pawn) == 100.f &&
		          ASC(Enemy.Get())->HasMatchingGameplayTag(CCLTags::State_Stagger),
		    TEXT("parry window cancels attack and staggers"));
		Advance();
	}
	else if (Step == 5 && Component->HasMatchingGameplayTag(CCLTags::State_Invulnerable))
	{
		Enemy->SetActorLocation(Pawn->GetActorLocation() + FVector(100.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		Check(Hit(Enemy.Get(), Pawn) == CCLTags::Outcome_Evaded && Health(Pawn) == 100.f, TEXT("dodge window"));
		Advance();
	}
	else if (Step == 6 && Now - StepStarted > 1.5)
	{
		Check(Health(Enemy.Get()) == 80.f && Stamina(Pawn) == 0.f && !Component->HasMatchingGameplayTag(CCLTags::State_Busy), TEXT("insufficient cost rejects activation"));
		RunPolicyChecks();
		Advance();
	}
	else if (Step == 7 && Pawn != OldPawn.Get() && !Pawn->IsDead())
	{
		Check(Component == OriginalASC.Get() && Component->GetAvatarActor() == Pawn, TEXT("persistent ASC and new avatar"));
		Check(Health(Pawn) == 100.f && Stamina(Pawn) == 100.f, TEXT("respawn resources reset"));
		Check(Component->GetActivatableAbilities().Num() == AbilityCount, TEXT("no duplicate ability grants"));
		Check(!Component->HasMatchingGameplayTag(CCLTags::State_Stagger) && !Component->HasMatchingGameplayTag(CCLTags::State_Guard), TEXT("no previous life effects"));

		if (WitnessPawn.IsValid())
		{
			Check(!Cast<ACCLCharacter>(WitnessPawn.Get())->IsDead(), TEXT("witness survives individual respawn"));
		}

		if (!bFailed)
		{
			bComplete = 1;
			UE_LOG(LogTemp, Display, TEXT("CCL_COMBAT PASS network abilities, policies and persistent ASC respawn"));
		}
	}
}

void UCCLCombatSmokeSubsystem::PrepareStep()
{
	auto* Pawn = Cast<ACCLCharacter>(Driver->GetPawn());

	if (!Pawn)
	{
		Check(false, TEXT("missing driver pawn"));
		return;
	}

	if (!Enemy.IsValid())
	{
		for (TActorIterator<ACCLEnemyCharacter> It(GetWorld()); It; ++It)
		{
			Enemy = *It;
			break;
		}
	}

	if (!Enemy.IsValid())
	{
		Check(false, TEXT("missing enemy"));
		return;
	}

	if (auto* AI = Cast<AAIController>(Enemy->GetController()))
	{
		AI->StopMovement();

		if (AI->GetBrainComponent())
		{
			AI->GetBrainComponent()->StopLogic(TEXT("Automated scenario"));
		}
	}

	if (Step == 7)
	{
		OldPawn = Pawn;
		OriginalASC = ASC(Pawn);
		AbilityCount = OriginalASC->GetActivatableAbilities().Num();

		for (TActorIterator<ACCLCharacter> It(GetWorld()); It; ++It)
		{
			if (*It != Pawn)
			{
				WitnessPawn = *It;
			}
		}

		Pawn->Die();
	}
	else
	{
		auto* Component = ASC(Pawn);
		Pawn->FindComponentByClass<UCCLFighterComponent>()->Initialize(Component);
		Enemy->FindComponentByClass<UCCLFighterComponent>()->Initialize(ASC(Enemy.Get()));
		Component->RemoveActiveGameplayEffectBySourceEffect(UCCLStaminaRegenEffect::StaticClass(), Component);
		Pawn->GetCharacterMovement()->StopMovementImmediately();
		Pawn->SetActorLocation(FVector(-500.f, -800.f, 96.f), false, nullptr, ETeleportType::TeleportPhysics);
		Pawn->SetActorRotation(FRotator::ZeroRotator);
		Driver->SetControlRotation(FRotator::ZeroRotator);
		Enemy->SetActorLocation(Pawn->GetActorLocation() + FVector(110.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		Enemy->SetActorRotation(FRotator(0.f, 180.f, 0.f));

		if (Step == 2)
		{
			Component->ApplyEffect(UCCLStaminaChangeEffect::StaticClass(), -80.f);
		}

		if (Step == 6)
		{
			Component->ApplyEffect(UCCLStaminaChangeEffect::StaticClass(), -100.f);
		}
	}

	Substep = 0;
	StepStarted = GetWorld()->GetTimeSeconds();
	Driver->ClientCombatTestStep(Step);
	UE_LOG(LogTemp, Display, TEXT("CCL_COMBAT STAGE %d"), Step);
}

void UCCLCombatSmokeSubsystem::Advance()
{
	if (bFailed)
	{
		return;
	}

	Driver->ClientCombatTestStep(-1);
	UE_LOG(LogTemp, Display, TEXT("CCL_COMBAT stage %d passed"), Step);
	++Step;
	NextStepAt = GetWorld()->GetTimeSeconds() + 1.2;
}

void UCCLCombatSmokeSubsystem::Check(bool bCondition, const TCHAR* Message)
{
	if (bCondition)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_COMBAT CHECK %s"), Message);
	}
	else
	{
		bFailed = 1;
		UE_LOG(LogTemp, Error, TEXT("CCL_COMBAT FAIL %s"), Message);
	}
}

void UCCLCombatSmokeSubsystem::RunPolicyChecks()
{
	auto* Pawn = Cast<ACCLCharacter>(Driver->GetPawn());
	auto* Component = ASC(Pawn);
	auto* EnemyASC = ASC(Enemy.Get());
	Component->CancelAllAbilities();
	EnemyASC->CancelAllAbilities();
	Component->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(CCLTags::Effect_Life));
	EnemyASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(CCLTags::Effect_Life));
	// A health-only actor exercises the shared resolver without a stamina set or Character.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Enemy->SetActorLocation(Pawn->GetActorLocation() + FVector(500.f, 0.f, 0.f));
	auto* Target = GetWorld()->SpawnActor<ACCLHealthTarget>(Pawn->GetActorLocation() + FVector(110.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
	Check(Target && !ASC(Target)->GetSet<UCCLStaminaSet>(), TEXT("health-only target has no stamina"));

	if (!Target)
	{
		return;
	}

	auto* Combat = Pawn->FindComponentByClass<UCCLCombatComponent>();
	const uint32 Id = Combat->BeginAttack(Pawn->FindComponentByClass<UCCLFighterComponent>()->GetAttack(), FVector::ForwardVector);
	FHitResult Result(Target, nullptr, Target->GetActorLocation(), FVector::UpVector);
	Result.ImpactPoint = Target->GetActorLocation();
	Check(Combat->ResolveHit(Target, Result, Id) == CCLTags::Outcome_Damage && Health(Target) == 80.f, TEXT("generic target damaged"));
	Check(!Combat->ResolveHit(Target, Result, Id).IsValid() && Health(Target) == 80.f, TEXT("duplicate hit rejected"));
	Combat->EndAttack();
	Check(!Combat->ResolveHit(Target, Result, Id).IsValid(), TEXT("ended attack rejected"));
	Target->Destroy();
	Component->ApplyEffect(UCCLHealthChangeEffect::StaticClass(), 10000.f);
	Check(Health(Pawn) == 100.f, TEXT("health clamped to maximum"));
	Component->ApplyEffect(UCCLStaminaChangeEffect::StaticClass(), 10000.f);
	Check(Stamina(Pawn) == 100.f, TEXT("stamina clamped to maximum"));
}
