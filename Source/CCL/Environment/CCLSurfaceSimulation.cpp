#include "CCLSurfaceSimulation.h"

#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr uint32 Magic = 0x534C4343;
	constexpr double MaximumQuantity = 1.e15;

	bool Quantity(double Value)
	{
		return FMath::IsFinite(Value) && Value >= 0. && Value <= MaximumQuantity;
	}

	bool Fail(FString& Error, const TCHAR* Message)
	{
		Error = Message;
		return false;
	}

	bool ValidForcing(const FCCLSurfaceForcing& F)
	{
		return Quantity(F.RainMetersPerWorldSecond) && F.RainMetersPerWorldSecond <= 1.
			&& Quantity(F.SnowMetersPerWorldSecond) && F.SnowMetersPerWorldSecond <= 1.
			&& Quantity(F.EvaporationMetersPerWorldSecond) && F.EvaporationMetersPerWorldSecond <= 1.
			&& Quantity(F.InfiltrationMetersPerWorldSecond) && F.InfiltrationMetersPerWorldSecond <= 1.
			&& Quantity(F.DrainageMetersPerWorldSecond) && F.DrainageMetersPerWorldSecond <= 1.
			&& Quantity(F.SoilCapacityMeters) && F.SoilCapacityMeters <= 10.
			&& FMath::IsFinite(F.TemperatureCelsius) && FMath::Abs(F.TemperatureCelsius) <= 1000.
			&& Quantity(F.PhaseMetersPerDegreeWorldSecond) && F.PhaseMetersPerDegreeWorldSecond <= 1.
			&& Quantity(F.FlowConductance) && F.FlowConductance <= 1000.
			&& FMath::IsFinite(F.BoundaryLevelMeters) && FMath::Abs(F.BoundaryLevelMeters) <= 1.e7
			&& Quantity(F.BoundaryConductance) && F.BoundaryConductance <= 1000.;
	}

	double Capacity(const FCCLSurfaceGrid& G, const FCCLSurfaceCell& Cell)
	{
		return (Cell.CeilingMeters - Cell.BedMeters) * FMath::Square(G.SpacingMeters);
	}

	bool GuidLess(const FGuid& A, const FGuid& B)
	{
		if (A.A != B.A)
		{
			return A.A < B.A;
		}
		if (A.B != B.B)
		{
			return A.B < B.B;
		}
		if (A.C != B.C)
		{
			return A.C < B.C;
		}
		return A.D < B.D;
	}

	struct FTransfer
	{
		int32 From;
		int32 To;
		double Amount;
	};

	bool AdvanceGrid(FCCLSurfaceGrid& G, double GameDelta, double WorldDelta, int32 Steps, FString& Error)
	{
		const double Area = FMath::Square(G.SpacingMeters);
		const double Dt = GameDelta / Steps;
		const double WorldDt = WorldDelta / Steps;
		const auto& F = G.Forcing;
		TArray<FTransfer> Transfers;
		TArray<double> Outgoing, Incoming, SourceScales, TargetScales, Remaining, RoomLeft, Added;
		Outgoing.SetNumZeroed(G.Cells.Num());
		Incoming.SetNumZeroed(G.Cells.Num());
		SourceScales.SetNumUninitialized(G.Cells.Num());
		TargetScales.SetNumUninitialized(G.Cells.Num());
		Remaining.SetNumUninitialized(G.Cells.Num());
		RoomLeft.SetNumUninitialized(G.Cells.Num());
		Added.SetNumUninitialized(G.Cells.Num());
		Transfers.Reserve(G.Cells.Num() * 2);
		for (int32 Substep = 0; Substep < Steps; ++Substep)
		{
			for (auto& Cell : G.Cells)
			{
				const double Rain = F.RainMetersPerWorldSecond * WorldDt * Area * Cell.RainExposure;
				Cell.WaterCubicMeters += Rain;
				G.Ledger.RainCubicMeters += Rain;
				const double Snow = F.SnowMetersPerWorldSecond * WorldDt * Area * Cell.RainExposure;
				Cell.SnowCubicMeters += Snow;
				Cell.SnowVolumeCubicMeters += Snow / 0.1;
				G.Ledger.SnowCubicMeters += Snow;
				const double Infiltration = FMath::Min3(Cell.WaterCubicMeters, F.InfiltrationMetersPerWorldSecond * WorldDt * Area,
					FMath::Max(0., F.SoilCapacityMeters * Area - Cell.SoilCubicMeters));
				Cell.WaterCubicMeters -= Infiltration;
				Cell.SoilCubicMeters += Infiltration;
				const double Drainage = FMath::Min(Cell.SoilCubicMeters, F.DrainageMetersPerWorldSecond * WorldDt * Area);
				Cell.SoilCubicMeters -= Drainage;
				Cell.WaterCubicMeters += Drainage;
				const double Phase = F.PhaseMetersPerDegreeWorldSecond * FMath::Abs(F.TemperatureCelsius) * WorldDt * Area;
				if (F.TemperatureCelsius < 0.)
				{
					const double Frozen = FMath::Min(Cell.WaterCubicMeters, Phase);
					Cell.WaterCubicMeters -= Frozen;
					Cell.IceCubicMeters += Frozen;
				}
				else
				{
					const double Melted = FMath::Min(Cell.IceCubicMeters, Phase);
					Cell.IceCubicMeters -= Melted;
					Cell.WaterCubicMeters += Melted;
					const double SnowMelt = FMath::Min(Cell.SnowCubicMeters, Phase - Melted);
					const double Fraction = Cell.SnowCubicMeters > 0. ? SnowMelt / Cell.SnowCubicMeters : 0.;
					Cell.SnowVolumeCubicMeters *= 1. - Fraction;
					Cell.SnowCubicMeters -= SnowMelt;
					Cell.WaterCubicMeters += SnowMelt;
				}

				const double Demand = F.EvaporationMetersPerWorldSecond * WorldDt * Area;
				const double LiquidLoss = FMath::Min(Cell.WaterCubicMeters, Demand);
				const double SoilLoss = FMath::Min(Cell.SoilCubicMeters, Demand - LiquidLoss);
				Cell.WaterCubicMeters -= LiquidLoss;
				Cell.SoilCubicMeters -= SoilLoss;
				G.Ledger.EvaporatedCubicMeters += LiquidLoss + SoilLoss;
				if (Cell.WaterCubicMeters + Cell.IceCubicMeters + Cell.SnowVolumeCubicMeters > Capacity(G, Cell) + 1.e-9)
				{
					return Fail(Error, TEXT("Surface input exceeds the cell's explicit storage capacity."));
				}
			}

			Transfers.Reset();
			for (double& Value : Outgoing)
			{
				Value = 0.;
			}
			for (double& Value : Incoming)
			{
				Value = 0.;
			}
			auto Propose = [&](int32 A, int32 B)
			{
				const double Difference = G.WaterLevelMeters(A) - G.WaterLevelMeters(B);
				const int32 From = Difference > 0. ? A : B;
				const int32 To = Difference > 0. ? B : A;
				const double Sill = FMath::Max(G.Cells[A].BedMeters, G.Cells[B].BedMeters);
				if (FMath::Min(G.Cells[A].CeilingMeters, G.Cells[B].CeilingMeters) <= Sill)
				{
					return;
				}
				const double AboveSill = FMath::Max(0., G.WaterLevelMeters(From) - FMath::Max(G.Cells[A].BedMeters, G.Cells[B].BedMeters)) * Area;
				const double Amount = FMath::Min3(FMath::Abs(Difference) * Area * 0.5,
					F.FlowConductance * FMath::Abs(Difference) * Dt, FMath::Min(AboveSill, G.Cells[From].WaterCubicMeters));
				if (Amount > 0.)
				{
					Transfers.Add({From, To, Amount});
					Outgoing[From] += Amount;
					Incoming[To] += Amount;
				}
			};
			for (int32 Y = 0; Y < G.Size.Y; ++Y)
			{
				for (int32 X = 0; X < G.Size.X; ++X)
				{
					const int32 I = X + Y * G.Size.X;
					if (X + 1 < G.Size.X)
					{
						Propose(I, I + 1);
					}
					if (Y + 1 < G.Size.Y)
					{
						Propose(I, I + G.Size.X);
					}
				}
			}
			// Freeze both limits before applying any edge so iteration order cannot change flux.
			for (int32 I = 0; I < G.Cells.Num(); ++I)
			{
				const auto& Cell = G.Cells[I];
				SourceScales[I] = Outgoing[I] > 0. ? FMath::Min(1., Cell.WaterCubicMeters / Outgoing[I]) : 1.;
				const double Room = FMath::Max(0., Capacity(G, Cell) - Cell.WaterCubicMeters - Cell.IceCubicMeters - Cell.SnowVolumeCubicMeters);
				TargetScales[I] = Incoming[I] > 0. ? FMath::Min(1., Room / Incoming[I]) : 1.;
				Remaining[I] = Cell.WaterCubicMeters;
				RoomLeft[I] = Room;
				Added[I] = 0.;
			}
			for (const auto& Transfer : Transfers)
			{
				const double Requested = Transfer.Amount * FMath::Min(SourceScales[Transfer.From], TargetScales[Transfer.To]);
				// Remaining frozen budgets absorb final-ulp rounding without spending newly received water.
				const double Amount = FMath::Min3(Requested, Remaining[Transfer.From], RoomLeft[Transfer.To]);
				Remaining[Transfer.From] -= Amount;
				RoomLeft[Transfer.To] -= Amount;
				Added[Transfer.To] += Amount;
			}

			for (int32 I = 0; I < G.Cells.Num(); ++I)
			{
				auto& Cell = G.Cells[I];
				Cell.WaterCubicMeters = Remaining[I] + Added[I];
				if (!Cell.bOpenBoundary || F.BoundaryConductance == 0.)
				{
					continue;
				}

				const double Difference = F.BoundaryLevelMeters - G.WaterLevelMeters(I);
				const double Requested = FMath::Min(FMath::Abs(Difference) * Area, F.BoundaryConductance * FMath::Abs(Difference) * Dt);
				if (Difference > 0.)
				{
					const double Amount = FMath::Min(Requested, FMath::Max(0., Capacity(G, Cell) - Cell.WaterCubicMeters - Cell.IceCubicMeters - Cell.SnowVolumeCubicMeters));
					Cell.WaterCubicMeters += Amount;
					G.Ledger.BoundaryInCubicMeters += Amount;
				}
				else
				{
					const double Amount = FMath::Min(Requested, Cell.WaterCubicMeters);
					Cell.WaterCubicMeters -= Amount;
					G.Ledger.BoundaryOutCubicMeters += Amount;
				}
			}
		}

		++G.Revision;
		return FCCLSurfaceSimulation::ValidateRegion(G, Error);
	}
}

double FCCLSurfaceGrid::TotalCubicMeters() const
{
	double Total = 0.;
	for (const auto& Cell : Cells)
	{
		Total += Cell.WaterCubicMeters + Cell.IceCubicMeters + Cell.SoilCubicMeters + Cell.SnowCubicMeters;
	}
	return Total;
}

double FCCLSurfaceGrid::BalanceErrorCubicMeters() const
{
	return TotalCubicMeters() - (Ledger.InitialCubicMeters + Ledger.RainCubicMeters + Ledger.SnowCubicMeters + Ledger.BoundaryInCubicMeters
		- Ledger.BoundaryOutCubicMeters - Ledger.EvaporatedCubicMeters);
}

double FCCLSurfaceGrid::WaterLevelMeters(int32 Index) const
{
	const auto& Cell = Cells[Index];
	return Cell.BedMeters + (Cell.WaterCubicMeters + Cell.IceCubicMeters) / FMath::Square(SpacingMeters);
}

double FCCLSurfaceGrid::Wetness(int32 Index) const
{
	const auto& Cell = Cells[Index];
	const double Capacity = FMath::Max(1.e-9, Forcing.SoilCapacityMeters * FMath::Square(SpacingMeters));
	return FMath::Clamp(Cell.SoilCubicMeters / Capacity + Cell.WaterCubicMeters / FMath::Square(SpacingMeters) * 10., 0., 1.);
}

double FCCLSurfaceGrid::Mud(int32 Index) const
{
	return FMath::Clamp((Wetness(Index) - 0.5) * 2., 0., 1.) * (Cells[Index].IceCubicMeters > 0.001 ? 0. : 1.);
}

bool FCCLSurfaceSimulation::Initialize(FGuid InWorldId, double InGameSeconds, double InWorldSeconds, uint64 InStepId, FString& Error)
{
	if (!InWorldId.IsValid() || !Quantity(InGameSeconds) || !Quantity(InWorldSeconds))
	{
		return Fail(Error, TEXT("Surface simulation requires valid world and completed times."));
	}

	Regions.Reset();
	SnowContacts.Reset();
	WorldId = InWorldId;
	GameSeconds = InGameSeconds;
	WorldSeconds = InWorldSeconds;
	StepId = InStepId;
	return true;
}

bool FCCLSurfaceSimulation::AddRegion(FCCLSurfaceGrid Region, FString& Error)
{
	if (!IsInitialized() || Regions.Contains(Region.RegionId) || Regions.Num() >= MaximumRegions || !ValidateRegion(Region, Error))
	{
		return Fail(Error, TEXT("Surface region is invalid, duplicated or over budget."));
	}

	int32 Total = Region.Cells.Num();
	for (const auto& Pair : Regions)
	{
		Total += Pair.Value.Cells.Num();
	}
	if (Total > MaximumCells)
	{
		return Fail(Error, TEXT("Surface cell budget exceeded."));
	}

	Regions.Add(Region.RegionId, MoveTemp(Region));
	return true;
}

bool FCCLSurfaceSimulation::ChangeForcing(FGuid RegionId, const FCCLSurfaceForcing& Forcing, FString& Error)
{
	auto* Region = Regions.Find(RegionId);
	if (!Region || !ValidForcing(Forcing) || Region->Revision == MAX_uint64)
	{
		return Fail(Error, TEXT("Invalid surface forcing or exhausted revision."));
	}

	Region->Forcing = Forcing;
	++Region->Revision;
	return true;
}

bool FCCLSurfaceSimulation::Advance(const FCCLWorldStep& Step, FString& Error)
{
	Error.Reset();
	if (!Validate(Error) || Step.StepId != StepId + 1 || StepId == MAX_uint64
		|| Step.GameFromSeconds != GameSeconds || Step.WorldFromSeconds != WorldSeconds
		|| !Quantity(Step.GameToSeconds) || !Quantity(Step.WorldToSeconds)
		|| Step.GameToSeconds <= GameSeconds || Step.WorldToSeconds < WorldSeconds)
	{
		return Fail(Error, TEXT("Surface step does not match its committed world clock."));
	}

	const double Delta = Step.GameToSeconds - GameSeconds;
	const double StepCount = FMath::CeilToDouble(Delta / 0.25);
	if (!Regions.IsEmpty() && StepCount > MaximumSubsteps)
	{
		return Fail(Error, TEXT("Surface substep budget exceeded; world time remains pending."));
	}

	FCCLSurfaceSimulation Candidate = *this;
	for (auto& Pair : Candidate.Regions)
	{
		if (Pair.Value.Revision == MAX_uint64 || !AdvanceGrid(Pair.Value, Delta, Step.WorldToSeconds - WorldSeconds, int32(StepCount), Error))
		{
			return false;
		}
	}

	Candidate.GameSeconds = Step.GameToSeconds;
	Candidate.WorldSeconds = Step.WorldToSeconds;
	Candidate.StepId = Step.StepId;
	*this = MoveTemp(Candidate);
	return true;
}

bool FCCLSurfaceSimulation::RebaseTerrain(FGuid RegionId, const TArray<double>& BedsMeters, uint64 TerrainRevision, FString& Error, bool bRestoring)
{
	const auto* Existing = Regions.Find(RegionId);
	if (!Existing || BedsMeters.Num() != Existing->Cells.Num() || (!bRestoring && TerrainRevision <= Existing->TerrainRevision) || TerrainRevision == 0 || Existing->Revision == MAX_uint64)
	{
		return Fail(Error, TEXT("Terrain rebase requires the complete newer surface geometry."));
	}

	FCCLSurfaceGrid Candidate = *Existing;
	for (int32 I = 0; I < BedsMeters.Num(); ++I)
	{
		if (!FMath::IsFinite(BedsMeters[I]) || BedsMeters[I] > Candidate.Cells[I].CeilingMeters || FMath::Abs(BedsMeters[I]) > 1.e7)
		{
			return Fail(Error, TEXT("New terrain bed is outside its explicit surface storage."));
		}
		Candidate.Cells[I].BedMeters = BedsMeters[I];
	}

	for (int32 I = 0; I < Candidate.Cells.Num(); ++I)
	{
		auto& Source = Candidate.Cells[I];
		const double Room = Capacity(Candidate, Source) - Source.IceCubicMeters - Source.SnowVolumeCubicMeters;
		if (Room < 0.)
		{
			return Fail(Error, TEXT("Terrain would remove the support volume of persistent ice."));
		}

		double Excess = FMath::Max(0., Source.WaterCubicMeters - Room);
		Source.WaterCubicMeters -= Excess;
		TArray<int32> Queue = {I};
		TSet<int32> Seen = {I};
		for (int32 Head = 0; Head < Queue.Num() && Excess > 0.; ++Head)
		{
			const int32 At = Queue[Head];
			auto& Target = Candidate.Cells[At];
			if (At != I)
			{
				const double Amount = FMath::Min(Excess, FMath::Max(0., Capacity(Candidate, Target) - Target.WaterCubicMeters - Target.IceCubicMeters - Target.SnowVolumeCubicMeters));
				Target.WaterCubicMeters += Amount;
				Excess -= Amount;
			}
			if (Target.bOpenBoundary && Excess > 0.)
			{
				Candidate.Ledger.BoundaryOutCubicMeters += Excess;
				Excess = 0.;
			}

			const int32 X = At % Candidate.Size.X;
			const int32 Y = At / Candidate.Size.X;
			for (const FIntPoint Offset : {FIntPoint(-1, 0), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(0, 1)})
			{
				const int32 NX = X + Offset.X, NY = Y + Offset.Y;
				if (NX >= 0 && NY >= 0 && NX < Candidate.Size.X && NY < Candidate.Size.Y)
				{
					const int32 Next = NX + NY * Candidate.Size.X;
					if (!Seen.Contains(Next) && FMath::Min(Target.CeilingMeters, Candidate.Cells[Next].CeilingMeters) > FMath::Max(Target.BedMeters, Candidate.Cells[Next].BedMeters))
					{
						Seen.Add(Next);
						Queue.Add(Next);
					}
				}
			}
		}
		if (Excess > 0.)
		{
			return Fail(Error, TEXT("Terrain displacement has no connected capacity or external outlet."));
		}
	}

	Candidate.TerrainRevision = TerrainRevision;
	++Candidate.Revision;
	if (!ValidateRegion(Candidate, Error))
	{
		return false;
	}
	Regions[RegionId] = MoveTemp(Candidate);
	return true;
}

bool FCCLSurfaceSimulation::Capture(TArray<uint8>& Bytes, FString& Error) const
{
	if (!Validate(Error))
	{
		return false;
	}

	FCCLSurfaceSimulation Copy = *this;
	TArray<uint8> Candidate;
	FMemoryWriter Writer(Candidate, true);
	if (!Copy.Serialize(Writer) || Candidate.Num() > MaximumBytes - 4)
	{
		return Fail(Error, TEXT("Surface snapshot exceeds its byte budget."));
	}
	uint32 CRC = FCrc::MemCrc32(Candidate.GetData(), Candidate.Num());
	Writer << CRC;
	Bytes = MoveTemp(Candidate);
	return true;
}

bool FCCLSurfaceSimulation::Restore(const TArray<uint8>& Bytes, FString& Error)
{
	if (Bytes.Num() < 56 || Bytes.Num() > MaximumBytes)
	{
		return Fail(Error, TEXT("Invalid surface snapshot length."));
	}

	uint32 CRC;
	FMemory::Memcpy(&CRC, Bytes.GetData() + Bytes.Num() - 4, 4);
	if (CRC != FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4))
	{
		return Fail(Error, TEXT("Surface snapshot checksum mismatch."));
	}

	TArray<uint8> Payload;
	Payload.Append(Bytes.GetData(), Bytes.Num() - 4);
	FMemoryReader Reader(Payload, true);
	FCCLSurfaceSimulation Candidate;
	if (!Candidate.Serialize(Reader) || Reader.Tell() != Payload.Num() || !Candidate.Validate(Error))
	{
		return Fail(Error, TEXT("Malformed surface snapshot."));
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool FCCLSurfaceSimulation::Validate(FString& Error) const
{
	if (!WorldId.IsValid() || !Quantity(GameSeconds) || !Quantity(WorldSeconds) || Regions.Num() > MaximumRegions)
	{
		return Fail(Error, TEXT("Invalid surface world identity or time."));
	}

	int32 Count = 0;
	for (const auto& Pair : Regions)
	{
		Count += Pair.Value.Cells.Num();
		if (Pair.Key != Pair.Value.RegionId || !ValidateRegion(Pair.Value, Error) || Count > MaximumCells)
		{
			return false;
		}
	}
	return true;
}

bool FCCLSurfaceSimulation::ValidateRegion(const FCCLSurfaceGrid& G, FString& Error)
{
	if (!G.RegionId.IsValid() || G.Size.X < 1 || G.Size.Y < 1 || G.Size.X > 128 || G.Size.Y > 128
		|| G.Cells.Num() != G.Size.X * G.Size.Y || G.OriginMeters.ContainsNaN() || G.OriginMeters.GetAbsMax() > 1.e7
		|| !FMath::IsFinite(G.SpacingMeters) || G.SpacingMeters < 0.05 || G.SpacingMeters > 16.
		|| G.Revision == 0 || (G.TerrainId.IsValid() && G.TerrainRevision == 0) || !ValidForcing(G.Forcing))
	{
		return Fail(Error, TEXT("Invalid surface region definition."));
	}

	for (const auto& Cell : G.Cells)
	{
		if (!FMath::IsFinite(Cell.BedMeters) || !FMath::IsFinite(Cell.CeilingMeters)
			|| FMath::Abs(Cell.BedMeters) > 1.e7 || FMath::Abs(Cell.CeilingMeters) > 1.e7 || Cell.CeilingMeters < Cell.BedMeters
			|| !Quantity(Cell.SnowCubicMeters) || !Quantity(Cell.SnowVolumeCubicMeters)
			|| Cell.SnowVolumeCubicMeters < Cell.SnowCubicMeters / 0.65 - 1.e-12
			|| Cell.SnowVolumeCubicMeters > Cell.SnowCubicMeters / 0.05 + 1.e-12
			|| !Quantity(Cell.WaterCubicMeters) || !Quantity(Cell.IceCubicMeters) || !Quantity(Cell.SoilCubicMeters)
			|| Cell.WaterCubicMeters + Cell.IceCubicMeters + Cell.SnowVolumeCubicMeters > Capacity(G, Cell) + 1.e-8
			|| !FMath::IsFinite(Cell.RainExposure) || Cell.RainExposure < 0. || Cell.RainExposure > 1. || Cell.bOpenBoundary > 1)
		{
			Error = FString::Printf(TEXT("Invalid surface cell: water=%.17g ice=%.17g soil=%.17g capacity=%.17g bed=%.17g ceiling=%.17g"),
				Cell.WaterCubicMeters, Cell.IceCubicMeters, Cell.SoilCubicMeters, Capacity(G, Cell), Cell.BedMeters, Cell.CeilingMeters);
			return false;
		}
	}

	const auto& L = G.Ledger;
	if (!Quantity(L.InitialCubicMeters) || !Quantity(L.RainCubicMeters) || !Quantity(L.SnowCubicMeters) || !Quantity(L.BoundaryInCubicMeters)
		|| !Quantity(L.BoundaryOutCubicMeters) || !Quantity(L.EvaporatedCubicMeters)
		|| FMath::Abs(G.BalanceErrorCubicMeters()) > FMath::Max(1.e-8, G.TotalCubicMeters() * 1.e-8))
	{
		return Fail(Error, TEXT("Surface water ledger does not balance."));
	}
	return true;
}

bool FCCLSurfaceSimulation::Serialize(FArchive& Ar)
{
	uint32 Tag = Magic, Version = 2;
	Ar << Tag << Version << WorldId << GameSeconds << WorldSeconds << StepId;
	int32 Count = Regions.Num();
	Ar << Count;
	if (Ar.IsError() || Tag != Magic || (Version != 1 && Version != 2) || Count < 0 || Count > MaximumRegions)
	{
		return false;
	}

	TArray<FGuid> Ids;
	Regions.GetKeys(Ids);
	Ids.Sort(GuidLess);
	int32 TotalCells = 0;
	for (int32 I = 0; I < Count; ++I)
	{
		FCCLSurfaceGrid G;
		if (Ar.IsSaving())
		{
			G = Regions[Ids[I]];
		}
		Ar << G.RegionId << G.TerrainId << G.OriginMeters << G.Size << G.SpacingMeters << G.Revision << G.TerrainRevision;
		auto& F = G.Forcing;
		Ar << F.RainMetersPerWorldSecond << F.EvaporationMetersPerWorldSecond << F.InfiltrationMetersPerWorldSecond
			<< F.DrainageMetersPerWorldSecond << F.SoilCapacityMeters << F.TemperatureCelsius << F.PhaseMetersPerDegreeWorldSecond
			<< F.FlowConductance << F.BoundaryLevelMeters << F.BoundaryConductance;
		if (Version >= 2)
		{
			Ar << F.SnowMetersPerWorldSecond;
		}
		auto& L = G.Ledger;
		Ar << L.InitialCubicMeters << L.RainCubicMeters << L.BoundaryInCubicMeters << L.BoundaryOutCubicMeters << L.EvaporatedCubicMeters;
		if (Version >= 2)
		{
			Ar << L.SnowCubicMeters;
		}
		int32 Cells = G.Cells.Num();
		Ar << Cells;
		if (Ar.IsError() || Cells < 1 || Cells > 16384 || TotalCells > MaximumCells - Cells
			|| (Ar.IsLoading() && int64(Cells) * (Version >= 2 ? 65 : 49) > Ar.TotalSize() - Ar.Tell()))
		{
			return false;
		}
		TotalCells += Cells;
		G.Cells.SetNum(Cells);
		for (auto& C : G.Cells)
		{
			Ar << C.BedMeters << C.CeilingMeters << C.WaterCubicMeters << C.IceCubicMeters << C.SoilCubicMeters
				<< C.RainExposure << C.bOpenBoundary;
			if (Version >= 2)
			{
				Ar << C.SnowCubicMeters << C.SnowVolumeCubicMeters;
			}
		}
		if (Ar.IsLoading())
		{
			if (Regions.Contains(G.RegionId))
			{
				return false;
			}
			Regions.Add(G.RegionId, MoveTemp(G));
		}
	}
	if (Version >= 2)
	{
		int32 ContactCount = SnowContacts.Num();
		Ar << ContactCount;
		if (ContactCount < 0 || ContactCount > 128 || (Ar.IsLoading() && int64(ContactCount) * 24 > Ar.TotalSize() - Ar.Tell()))
		{
			return false;
		}
		TArray<FGuid> Sources;
		SnowContacts.GetKeys(Sources);
		Sources.Sort(GuidLess);
		for (int32 I = 0; I < ContactCount; ++I)
		{
			FGuid Id;
			uint64 Sequence = 0;
			if (Ar.IsSaving())
			{
				Id = Sources[I];
				Sequence = SnowContacts[Id];
			}
			Ar << Id << Sequence;
			if (!Id.IsValid() || Sequence == 0 || (Ar.IsLoading() && SnowContacts.Contains(Id)))
			{
				return false;
			}
			if (Ar.IsLoading())
			{
				SnowContacts.Add(Id, Sequence);
			}
		}
	}
	return !Ar.IsError();
}
