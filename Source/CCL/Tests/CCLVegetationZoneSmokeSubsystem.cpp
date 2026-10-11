#include "CCLVegetationZoneSmokeSubsystem.h"

#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLSurfaceReplication.h"
#include "Environment/CCLVegetationPresentation.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "UI/CCLGameUI.h"
#include "KSIUISubsystem.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "UnrealClient.h"

bool UCCLVegetationZoneSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLVegetationZoneSmoke"));
#endif
}

void UCCLVegetationZoneSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}

	if (Now - Started > 240.)
	{
		Finish(false, FString::Printf(TEXT("timeout step=%d"), Step));
		return;
	}
	auto* World = GetWorld();
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	auto* Runtime = World ? World->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	const auto* Surface = World ? UCCLSurfaceReplication::View(World) : nullptr;
	if (!Director || !Director->IsReady() || !Surface || !Runtime || Now < Next)
	{
		return;
	}

	const auto* Grid = Surface->FindRegion(ACCLExperimentDirector::VegetationRegionId());
	if (!Grid && Step != 9)
	{
		return;
	}

	FString Error;
	auto Check = [&](bool bOK)
	{
		if (!bOK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s"), Step, *Error));
		}

		return bOK;
	};
	auto* PC = Cast<ACCLExperimentPlayerController>(World->GetFirstPlayerController());
	auto Execute = [&](ECCLExperimentAction Action)
	{
		return Director->Execute(PC, Action, TEXT("Zone_07"), Director->GetGeneration(), Director->FindResult(TEXT("Zone_07"))->RunId, Error);
	};
	const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLVegetationZoneCapture"));
	auto Capture = [&](const TCHAR* Name)
	{
		if (bRendered)
		{
			FString Directory;
			FParse::Value(FCommandLine::Get(), TEXT("CCLVegetationZoneSmokeDir="), Directory);
			FScreenshotRequest::RequestScreenshot(Directory / Name, true, false);
		}
	};
	auto ShowMenu = [&]()
	{
		if (!bRendered || !PC)
		{
			return;
		}

		auto* UI = CCLGameUI::Get(PC);
		if (UI && !UI->IsViewOpen(PC->GetExperimentView()))
		{
			PC->CCLExperiment();
		}

		if (auto* Screen = UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr)
		{
			Screen->SelectCase(TEXT("Zone_07"));
		}
	};
	if (World->GetNetMode() == NM_Client)
	{
		if (Grid->Forcing.RainMetersPerWorldSecond != 0.12345)
		{
			return;
		}

		int32 Displayed = 0;
		for (TActorIterator<ACCLVegetationPresentation> It(World); It; ++It)
		{
			Displayed += It->GetRenderedPatchCount();
		}

		if (!Displayed)
		{
			return;
		}

		if (Check(Surface->Validate(Error) && Grid->Vegetation.Cells.Num() == 27
			&& Grid->Vegetation.Cells[0].BurnedBiomassKg > 0. && !Grid->Vegetation.Cells[0].bRootsViable
			&& Grid->Vegetation.Cells[17].GrownBiomassKg > 0.
			&& !Runtime->IgniteSurface(Grid->RegionId, 0, 1000., Error)))
		{
			Finish(true, TEXT("client persisted vegetation and presentation received; authority enforced"));
		}
		return;
	}
	if (World->GetNetMode() == NM_DedicatedServer && Step == 0 && !PC)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_VEGETATION_ZONE BASE_READY"));
		Step = -1;
		return;
	}
	if (Step == -1)
	{
		if (!PC || !Director->CanOperate(PC))
		{
			return;
		}

		Step = 0;
	}
	if (Step == 0)
	{
		if (!Check(Runtime->ChangeTimeScale(0., Error) && Execute(ECCLExperimentAction::Start)))
		{
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_VEGETATION_ZONE BASE_READY"));
		Step = 1;
		return;
	}
	if (Step == 1)
	{
		const auto* Result = Director->FindResult(TEXT("Zone_07"));
		if (!Check(Result->Status != ECCLExperimentStatus::Failed) || Result->Status != ECCLExperimentStatus::Passed)
		{
			return;
		}

		if (bRendered && PC)
		{
			const FVector Center = UCCLExperimentDefinition::ZoneCenter(7);
			auto* View = World->SpawnActor<ACameraActor>(Center + FVector(0., -1000., 680.), FRotator(-34., 90., 0.));
			PC->SetViewTarget(View);
		}
		ShowMenu();
		Step = 2;
		Next = Now + 3.;
		return;
	}
	if (Step == 2)
	{
		Capture(TEXT("vegetation-controls.png"));
		if (!Check(Execute(ECCLExperimentAction::VegetationWinter) && Runtime->ChangeTimeScale(60., Error)))
		{
			return;
		}

		TargetWorld = Runtime->GetClock().GetWorldSeconds() + 60.;
		Step = 3;
		return;
	}
	if (Step == 3)
	{
		if (Runtime->GetClock().GetWorldSeconds() < TargetWorld)
		{
			return;
		}

		int32 Dormant = 0;
		for (const auto& C : Grid->Vegetation.Cells)
		{
			Dormant += C.State == EKSIVegetationState::Dormant;
		}

		if (!Check(Dormant == 27 && Execute(ECCLExperimentAction::VegetationRain)))
		{
			return;
		}

		TargetWorld = Runtime->GetClock().GetWorldSeconds() + 120.;
		Step = 4;
		return;
	}
	if (Step == 4)
	{
		if (Runtime->GetClock().GetWorldSeconds() < TargetWorld)
		{
			return;
		}

		if (!Check(Grid->Ledger.RainCubicMeters > 0. && Grid->Vegetation.Cells[17].GrownBiomassKg > 0.
			&& Runtime->ChangeTimeScale(0., Error)))
		{
			return;
		}

		if (bRendered && PC)
		{
			auto* UI = CCLGameUI::Get(PC);
			if (UI && UI->IsViewOpen(PC->GetExperimentView()))
			{
				PC->CCLExperiment();
			}
		}
		Step = 5;
		Next = Now + 2.;
		return;
	}
	if (Step == 5)
	{
		Capture(TEXT("vegetation-growing.png"));
		if (!Check(Execute(ECCLExperimentAction::VegetationDry) && Runtime->ChangeTimeScale(60., Error)))
		{
			return;
		}

		TargetWorld = Runtime->GetClock().GetWorldSeconds() + 850.;
		Step = 6;
		return;
	}
	if (Step == 6)
	{
		if (Runtime->GetClock().GetWorldSeconds() < TargetWorld)
		{
			return;
		}

		if (!Check(Grid->Cells[0].SoilCubicMeters < 0.000001 && Runtime->ChangeTimeScale(0., Error)
			&& Execute(ECCLExperimentAction::VegetationIgnite)))
		{
			return;
		}

		Step = 7;
		Next = Now + 3.;
		return;
	}
	if (Step == 7)
	{
		if (!Check(Grid->Vegetation.Cells[0].BurnedBiomassKg > 0. && !Grid->Vegetation.Cells[0].bRootsViable))
		{
			return;
		}

		Capture(TEXT("vegetation-burned.png"));
		auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
		if (!Check(Runtime->Save(Agents->GetSimulation(), Saved, Error) && Surface->Capture(SurfaceBefore, Error)
			&& Execute(ECCLExperimentAction::VegetationWinter)))
		{
			return;
		}

		TArray<uint8> Actual;
		if (!Check(Runtime->Restore(Agents->GetSimulation(), Saved, Error) && Surface->Capture(Actual, Error) && Actual == SurfaceBefore))
		{
			return;
		}

		// Explicit reset must create the authored baseline, while normal restore preserves the saved absence/state.
		if (!Check(Execute(ECCLExperimentAction::Reset)))
		{
			return;
		}

		Step = 8;
		return;
	}
	if (Step == 8)
	{
		if (Director->IsRestoring())
		{
			return;
		}

		if (!Check(Grid->Vegetation.Cells[0].BurnedBiomassKg == 0. && Grid->Vegetation.Cells[0].bRootsViable))
		{
			return;
		}

		auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
		if (!Check(Runtime->Restore(Agents->GetSimulation(), Saved, Error)))
		{
			return;
		}

		FCCLWorldSnapshot WithoutPlants;
		FKSISurfaceSimulation Legacy;
		if (!Check(FCCLWorldSnapshotCodec::Decode(Saved, WithoutPlants, Error)
			&& Legacy.Initialize(Runtime->GetIdentity().WorldId, Surface->GetGameSeconds(), Surface->GetWorldSeconds(), Surface->GetStepId(), Error)))
		{
			return;
		}

		for (const auto& Pair : Surface->GetRegions())
		{
			if (Pair.Key != ACCLExperimentDirector::VegetationRegionId() && !Check(Legacy.AddRegion(Pair.Value, Error)))
			{
				return;
			}
		}
		for (const auto& Pair : Surface->GetAirSimulation().GetSpaces())
		{
			auto Fresh = Pair.Value;
			// This absence fixture starts its air ledger at the current inventory.
			// AddAirSpace accepts initial spaces, not an already-running ledger.
			Fresh.Initial = Fresh.Contents;
			Fresh.Added = {};
			Fresh.Removed = {};
			if (!Check(Legacy.AddAirSpace(Fresh, Error)))
			{
				return;
			}
		}
		TArray<uint8> LegacyBytes;
		if (!Check(Legacy.Capture(WithoutPlants.Surface, Error) && FCCLWorldSnapshotCodec::Encode(WithoutPlants, LegacyBytes, Error)
			&& Runtime->Restore(Agents->GetSimulation(), LegacyBytes, Error)))
		{
			return;
		}

		Step = 9;
		Next = Now + 2.;
		return;
	}
	if (Step == 9)
	{
		if (!Check(!Surface->FindRegion(ACCLExperimentDirector::VegetationRegionId())))
		{
			return;
		}

		auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
		if (!Check(Runtime->Restore(Agents->GetSimulation(), Saved, Error)))
		{
			return;
		}

		auto Marker = Surface->FindRegion(ACCLExperimentDirector::VegetationRegionId())->Forcing;
		Marker.RainMetersPerWorldSecond = 0.12345;
		if (!Check(Runtime->ChangeSurfaceForcing(ACCLExperimentDirector::VegetationRegionId(), Marker, Error)))
		{
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_VEGETATION_ZONE RESTORE_READY"));
		Step = 10;
		Next = Now + 3.;
		return;
	}
	Finish(true, TEXT("zone07 vegetation controls dormancy growth fire reset and restoration verified"));
}

void UCCLVegetationZoneSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_VEGETATION_ZONE %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!bSuccess || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLVegetationZoneSmoke"));
	}
}

TStatId UCCLVegetationZoneSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLVegetationZoneSmokeSubsystem, STATGROUP_Tickables);
}
