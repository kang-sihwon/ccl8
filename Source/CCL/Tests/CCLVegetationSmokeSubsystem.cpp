#include "CCLVegetationSmokeSubsystem.h"

#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Crc.h"

namespace
{
	const FGuid PlantRegion(968, 1, 1, 1);
}

bool UCCLVegetationSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLVegetationSmoke"));
#endif
}

void UCCLVegetationSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}

	if (Now - Started > 180.)
	{
		Finish(false, FString::Printf(TEXT("timeout step=%d"), Step));
		return;
	}
	auto* World = GetWorld();
	auto* Runtime = World ? World->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	auto* Agents = World ? World->GetSubsystem<UCCLAgentWorldSubsystem>() : nullptr;
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	if (!Runtime || !Runtime->IsRunning() || !Agents || !Agents->IsRunning() || !Director || !Director->IsReady() || Now < Next)
	{
		return;
	}

	FString Error, Directory;
	FParse::Value(FCommandLine::Get(), TEXT("CCLVegetationSmokeDir="), Directory);
	const FString SavePath = Directory / TEXT("vegetation-world.bin");
	auto Check = [&](bool bOK)
	{
		if (!bOK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s"), Step, *Error));
		}

		return bOK;
	};
	if (Step == 0)
	{
		if (!Check(Runtime->ChangeTimeScale(0., Error)))
		{
			return;
		}

		Step = 1;
		Next = Now + 2.;
		return;
	}
	if (Step == 1)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("CCLVegetationResume")))
		{
			TArray<uint8> Saved, Actual;
			FCCLWorldSnapshot Decoded;
			if (!Check(FFileHelper::LoadFileToArray(Saved, *SavePath)
				&& FCCLWorldSnapshotCodec::Decode(Saved, Decoded, Error) && Agents->Restore(Saved, Error)
				&& Runtime->GetSurfaceSimulation().Capture(Actual, Error) && Actual == Decoded.Surface))
			{
				return;
			}

			const auto* G = Runtime->GetSurfaceSimulation().FindRegion(PlantRegion);
			if (!Check(G && G->Vegetation.Cells.Num() == 9))
			{
				return;
			}

			CyclesBefore = G->Vegetation.Cells[8].RegrowthCount;
			InitialTime = Runtime->GetClock().GetWorldSeconds();
			UE_LOG(LogTemp, Display, TEXT("CCL_VEGETATION RESTART_RESTORED world=%.6f crc=%u"), InitialTime,
				FCrc::MemCrc32(Actual.GetData(), Actual.Num() - 4));
		}
		else
		{
			InitialTime = Runtime->GetClock().GetWorldSeconds();
			FKSISurfaceGrid G;
			G.RegionId = PlantRegion;
			G.OriginMeters = UCCLExperimentDefinition::ZoneCenter(7) / 100. + FVector(-4.5, 0., 0.);
			G.Size = FIntPoint(9, 1);
			G.SpacingMeters = 1.;
			G.Cells.SetNum(9);
			G.Forcing.TemperatureCelsius = 20.;
			G.Forcing.InfiltrationMetersPerWorldSecond = 0.;
			G.Forcing.DrainageMetersPerWorldSecond = 0.;
			G.Forcing.FlowConductance = 0.;
			G.Forcing.PhaseMetersPerDegreeWorldSecond = 0.;
			G.Fire.SpreadWattsPerKelvin = 0.;
			G.Fire.Material.CoolingWattsPerKelvin = 0.;
			G.Fire.Material.SurfaceWattsPerKelvin = 0.;
			TArray<FKSIVegetationSpecies> Species;
			TArray<FKSIVegetationSeed> Seeds;
			for (int32 I = 0; I < 9; ++I)
			{
				FKSIVegetationSpecies S;
				S.Id = FGuid(968, 2, 1, I + 1);
				S.Kind = EKSIVegetationKind(I);
				S.LifespanSeconds = 180.;
				S.RegrowthDelaySeconds = 30.;
				S.GrowthKgPerWorldSecond = 0.001;
				S.MinimumDaylightHours = 0.;
				S.DecayPerWorldSecond = 0.001;
				Species.Add(S);
				FKSIVegetationSeed Seed;
				Seed.Id = FGuid(968, 3, 1, I + 1);
				Seed.SpeciesIndex = I;
				Seed.BiomassKg = 0.1;
				Seed.SoilNutrientsKg = 1.;
				Seed.SoilOrganicKg = 2.;
				Seeds.Add(Seed);
				G.Cells[I].BedMeters = 0.2;
				G.Cells[I].SoilCubicMeters = 0.05;
				G.Cells[I].WaterCubicMeters = I == 6 || I == 7 ? 0.05 : 0.;
				G.Cells[I].SunExposure = I == 8 ? 0. : 1.;
			}
			G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
			if (!Check(KSISurfaceVegetation::Initialize(G, Species, Seeds, InitialTime, Error) && Runtime->AddSurfaceRegion(G, Error)))
			{
				return;
			}
		}

		if (!Check(Runtime->ChangeTimeScale(60., Error)))
		{
			return;
		}

		Step = 2;
		return;
	}
	if (Step == 2)
	{
		if (Runtime->GetClock().GetWorldSeconds() < InitialTime + 240.)
		{
			return;
		}

		if (!Check(Runtime->ChangeTimeScale(0., Error)))
		{
			return;
		}

		Step = 3;
		Next = Now + 2.;
		return;
	}
	if (Step == 3)
	{
		const auto& Surface = Runtime->GetSurfaceSimulation();
		const auto* G = Surface.FindRegion(PlantRegion);
		if (!Check(G && G->Vegetation.Cells.Num() == 9 && Surface.Validate(Error)
			&& Surface.GetWorldSeconds() == Runtime->GetClock().GetWorldSeconds()
			&& Surface.GetWorldSeconds() == Agents->GetSimulation().GetTime()))
		{
			return;
		}

		for (const auto& C : G->Vegetation.Cells)
		{
			if (!Check(C.RegrowthCount > CyclesBefore && C.DecayedBiomassKg > 0.))
			{
				return;
			}
		}
		if (!Check(G->Vegetation.Cells[8].GrownBiomassKg > 0.
			&& G->Vegetation.Cells[8].FungalConsumptionKg > 0.))
		{
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("CCLVegetationResume")))
		{
			Finish(true, TEXT("new process restored exact surface state and continued all nine plant life cycles"));
			return;
		}
		TArray<uint8> Saved, Before, After;
		if (!Check(Surface.Capture(Before, Error) && Agents->Save(Saved) && FFileHelper::SaveArrayToFile(Saved, *SavePath)))
		{
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_VEGETATION SAVED world=%.6f crc=%u"), Surface.GetWorldSeconds(),
			FCrc::MemCrc32(Before.GetData(), Before.Num() - 4));
		if (!Check(Runtime->IgniteSurface(PlantRegion, 0, 800000., Error) && Surface.Capture(After, Error) && Before != After
			&& Agents->Restore(Saved, Error) && Surface.Capture(After, Error) && Before == After))
		{
			return;
		}

		Finish(true, TEXT("nine species world-time lifecycle, resources and same-generation save rollback"));
	}
}

void UCCLVegetationSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_VEGETATION %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLVegetationSmoke"));
}

TStatId UCCLVegetationSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLVegetationSmokeSubsystem, STATGROUP_Tickables);
}
