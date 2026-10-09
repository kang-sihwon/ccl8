#include "CCLExperimentDirector.h"

#include "CCLWorldSimulationSubsystem.h"
#include "GameFramework/Pawn.h"

FCCLSurfaceGrid ACCLExperimentDirector::SnowGrid()
{
	FCCLSurfaceGrid G;
	G.RegionId = SnowRegionId();
	G.OriginMeters = UCCLExperimentDefinition::ZoneCenter(3) / 100. + FVector(-4., -4., 0.);
	G.Size = FIntPoint(64, 64);
	G.SpacingMeters = 0.125;
	G.Forcing.TemperatureCelsius = -5.;
	G.Forcing.FlowConductance = 0.2;
	G.Cells.SetNum(4096);
	const double Area = FMath::Square(G.SpacingMeters);
	for (int32 I = 0; I < G.Cells.Num(); ++I)
	{
		auto& C = G.Cells[I];
		C.BedMeters = 0.2;
		C.CeilingMeters = 4.;
		C.SnowVolumeCubicMeters = Area * (I % 64 < 8 ? 0.08 : 0.6);
		C.SnowCubicMeters = C.SnowVolumeCubicMeters * 0.1;
	}
	G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
	return G;
}

bool ACCLExperimentDirector::SnowAction(ECCLExperimentAction Action, FName CaseId, FString& Error)
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* Grid = Runtime->GetSurfaceSimulation().FindRegion(SnowRegionId());
	if (CaseId != TEXT("Zone_03") || !Grid)
	{
		Error = TEXT("눈 구역을 선택하고 준비를 기다려 줘.");
		return false;
	}
	auto F = Grid->Forcing;
	F.SnowMetersPerWorldSecond = Action == ECCLExperimentAction::SnowFall ? (F.SnowMetersPerWorldSecond > 0. ? 0. : 0.00002) : 0.;
	F.TemperatureCelsius = Action == ECCLExperimentAction::SnowMelt ? 8. : -5.;
	F.PhaseMetersPerDegreeWorldSecond = 0.00001;
	return Runtime->ChangeSurfaceForcing(SnowRegionId(), F, Error);
}

bool ACCLExperimentDirector::StartSnowExperiment(FString& Error)
{
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* Grid = Runtime->GetSurfaceSimulation().FindRegion(SnowRegionId());
	if (!Grid || !SnowWalker.IsValid())
	{
		Error = TEXT("눈 구역과 조작 담당 캐릭터가 필요하다.");
		return false;
	}
	SnowStartVolume = 0.;
	for (const auto& C : Grid->Cells)
	{
		SnowStartVolume += C.SnowVolumeCubicMeters;
	}
	SnowWalkStart = SnowWalker->GetActorLocation();
	if (auto* R = Results.FindByPredicate([](const auto& V) { return V.CaseId == TEXT("Zone_03"); }))
	{
		R->Detail = TEXT("창을 닫고 눈 속을 3m 이상 걸어 줘. 압축 흔적과 질량을 검사한다.");
	}
	return true;
}

void ACCLExperimentDirector::TickSnowExperiment()
{
	if (ActiveCase != TEXT("Zone_03"))
	{
		return;
	}
	auto* R = Results.FindByPredicate([](const auto& V) { return V.CaseId == TEXT("Zone_03"); });
	const auto* Grid = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>()->GetSurfaceSimulation().FindRegion(SnowRegionId());
	if (!R || !Grid || !SnowWalker.IsValid())
	{
		return;
	}
	double Volume = 0.;
	int32 Compacted = 0;
	for (const auto& C : Grid->Cells)
	{
		Volume += C.SnowVolumeCubicMeters;
		Compacted += C.SnowCubicMeters > 0. && C.SnowVolumeCubicMeters < C.SnowCubicMeters / 0.12;
	}
	if (Compacted >= 12 && SnowStartVolume - Volume > 0.04 && FVector::Dist2D(SnowWalkStart, SnowWalker->GetActorLocation()) >= 300.)
	{
		FString Error;
		const bool bValid = FCCLSurfaceSimulation::ValidateRegion(*Grid, Error);
		Finish(*R, bValid, bValid ? FString::Printf(TEXT("보행·압축 흔적 %d셀, 물질 수지 오차 %.9f m³. 발 배치 화면도 확인해 줘."), Compacted, Grid->BalanceErrorCubicMeters()) : Error);
	}
}
