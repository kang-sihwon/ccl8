#include "CCLCombustionModel.h"
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCombustionBalanceTest, "CCL.Environment.Fire.FuelHeatWaterAndWind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLCombustionBalanceTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLCombustionMaterial M;
	FCCLCombustionCell Dry, Wet;
	TestTrue(TEXT("initialize"), FCCLCombustionModel::Initialize(M, 1., 20., Dry, Error));
	TestTrue(TEXT("external ignition heat"), FCCLCombustionModel::AddHeat(M, 500000., Dry, Error));
	Wet = Dry;
	FCCLCombustionCell Windy = Dry;
	FCCLCombustionFlux A, B, C;
	TestTrue(TEXT("dry burn"), FCCLCombustionModel::Advance(M, 10., 20., 0., 0., Dry, A, Error));
	TestTrue(TEXT("wet fuel"), FCCLCombustionModel::Advance(M, 10., 20., 0., 1., Wet, B, Error));
	TestTrue(TEXT("wind"), FCCLCombustionModel::Advance(M, 10., 20., 8., 0., Windy, C, Error));
	TestTrue(TEXT("dry fuel consumed"), Dry.BurnedKg > 0.);
	TestEqual(TEXT("wet fuel inhibits ignition"), Wet.BurnedKg, 0.);
	TestTrue(TEXT("wind affects combustion"), Windy.BurnedKg > Dry.BurnedKg);
	TestTrue(TEXT("wet surface draws heat"), B.VaporizedWaterKg > 0. && B.VaporizedWaterKg <= 1.);
	TestTrue(TEXT("snow can receive excess heat"), A.SurfaceHeatJoules > 0.);
	TestTrue(TEXT("fuel ledger"), FMath::IsNearlyEqual(Dry.FuelKg + Dry.BurnedKg, Dry.InitialFuelKg, 1.e-10));
	TestTrue(TEXT("energy ledger"), FMath::Abs(Dry.HeatBalanceError()) < 1.e-6);
	TestTrue(TEXT("smoke derives from burned fuel"), FMath::IsNearlyEqual(A.SmokeKg, Dry.BurnedKg * M.SmokeKgPerFuelKg, 1.e-12));
	FCCLCombustionFlux Quenched;
	const double BurnedBefore = Dry.BurnedKg;
	TestTrue(TEXT("rain or deliberate water extinguishes"), FCCLCombustionModel::Advance(M, 10., 20., 0., 10., Dry, Quenched, Error));
	TestEqual(TEXT("no fuel consumed during quench"), Dry.BurnedKg, BurnedBefore);
	TestFalse(TEXT("quenched fire no longer burning"), Dry.bBurning != 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCombustionResumeTest, "CCL.Environment.Fire.ResumeValidationAndExhaustion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLCombustionResumeTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLCombustionMaterial M;
	FCCLCombustionCell S;
	TestTrue(TEXT("initialize hot small fuel"), FCCLCombustionModel::Initialize(M, 0.001, 300., S, Error));
	FCCLCombustionFlux F;
	TestTrue(TEXT("burn all available fuel"), FCCLCombustionModel::Advance(M, 1., 20., 0., 0., S, F, Error));
	TestEqual(TEXT("cannot overspend fuel"), S.FuelKg, 0.);
	TestFalse(TEXT("exhausted fuel stops fire"), S.bBurning != 0);
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	FCCLCombustionModel::Serialize(Writer, M, S);
	FCCLCombustionCell Restored;
	FCCLCombustionMaterial RestoredMaterial;
	FMemoryReader Reader(Bytes);
	FCCLCombustionModel::Serialize(Reader, RestoredMaterial, Restored);
	TestTrue(TEXT("restore validates"), FCCLCombustionModel::Validate(RestoredMaterial, Restored, Error));
	FCCLCombustionFlux RestoredFlux;
	TestTrue(TEXT("resume original"), FCCLCombustionModel::Advance(M, 30., -8., 3., 0., S, F, Error));
	TestTrue(TEXT("resume loaded"), FCCLCombustionModel::Advance(RestoredMaterial, 30., -8., 3., 0., Restored, RestoredFlux, Error));
	TestEqual(TEXT("restored heat follows same history"), Restored.HeatJoules, S.HeatJoules);
	const double Heat = S.HeatJoules;
	F.SmokeKg = 123.;
	TestFalse(TEXT("invalid wind"), FCCLCombustionModel::Advance(M, 1., 20., std::numeric_limits<double>::quiet_NaN(), 0., S, F, Error));
	TestEqual(TEXT("invalid input retains state"), S.HeatJoules, Heat);
	TestEqual(TEXT("invalid input retains outputs"), F.SmokeKg, 123.);
	TestFalse(TEXT("unbounded work refused"), FCCLCombustionModel::Advance(M, 410., 20., 0., 0., S, F, Error));
	TestTrue(TEXT("zero game time"), FCCLCombustionModel::Advance(M, 0., 20., 0., 0., S, F, Error));
	TestEqual(TEXT("zero game time retains heat"), S.HeatJoules, Heat);
	TestEqual(TEXT("zero game time no transfers"), F.SurfaceHeatJoules, 0.);
	return true;
}
#endif
