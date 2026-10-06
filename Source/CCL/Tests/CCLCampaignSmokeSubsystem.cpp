#include "CCLCampaignSmokeSubsystem.h"

#include "CCLCharacter.h"
#include "CCLPlayerController.h"
#include "Campaign/CCLCampaignDirector.h"
#include "Campaign/CCLCampaignState.h"
#include "Combat/CCLEnemyCharacter.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLStaminaSet.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool UCCLCampaignSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	FString Role;
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CCLCampaignSmoke="), Role);
#endif
}

TStatId UCCLCampaignSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLCampaignSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLCampaignSmokeSubsystem::Tick(float DeltaTime)
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
	if (FPlatformTime::Seconds() - Started > 180.)
	{
		Check(false, TEXT("watchdog"));
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	auto* State = GetWorld()->GetGameState<ACCLCampaignState>();
	auto* Local = Cast<ACCLPlayerController>(GetWorld()->GetFirstPlayerController());
	auto* Pawn = Local ? Cast<ACCLCharacter>(Local->GetPawn()) : nullptr;
	auto* ASC = Pawn ? Cast<UCCLAbilitySystemComponent>(Pawn->GetAbilitySystemComponent()) : nullptr;
	FString Role;
	FParse::Value(FCommandLine::Get(), TEXT("CCLCampaignSmoke="), Role);
	if (State && Local && Local->IsLocalController() && Pawn &&
		FParse::Param(FCommandLine::Get(), TEXT("CCLCampaignCapture")) && CapturedPhase != static_cast<int32>(State->GetPhase()))
	{
		CapturedPhase = static_cast<int32>(State->GetPhase());
		const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Tests/CampaignVisual"));
		IFileManager::Get().MakeDirectory(*Directory, true);
		Local->SetControlRotation(FRotator(-12.f, 0.f, 0.f));
		FScreenshotRequest::RequestScreenshot(Directory / FString::Printf(TEXT("phase-%d.png"), CapturedPhase), true, false);
	}
	if (Role == TEXT("driver") && Local && Local->IsLocalController() && ASC && ASC->GetActivatableAbilities().Num() >= 4)
	{
		if (!bRegistered)
		{
			Local->ServerCampaignTestReady();
			bRegistered = 1;
		}
		if (bAttacking && Now >= NextInputAt)
		{
			Local->SetControlRotation(FRotator::ZeroRotator);
			ASC->AbilityInputTagPressed(CCLTags::Input_Attack);
			ASC->AbilityInputTagReleased(CCLTags::Input_Attack);
			NextInputAt = Now + 1.3;
		}
	}

	if (Local && Local->IsLocalController() && Pawn && State && State->GetPhase() == ECCLCampaignPhase::Victory && !bClientReported)
	{
		if (!Check(State->GetRemainingGuards() == 0, TEXT("replicated victory has no remaining guards")))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN CLIENT PASS role=%s"), *Role);
		bClientReported = 1;
	}

	if (GetWorld()->GetNetMode() == NM_Client || !Driver.IsValid() || !State || Now < NextStageAt)
	{
		return;
	}
	auto* Character = Cast<ACCLCharacter>(Driver->GetPawn());
	if (!Character)
	{
		return;
	}
	ACCLCampaignDirector* Director = nullptr;
	for (TActorIterator<ACCLCampaignDirector> It(GetWorld()); It; ++It)
	{
		Director = *It;
		break;
	}
	if (!Director)
	{
		Check(false, TEXT("director exists"));
		return;
	}

	if (Stage == 0)
	{
		if (!Check(State->GetPhase() == ECCLCampaignPhase::Village && State->GetRemainingGuards() == 2 &&
			Director->GetGuards().Num() == 2 && !Director->GetBoss(), TEXT("village starts with two guards and no boss")))
		{
			return;
		}
		Director->NotifyEnemyDefeated(Director->GetGuards()[0].Get());
		if (!Check(State->GetRemainingGuards() == 2, TEXT("living enemy notification rejected")))
		{
			return;
		}
		Target = Director->GetGuards()[0];
		NavigationStart = Target->GetActorLocation();
		Character->SetActorLocation(NavigationStart - FVector(450.f, 0.f, 0.f));
		NextStageAt = Now + 6.;
		Stage = 10;
		return;
	}
	if (Stage == 10)
	{
		const auto* AI = Target.IsValid() ? Cast<AAIController>(Target->GetController()) : nullptr;
		FNavLocation Projected;
		auto* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		const bool bNavigation = Navigation && Navigation->ProjectPointToNavigation(NavigationStart, Projected);
		UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN navigation moved=%.1f health=%.1f brain=%d nav=%d"),
			Target.IsValid() ? FVector::Dist2D(Target->GetActorLocation(), NavigationStart) : -1.f,
			Character->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()),
			AI && AI->GetBrainComponent() && AI->GetBrainComponent()->IsRunning(), bNavigation);
		if (!Check(Target.IsValid() && FVector::Dist2D(Target->GetActorLocation(), NavigationStart) > 100.f &&
			Character->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) < 100.f,
			TEXT("live StateTree navigates and attacks on campaign map")))
		{
			return;
		}
		Character->GetAbilitySystemComponent()->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 100.f);
		PrepareEnemy(Target.Get());
		Stage = 1;
		return;
	}
	if (Stage == 1 || Stage == 2 || Stage == 3)
	{
		if (!Target.IsValid())
		{
			Check(false, TEXT("encounter target remains valid"));
			return;
		}
		if (!Target->IsDead())
		{
			return;
		}
		const int32 Remaining = State->GetRemainingGuards();
		Director->NotifyEnemyDefeated(Target.Get());
		if (!Check(State->GetRemainingGuards() == Remaining, TEXT("duplicate death ignored")))
		{
			return;
		}
		if (Stage == 1)
		{
			if (!Check(State->GetPhase() == ECCLCampaignPhase::Road && Remaining == 1 && !Director->GetBoss(), TEXT("first guard does not unlock boss")))
			{
				return;
			}
			PrepareEnemy(Director->GetGuards()[1].Get());
			Stage = 2;
		}
		else if (Stage == 2)
		{
			if (!Check(State->GetPhase() == ECCLCampaignPhase::Boss && Remaining == 0 && Director->GetBoss(), TEXT("all guards unlock boss")))
			{
				return;
			}
			PrepareEnemy(Director->GetBoss());
			Stage = 3;
		}
		else
		{
			if (!Check(State->GetPhase() == ECCLCampaignPhase::Victory, TEXT("boss defeat completes expedition")))
			{
				return;
			}
			Driver->ClientCampaignTestStep(0);
			OldPawn = Character;
			Character->Die();
			Driver->ClientCampaignTestStep(2);
			NextStageAt = Now + 2.;
			Stage = 4;
		}
		return;
	}
	if (Stage == 4 && Character != OldPawn.Get() && !Character->IsDead())
	{
		if (!Check(State->GetPhase() == ECCLCampaignPhase::Victory, TEXT("victory survives individual respawn")))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN SERVER PASS guards, boss, duplicate events and respawn"));
		bComplete = 1;
	}
}

void UCCLCampaignSmokeSubsystem::RegisterDriver(ACCLPlayerController* Controller)
{
	if (GetWorld()->GetNetMode() != NM_Client && !Driver.IsValid())
	{
		Driver = Controller;
		NextStageAt = GetWorld()->GetTimeSeconds() + (FParse::Param(FCommandLine::Get(), TEXT("CCLCampaignCapture")) ? 30. : 2.);
	}
}

void UCCLCampaignSmokeSubsystem::ExecuteClientStep(int32 Step)
{
	bAttacking = Step == 1;
	NextInputAt = GetWorld()->GetTimeSeconds() + 1.;
	if (Step == 2)
	{
		if (auto* Local = Cast<ACCLPlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			Local->CCLRetry();
		}
	}
}

void UCCLCampaignSmokeSubsystem::PrepareEnemy(ACCLEnemyCharacter* Enemy)
{
	if (!Check(IsValid(Enemy), TEXT("next enemy exists")))
	{
		return;
	}
	for (TActorIterator<ACCLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (auto* AI = Cast<AAIController>(It->GetController()))
		{
			AI->StopMovement();
			if (AI->GetBrainComponent())
			{
				AI->GetBrainComponent()->StopLogic(TEXT("Campaign progression test"));
			}
		}
		It->GetAbilitySystemComponent()->CancelAllAbilities();
	}
	Target = Enemy;
	auto* Character = Cast<ACCLCharacter>(Driver->GetPawn());
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->SetActorLocation(Enemy->GetActorLocation() - FVector(110.f, 0.f, 0.f));
	Character->SetActorRotation(FRotator::ZeroRotator);
	Character->GetAbilitySystemComponent()->SetNumericAttributeBase(UCCLStaminaSet::GetStaminaAttribute(), 100.f);
	Driver->ClientCampaignTestStep(1);
	NextStageAt = GetWorld()->GetTimeSeconds() + 1.;
}

bool UCCLCampaignSmokeSubsystem::Check(bool bCondition, const TCHAR* Description)
{
	if (!bCondition)
	{
		bFailed = 1;
		UE_LOG(LogTemp, Error, TEXT("CCL_CAMPAIGN FAIL %s stage=%d"), Description, Stage);
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN CHECK %s"), Description);
	return true;
}
