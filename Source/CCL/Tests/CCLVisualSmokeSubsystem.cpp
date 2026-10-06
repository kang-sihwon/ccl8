#include "CCLVisualSmokeSubsystem.h"

#include "CCLCharacter.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "Combat/CCLEnemyCharacter.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool UCCLVisualSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Param(FCommandLine::Get(), TEXT("CCLVisualSmoke"));
#endif
}

TStatId UCCLVisualSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLVisualSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLVisualSmokeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UWorld* World = GetWorld();

	if (World->GetNetMode() != NM_Standalone || World->GetTimeSeconds() < NextStepAt)
	{
		return;
	}

	APlayerController* Controller = World->GetFirstPlayerController();
	Character = Controller ? Cast<ACCLCharacter>(Controller->GetPawn()) : nullptr;
	UCCLAbilitySystemComponent* ASC = Character.IsValid() ? Cast<UCCLAbilitySystemComponent>(Character->GetAbilitySystemComponent()) : nullptr;

	if (!ASC || ASC->GetActivatableAbilities().Num() < 4)
	{
		return;
	}

	double Delay = 1.;

	switch (Step)
	{
	case 0:
		Character->SetActorLocation(FVector(-500.f, -800.f, 96.f));
		Character->SetActorRotation(FRotator::ZeroRotator);
		Controller->SetControlRotation(FRotator(-12.f, 0.f, 0.f));

		for (TActorIterator<ACCLEnemyCharacter> It(World); It; ++It)
		{
			if (auto* AI = Cast<AAIController>(It->GetController()))
			{
				AI->StopMovement();

				if (AI->GetBrainComponent())
				{
					AI->GetBrainComponent()->StopLogic(TEXT("Visual smoke snapshot"));
				}
			}

			It->SetActorLocation(FVector(-390.f, -800.f, 96.f));
			It->SetActorRotation(FRotator(0.f, 180.f, 0.f));
		}

		Delay = 30.;
		break;
	case 1:
		Capture(TEXT("01-idle"));
		break;
	case 2:
		ASC->AbilityInputTagPressed(CCLTags::Input_Attack);
		ASC->AbilityInputTagReleased(CCLTags::Input_Attack);
		Delay = 0.4;
		break;
	case 3:
		Capture(TEXT("02-attack"));
		break;
	case 4:
		ASC->AbilityInputTagPressed(CCLTags::Input_Guard);
		Delay = 0.3;
		break;
	case 5:
		Capture(TEXT("03-guard"));
		break;
	case 6:
		ASC->AbilityInputTagReleased(CCLTags::Input_Guard);
		break;
	case 7:
		ASC->AbilityInputTagPressed(CCLTags::Input_Parry);
		ASC->AbilityInputTagReleased(CCLTags::Input_Parry);
		Delay = 0.15;
		break;
	case 8:
		Capture(TEXT("04-parry"));
		break;
	case 9:
		ASC->AbilityInputTagPressed(CCLTags::Input_Dodge);
		ASC->AbilityInputTagReleased(CCLTags::Input_Dodge);
		Delay = 0.15;
		break;
	case 10:
		Capture(TEXT("05-dodge"));
		break;
	case 11:
		Character->Die();
		break;
	case 12:
		Capture(TEXT("06-death"));
		break;
	default:
		UE_LOG(LogTemp, Display, TEXT("CCL_VISUAL sequence complete; inspect six images"));
		FPlatformMisc::RequestExit(false);
		return;
	}

	++Step;
	NextStepAt = World->GetTimeSeconds() + Delay;
}

void UCCLVisualSmokeSubsystem::Capture(const TCHAR* Name)
{
	const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Tests/VisualSmoke"));
	IFileManager::Get().MakeDirectory(*Directory, true);
	FScreenshotRequest::RequestScreenshot(Directory / FString(Name).Append(TEXT(".png")), true, false);
	UE_LOG(LogTemp, Display, TEXT("CCL_VISUAL requested %s"), Name);
}
