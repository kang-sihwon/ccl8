#include "KSIAirSimulation.h"

#include "KSISurfaceQuery.h"
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FGuid RoomId(int32 I)
	{
		return FGuid(810, 811, 812, I);
	}

	FKSIAirSpace Room(int32 Id, double Temperature = 20., double Vapor = 0.008)
	{
		FKSIAirSpace S;
		S.SpaceId = RoomId(Id);
		S.MinimumMeters = FVector(Id * 4., 0., 0.);
		S.MaximumMeters = S.MinimumMeters + FVector(4.);
		S.VolumeCubicMeters = 64.;
		FString Error;
		S.Initialize(Temperature, Vapor, Error);
		return S;
	}

	FKSISurfaceOpening Door(int32 Id, FGuid A, FGuid B, double Fraction)
	{
		FKSISurfaceOpening O;
		O.OpeningId = FGuid(820, 821, 822, Id);
		O.SurfaceId = FGuid(830, 831, 832, Id);
		O.SpaceA = A;
		O.SpaceB = B;
		O.OpenFraction = Fraction;
		return O;
	}

	TArray<uint8> CaptureAir(FKSIAirSimulation S)
	{
		TArray<uint8> Bytes;
		FMemoryWriter Ar(Bytes);
		S.Serialize(Ar);
		return Bytes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAirDoorTest, "CCL.Environment.Air.ClosedHalfOpenAndExteriorLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAirDoorTest::RunTest(const FString& Parameters)
{
	FString Error;
	FKSIAirSimulation Closed, Half, Open, Windy;
	const auto S = Room(1, 60., 0.04);
	TestTrue(TEXT("room"), Closed.AddSpace(S, Error));
	TestTrue(TEXT("smoke and heat source"), Closed.AddSource(S.SpaceId, 10000., 0.2, 1., Error));
	Half = Open = Windy = Closed;
	TMap<FGuid, FKSIAirExterior> Boundary;
	Boundary.Add(S.SpaceId, {});
	const auto Initial = CaptureAir(Closed);
	TestTrue(TEXT("closed"), Closed.Advance(30., {Door(1, S.SpaceId, {}, 0.)}, Boundary, Error));
	TestTrue(TEXT("closed room has no unexplained cooling or loss"), Initial == CaptureAir(Closed));
	TestTrue(TEXT("half open"), Half.Advance(30., {Door(1, S.SpaceId, {}, 0.5)}, Boundary, Error));
	TestTrue(TEXT("fully open"), Open.Advance(30., {Door(1, S.SpaceId, {}, 1.)}, Boundary, Error));
	Boundary[S.SpaceId].WindMetersPerSecond = 20.;
	TestTrue(TEXT("windy opening"), Windy.Advance(30., {Door(1, S.SpaceId, {}, 1.)}, Boundary, Error));
	TestTrue(TEXT("wind increases exchange"), Windy.Total().SmokeKg < Open.Total().SmokeKg);
	TestTrue(TEXT("half door vents smoke"), Half.Total().SmokeKg < Closed.Total().SmokeKg);
	TestTrue(TEXT("full door vents more"), Open.Total().SmokeKg < Half.Total().SmokeKg);
	TestTrue(TEXT("opening releases heat"), Open.FindSpace(S.SpaceId)->TemperatureCelsius() < Half.FindSpace(S.SpaceId)->TemperatureCelsius());
	TestTrue(TEXT("opening releases moisture"), Open.Total().VaporKg < Half.Total().VaporKg);
	TestTrue(TEXT("smoke exported exactly"), FMath::IsNearlyEqual(Open.Total().SmokeKg + Open.GetExteriorExported().SmokeKg, 1., 1.e-10));
	TestTrue(TEXT("exterior heat replaces some outgoing air"), Open.GetExteriorImported().HeatJoules > 0.);
	TestTrue(TEXT("all inventories validate"), Closed.Validate(Error) && Half.Validate(Error) && Open.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAirInteriorTest, "CCL.Environment.Air.InteriorConservesHeatVaporSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAirInteriorTest::RunTest(const FString& Parameters)
{
	FString Error;
	FKSIAirSimulation S;
	TestTrue(TEXT("hot room"), S.AddSpace(Room(1, 80., 0.04), Error));
	auto Cold = Room(2, 0., 0.001);
	Cold.MaximumMeters.X += 4.;
	Cold.VolumeCubicMeters = 128.;
	TestTrue(TEXT("larger cold room initialized"), Cold.Initialize(0., 0.001, Error));
	TestTrue(TEXT("larger cold room"), S.AddSpace(Cold, Error));
	TestTrue(TEXT("smoke source"), S.AddSource(RoomId(1), 0., 0., 2., Error));
	const auto Initial = S.Total();
	TestTrue(TEXT("exchange through internal door"), S.Advance(120., {Door(1, RoomId(1), RoomId(2), 1.)}, {}, Error));
	TestTrue(TEXT("cold room warms"), S.FindSpace(RoomId(2))->TemperatureCelsius() > 0.);
	TestTrue(TEXT("hot room cools"), S.FindSpace(RoomId(1))->TemperatureCelsius() < 80.);
	TestTrue(TEXT("smoke reaches adjacent room"), S.FindSpace(RoomId(2))->Contents.SmokeKg > 0.);
	TestTrue(TEXT("total heat conserved"), FMath::IsNearlyEqual(S.Total().HeatJoules, Initial.HeatJoules, 1.e-5));
	TestTrue(TEXT("total vapor conserved"), FMath::IsNearlyEqual(S.Total().VaporKg, Initial.VaporKg, 1.e-10));
	TestTrue(TEXT("total smoke conserved"), FMath::IsNearlyEqual(S.Total().SmokeKg, Initial.SmokeKg, 1.e-10));
	TestEqual(TEXT("internal door exports nothing"), S.GetExteriorExported().HeatJoules, 0.);
	TestEqual(TEXT("internal door imports nothing"), S.GetExteriorImported().HeatJoules, 0.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAirBudgetTest, "CCL.Environment.Air.MultipleDoorsFrozenBudgetAndOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAirBudgetTest::RunTest(const FString& Parameters)
{
	FString Error;
	FKSIAirSimulation S;
	for (int32 I = 1; I <= 6; ++I)
	{
		TestTrue(TEXT("room"), S.AddSpace(Room(I), Error));
	}
	TestTrue(TEXT("central smoke"), S.AddSource(RoomId(1), 0., 0., 1., Error));
	TArray<FKSISurfaceOpening> Doors;
	for (int32 I = 2; I <= 5; ++I)
	{
		auto O = Door(I, RoomId(1), RoomId(I), 1.);
		O.HalfExtentsMeters = FVector2D(1000.);
		Doors.Add(O);
	}
	auto Next = Door(6, RoomId(5), RoomId(6), 1.);
	Next.HalfExtentsMeters = FVector2D(1000.);
	Doors.Add(Next);
	auto Reversed = S;
	TArray<FKSISurfaceOpening> Reverse;
	for (int32 I = Doors.Num() - 1; I >= 0; --I)
	{
		Reverse.Add(Doors[I]);
	}
	TestTrue(TEXT("many doors"), S.Advance(0.25, Doors, {}, Error));
	TestTrue(TEXT("reversed input order"), Reversed.Advance(0.25, Reverse, {}, Error));
	TestTrue(TEXT("opening order cannot alter state"), CaptureAir(S) == CaptureAir(Reversed));
	TestEqual(TEXT("donor retains half despite four large doors"), S.FindSpace(RoomId(1))->Contents.SmokeKg, 0.5);
	for (int32 I = 2; I <= 5; ++I)
	{
		TestEqual(TEXT("four recipients symmetric"), S.FindSpace(RoomId(I))->Contents.SmokeKg, 0.125);
	}
	TestEqual(TEXT("received smoke cannot cross a second door in the same step"), S.FindSpace(RoomId(6))->Contents.SmokeKg, 0.);
	TestEqual(TEXT("total smoke"), S.Total().SmokeKg, 1.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAirResumeTest, "CCL.Environment.Air.ResumeAndAtomicFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAirResumeTest::RunTest(const FString& Parameters)
{
	FString Error;
	FKSIAirSimulation S, Loaded;
	TestTrue(TEXT("room"), S.AddSpace(Room(1), Error));
	TestTrue(TEXT("source"), S.AddSource(RoomId(1), 500000., 0.4, 0.5, Error));
	TMap<FGuid, FKSIAirExterior> Boundary;
	Boundary.Add(RoomId(1), {-8., 0.004, 8.});
	const TArray<FKSISurfaceOpening> Doors = {Door(1, {}, RoomId(1), 0.5)};
	TestTrue(TEXT("advance"), S.Advance(5., Doors, Boundary, Error));
	const auto Bytes = CaptureAir(S);
	FMemoryReader Ar(Bytes);
	TestTrue(TEXT("restore"), Loaded.Serialize(Ar));
	TestTrue(TEXT("original advance"), S.Advance(20., Doors, Boundary, Error));
	TestTrue(TEXT("restored advance"), Loaded.Advance(20., Doors, Boundary, Error));
	TestTrue(TEXT("resume exact"), CaptureAir(S) == CaptureAir(Loaded));
	const auto Before = CaptureAir(S);
	TestFalse(TEXT("cannot overdraft heat"), S.AddSource(RoomId(1), -1.e15, 0., 0., Error));
	TestFalse(TEXT("cannot create negative vapor"), S.AddSource(RoomId(1), 0., -1., 0., Error));
	TestFalse(TEXT("unknown neighbour refused"), S.Advance(1., {Door(2, RoomId(1), RoomId(99), 1.)}, Boundary, Error));
	TestFalse(TEXT("duplicate edge refused"), S.Advance(1., {Doors[0], Doors[0]}, Boundary, Error));
	Boundary[RoomId(1)].WindMetersPerSecond = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("invalid outdoor state refused"), S.Advance(1., Doors, Boundary, Error));
	TestTrue(TEXT("failed source and ventilation preserve all state"), Before == CaptureAir(S));
	TestTrue(TEXT("zero time retains state"), S.Advance(0., {}, {}, Error) && Before == CaptureAir(S));
	TestFalse(TEXT("work budget"), S.Advance(1025., {}, {}, Error));
	TArray<uint8> Truncated = Bytes;
	Truncated.SetNum(Truncated.Num() - 1);
	FMemoryReader Bad(Truncated);
	TestFalse(TEXT("truncated payload refused"), S.Serialize(Bad));
	TestTrue(TEXT("bad restore preserves all state"), Before == CaptureAir(S));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAirOwnershipTest, "CCL.Environment.Air.SpaceOwnershipAndHeatWithdrawal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAirOwnershipTest::RunTest(const FString& Parameters)
{
	FString Error;
	FKSIAirSimulation S;
	TestTrue(TEXT("first"), S.AddSpace(Room(1), Error));
	TestTrue(TEXT("adjacent"), S.AddSpace(Room(2), Error));
	const auto* Found = S.FindSpaceAt(FVector(8., 1., 1.));
	if (TestNotNull(TEXT("shared boundary has one owner"), Found))
	{
		TestEqual(TEXT("half open bounds pick second room"), Found->SpaceId, RoomId(2));
	}
	auto Overlap = Room(3);
	Overlap.MinimumMeters = FVector(5., 0., 0.);
	TestFalse(TEXT("overlap refused"), S.AddSpace(Overlap, Error));
	TestFalse(TEXT("duplicate room refused"), S.AddSpace(Room(1), Error));
	const double Heat = S.Total().HeatJoules;
	TestTrue(TEXT("surface may draw heat from air"), S.AddSource(RoomId(1), -10000., 0., 0., Error));
	TestTrue(TEXT("heat removed once"), FMath::IsNearlyEqual(S.Total().HeatJoules, Heat - 10000., 1.e-8));
	TestTrue(TEXT("withdrawal ledger"), S.Validate(Error));
	return true;
}
#endif
