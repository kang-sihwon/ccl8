#include "CCLFireSmokeSubsystem.h"

#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLSurfaceReplication.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"

namespace
{
	const FGuid FireRegion(710, 711, 712, 713);
	const FGuid FireSpace(714, 715, 716, 717);
	const FGuid FireWall(718, 719, 720, 721);
	const FGuid FireDoor(722, 723, 724, 725);
	const FGuid FireBlocker(726, 727, 728, 729);
}

bool UCCLFireSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLFireSmoke"));
#endif
}

void UCCLFireSmokeSubsystem::Tick(float DeltaTime)
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
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	auto* Runtime = World ? World->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	const auto* Surface = World ? UCCLSurfaceReplication::View(World) : nullptr;
	if (!Director || !Director->IsReady() || !Runtime || !Surface || Now < Next)
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
	const auto* Grid = Surface->FindRegion(FireRegion);
	const auto* Room = Surface->GetAirSimulation().FindSpace(FireSpace);
	auto SetDoor = [&](double Fraction)
	{
		auto Inputs = Runtime->GetEnvironmentInputs();
		++Inputs.Revision;
		++Inputs.SurfaceRevision;
		for (auto& Opening : Inputs.Openings)
		{
			if (Opening.OpeningId == FireDoor)
			{
				Opening.OpenFraction = Fraction;
			}
		}
		return Runtime->ReplaceEnvironmentInputs(Inputs, Error);
	};
	auto SetBlocker = [&](bool bBlocked)
	{
		auto Inputs = Runtime->GetEnvironmentInputs();
		Inputs.Surfaces.RemoveAll([](const auto& P) { return P.SurfaceId == FireBlocker; });
		if (bBlocked)
		{
			const auto* Wall = Inputs.Surfaces.FindByPredicate([](const auto& P) { return P.SurfaceId == FireWall; });
			if (!Wall)
			{
				Error = TEXT("Missing fire ventilation wall.");
				return false;
			}

			auto Blocker = *Wall;
			Blocker.SurfaceId = FireBlocker;
			Blocker.CenterMeters.X += 0.25;
			Inputs.Surfaces.Add(Blocker);
		}

		++Inputs.Revision;
		++Inputs.SurfaceRevision;
		return Runtime->ReplaceEnvironmentInputs(Inputs, Error);
	};
	if (World->GetNetMode() == NM_Client)
	{
		if (!Grid || !Room || Grid->Fire.Cells.Num() != 3 || Grid->Forcing.RainMetersPerWorldSecond != 0.2)
		{
			return;
		}
		if (!Check(Grid->Fire.Cells[0].BurnedKg > 0. && Grid->Fire.Cells[1].BurnedKg == 0.
			&& Grid->Fire.MeltHeatJoules > 0. && !Grid->Fire.Cells[0].bBurning
			&& Room->Contents.SmokeKg > 0. && Room->TemperatureCelsius() > 20.
			&& Surface->GetAirSimulation().GetExteriorExported().SmokeKg > 0.
			&& Surface->Validate(Error) && !Runtime->AddAirSpace(*Room, Error)
			&& !Runtime->IgniteSurface(FireRegion, 0, 1000., Error)))
		{
			return;
		}
		Finish(true, FString::Printf(TEXT("client combustion burned=%.9f smoke=%.9f melt=%.6f air=%.9f exported=%.9f"),
			Grid->Fire.Cells[0].BurnedKg, Grid->Fire.EmittedSmokeKg, Grid->Fire.MeltHeatJoules,
			Room->Contents.SmokeKg, Surface->GetAirSimulation().GetExteriorExported().SmokeKg));
		return;
	}

	if (Step == 0)
	{
		FCCLSurfaceGrid G;
		G.RegionId = FireRegion;
		G.OriginMeters = FVector(148., 0., 0.);
		G.SpacingMeters = 1.;
		G.Size = FIntPoint(3, 1);
		G.Cells.SetNum(3);
		G.Fire.Cells.SetNum(3);
		G.Fire.SpreadWattsPerKelvin = 0.;
		G.Forcing.InfiltrationMetersPerWorldSecond = 0.;
		G.Forcing.DrainageMetersPerWorldSecond = 0.;
		G.Forcing.PhaseMetersPerDegreeWorldSecond = 0.;
		G.Forcing.FlowConductance = 0.;
		G.Forcing.TemperatureCelsius = 20.;
		G.Cells[1].SoilCubicMeters = 0.01;
		G.Cells[2].SnowCubicMeters = 0.0001;
		G.Cells[2].SnowVolumeCubicMeters = 0.001;
		G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
		for (auto& Cell : G.Fire.Cells)
		{
			if (!Check(FCCLCombustionModel::Initialize(G.Fire.Material, 2., 20., Cell, Error)))
			{
				return;
			}
		}
		if (!Check(Runtime->ChangeTimeScale(0., Error) && Runtime->AddSurfaceRegion(G, Error)))
		{
			return;
		}
		FCCLAirSpace Space;
		Space.SpaceId = FireSpace;
		Space.ClimateRegionId = FireRegion;
		Space.MinimumMeters = FVector(148., -1., 0.);
		Space.MaximumMeters = FVector(152., 2., 3.);
		Space.VolumeCubicMeters = 36.;
		if (!Check(Space.Initialize(20., 0.004, Error) && Runtime->AddAirSpace(Space, Error)))
		{
			return;
		}
		auto Inputs = Runtime->GetEnvironmentInputs();
		FCCLSurfacePatch Wall;
		Wall.SurfaceId = FireWall;
		Wall.BodyId = Inputs.Observer.BodyId;
		Wall.MaterialId = TEXT("FireSmokeWall");
		Wall.CenterMeters = FVector(148., 0.5, 1.5);
		Wall.Normal = FVector::ForwardVector;
		Wall.TangentU = FVector::RightVector;
		Wall.TangentV = FVector::UpVector;
		Wall.HalfExtentsMeters = FVector2D(1.5, 1.5);
		FCCLSurfaceOpening Door;
		Door.OpeningId = FireDoor;
		Door.SurfaceId = FireWall;
		Door.SpaceA = FireSpace;
		Door.HalfExtentsMeters = FVector2D(1., 1.);
		Inputs.Surfaces.Add(Wall);
		Inputs.Openings.Add(Door);
		++Inputs.Revision;
		++Inputs.SurfaceRevision;
		if (!Check(Runtime->ReplaceEnvironmentInputs(Inputs, Error)))
		{
			return;
		}
		for (int32 I = 0; I < 3; ++I)
		{
			if (!Check(Runtime->IgniteSurface(FireRegion, I, 600000., Error)))
			{
				return;
			}
		}
		GameStarted = Runtime->GetClock().GetGameSeconds();
		UE_LOG(LogTemp, Display, TEXT("CCL_FIRE_SMOKE BASE_READY"));
		Step = 1;
		return;
	}
	if (!Check(Grid != nullptr && Room != nullptr))
	{
		return;
	}
	if (Step == 1)
	{
		if (Runtime->GetClock().GetGameSeconds() - GameStarted < 10.)
		{
			return;
		}
		if (!Check(Grid->Fire.Cells[0].BurnedKg > 0. && Grid->Fire.Cells[1].BurnedKg == 0.
			&& Grid->Cells[1].SoilCubicMeters < 0.01 && Grid->Cells[2].SnowCubicMeters < 0.0001
			&& Grid->Fire.MeltHeatJoules > 0. && Room->TemperatureCelsius() > 20.
			&& FMath::IsNearlyEqual(Room->Contents.SmokeKg, Grid->Fire.EmittedSmokeKg, 1.e-9)
			&& Surface->Validate(Error)))
		{
			return;
		}
		auto Rain = Grid->Forcing;
		Rain.RainMetersPerWorldSecond = 0.1;
		if (!Check(Runtime->ChangeSurfaceForcing(FireRegion, Rain, Error) && Runtime->ChangeTimeScale(1., Error)))
		{
			return;
		}
		WorldStarted = Runtime->GetClock().GetWorldSeconds();
		Step = 2;
		return;
	}
	if (Step == 2)
	{
		if (Runtime->GetClock().GetWorldSeconds() - WorldStarted < 1.)
		{
			return;
		}
		auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
		TArray<uint8> Saved, Before, After;
		if (!Check(!Grid->Fire.Cells[0].bBurning && Grid->Cells[0].WaterCubicMeters > 0.05
			&& Runtime->ChangeTimeScale(0., Error) && Runtime->Save(Agents->GetSimulation(), Saved, Error)
			&& Surface->Capture(Before, Error) && Runtime->IgniteSurface(FireRegion, 0, 100000., Error)
			&& Runtime->Restore(Agents->GetSimulation(), Saved, Error) && Surface->Capture(After, Error) && Before == After))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_FIRE_SMOKE ROUNDTRIP_READY"));
		Step = 3;
		Next = Now + 1.;
		return;
	}
	if (Step == 3)
	{
		RoomSmokeBefore = Room->Contents.SmokeKg;
		if (!Check(SetDoor(0.5)))
		{
			return;
		}
		GameStarted = Runtime->GetClock().GetGameSeconds();
		Step = 4;
		return;
	}
	if (Step == 4 || Step == 5)
	{
		if (Runtime->GetClock().GetGameSeconds() - GameStarted < 5.)
		{
			return;
		}
		if (!Check(Room->Contents.SmokeKg < RoomSmokeBefore && Surface->Validate(Error)))
		{
			return;
		}
		RoomSmokeBefore = Room->Contents.SmokeKg;
		if (!Check(Step == 4 ? SetDoor(1.) : SetBlocker(true)))
		{
			return;
		}
		GameStarted = Runtime->GetClock().GetGameSeconds();
		++Step;
		return;
	}
	if (Step == 6 || Step == 7)
	{
		if (Runtime->GetClock().GetGameSeconds() - GameStarted < 1.)
		{
			return;
		}

		if (!Check(Step == 6 ? Room->Contents.SmokeKg == RoomSmokeBefore : Room->Contents.SmokeKg < RoomSmokeBefore))
		{
			return;
		}

		RoomSmokeBefore = Room->Contents.SmokeKg;
		if (!Check(Step == 6 ? SetBlocker(false) : SetDoor(0.)))
		{
			return;
		}

		GameStarted = Runtime->GetClock().GetGameSeconds();
		if (Step == 7)
		{
			UE_LOG(LogTemp, Display, TEXT("CCL_FIRE_SMOKE GEOMETRY_READY"));
		}

		++Step;
		return;
	}
	if (Step == 8)
	{
		if (Runtime->GetClock().GetGameSeconds() - GameStarted < 1.)
		{
			return;
		}
		if (!Check(Room->Contents.SmokeKg == RoomSmokeBefore
			&& FMath::IsNearlyEqual(Room->Contents.SmokeKg + Surface->GetAirSimulation().GetExteriorExported().SmokeKg,
				Grid->Fire.EmittedSmokeKg, 1.e-9)))
		{
			return;
		}
		auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
		TArray<uint8> Saved, Before, After;
		if (!Check(Runtime->Save(Agents->GetSimulation(), Saved, Error) && Surface->Capture(Before, Error)
			&& SetDoor(1.) && Runtime->Restore(Agents->GetSimulation(), Saved, Error)
			&& Surface->Capture(After, Error) && Before == After))
		{
			return;
		}
		const auto* RestoredDoor = Runtime->GetEnvironmentInputs().Openings.FindByPredicate(
			[](const auto& O) { return O.OpeningId == FireDoor; });
		if (!Check(RestoredDoor && RestoredDoor->OpenFraction == 0.))
		{
			return;
		}
		auto Marker = Surface->FindRegion(FireRegion)->Forcing;
		Marker.RainMetersPerWorldSecond = 0.2;
		if (!Check(Runtime->ChangeSurfaceForcing(FireRegion, Marker, Error)))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_FIRE_SMOKE RESTORE_READY"));
		Step = 9;
		Next = Now + 1.;
		return;
	}
	Finish(true, FString::Printf(TEXT("server combustion burned=%.9f smoke=%.9f melt=%.6f air=%.9f exported=%.9f"),
		Grid->Fire.Cells[0].BurnedKg, Grid->Fire.EmittedSmokeKg, Grid->Fire.MeltHeatJoules,
		Room->Contents.SmokeKg, Surface->GetAirSimulation().GetExteriorExported().SmokeKg));
}

void UCCLFireSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_FIRE_SMOKE %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!bSuccess || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLFireSmoke"));
	}
}

TStatId UCCLFireSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLFireSmokeSubsystem, STATGROUP_Tickables);
}
