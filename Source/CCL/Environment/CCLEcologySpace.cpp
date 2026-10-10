#include "CCLEcologySpace.h"

#include "CCLSurfaceSimulation.h"
#include "CCLTerrainRegion.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavigationData.h"
#include "Algo/Reverse.h"

bool FCCLEcologyWorldSpace::IsInLiquid(const FCCLSurfaceSimulation& Surface, const FVector& P, double Radius)
{
    if (!Surface.IsInitialized() || P.ContainsNaN() || !FMath::IsFinite(Radius) || Radius < .02 || Radius > 10.) { return false; }
    for (const auto& Pair : Surface.GetRegions())
    {
        const auto& G = Pair.Value;
        if (P.X - Radius < G.OriginMeters.X || P.Y - Radius < G.OriginMeters.Y
            || P.X + Radius >= G.OriginMeters.X + G.Size.X * G.SpacingMeters
            || P.Y + Radius >= G.OriginMeters.Y + G.Size.Y * G.SpacingMeters) { continue; }
        const int32 X0 = FMath::FloorToInt((P.X - Radius - G.OriginMeters.X) / G.SpacingMeters);
        const int32 X1 = FMath::FloorToInt((P.X + Radius - G.OriginMeters.X) / G.SpacingMeters);
        const int32 Y0 = FMath::FloorToInt((P.Y - Radius - G.OriginMeters.Y) / G.SpacingMeters);
        const int32 Y1 = FMath::FloorToInt((P.Y + Radius - G.OriginMeters.Y) / G.SpacingMeters);
        if (X0 < 0 || Y0 < 0 || X1 >= G.Size.X || Y1 >= G.Size.Y || int64(X1 - X0 + 1) * (Y1 - Y0 + 1) > 4096) { continue; }
        bool bInside = true;
        const double Area = FMath::Square(G.SpacingMeters);
        for (int32 Y = Y0; Y <= Y1 && bInside; ++Y)
        {
            for (int32 X = X0; X <= X1; ++X)
            {
                const int32 Index = Y * G.Size.X + X;
                const auto& Cell = G.Cells[Index];
                const double Bottom = Cell.BedMeters + Cell.IceCubicMeters / Area;
                const double Top = FMath::Min(Cell.CeilingMeters, G.WaterLevelMeters(Index));
                if (P.Z - Radius < Bottom || P.Z + Radius > Top || Cell.WaterCubicMeters <= 0.) { bInside = false; break; }
            }
        }
        if (bInside) { return true; }
    }
    return false;
}

bool FCCLEcologyWorldSpace::FindRoute(const FCCLEcologyRouteRequest& Q, const FCCLSurfaceSimulation* Surface,
    TArray<FVector>& OutRoute, FString& Error) const
{
    if (!IsInGameThread() || !World || World->GetNetMode() == NM_Client || !Q.HabitatMeters.IsValid
        || Q.FromMeters.ContainsNaN() || Q.TargetMeters.ContainsNaN() || !FMath::IsFinite(Q.RadiusMeters)
        || Q.RadiusMeters < .02 || Q.RadiusMeters > 10. || uint8(Q.Medium) > 3)
    {
        Error = TEXT("Ecology spatial query requires a server game thread and bounded geometry.");
        return false;
    }
    const bool bWater = Q.Medium == ECCLEcologyMovement::Water;
    const bool bGround = Q.Medium == ECCLEcologyMovement::Ground || Q.Medium == ECCLEcologyMovement::Insect;
    if (bWater && (!Surface || Surface->GetWorldSeconds() != Q.WorldSeconds))
    {
        Error = TEXT("Aquatic movement requires water from the same committed behavior boundary.");
        return false;
    }
    for (TActorIterator<ACCLTerrainRegion> It(World); It; ++It)
    {
        const FBox Bounds(It->GetWorldTerrainBounds().Min / 100., It->GetWorldTerrainBounds().Max / 100.);
        if (Bounds.Intersect(Q.HabitatMeters) && (It->IsPreparing() || !It->IsTerrainReady() || (bGround && !It->IsNavigationReady())))
        {
            Error = TEXT("Ecology route awaits matching terrain collision and navigation publication.");
            return false;
        }
    }
    const FBox Centers(Q.HabitatMeters.Min + FVector(Q.RadiusMeters), Q.HabitatMeters.Max - FVector(Q.RadiusMeters));
    TArray<FVector> Route;
    auto NoRoute = [&]() { OutRoute.Reset(); Error.Reset(); return true; };
    if (!Centers.IsInsideOrOn(Q.FromMeters) || !Centers.IsInsideOrOn(Q.TargetMeters)) { return NoRoute(); }
    double LiquidStep = .1;
    if (bWater)
    {
        for (const auto& Pair : Surface->GetRegions()) { LiquidStep = FMath::Min(LiquidStep, Pair.Value.SpacingMeters * .5); }
    }
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EcologyRoute), false);
    const FCollisionShape Shape = FCollisionShape::MakeSphere(float(Q.RadiusMeters * 100.));
    auto Clear = [&](const FVector& A, const FVector& B)
    {
        if (!Centers.IsInsideOrOn(A) || !Centers.IsInsideOrOn(B)) { return false; }
        if (World->OverlapBlockingTestByChannel(A * 100., FQuat::Identity, ECC_Pawn, Shape, Params)
            || World->OverlapBlockingTestByChannel(B * 100., FQuat::Identity, ECC_Pawn, Shape, Params)) { return false; }
        FHitResult Hit;
        if (World->SweepSingleByChannel(Hit, A * 100., B * 100., FQuat::Identity, ECC_Pawn, Shape, Params)) { return false; }
        if (bWater)
        {
            const double RawSteps = FMath::CeilToDouble(FVector::Distance(A, B) / LiquidStep);
            if (RawSteps > 4096.) { return false; }
            const int32 Steps = FMath::Max(1, int32(RawSteps));
            for (int32 I = 0; I <= Steps; ++I)
            {
                if (!IsInLiquid(*Surface, FMath::Lerp(A, B, double(I) / Steps), Q.RadiusMeters)) { return false; }
            }
        }
        return true;
    };
    if (bGround)
    {
        auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
        auto* Data = Nav ? Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;
        // IsNavigationDirty can retain editor-only dirty history after the relevant terrain generation is ready.
        if (!Data || Nav->GetNumRemainingBuildTasks() != 0 || UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(World))
        {
            Error = TEXT("Ground ecology route awaits ready navigation data.");
            return false;
        }
        if (Data->GetConfig().AgentRadius + .1 < Q.RadiusMeters * 100.)
        {
            Error = TEXT("Ecology body radius exceeds the available navigation agent radius.");
            return false;
        }
        auto* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World,
            (Q.FromMeters - FVector(0., 0., Q.RadiusMeters)) * 100.,
            (Q.TargetMeters - FVector(0., 0., Q.RadiusMeters)) * 100., Data);
        if (!Path || !Path->IsValid() || Path->IsPartial() || Path->PathPoints.Num() > 1022)
        {
            NoRoute(); Error = TEXT("No complete ground navigation path."); return true;
        }
        Route.Add(Q.FromMeters);
        for (const FVector& Point : Path->PathPoints)
        {
            const FVector Center = Point / 100. + FVector(0., 0., Q.RadiusMeters + .02);
            if (!Clear(Route.Last(), Center))
            {
                Error = FString::Printf(TEXT("Ground path body blocked between %s and %s."), *Route.Last().ToString(), *Center.ToString());
                OutRoute.Reset(); return true;
            }
            if (!Route.Last().Equals(Center, 1.e-5)) { Route.Add(Center); }
        }
        if (FVector::Distance(Route.Last(), Q.TargetMeters) > .3 || !Clear(Route.Last(), Q.TargetMeters))
        {
            Error = FString::Printf(TEXT("Ground path cannot reach exact target: path=%s target=%s."), *Route.Last().ToString(), *Q.TargetMeters.ToString());
            OutRoute.Reset(); return true;
        }
        if (!Route.Last().Equals(Q.TargetMeters, 1.e-5)) { Route.Add(Q.TargetMeters); }
        OutRoute = MoveTemp(Route); Error.Reset(); return true;
    }
    if (Clear(Q.FromMeters, Q.TargetMeters))
    {
        OutRoute = {Q.FromMeters, Q.TargetMeters}; Error.Reset(); return true;
    }
    // Bounded 3D routing for media not represented by the ground navigation mesh.
    const double Spacing = FMath::Max(.5, Q.RadiusMeters * 2.);
    const FVector Size = Centers.GetSize() / Spacing;
    if (Size.GetMax() > 63.)
    {
        Error = TEXT("Ecology volume route exceeds its local grid extent; use a smaller routing habitat.");
        return false;
    }
    const FIntVector Dimensions(FMath::FloorToInt(Size.X) + 1, FMath::FloorToInt(Size.Y) + 1, FMath::FloorToInt(Size.Z) + 1);
    auto Point = [&](FIntVector Cell) { return Centers.Min + FVector(Cell) * Spacing; };
    const FVector Local = (Q.FromMeters - Centers.Min) / Spacing;
    const FIntVector Start(FMath::Clamp(FMath::RoundToInt(Local.X), 0, Dimensions.X - 1),
        FMath::Clamp(FMath::RoundToInt(Local.Y), 0, Dimensions.Y - 1), FMath::Clamp(FMath::RoundToInt(Local.Z), 0, Dimensions.Z - 1));
    if (!Clear(Q.FromMeters, Point(Start))) { return NoRoute(); }
    struct FNode { double Cost = 0.; FIntVector Parent = FIntVector::ZeroValue; };
    TMap<FIntVector, FNode> Nodes;
    TSet<FIntVector> Closed;
    TArray<FIntVector> Open;
    Nodes.Add(Start, {FVector::Distance(Q.FromMeters, Point(Start)), Start});
    Open.Add(Start);
    const FIntVector Offsets[] = {FIntVector(1,0,0),FIntVector(-1,0,0),FIntVector(0,1,0),FIntVector(0,-1,0),FIntVector(0,0,1),FIntVector(0,0,-1)};
    while (!Open.IsEmpty() && Closed.Num() < 512)
    {
        int32 Best = 0;
        double BestScore = TNumericLimits<double>::Max();
        for (int32 I = 0; I < Open.Num(); ++I)
        {
            const double Score = Nodes[Open[I]].Cost + FVector::Distance(Point(Open[I]), Q.TargetMeters);
            if (Score < BestScore) { BestScore = Score; Best = I; }
        }
        const FIntVector Current = Open[Best];
        Open.RemoveAt(Best);
        Closed.Add(Current);
        if (Clear(Point(Current), Q.TargetMeters))
        {
            FIntVector Cursor = Current;
            Route.Add(Q.TargetMeters);
            for (int32 Count = 0; Count <= Nodes.Num(); ++Count)
            {
                Route.Add(Point(Cursor));
                if (Cursor == Start) { break; }
                Cursor = Nodes[Cursor].Parent;
            }
            Route.Add(Q.FromMeters);
            Algo::Reverse(Route);
            OutRoute = MoveTemp(Route); Error.Reset(); return true;
        }
        const double Cost = Nodes[Current].Cost;
        for (FIntVector Offset : Offsets)
        {
            const FIntVector Next = Current + Offset;
            if (Next.X < 0 || Next.Y < 0 || Next.Z < 0 || Next.X >= Dimensions.X || Next.Y >= Dimensions.Y || Next.Z >= Dimensions.Z
                || Closed.Contains(Next) || !Clear(Point(Current), Point(Next))) { continue; }
            const double NewCost = Cost + Spacing;
            if (const auto* Existing = Nodes.Find(Next); Existing && Existing->Cost <= NewCost) { continue; }
            Nodes.Add(Next, {NewCost, Current});
            Open.AddUnique(Next);
        }
    }
    return NoRoute();
}
