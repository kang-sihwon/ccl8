#include "CCLSnowSmokeSubsystem.h"

#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLSnowMovementComponent.h"
#include "Environment/CCLSnowAnimInstance.h"
#include "Environment/CCLSurfaceReplication.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLTerrainRegion.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Camera/CameraActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Environment/CCLSurfacePresentation.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool UCCLSnowSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLSnowSmoke"));
#endif
}

void UCCLSnowSmokeSubsystem::Tick(float Dt)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}
	if (Now - Started > 200.)
	{
		Finish(false, FString::Printf(TEXT("watchdog step=%d"), Step));
		return;
	}
	auto* World = GetWorld();
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	const auto* Model = World ? UCCLSurfaceReplication::View(World) : nullptr;
	const auto* Grid = Model ? Model->FindRegion(ACCLExperimentDirector::SnowRegionId()) : nullptr;
	if (!World || !World->HasBegunPlay() || !Director || !Director->IsReady() || !Grid || Now < Next)
	{
		return;
	}
	auto* PC = Cast<ACCLExperimentPlayerController>(World->GetFirstPlayerController());
	auto* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	auto* Move = Character ? Cast<UCCLSnowMovementComponent>(Character->GetCharacterMovement()) : nullptr;
	const FVector Center = UCCLExperimentDefinition::ZoneCenter(3);
	FString Directory, Role, Error;
	FParse::Value(FCommandLine::Get(), TEXT("CCLSnowSmokeDir="), Directory);
	FParse::Value(FCommandLine::Get(), TEXT("CCLSnowRole="), Role);
	const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLSnowCapture"));
	const bool bCorrection = FParse::Param(FCommandLine::Get(), TEXT("CCLSnowCorrection"));
	if (Move && bCorrection)
	{
		Move->MaxWalkSpeed = 60.f;
	}
	if (Move)
	{
		MaxDepth = FMath::Max(MaxDepth, double(Move->GetSnowDepth()));
		PeakReplays = FMath::Max(PeakReplays, Move->GetSnowReplays());
		PeakCorrections = FMath::Max(PeakCorrections, Move->GetSnowCorrections());
	}
	if (Character && PC->IsLocalController() && Grid->Forcing.TemperatureCelsius == -4.321
		&& FVector::DistSquared2D(Character->GetActorLocation(), Center) < FMath::Square(900.))
	{
		if (auto* UI = CCLGameUI::Get(PC); UI && UI->IsViewOpen(PC->GetExperimentView()))
		{
			PC->CCLExperiment();
		}
		if (Character->GetActorLocation().X < Center.X + 220.)
		{
			Character->AddMovementInput(FVector::ForwardVector, 1.);
		}
	}
	auto ShowSnow = [&](bool bVisible)
	{
		for (TActorIterator<ACCLSurfacePresentation> It(World); It; ++It)
		{
			TArray<UPrimitiveComponent*> Components;
			It->GetComponents(Components);
			for (auto* Component : Components)
			{
				if (Component->GetFName() == TEXT("Snow"))
				{
					Component->SetVisibility(bVisible);
				}
			}
		}
	};
	if (bShowSnowNextTick)
	{
		ShowSnow(true);
		bShowSnowNextTick = 0;
	}
	if (Camera.IsValid() && Step == 1 && Character)
	{
		const FVector Target = Character->GetActorLocation();
		Camera->SetActorLocation(Target + FVector(-280., -430., 150.));
		Camera->SetActorRotation((Target - Camera->GetActorLocation()).Rotation());
		if (Character->GetVelocity().Size2D() > 50.)
		{
			const double Height = Character->GetMesh()->GetSocketLocation(TEXT("foot_l")).Z - 20.;
			if (!FMath::IsFinite(Height) || Height < -10. || Height > 100.)
			{
				Finish(false, TEXT("invalid evaluated foot pose"));
				return;
			}
			MinFootHeight = FMath::Min(MinFootHeight, Height);
			MaxFootHeight = FMath::Max(MaxFootHeight, Height);
			TArray<UInstancedStaticMeshComponent*> Components;
			Character->GetComponents(Components);
			for (const auto* Component : Components)
			{
				bPowderObserved |= Component->GetFName() == TEXT("SnowPowder") && Component->GetInstanceCount() > 0;
			}
		}
		if (!bFeetCaptured && Target.X > Center.X - 150.)
		{
			bFeetCaptured = bShowSnowNextTick = 1;
			ShowSnow(false);
			FScreenshotRequest::RequestScreenshot(Directory / TEXT("snow-feet.png"), false, false);
		}
		else if (!bWalkCaptured && Target.X > Center.X)
		{
			bWalkCaptured = 1;
			FScreenshotRequest::RequestScreenshot(Directory / TEXT("snow-walk.png"), false, false);
		}
	}
	if (World->GetNetMode() == NM_Client)
	{
		if (Grid->Forcing.TemperatureCelsius != -1.234)
		{
			return;
		}
		if (Step != 100)
		{
			Step = 100;
			Next = Now + 1.;
			return;
		}
		TArray<uint8> Bytes;
		if (!Model->Validate(Error) || !Model->Capture(Bytes, Error))
		{
			Finish(false, Error);
			return;
		}
		FFileHelper::SaveArrayToFile(Bytes, *(Directory / (Role + TEXT("-surface.bin"))));
		if (Character)
		{
			FFileHelper::SaveStringToFile(Character->GetActorLocation().ToString(), *(Directory / (Role + TEXT("-position.txt"))));
		}
		if (bCorrection && Role == TEXT("first") && PeakReplays == 0)
		{
			Finish(false, TEXT("autonomous client did not replay a corrected snow move"));
			return;
		}
		Finish(true, FString::Printf(TEXT("client canonical snow receipt and trail restored; peak depth=%.3f replays=%u"), MaxDepth, PeakReplays));
		return;
	}
	auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>();
	Runtime->SetTickableTickType(ETickableTickType::Never);
	if (!bBaseReported)
	{
		bBaseReported = 1;
		UE_LOG(LogTemp, Display, TEXT("CCL_SNOW_SMOKE BASE_READY"));
	}
	if (!PC || !Character || !Move || !Director->CanOperate(PC) || !Director->GetTerrainRegion()->IsTerrainReady())
	{
		return;
	}
	auto Check = [&](bool Good)
	{
		if (!Good)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s"), Step, *Error));
		}
		return Good;
	};
	auto Execute = [&](ECCLExperimentAction Action)
	{
		const auto* Result = Director->FindResult(TEXT("Zone_03"));
		return Director->Execute(PC, Action, TEXT("Zone_03"), Director->GetGeneration(),
			Result ? Result->RunId : FGuid(), Error, Director->GetTerrainRegion()->GetPublicationSerial());
	};
	if (Step == 0)
	{
		if (bRendered)
		{
			auto* UI = CCLGameUI::Get(PC);
			auto* Screen = UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
			if (!Screen)
			{
				return;
			}
			Screen->SelectCase(TEXT("Zone_03"));
			Next = Now + 0.7;
		}
		Step = 10;
		return;
	}
	if (Step == 10)
	{
		if (bRendered)
		{
			FScreenshotRequest::RequestScreenshot(Directory / TEXT("snow-controls.png"), true, false);
			Camera = World->SpawnActor<ACameraActor>();
			PC->SetViewTarget(Camera.Get());
		}
		Character->TeleportTo(Center + FVector(-340., 0., 119.), FRotator::ZeroRotator);
		Move->StopMovementImmediately();
		auto Forcing = Grid->Forcing;
		Forcing.TemperatureCelsius = -4.321;
		if (!Check(Runtime->ChangeSurfaceForcing(Grid->RegionId, Forcing, Error) && Execute(ECCLExperimentAction::Start)))
		{
			return;
		}
		Step = 1;
		Next = Now + 0.4;
		return;
	}
	if (Step == 1)
	{
		auto* Rep = PC->FindComponentByClass<UCCLSurfaceReplication>();
		if (bCorrection && !bHistoryPaused && Character->GetActorLocation().X > Center.X - 250. && Rep)
		{
			Rep->SetComponentTickEnabled(false);
			bHistoryPaused = 1;
			UE_LOG(LogTemp, Display, TEXT("CCL_SNOW_SMOKE HISTORY_PAUSED"));
		}
		const auto* Result = Director->FindResult(TEXT("Zone_03"));
		if (Character->GetActorLocation().X < Center.X + 210. || !Result || Result->Status != ECCLExperimentStatus::Passed)
		{
			return;
		}
		Error = FString::Printf(TEXT("depth=%.4f speed=%.3f postprocess=%s"), MaxDepth, Move->GetMaxSpeed(), *GetNameSafe(Character->GetMesh()->GetPostProcessInstance()));
		if (!Check(MaxDepth >= 0.5 && Move->GetMaxSpeed() < 500.f && (!bRendered || Cast<UCCLSnowAnimInstance>(Character->GetMesh()->GetPostProcessInstance()))))
		{
			return;
		}
		if (!Check((!bCorrection || Move->GetSnowCorrections() > 0)
			&& (!bRendered || (MaxFootHeight - MinFootHeight > 4. && bPowderObserved))))
		{
			return;
		}
		if (Rep)
		{
			Rep->SetComponentTickEnabled(true);
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_SNOW_SMOKE MOTION corrections=%u foot_range_cm=%.3f..%.3f powder=%d"),
			Move->GetSnowCorrections(), MinFootHeight, MaxFootHeight, int32(bPowderObserved));
		Move->StopMovementImmediately();
		Move->DisableMovement();
		Step = 2;
		Next = Now + 1.;
		return;
	}
	if (Step == 2)
	{
		if (!Check(Model->Capture(Saved, Error) && Execute(ECCLExperimentAction::Save)))
		{
			return;
		}
		auto F = Grid->Forcing;
		F.TemperatureCelsius = 20.;
		F.PhaseMetersPerDegreeWorldSecond = 0.01;
		if (!Check(Runtime->ChangeSurfaceForcing(Grid->RegionId, F, Error)
			&& Runtime->QueueGameTime(0.25, Error) && Runtime->AdvancePending(0.25, 15., Error)))
		{
			return;
		}
		Step = 3;
		return;
	}
	if (Step == 3)
	{
		if (!Check(Execute(ECCLExperimentAction::Load)))
		{
			return;
		}
		Step = 4;
		return;
	}
	if (Step == 4)
	{
		TArray<uint8> Restored;
		if (!Model->Capture(Restored, Error) || Saved != Restored)
		{
			return;
		}
		auto F = Grid->Forcing;
		F.TemperatureCelsius = -1.234;
		if (!Check(Runtime->ChangeSurfaceForcing(Grid->RegionId, F, Error)))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_SNOW_SMOKE RESTORE_READY"));
		if (Camera.IsValid())
		{
			Camera->SetActorLocation(Center + FVector(-650., -900., 850.));
			Camera->SetActorRotation((Center - Camera->GetActorLocation()).Rotation());
		}
		Step = 5;
		Next = Now + 1.;
		return;
	}
	if (Step == 5)
	{
		if (bRendered)
		{
			FScreenshotRequest::RequestScreenshot(Directory / TEXT("snow-trail.png"), false, false);
		}
		Step = 6;
		Next = Now + 1.;
		return;
	}
	if (Step == 6)
	{
		if (World->GetNetMode() != NM_Standalone)
		{
			int32 Ready = 0;
			for (TActorIterator<APlayerController> It(World); It; ++It)
			{
				const auto* Rep = It->FindComponentByClass<UCCLSurfaceReplication>();
				Ready += !It->IsLocalController() && Rep && Rep->HasCurrentState(*Model);
			}
			if (Ready < 2)
			{
				return;
			}
		}
		TArray<uint8> Bytes;
		if (!Check(Model->Capture(Bytes, Error) && Model->Validate(Error)))
		{
			return;
		}
		FFileHelper::SaveArrayToFile(Bytes, *(Directory / TEXT("server-surface.bin")));
		FFileHelper::SaveStringToFile(Character->GetActorLocation().ToString(), *(Directory / TEXT("server-position.txt")));
		Finish(true, FString::Printf(TEXT("CMC knee-depth traversal, receipt, thaw, exact restore and peers; depth=%.3f balance=%.12f corrections=%u"),
			MaxDepth, Grid->BalanceErrorCubicMeters(), PeakCorrections));
	}
}

void UCCLSnowSmokeSubsystem::Finish(bool Success, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_SNOW_SMOKE %s %s"), Success ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!Success || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1, TEXT("CCLSnowSmoke"));
	}
}
TStatId UCCLSnowSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLSnowSmokeSubsystem, STATGROUP_Tickables);
}
