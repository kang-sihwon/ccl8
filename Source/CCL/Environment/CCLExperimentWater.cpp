#include "CCLExperimentDirector.h"

#include "CCLWorldSimulationSubsystem.h"
#include "CCLTerrainWaterParticipant.h"
#include "CCLSurfacePresentation.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

namespace
{
	FCCLSurfaceGrid WaterGrid()
	{
		FCCLSurfaceGrid G;
		G.RegionId = ACCLExperimentDirector::WaterRegionId();
		G.Size = FIntPoint(24, 16);
		G.OriginMeters = UCCLExperimentDefinition::ZoneCenter(4) / 100. + FVector(-6., -4., 0.);
		G.Cells.SetNum(G.Size.X * G.Size.Y);
		G.Forcing.BoundaryLevelMeters = 0.5;
		G.Forcing.BoundaryConductance = 0.02;
		for (int32 I = 0; I < G.Cells.Num(); ++I)
		{
			auto& C = G.Cells[I];
			const int32 X = I % G.Size.X, Y = I / G.Size.X;
			C.BedMeters = X < 8 ? (8 - X) * 0.12 + 0.2 : 0.2;
			C.CeilingMeters = 8.;
			C.WaterCubicMeters = FMath::Max(0., 0.6 - C.BedMeters) * FMath::Square(G.SpacingMeters);
			C.bOpenBoundary = X == G.Size.X - 1;
			C.RainExposure = Y < 4 ? 0.25 : 1.;
		}
		G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
		return G;
	}

	bool TerrainGrid(const FCCLTerrainSnapshot& Terrain, const FTransform& Transform, FCCLSurfaceGrid& G, FString& Error)
	{
		G.RegionId = ACCLExperimentDirector::TerrainWaterRegionId();
		G.TerrainId = Terrain.Definition.RegionId;
		G.TerrainRevision = Terrain.Revision;
		G.OriginMeters = Transform.GetLocation() / 100. + FVector(-8., -8., 0.);
		G.Size = FIntPoint(32, 32);
		G.Cells.SetNum(1024);
		TArray<double> Beds;
		if (!FCCLTerrainWaterParticipant::ReadBeds(Terrain, Transform, G, Beds, Error))
		{
			return false;
		}
		for (int32 I = 0; I < G.Cells.Num(); ++I)
		{
			auto& C = G.Cells[I];
			C.BedMeters = Beds[I];
			C.CeilingMeters = Transform.GetLocation().Z / 100. + 8.;
			// A small initial reservoir straddles the two excavation locations.
			const int32 X = I % G.Size.X, Y = I / G.Size.X;
			C.WaterCubicMeters = X >= 12 && X < 20 && Y >= 12 && Y < 23 ? 0.125 : 0.;
		}
		G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
		return FCCLSurfaceSimulation::ValidateRegion(G, Error);
	}

	double IceTotal(const FCCLSurfaceGrid& G)
	{
		double Total = 0.;
		for (const auto& C : G.Cells)
		{
			Total += C.IceCubicMeters;
		}
		return Total;
	}
}

void ACCLExperimentDirector::InitializeWaterExperiment()
{
	if (!TerrainRegion || !TerrainRegion->IsTerrainReady() || TerrainRegion->IsPreparing())
	{
		return;
	}
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!Runtime || !Runtime->IsRunning())
	{
		return;
	}
	if (bWaterReady && Runtime->GetSurfaceSimulation().FindRegion(WaterRegionId())
		&& Runtime->GetSurfaceSimulation().FindRegion(TerrainWaterRegionId())
		&& Runtime->GetSurfaceSimulation().FindRegion(SnowRegionId()))
	{
		return;
	}
	FString Error;
	FCCLSurfaceGrid TerrainWater;
	if (!TerrainGrid(TerrainRegion->GetTerrainStore().GetSnapshot(), TerrainRegion->GetActorTransform(), TerrainWater, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_WATER_INIT terrain: %s"), *Error);
		return;
	}
	if ((!Runtime->GetSurfaceSimulation().FindRegion(WaterRegionId()) && !Runtime->AddSurfaceRegion(WaterGrid(), Error))
		|| (!Runtime->GetSurfaceSimulation().FindRegion(SnowRegionId()) && !Runtime->AddSurfaceRegion(SnowGrid(), Error))
		|| (!Runtime->GetSurfaceSimulation().FindRegion(TerrainWaterRegionId()) && !Runtime->AddSurfaceRegion(TerrainWater, Error)))
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_WATER_INIT regions: %s"), *Error);
		return;
	}
	if (!bWaterParticipantBound)
	{
		if (!TerrainRegion->AddParticipant(MakeShared<FCCLTerrainWaterParticipant>(Runtime, TerrainWaterRegionId(), TerrainRegion->GetActorTransform()), Error))
		{
			UE_LOG(LogTemp, Error, TEXT("CCL_WATER_INIT participant: %s"), *Error);
			return;
		}
		bWaterParticipantBound = 1;
		GetWorld()->SpawnActor<ACCLSurfacePresentation>();
	}

	// The reset baseline keeps its original time and base terrain, even after revisiting an edited map.
	FCCLWorldSnapshot Initial;
	FCCLSurfaceSimulation InitialWater;
	FCCLTerrainStore Base;
	auto Definition = TerrainRegion->GetTerrainStore().GetSnapshot().Definition;
	if (!FCCLWorldSnapshotCodec::Decode(InitialSnapshot, Initial, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_WATER_INIT baseline: %s"), *Error);
		return;
	}
	Definition.WorldId = Initial.Identity.WorldId;
	if (!Base.Initialize(Definition, Error) || !TerrainGrid(Base.GetSnapshot(), TerrainRegion->GetActorTransform(), TerrainWater, Error)
		|| !InitialWater.Initialize(Initial.Identity.WorldId, Initial.Clock.GameSeconds, Initial.Clock.WorldSeconds, Initial.Clock.CompletedStepId, Error)
		|| !InitialWater.AddRegion(SnowGrid(), Error) || !InitialWater.AddRegion(WaterGrid(), Error) || !InitialWater.AddRegion(TerrainWater, Error)
		|| !InitialWater.Capture(Initial.Surface, Error) || !FCCLWorldSnapshotCodec::Encode(Initial, InitialSnapshot, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_WATER_INIT baseline water: %s"), *Error);
		return;
	}
	GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>()->ExperimentInitialSnapshots.FindOrAdd(Initial.Identity.Domain) = InitialSnapshot;
	bWaterReady = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_WATER_INIT ready cells=5504 world=%s"), *Runtime->GetIdentity().WorldId.ToString());
}

bool ACCLExperimentDirector::WaterAction(ECCLExperimentAction Action, FName CaseId, FString& Error)
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const FGuid Id = CaseId == TEXT("Zone_05") ? TerrainWaterRegionId() : WaterRegionId();
	const auto* G = Runtime->GetSurfaceSimulation().FindRegion(Id);
	if (!bWaterReady || !G || (CaseId != TEXT("Zone_04") && CaseId != TEXT("Zone_05")))
	{
		Error = TEXT("물 또는 지형 구역을 선택한 뒤 준비를 기다려 줘.");
		return false;
	}
	auto F = G->Forcing;
	if (Action == ECCLExperimentAction::WaterRain)
	{
		F.RainMetersPerWorldSecond = F.RainMetersPerWorldSecond > 0. ? 0. : 0.00005;
	}
	else if (Action == ECCLExperimentAction::WaterFreeze)
	{
		F.TemperatureCelsius = -15.;
		F.RainMetersPerWorldSecond = 0.;
		F.EvaporationMetersPerWorldSecond = 0.;
		F.PhaseMetersPerDegreeWorldSecond = 0.0002;
	}
	else
	{
		F.TemperatureCelsius = 20.;
		F.PhaseMetersPerDegreeWorldSecond = 0.0002;
		F.RainMetersPerWorldSecond = 0.;
		F.EvaporationMetersPerWorldSecond = Action == ECCLExperimentAction::WaterDry ? 0.0001 : 0.;
	}
	if (!Runtime->ChangeSurfaceForcing(Id, F, Error))
	{
		return false;
	}
	Error = TEXT("선택 구역의 강수·기온 시험 입력을 적용했다. 자연 날씨는 6단계에서 연결한다.");
	return true;
}

bool ACCLExperimentDirector::StartWaterExperiment(FString& Error)
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* G = Runtime->GetSurfaceSimulation().FindRegion(WaterRegionId());
	if (!bWaterReady || !G || Runtime->GetClock().GetTimeScale() <= 0.)
	{
		Error = TEXT("물이 준비되고 세계 시간이 흐를 때 시작할 수 있다.");
		return false;
	}
	WaterCaseForcing = G->Forcing;
	WaterCaseTime = Runtime->GetClock().GetWorldSeconds();
	WaterCaseRain = G->Ledger.RainCubicMeters;
	auto F = G->Forcing;
	F.RainMetersPerWorldSecond = 0.0002;
	F.TemperatureCelsius = 15.;
	F.EvaporationMetersPerWorldSecond = 0.;
	F.PhaseMetersPerDegreeWorldSecond = 0.002;
	if (!Runtime->ChangeSurfaceForcing(WaterRegionId(), F, Error))
	{
		return false;
	}
	WaterCaseStep = 1;
	return true;
}

void ACCLExperimentDirector::TickWaterExperiment()
{
	if (ActiveCase != TEXT("Zone_04") || !WaterCaseStep)
	{
		return;
	}
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const double Now = Runtime->GetClock().GetWorldSeconds();
	if (Now - WaterCaseTime < 30.)
	{
		return;
	}
	auto* Result = Results.FindByPredicate([](const auto& R) { return R.CaseId == TEXT("Zone_04"); });
	const auto* G = Runtime->GetSurfaceSimulation().FindRegion(WaterRegionId());
	FString Error;
	if (!G || !Runtime->GetSurfaceSimulation().Validate(Error))
	{
		Finish(*Result, false, Error);
		return;
	}
	auto F = G->Forcing;
	if (WaterCaseStep == 1)
	{
		if (G->Ledger.RainCubicMeters <= WaterCaseRain)
		{
			Finish(*Result, false, TEXT("강수 입력이 물 수지에 반영되지 않았다."));
			return;
		}
		F.RainMetersPerWorldSecond = 0.;
		F.TemperatureCelsius = -15.;
	}
	else if (WaterCaseStep == 2)
	{
		WaterCaseIce = IceTotal(*G);
		if (WaterCaseIce <= 0.)
		{
			Finish(*Result, false, TEXT("영하 입력에서 결빙이 일어나지 않았다."));
			return;
		}
		F.TemperatureCelsius = 15.;
		F.EvaporationMetersPerWorldSecond = 0.0001;
	}
	else
	{
		const bool bPassed = IceTotal(*G) < WaterCaseIce && FMath::Abs(G->BalanceErrorCubicMeters()) < 1.e-6;
		Finish(*Result, bPassed, FString::Printf(TEXT("강수·결빙·융해·건조 검사: 총량 %.6f m³, 수지 오차 %.9f m³"), G->TotalCubicMeters(), G->BalanceErrorCubicMeters()));
		return;
	}
	if (!Runtime->ChangeSurfaceForcing(WaterRegionId(), F, Error))
	{
		Finish(*Result, false, Error);
		return;
	}
	WaterCaseTime = Now;
	++WaterCaseStep;
}
