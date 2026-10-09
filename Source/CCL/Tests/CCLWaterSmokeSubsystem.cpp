#include "CCLWaterSmokeSubsystem.h"

#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLSurfaceReplication.h"
#include "Environment/CCLTerrainRegion.h"
#include "Environment/CCLTerrainReplication.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Camera/CameraActor.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool UCCLWaterSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLWaterSmoke"));
#endif
}

void UCCLWaterSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}
	if (Now - Started > 180.)
	{
		Finish(false, FString::Printf(TEXT("watchdog step=%d"), Step));
		return;
	}
	auto* World = GetWorld();
	const auto* Model = UCCLSurfaceReplication::View(World);
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	auto* Terrain = Director ? Director->GetTerrainRegion() : nullptr;
	if (Now < Next || !World || !World->HasBegunPlay() || !Director || !Director->IsReady()
		|| !Terrain || !Terrain->IsTerrainReady() || !Model)
	{
		return;
	}
	const auto* Water = Model->FindRegion(Director->WaterRegionId());
	const auto* TerrainWater = Model->FindRegion(Director->TerrainWaterRegionId());
	if (!Water || !TerrainWater)
	{
		return;
	}
	FString Error;
	if (World->GetNetMode() == NM_Client)
	{
		if (Water->Forcing.TemperatureCelsius == 13.37 && UCCLSurfaceReplication::IsGeometryReady(World, *Model, *TerrainWater))
		{
			Finish(Model->Validate(Error) && TerrainWater->TerrainRevision == 1,
				FString::Printf(TEXT("client restored surface total=%.9f terrain=%llu %s"), Water->TotalCubicMeters(), TerrainWater->TerrainRevision, *Error));
		}
		return;
	}
	auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>();
	Runtime->SetTickableTickType(ETickableTickType::Never);
	if (!bBaseReported)
	{
		bBaseReported = 1;
		UE_LOG(LogTemp, Display, TEXT("CCL_WATER_SMOKE BASE_READY"));
	}
	auto* PC = Cast<ACCLExperimentPlayerController>(World->GetFirstPlayerController());
	if (!PC || !PC->GetPawn() || !Director->CanOperate(PC) || Terrain->IsPreparing())
	{
		return;
	}
	auto Check = [&](bool bSuccess)
	{
		if (!bSuccess)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s"), Step, *Error));
		}
		return bSuccess;
	};
	auto Execute = [&](ECCLExperimentAction Action, FName Case = TEXT("Zone_04"))
	{
		const auto* Result = Director->FindResult(Case);
		return Director->Execute(PC, Action, Case, Director->GetGeneration(), Result ? Result->RunId : FGuid(), Error, Terrain->GetPublicationSerial());
	};
	auto Advance = [&]()
	{
		return Runtime->QueueGameTime(0.25, Error) && Runtime->AdvancePending(0.25, 15., Error);
	};
	FString Directory;
	FParse::Value(FCommandLine::Get(), TEXT("CCLWaterSmokeDir="), Directory);
	const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLWaterCapture"));
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
			Screen->SelectCase(TEXT("Zone_04"));
			Step = 20;
			Next = Now + 0.5;
			return;
		}
		if (!Check(Execute(ECCLExperimentAction::Start)))
		{
			return;
		}
		Step = 1;
		Next = Now + (bRendered ? 1. : 0.);
		return;
	}
	if (Step == 20)
	{
		auto* UI = CCLGameUI::Get(PC);
		auto* Screen = UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
		const auto Button = Screen ? Screen->GetStartButton() : nullptr;
		if (!Button || Button->GetCachedGeometry().GetAbsoluteSize().IsNearlyZero())
		{
			return;
		}
		const FVector2D Point = Button->GetCachedGeometry().GetAbsolutePosition() + Button->GetCachedGeometry().GetAbsoluteSize() * 0.5f;
		auto& App = FSlateApplication::Get();
		const auto Window = App.FindWidgetWindow(Button.ToSharedRef());
		App.ProcessMouseMoveEvent(FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()));
		App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
			FPointerEvent(0, Point, Point, TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0, FModifierKeysState()));
		App.ProcessMouseButtonUpEvent(FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
		++ClickAttempts;
		Step = 21;
		Next = Now + 0.25;
		return;
	}
	if (Step == 21)
	{
		const auto* Result = Director->FindResult(TEXT("Zone_04"));
		if (Result->Status != ECCLExperimentStatus::Running)
		{
			if (ClickAttempts < 4)
			{
				Step = 20;
				Next = Now + 0.25;
				return;
			}
			Finish(false, TEXT("Water Start pointer click did not reach the server experiment."));
			return;
		}
		Step = 1;
	}
	if (Step == 1)
	{
		if (bRendered)
		{
			FScreenshotRequest::RequestScreenshot(Directory / TEXT("water-controls.png"), true, false);
		}
		Step = 2;
	}
	if (Step == 2)
	{
		const auto* Result = Director->FindResult(TEXT("Zone_04"));
		if (Result->Status == ECCLExperimentStatus::Running)
		{
			Check(Advance());
			return;
		}
		Error = Result->Detail;
		if (!Check(Result->Status == ECCLExperimentStatus::Passed && Model->Validate(Error)))
		{
			return;
		}
		TerrainTotal = TerrainWater->TotalCubicMeters();
		if (!Check(Execute(ECCLExperimentAction::TerrainExcavate, TEXT("Zone_05"))))
		{
			return;
		}
		// Water must keep its later state when the asynchronous terrain cook completes.
		if (!Check(Advance()))
		{
			return;
		}
		Step = 3;
		return;
	}
	if (Step == 3)
	{
		if (!Check(Terrain->DidLastRequestSucceed() && TerrainWater->TerrainRevision == Terrain->GetTerrainStore().GetRevision()
			&& FMath::IsNearlyEqual(TerrainWater->TotalCubicMeters(), TerrainTotal, 1.e-7)
			&& Execute(ECCLExperimentAction::TerrainDeposit, TEXT("Zone_05"))))
		{
			return;
		}
		Step = 4;
		return;
	}
	if (Step == 4)
	{
		if (!Check(Terrain->DidLastRequestSucceed() && TerrainWater->TerrainRevision == 3 && Model->Validate(Error)
			&& FMath::IsNearlyEqual(TerrainWater->TotalCubicMeters(), TerrainTotal, 1.e-7)
			&& Execute(ECCLExperimentAction::Save) && Model->Capture(SavedSurface, Error)
			&& FFileHelper::SaveArrayToFile(SavedSurface, *(Directory / TEXT("surface.bin")))
			&& Execute(ECCLExperimentAction::WaterRain) && Advance() && Execute(ECCLExperimentAction::Load)))
		{
			return;
		}
		Step = 5;
		return;
	}
	if (Step == 5)
	{
		TArray<uint8> Restored;
		if (!Check(Terrain->DidLastRequestSucceed() && Model->Capture(Restored, Error) && Restored == SavedSurface
			&& Execute(ECCLExperimentAction::Reset)))
		{
			return;
		}
		Step = 6;
		return;
	}
	if (Step == 6)
	{
		if (!Check(Terrain->DidLastRequestSucceed() && TerrainWater->TerrainRevision == 1 && Model->Validate(Error)))
		{
			return;
		}
		auto Forcing = Water->Forcing;
		Forcing.TemperatureCelsius = 13.37;
		if (!Check(Runtime->ChangeSurfaceForcing(Water->RegionId, Forcing, Error)))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_WATER_SMOKE RESTORE_READY"));
		if (bRendered)
		{
			PC->CCLExperiment();
			auto* Camera = World->SpawnActor<ACameraActor>();
			const FVector Target = UCCLExperimentDefinition::ZoneCenter(4) + FVector(0., 0., 30.);
			Camera->SetActorLocation(Target + FVector(-1100., -1400., 1500.));
			Camera->SetActorRotation((Target - Camera->GetActorLocation()).Rotation());
			PC->SetViewTarget(Camera);
			Next = Now + 2.;
		}
		Step = 7;
		return;
	}
	if (Step == 7)
	{
		if (bRendered)
		{
			FScreenshotRequest::RequestScreenshot(Directory / TEXT("water-overview.png"), false, false);
			Next = Now + 2.;
		}
		Step = bRendered ? 9 : 8;
		return;
	}
	if (Step == 9)
	{
		if (!Check(Execute(ECCLExperimentAction::WaterFreeze)))
		{
			return;
		}
		for (int32 I = 0; I < 12; ++I)
		{
			if (!Check(Advance()))
			{
				return;
			}
		}
		Next = Now + 1.;
		Step = 10;
		return;
	}
	if (Step == 10)
	{
		FScreenshotRequest::RequestScreenshot(Directory / TEXT("water-ice.png"), false, false);
		Next = Now + 1.;
		Step = 11;
		return;
	}
	if (Step == 11)
	{
		// Laboratory forcing exposes wet soil after thaw; natural weather is a later stage.
		auto F = Water->Forcing;
		F.TemperatureCelsius = 20.;
		F.PhaseMetersPerDegreeWorldSecond = 0.01;
		F.FlowConductance = 0.;
		F.BoundaryConductance = 0.;
		F.InfiltrationMetersPerWorldSecond = 0.001;
		F.DrainageMetersPerWorldSecond = 0.;
		F.EvaporationMetersPerWorldSecond = 0.002;
		if (!Check(Runtime->ChangeSurfaceForcing(Water->RegionId, F, Error)))
		{
			return;
		}
		for (int32 I = 0; I < 10; ++I)
		{
			if (!Check(Advance()))
			{
				return;
			}
		}
		Next = Now + 1.;
		Step = 12;
		return;
	}
	if (Step == 12)
	{
		if (!Check(Model->Validate(Error)))
		{
			return;
		}
		FScreenshotRequest::RequestScreenshot(Directory / TEXT("water-mud.png"), false, false);
		Next = Now + 1.;
		Step = 8;
		return;
	}
	if (Step == 8)
	{
		if (World->GetNetMode() != NM_Standalone)
		{
			int32 Ready = 0;
			for (TActorIterator<APlayerController> It(World); It; ++It)
			{
				const auto* SurfacePeer = It->FindComponentByClass<UCCLSurfaceReplication>();
				const auto* TerrainPeer = It->FindComponentByClass<UCCLTerrainReplication>();
				if (!It->IsLocalController() && SurfacePeer && SurfacePeer->HasCurrentState(*Model)
					&& TerrainPeer && TerrainPeer->IsClientReady(Terrain))
				{
					++Ready;
				}
			}
			if (Ready < 2)
			{
				return;
			}
		}
		Finish(true, FString::Printf(TEXT("water cycle, terrain, canonical restore, reset and replicas; mass error=%.12f"), Water->BalanceErrorCubicMeters()));
	}
}

void UCCLWaterSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_WATER_SMOKE %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!bSuccess || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLWaterSmoke"));
	}
}

TStatId UCCLWaterSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLWaterSmokeSubsystem, STATGROUP_Tickables);
}
