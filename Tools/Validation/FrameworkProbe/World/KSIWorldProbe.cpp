#include "KSIWorldProbe.h"
#include "CoreMinimal.h"
#if WITH_KSIWORLDTIME
#include "KSIWorldClock.h"
#endif
#if WITH_KSICELESTIAL
#include "KSICelestialSystem.h"
#endif
#if WITH_KSITERRAIN
#include "KSITerrainStore.h"
#endif
#if WITH_KSIAGENT
#include "KSIAgentTypes.h"
#endif
#if WITH_KSIENVIRONMENT
#include "KSISurfaceSimulation.h"
#endif
#if WITH_KSISAVE
#include "KSIFileBundleStore.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#endif

bool RunKSIWorldProbe()
{
    bool bPassed = true;
    auto Check = [&bPassed](bool Value, const TCHAR* Name)
    {
        UE_LOG(LogTemp, Display, TEXT("KSI_PROBE %s %s"), Value ? TEXT("CHECK") : TEXT("FAIL"), Name);
        bPassed &= Value;
    };
    FString Error;
#if WITH_KSIWORLDTIME
    FKSIWorldClock Clock;
    FKSIWorldStep First, Retry;
    Check(Clock.QueueGameTime(1, false, Error) && Clock.Prepare(1, 15, First, Error), TEXT("bounded prepare"));
    Check(Clock.GetWorldSeconds() == 0. && First.WorldToSeconds == 15., TEXT("prepare is unpublished"));
    Check(Clock.Abort(First.Ticket, Error) && Clock.Prepare(1, 15, Retry, Error), TEXT("retry pending time"));
    Check(!Clock.Commit(First.Ticket, Error) && Clock.Commit(Retry.Ticket, Error), TEXT("stale ticket rejected"));
    FKSIWorldClockSnapshot Snapshot;
    FKSIWorldClock Restored;
    Check(Clock.Capture(Snapshot, Error) && Restored.Restore(Snapshot, Error)
        && Restored.GetWorldSeconds() == 15. && Restored.HasPendingTime(), TEXT("pending time snapshot"));
#elif WITH_KSICELESTIAL
    FKSICelestialSystem System;
    auto Definition = FKSICelestialSystem::MakeDefaultDefinition(37);
    Check(System.Initialize(Definition, Error), TEXT("independent celestial definition"));
    TArray<FKSICelestialBodyState> A, B;
    Check(System.Evaluate(0., A, Error) && System.Evaluate(10000., B, Error) && A.Num() == B.Num() && A.Num() > 1,
        TEXT("hierarchical orbital evaluation"));
    bool bMoved = false;
    for (int32 Index = 0; Index < A.Num() && Index < B.Num(); ++Index)
    {
        bMoved |= !A[Index].PositionKm.Equals(B[Index].PositionKm);
    }
    Check(bMoved, TEXT("world time changes orbit"));
    Definition.Bodies[0].RadiusKm = -1.;
    Check(!System.Initialize(Definition, Error) && System.Evaluate(0., B, Error) && B.Num() == A.Num(), TEXT("invalid replacement retains system"));
#elif WITH_KSITERRAIN
    FKSITerrainDefinition Definition;
    Definition.WorldId = FGuid::NewGuid(); Definition.RegionId = FGuid::NewGuid(); Definition.DefinitionId = FGuid::NewGuid();
    FKSITerrainStore Store;
    Check(Store.Initialize(Definition, Error), TEXT("plain terrain init"));
    FKSITerrainAuthority Authority;
    Authority.PrincipalId = FGuid::NewGuid(); Authority.AllowedBoundsMeters = FKSITerrainStore::EditableBoundsMeters(Definition);
    Authority.bCanExcavate = 1;
    FKSITerrainEdit Edit;
    Edit.PrincipalId = Authority.PrincipalId; Edit.Sequence = 1; Edit.Epoch = Store.GetEpoch(); Edit.ExpectedRevision = Store.GetRevision(); Edit.RadiusMeters = 2.;
    FKSITerrainCandidate Candidate;
    Check(Store.PrepareEdit(Edit, Authority, Candidate, Error) == EKSITerrainPrepareResult::Prepared && Store.GetRevision() == 1,
        TEXT("terrain candidate unpublished"));
    Check(Store.CommitEdit(Candidate, Authority, Error) && Store.GetRevision() == 2, TEXT("terrain commit"));
    Check(!Store.CommitEdit(Candidate, Authority, Error), TEXT("stale terrain candidate rejected"));
    FKSITerrainSaveContext Context;
    Context.WorldId = Definition.WorldId; Context.WorldGeneration = 7;
    TArray<uint8> Bytes;
    Check(Store.Capture(Context, Bytes, Error), TEXT("bounded terrain save"));
    FKSITerrainStore Restored;
    Check(Restored.Initialize(Definition, Error) && Restored.Restore(Bytes, Context, Error) && Restored.GetRevision() == 2,
        TEXT("terrain identity and revision restore"));
#elif WITH_KSIAGENT
    FKSIFeatureRegistry Registry;
    FKSIAgentStore Store;
    FKSIAgentRecord Record;
    Record.Id = FGuid::NewGuid(); Record.DefinitionId = FPrimaryAssetId(TEXT("Machine"), TEXT("Sensor"));
    const auto Handle = Store.Add(Record, Registry, Error);
    Check(Handle.Id.IsValid(), TEXT("agent with no biology or economy features"));
    const auto Lease = Store.Acquire(Handle, FGuid::NewGuid());
    Check(Lease.Writer.IsValid() && !Store.Acquire(Handle, FGuid::NewGuid()).Writer.IsValid(), TEXT("one execution owner"));
    TArray<FKSIAgentRecord> Snapshot;
    Check(!Store.Snapshot(Snapshot), TEXT("active writer blocks snapshot"));
    Record.LastSimulatedTime = 42.;
    Check(Store.Commit(Lease, Record, Registry, Error) && Store.Release(Lease), TEXT("lease commit and release"));
    Check(!Store.Commit(Lease, Record, Registry, Error), TEXT("late execution result rejected"));
    Check(Store.Snapshot(Snapshot) && Store.Replace(Snapshot, Registry, Error) && !Store.Find(Handle), TEXT("restore invalidates old handle"));
    const auto* Restored = Store.Find(Store.GetHandle(Record.Id));
    Check(Restored && Restored->LastSimulatedTime == 42., TEXT("persistent value survives execution lifetime"));
#elif WITH_KSIENVIRONMENT
    FKSISurfaceSimulation Surface;
    Check(Surface.Initialize(FGuid::NewGuid(), 0., 0., 0, Error), TEXT("independent surface init"));
    FKSISurfaceGrid Grid;
    Grid.RegionId = FGuid::NewGuid(); Grid.Size = FIntPoint(3, 3); Grid.SpacingMeters = 1.; Grid.Cells.SetNum(9);
    Grid.Cells[4].WaterCubicMeters = 4.; Grid.Ledger.InitialCubicMeters = 4.;
    Grid.Forcing.InfiltrationMetersPerWorldSecond = 0.; Grid.Forcing.DrainageMetersPerWorldSecond = 0.; Grid.Forcing.FlowConductance = 1000.;
    Check(Surface.AddRegion(Grid, Error), TEXT("data-driven surface region"));
    FKSIWorldStep Step; Step.Ticket = FGuid::NewGuid(); Step.StepId = 1; Step.GameToSeconds = .25;
    Check(Surface.Advance(Step, Error), TEXT("surface advance"));
    const auto* Region = Surface.FindRegion(Grid.RegionId);
    Check(Region && Region->Cells[1].WaterCubicMeters == 1. && FMath::Abs(Region->BalanceErrorCubicMeters()) < 1.e-9,
        TEXT("flow conserves water without game subsystem"));
    TArray<uint8> Bytes; FKSISurfaceSimulation Restored;
    Check(Surface.Capture(Bytes, Error) && Restored.Restore(Bytes, Error) && Restored.GetStepId() == 1, TEXT("surface snapshot round trip"));
    Check(!Surface.Advance(Step, Error) && Surface.GetStepId() == 1, TEXT("duplicate step rejected"));
#elif WITH_KSISAVE
    const FString Root = FPaths::ProjectSavedDir() / TEXT("BundleProbe") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FKSIFileBundleFormat Format;
    const FGuid Id = FGuid::NewGuid();
    TMap<FGuid, TArray<uint8>> Parts; Parts.Add(Id, {9, 8, 7});
    auto Validate = [Id](const TArray<uint8>& Payload, const TMap<FGuid, TArray<uint8>>& Items, uint64& Generation, FString& Failure)
    {
        if (Payload.Num() != 1 || Payload[0] == 0 || !Items.Contains(Id))
        {
            Failure = TEXT("Invalid probe payload");
            return false;
        }
        Generation = Payload[0]; return true;
    };
    FString First, Second;
    Check(FKSIFileBundleStore::Write(Root, {1}, Parts, Format, Validate, First, Error)
        && FKSIFileBundleStore::Write(Root, {2}, Parts, Format, Validate, Second, Error), TEXT("immutable file generations"));
    FKSIFileBundle Bundle;
    Check(FKSIFileBundleStore::Recover(Root, Format, Validate, Bundle, Error) && Bundle.Generation == 2 && Bundle.Parts.FindChecked(Id) == Parts.FindChecked(Id),
        TEXT("newest semantically valid generation"));
    Check(FFileHelper::SaveArrayToFile(TArray<uint8>{0}, *(Second / TEXT("complete.bin"))), TEXT("simulate torn completion marker"));
    Check(FKSIFileBundleStore::Recover(Root, Format, Validate, Bundle, Error) && Bundle.Generation == 1, TEXT("torn newest falls back"));
    const FString Incomplete = Root / TEXT("g-99999999999999999999-partial");
    IFileManager::Get().MakeDirectory(*Incomplete, true);
    Check(FFileHelper::SaveArrayToFile(TArray<uint8>{3}, *(Incomplete / Format.RootFileName))
        && FKSIFileBundleStore::Recover(Root, Format, Validate, Bundle, Error) && Bundle.Generation == 1, TEXT("unpublished generation ignored"));
    FString Unchanged = TEXT("unchanged");
    Check(!FKSIFileBundleStore::Write(Root, {0}, Parts, Format, Validate, Unchanged, Error) && Unchanged == TEXT("unchanged"), TEXT("semantic rejection before publication"));
    auto Reject = [](const auto&, const auto&, uint64&, FString&) { return false; };
    Check(!FKSIFileBundleStore::Recover(Root, Format, Reject, Bundle, Error) && Bundle.Generation == 1, TEXT("failed recovery preserves caller output"));
    FKSIFileBundleFormat Bad = Format; Bad.RootFileName = TEXT("../escape.bin");
    Check(!FKSIFileBundleStore::Write(Root, {3}, Parts, Bad, Validate, Unchanged, Error), TEXT("unsafe leaf name rejected"));
    Bad.RootFileName = TEXT("complete.bin");
    Check(!FKSIFileBundleStore::Write(Root, {3}, Parts, Bad, Validate, Unchanged, Error), TEXT("marker name collision rejected"));
#endif
    return bPassed;
}
