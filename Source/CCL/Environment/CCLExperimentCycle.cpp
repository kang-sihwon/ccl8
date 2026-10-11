#include "CCLExperimentDirector.h"

#include "CCLWorldSimulationSubsystem.h"
#include "Engine/World.h"

namespace
{
	struct FCycleTotals
	{
		double Snow = 0.;
		double Volume = 0.;
		double Ice = 0.;
		double Downstream = 0.;
		double Mud = 0.;
	};

	FCycleTotals MeasureCycle(const FKSISurfaceGrid& Grid)
	{
		FCycleTotals Result;
		for (int32 Index = 0; Index < Grid.Cells.Num(); ++Index)
		{
			const auto& Cell = Grid.Cells[Index];
			Result.Snow += Cell.SnowCubicMeters;
			Result.Volume += Cell.SnowVolumeCubicMeters;
			Result.Ice += Cell.IceCubicMeters;
			if (Index % Grid.Size.X >= Grid.Size.X / 2)
			{
				Result.Downstream += Cell.WaterCubicMeters;
			}

			Result.Mud = FMath::Max(Result.Mud, Grid.Mud(Index));
		}
		return Result;
	}
}

FKSISurfaceGrid ACCLExperimentDirector::CycleGrid()
{
	FKSISurfaceGrid Grid;
	Grid.RegionId = CycleRegionId();
	Grid.OriginMeters = UCCLExperimentDefinition::ZoneCenter(9) / 100. + FVector(-4., -4., 0.);
	Grid.Size = FIntPoint(16, 16);
	Grid.Cells.SetNum(256);
	Grid.Forcing.InfiltrationMetersPerWorldSecond = 0.;
	Grid.Forcing.DrainageMetersPerWorldSecond = 0.;
	Grid.Forcing.SoilCapacityMeters = 0.005;
	for (int32 Index = 0; Index < Grid.Cells.Num(); ++Index)
	{
		auto& Cell = Grid.Cells[Index];
		const int32 X = Index % Grid.Size.X;
		Cell.BedMeters = 0.2 + (15 - X) * 0.04;
		Cell.CeilingMeters = 8.;
		Cell.RainExposure = X < 4 ? 1. : 0.;
	}
	return Grid;
}

bool ACCLExperimentDirector::StartCycleExperiment(FString& Error)
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* Grid = Runtime->GetSurfaceSimulation().FindRegion(CycleRegionId());
	if (!bWaterReady || !Grid || Runtime->GetClock().GetTimeScale() <= 0.)
	{
		Error = TEXT("순환 표면이 준비되고 세계 시간이 흐를 때 시작할 수 있다.");
		return false;
	}
	CycleOriginalForcing = Grid->Forcing;
	CycleCount = 0;
	return SetCyclePhase(0, Error);
}

bool ACCLExperimentDirector::SetCyclePhase(int32 Phase, FString& Error)
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* Grid = Runtime->GetSurfaceSimulation().FindRegion(CycleRegionId());
	if (!Grid || Phase < 0 || Phase > 5)
	{
		Error = TEXT("순환 단계의 표면이 없다.");
		return false;
	}

	auto Forcing = Grid->Forcing;
	Forcing.RainMetersPerWorldSecond = 0.;
	Forcing.SnowMetersPerWorldSecond = Phase == 1 ? 0.002 : 0.;
	Forcing.TemperatureCelsius = Phase == 1 || Phase == 2 || Phase == 4 ? -10. : 20.;
	Forcing.PhaseMetersPerDegreeWorldSecond = Phase == 1 || Phase == 2 ? 0. : 0.002;
	Forcing.InfiltrationMetersPerWorldSecond = Phase == 3 ? 0.0002 : 0.;
	Forcing.DrainageMetersPerWorldSecond = 0.;
	Forcing.EvaporationMetersPerWorldSecond = Phase == 0 || Phase == 5 ? 0.02 : 0.;
	if (!Runtime->ChangeSurfaceForcing(CycleRegionId(), Forcing, Error))
	{
		return false;
	}

	CyclePhase = Phase;
	CyclePhaseTime = Runtime->GetClock().GetWorldSeconds();
	CyclePhaseGameTime = Runtime->GetClock().GetGameSeconds();
	static const TCHAR* Labels[] = { TEXT("시작 상태 건조"), TEXT("경사 상단 적설"), TEXT("압축 흔적"),
		TEXT("융해·유출·진흙"), TEXT("결빙"), TEXT("융해·건조") };
	if (auto* Result = Results.FindByPredicate([](const auto& Value) { return Value.CaseId == TEXT("Zone_09"); }))
	{
		Result->Detail = FString::Printf(TEXT("순환 %d/2: %s. 명시적인 시험 입력을 사용한다."), CycleCount + 1, Labels[Phase]);
	}
	ForceNetUpdate();
	return true;
}

void ACCLExperimentDirector::StopCycleExperiment()
{
	if (CyclePhase < 0)
	{
		return;
	}

	FString Error;
	if (auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>())
	{
		if (!Runtime->ChangeSurfaceForcing(CycleRegionId(), CycleOriginalForcing, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("CCL_CYCLE restore forcing: %s"), *Error);
		}
	}
	CyclePhase = -1;
}

void ACCLExperimentDirector::TickCycleExperiment()
{
	if (ActiveCase != TEXT("Zone_09") || CyclePhase < 0)
	{
		return;
	}

	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const double Duration = CyclePhase == 1 || CyclePhase == 2 ? 30. : 60.;
	const double WorldElapsed = Runtime->GetClock().GetWorldSeconds() - CyclePhaseTime;
	const double GameElapsed = Runtime->GetClock().GetGameSeconds() - CyclePhaseGameTime;
	// Melting uses world time, but downhill transport needs elapsed game time.
	if (WorldElapsed < Duration || (CyclePhase == 3 && GameElapsed < 10.))
	{
		return;
	}

	auto* Result = Results.FindByPredicate([](const auto& Value) { return Value.CaseId == TEXT("Zone_09"); });
	const auto* Grid = Runtime->GetSurfaceSimulation().FindRegion(CycleRegionId());
	if (!Result)
	{
		StopCycleExperiment();
		return;
	}

	FString Error;
	if (!Grid || !FKSISurfaceSimulation::ValidateRegion(*Grid, Error)
		|| FMath::Abs(Grid->BalanceErrorCubicMeters()) > 1.e-7)
	{
		Finish(*Result, false, TEXT("순환 표면의 유효성 또는 물질 수지가 깨졌다. ") + Error);
		return;
	}
	const FCycleTotals Totals = MeasureCycle(*Grid);
	bool bPassed = true;
	if (CyclePhase == 0 || CyclePhase == 5)
	{
		bPassed = Grid->TotalCubicMeters() < 1.e-7;
	}
	else if (CyclePhase == 1)
	{
		bPassed = Totals.Snow > 0.5;
		const double MassBefore = Grid->TotalCubicMeters();
		for (int32 Index = 0; bPassed && Index < 8; ++Index)
		{
			const FVector Contact = Grid->OriginMeters + FVector(0.75, 0.5 + Index * 0.8, 0.);
			// One bounded contact source per experiment; sequences continue across runs.
			const FGuid Source(909, 910, 911, 912);
			const uint64 Sequence = Runtime->GetSurfaceSimulation().LastSnowContact(Source) + 1;
			bPassed = Runtime->ApplySnowContact(CycleRegionId(), Source, Sequence, Contact, 0.4, Error);
			Grid = Runtime->GetSurfaceSimulation().FindRegion(CycleRegionId());
		}
		if (bPassed)
		{
			bPassed = FMath::Abs(Grid->TotalCubicMeters() - MassBefore) < 1.e-9
				&& MeasureCycle(*Grid).Volume < Totals.Volume - 0.01;
		}
	}
	else if (CyclePhase == 2)
	{
		bPassed = Totals.Snow > 0.5;
	}
	else if (CyclePhase == 3)
	{
		bPassed = Totals.Snow < 1.e-7 && Totals.Downstream > 0.001 && Totals.Mud > 0.1;
	}
	else if (CyclePhase == 4)
	{
		bPassed = Totals.Ice > 0.001;
	}
	UE_LOG(LogTemp, Display, TEXT("CCL_CYCLE round=%d phase=%d mass=%.9f snow=%.9f downstream=%.9f mud=%.4f ice=%.9f balance=%.12f world_elapsed=%.3f game_elapsed=%.3f pass=%d"),
		CycleCount + 1, CyclePhase, Grid->TotalCubicMeters(), Totals.Snow, Totals.Downstream, Totals.Mud, Totals.Ice, Grid->BalanceErrorCubicMeters(), WorldElapsed, GameElapsed, bPassed);
	if (!bPassed)
	{
		Finish(*Result, false, FString::Printf(TEXT("순환 %d 단계 %d의 실제 상태가 예상 조건과 다르다. %s"), CycleCount + 1, CyclePhase, *Error));
		return;
	}
	if (CyclePhase == 5 && ++CycleCount == 2)
	{
		TArray<uint8> Bytes, RestoredBytes;
		FKSISurfaceSimulation Restored;
		bPassed = Runtime->GetSurfaceSimulation().Capture(Bytes, Error) && Restored.Restore(Bytes, Error)
			&& Restored.Capture(RestoredBytes, Error) && Bytes == RestoredBytes;
		Finish(*Result, bPassed, bPassed ? TEXT("적설·압축·융해·유출·진흙·결빙·건조 두 회차와 표면 저장 복원을 확인했다. 영구 수로는 구역 05에서 별도 검증한다.") : Error);
		return;
	}

	if (!SetCyclePhase(CyclePhase == 5 ? 1 : CyclePhase + 1, Error))
	{
		Finish(*Result, false, Error);
	}
}
