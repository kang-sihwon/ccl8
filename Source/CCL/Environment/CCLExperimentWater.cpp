#include "CCLExperimentDirector.h"
#include "CCLFoodExperiment.h"
#include "CCLBiologyExperiment.h"
#include "CCLEcologyExperiment.h"

#include "CCLWorldSimulationSubsystem.h"
#include "CCLWorldEnvironmentConfig.h"
#include "GameFramework/Character.h"
#include "CCLTerrainWaterParticipant.h"
#include "CCLSurfacePresentation.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

namespace
{
	FKSISurfaceGrid WaterGrid()
	{
		FKSISurfaceGrid G;
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

	bool TerrainGrid(const FKSITerrainSnapshot& Terrain, const FTransform& Transform, FKSISurfaceGrid& G, FString& Error)
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
		return FKSISurfaceSimulation::ValidateRegion(G, Error);
	}

	double IceTotal(const FKSISurfaceGrid& G)
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
	if (bWaterReady)
	{
		return;
	}
	const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld());
	const bool bLab = Config && Config->LaboratoryZone >= 0;
	const auto Allows = [&](int32 Zone, int32 Subject = -1) { return ACCLWorldEnvironmentConfig::AllowsLaboratory(GetWorld(), Zone, Subject); };
	const bool bTerrain = Allows(5) || Allows(8);
	if (bTerrain && (!TerrainRegion || !TerrainRegion->IsTerrainReady() || TerrainRegion->IsPreparing()))
	{
		return;
	}
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!Runtime || !Runtime->IsRunning())
	{
		return;
	}
	FString Error;
	FCCLWorldSnapshot Initial;
	if (!FCCLWorldSnapshotCodec::Decode(InitialSnapshot, Initial, Error))
	{
		return;
	}
	const bool bFresh = Runtime->GetSurfaceSimulation().GetRegions().IsEmpty();
	auto BuildRegions = [&](double Time, bool bBaseline, TArray<FKSISurfaceGrid>& Grids)
	{
		if (Allows(2))
		{
			Grids.Add(WeatherGrid());
		}
		if (Allows(3))
		{
			Grids.Add(SnowGrid());
		}
		if (Allows(4))
		{
			auto G = WaterGrid();
			if (bLab)
			{
				G.Forcing.BoundaryLevelMeters = 0.2;
				for (auto& Cell : G.Cells)
				{
					Cell.WaterCubicMeters = 0.;
				}
				G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
			}
			Grids.Add(MoveTemp(G));
		}
		if (Allows(6) && (!bLab || Config->LaboratoryVariant == 1))
		{
			Grids.Add(ACCLEcologyExperiment::Pond());
		}
		if (Allows(7, 0))
		{
			FKSISurfaceGrid Plants;
			if (!VegetationGrid(Time, Plants, Error, bLab ? Config->LaboratoryVariant : -1, bLab ? Config->SampleCount() : 3))
			{
				return false;
			}
			Grids.Add(MoveTemp(Plants));
		}
		if (Allows(9))
		{
			Grids.Add(CycleGrid());
		}
		if (Allows(10))
		{
			Grids.Add(FireGrid(false));
		}
		if (Allows(11))
		{
			Grids.Add(FireGrid(true));
		}
		if (bTerrain)
		{
			FKSITerrainStore Base;
			auto Definition = TerrainRegion->GetTerrainStore().GetSnapshot().Definition;
			Definition.WorldId = Initial.Identity.WorldId;
			if (bBaseline && !Base.Initialize(Definition, Error))
			{
				return false;
			}
			FKSISurfaceGrid G;
			if (!TerrainGrid(bBaseline ? Base.GetSnapshot() : TerrainRegion->GetTerrainStore().GetSnapshot(), TerrainRegion->GetActorTransform(), G, Error))
			{
				return false;
			}
			if (bLab && Config->LaboratoryZone == 5)
			{
				for (auto& Cell : G.Cells)
				{
					Cell.WaterCubicMeters = 0.;
				}
				G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
			}
			Grids.Add(MoveTemp(G));
		}
		return true;
	};
	TArray<FKSISurfaceGrid> CurrentGrids, BaselineGrids;
	if (!BuildRegions(Runtime->GetClock().GetWorldSeconds(), false, CurrentGrids) || !BuildRegions(Initial.Clock.WorldSeconds, true, BaselineGrids))
	{
		return;
	}
	for (const auto& G : CurrentGrids)
	{
		if (!Runtime->GetSurfaceSimulation().FindRegion(G.RegionId) && !Runtime->AddSurfaceRegion(G, Error))
		{
			return;
		}
	}
	if (Allows(11) && !Runtime->GetSurfaceSimulation().GetAirSimulation().FindSpace(FireSpaceId()) && !Runtime->AddAirSpace(FireRoom(), Error))
	{
		return;
	}
	if (!bWaterParticipantBound)
	{
		if (bTerrain && !TerrainRegion->AddParticipant(MakeShared<FCCLTerrainWaterParticipant>(Runtime, TerrainWaterRegionId(), TerrainRegion->GetActorTransform()), Error))
		{
			return;
		}
		bWaterParticipantBound = 1;
		GetWorld()->SpawnActor<ACCLSurfacePresentation>();
	}
	if (Allows(7, 1))
	{
		auto* Biology = ACCLBiologyExperiment::Find(GetWorld());
		if (!Biology)
		{
			Biology = GetWorld()->SpawnActor<ACCLBiologyExperiment>();
		}
		if (Biology && Config && !Biology->ResidentClass)
		{
			Biology->ResidentClass = Config->ExperimentResidentClass.LoadSynchronous();
		}
		if (!Biology || !Biology->Initialize(bFresh, Initial, Error))
		{
			return;
		}
	}
	if (Allows(7, 2))
	{
		auto* Food = ACCLFoodExperiment::Find(GetWorld());
		if (!Food)
		{
			Food = GetWorld()->SpawnActor<ACCLFoodExperiment>(UCCLExperimentDefinition::ZoneCenter(7) + FVector(0, -650, 0), FRotator::ZeroRotator);
		}
		if (!Food || !Food->Initialize(bFresh, Initial, Error))
		{
			return;
		}
	}
	if (Allows(6))
	{
		auto* Ecology = ACCLEcologyExperiment::Find(GetWorld());
		if (!Ecology)
		{
			Ecology = GetWorld()->SpawnActor<ACCLEcologyExperiment>(UCCLExperimentDefinition::ZoneCenter(6), FRotator::ZeroRotator);
		}
		if (!Ecology || !Ecology->Initialize(bFresh, Initial, Error))
		{
			return;
		}
	}
	FKSISurfaceSimulation InitialWater;
	if (!InitialWater.Initialize(Initial.Identity.WorldId, Initial.Clock.GameSeconds, Initial.Clock.WorldSeconds, Initial.Clock.CompletedStepId, Error))
	{
		return;
	}
	for (const auto& G : BaselineGrids)
	{
		if (!InitialWater.AddRegion(G, Error))
		{
			return;
		}
	}
	if (Allows(11) && !InitialWater.AddAirSpace(FireRoom(), Error))
	{
		return;
	}
	if (!InitialWater.Capture(Initial.Surface, Error) || !FCCLWorldSnapshotCodec::Encode(Initial, InitialSnapshot, Error))
	{
		return;
	}
	if (!bLab)
	{
		GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>()->ExperimentInitialSnapshots.FindOrAdd(Initial.Identity.Domain) = InitialSnapshot;
	}
	bWaterReady = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_LAB_READY zone=%d subject=%d regions=%d"), Config ? Config->LaboratoryZone : -1, Config ? Config->LaboratorySubject : 0, CurrentGrids.Num());
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
		F.RainMetersPerWorldSecond = F.RainMetersPerWorldSecond > 0. ? 0. : 0.000005;
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
	Error = TEXT("환경 조건을 적용했다. 물의 변화는 세계 시간에 따라 진행된다. 비는 18 mm/h이며 높은 배율에서는 수위도 빠르게 오른다.");
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
