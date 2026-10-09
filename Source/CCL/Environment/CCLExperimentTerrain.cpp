#include "CCLExperimentDirector.h"

#include "CCLTerrainRegion.h"
#include "CCLTerrainNavigation.h"
#include "CCLTerrainReplication.h"
#include "CCLWorldGenerationStore.h"
#include "CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

void ACCLExperimentDirector::InitializeTerrainExperiment()
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	FCCLTerrainDefinition Definition;
	Definition.WorldId = Runtime->GetIdentity().WorldId;
	Definition.RegionId = FGuid(501, 502, 503, 504);
	Definition.DefinitionId = FGuid(510, 520, 530, 540);
	Definition.MinimumCell = FIntVector(-16, -16, -8);
	Definition.MaximumCell = FIntVector(16, 16, 8);
	FCCLTerrainProtection Protection;
	Protection.Id = FGuid(551, 552, 553, 554);
	Protection.BoundsMeters = FBox(FVector(-8., -8., -4.), FVector(-4.5, -2., 4.));
	Definition.ProtectedRegions.Add(Protection);
	TerrainRegion = GetWorld()->SpawnActor<ACCLTerrainRegion>();
	TerrainRegion->SetActorLocation(UCCLExperimentDefinition::ZoneCenter(5) + FVector(0., 0., 400.));
	FString Error;
	TArray<uint8> SavedTerrain;
	const auto* SessionStore = GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
	if (const auto* Bundle = SessionStore->TravelSnapshots.Find(Runtime->GetIdentity().Domain))
	{
		const auto* Bytes = Bundle->Terrain.Find(Definition.RegionId);
		FCCLWorldSnapshot SavedWorld;
		FCCLTerrainSnapshot Saved;
		FCCLTerrainSaveContext Context;
		if (!Bytes || Bundle->Terrain.Num() != 1 || !FCCLWorldSnapshotCodec::Decode(Bundle->World, SavedWorld, Error)
			|| !(FCCLWorldGenerationStore::ContextFor(SavedWorld) == Bundle->Context)
			|| !FCCLTerrainCodec::Decode(*Bytes, Saved, Context, Error)
			|| !(Context == Bundle->Context))
		{
			UE_LOG(LogTemp, Error, TEXT("CCL_TERRAIN_EXPERIMENT invalid session terrain: %s"), *Error);
			InitialSnapshot.Reset();
			return;
		}

		SavedTerrain = *Bytes;
	}

	if (!TerrainRegion->InitializeTerrain(Definition, Error, SavedTerrain))
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_TERRAIN_EXPERIMENT initialization: %s"), *Error);
		return;
	}

	auto* Bounds = GetWorld()->SpawnActor<ACCLTerrainNavigationBounds>();
	Bounds->Configure(TerrainRegion->GetWorldTerrainBounds().ExpandBy(200.));
	if (auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		Nav->GetDefaultNavDataInstance(FNavigationSystem::Create);
	}
}

bool ACCLExperimentDirector::TerrainAction(APlayerController* Requester, ECCLExperimentAction Action, FString& Error, const FVector* Target)
{
	if (!TerrainRegion || !TerrainRegion->IsTerrainReady() || TerrainRegion->IsPreparing())
	{
		Error = TEXT("지형을 준비 중이다.");
		return false;
	}

	if (Action == ECCLExperimentAction::TerrainReset)
	{
		return ResetTerrain(Error);
	}

	if (Target)
	{
		if (Target->ContainsNaN() || !TerrainRegion->GetWorldTerrainBounds().ExpandBy(10.).IsInsideOrOn(*Target)
			|| Action < ECCLExperimentAction::TerrainExcavate || Action > ECCLExperimentAction::TerrainChannel)
		{
			Error = TEXT("편집 가능한 지형 바닥을 선택해 줘.");
			return false;
		}

		FHitResult SurfaceHit;
		FCollisionQueryParams Query;
		if (Requester && Requester->GetPawn())
		{
			Query.AddIgnoredActor(Requester->GetPawn());
		}
		if (!GetWorld()->LineTraceSingleByChannel(SurfaceHit, *Target + FVector(0., 0., 100.),
			*Target - FVector(0., 0., 100.), ECC_Visibility, Query) || SurfaceHit.GetActor() != TerrainRegion
			|| FVector::DistSquared(SurfaceHit.ImpactPoint, *Target) > FMath::Square(20.))
		{
			Error = TEXT("선택한 바닥이 바뀌었다. 미리보기를 확인하고 다시 클릭해 줘.");
			return false;
		}
	}

	const auto& Store = TerrainRegion->GetTerrainStore();
	const auto& Snapshot = Store.GetSnapshot();
	const FString PrincipalName = Requester && Requester->PlayerState ? FString::FromInt(Requester->PlayerState->GetPlayerId()) : TEXT("ServerExperiment");
	FCCLTerrainAuthority Authority;
	Authority.PrincipalId = FGuid::NewDeterministicGuid(Snapshot.Definition.WorldId.ToString() + TEXT("/Terrain/") + PrincipalName);
	Authority.PolicyRevision = TerrainRegion->GetPublicationSerial() + 1;
	Authority.AllowedBoundsMeters = FCCLTerrainStore::EditableBoundsMeters(Snapshot.Definition);
	Authority.MaximumRadiusMeters = 2.5;
	Authority.bCanExcavate = 1;
	Authority.bCanDeposit = 1;
	// Replacing an authority for successive edits needs a strictly increasing policy revision.
	Authority.PolicyRevision += ++TerrainAuthoritySequence;
	if (!TerrainRegion->SetEditAuthority(Authority, Error))
	{
		return false;
	}

	FCCLTerrainEdit Edit;
	Edit.PrincipalId = Authority.PrincipalId;
	const auto* Receipt = Snapshot.Receipts.Find(Authority.PrincipalId);
	Edit.Sequence = Receipt ? Receipt->Request.Sequence + 1 : 1;
	Edit.ExpectedRevision = Store.GetRevision();
	Edit.Epoch = Store.GetEpoch();
	Edit.RadiusMeters = 1.5;
	Edit.CenterMeters = FVector(0., 0., 0.);
	if (Action == ECCLExperimentAction::TerrainDeposit)
	{
		Edit.Kind = ECCLTerrainEditKind::Deposit;
		Edit.CenterMeters = FVector(4., 3., 0.);
		Edit.Material = 2;
	}
	else if (Action == ECCLExperimentAction::TerrainChannel)
	{
		Edit.CenterMeters = FVector(0., 2., 0.);
	}
	else if (Action == ECCLExperimentAction::TerrainProtection)
	{
		Edit.CenterMeters = FVector(-5., -5., 0.);
		Edit.RadiusMeters = 0.5;
	}

	if (Target)
	{
		Edit.CenterMeters = (*Target - TerrainRegion->GetActorLocation()) / 100.;
		// Leave room for the five-sample interpolation band above and below the clicked surface.
		Edit.RadiusMeters = 1.;
		if (Action == ECCLExperimentAction::TerrainChannel)
		{
			Edit.RadiusMeters = 0.8;
			Edit.CenterMeters.Z += 0.35;
		}
	}

	const auto Result = TerrainRegion->RequestEdit(Edit, Error);
	if (Action == ECCLExperimentAction::TerrainProtection)
	{
		const bool bProtected = Result == ECCLTerrainPrepareResult::Failed && Error.Contains(TEXT("protected"));
		Error = bProtected ? TEXT("보호 구역 편집을 거부했고 기존 지형을 유지했다.") : TEXT("보호 구역 거부 검사에 실패했다.");
		return bProtected;
	}

	if (Result == ECCLTerrainPrepareResult::Prepared)
	{
		Error = TEXT("후보 지형과 충돌을 준비하고 있다. 완료 후 함께 적용한다.");
		return true;
	}

	if (Target && Error.Contains(TEXT("allowed bounds")))
	{
		Error = TEXT("브러시와 보간 범위가 지형의 가장자리·높이 한계를 넘는다. 안쪽의 평평한 바닥을 선택해 줘.");
	}
	else if (Target && Error.Contains(TEXT("protected")))
	{
		Error = TEXT("보호 구역과 겹쳐 편집을 거부했다. 청록색 영역 안의 다른 위치를 선택해 줘.");
	}
	return false;
}

bool ACCLExperimentDirector::ResetTerrain(FString& Error)
{
	if (!TerrainRegion || !TerrainRegion->IsTerrainReady() || !MoveToSafety(nullptr, 0, Error))
	{
		return false;
	}

	TerrainRegion->CancelPendingEdit();
	FCCLTerrainStore Base;
	const auto& Definition = TerrainRegion->GetTerrainStore().GetSnapshot().Definition;
	FCCLTerrainSaveContext Context;
	Context.WorldId = Definition.WorldId;
	Context.BaseWorldVersion = Definition.BaseWorldVersion;
	Context.WorldGeneration = 1;
	TArray<uint8> Bytes;
	return Base.Initialize(Definition, Error) && Base.Capture(Context, Bytes, Error) && TerrainRegion->RequestRestore(Bytes, Context, Error);
}

void ACCLExperimentDirector::TickTerrainExperiment()
{
	if (!TerrainRegion)
	{
		return;
	}

	if (bTerrainRestorePending && !TerrainRegion->IsPreparing())
	{
		bTerrainRestorePending = 0;
		StorageMessage = TerrainRegion->DidLastRequestSucceed()
			? TEXT("복원 완료. 안전 지점으로 이동했다. 확인할 구역으로 다시 이동해 줘.")
			: TEXT("복원 실패: ") + TerrainRegion->GetLastError();
		ForceNetUpdate();
		if (!TerrainRegion->DidLastRequestSucceed())
		{
			if (auto* Result = Results.FindByPredicate([](const auto& R) { return R.CaseId == TEXT("Zone_08"); }))
			{
				Result->Status = ECCLExperimentStatus::Failed;
				Result->Detail = TerrainRegion->GetLastError();
				ForceNetUpdate();
			}
		}
	}

	if (ActiveCase != TEXT("Zone_05") || !TerrainCaseStep || TerrainRegion->IsPreparing())
	{
		return;
	}

	auto* Result = Results.FindByPredicate([](const auto& R) { return R.CaseId == TEXT("Zone_05"); });
	FString Error;
	if (!TerrainRegion->DidLastRequestSucceed())
	{
		TerrainCaseStep = 0;
		Finish(*Result, false, TerrainRegion->GetLastError());
		return;
	}

	if (TerrainCaseStep == 1)
	{
		if (!TerrainAction(nullptr, ECCLExperimentAction::TerrainExcavate, Error))
		{
			TerrainCaseStep = 0;
			Finish(*Result, false, Error);
			return;
		}

		TerrainCaseStep = 2;
		return;
	}

	if (!TerrainRegion->IsNavigationReady())
	{
		return;
	}

	const FVector Origin = TerrainRegion->GetActorLocation();
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Origin + FVector(0., 0., 500.), Origin - FVector(0., 0., 350.), ECC_Visibility);
	auto* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Origin + FVector(-300., 500., 10.), Origin + FVector(300., 500., 10.));
	const bool bProtected = TerrainAction(nullptr, ECCLExperimentAction::TerrainProtection, Error);
	const bool bPassed = bHit && Hit.GetActor() == TerrainRegion && FMath::Abs(Hit.ImpactPoint.Z - (Origin.Z - 150.)) < 10.
		&& Path && Path->IsValid() && !Path->IsPartial() && bProtected;
	TerrainCaseStep = 0;
	Finish(*Result, bPassed, bPassed ? TEXT("굴착의 실제 충돌·경로 탐색·보호 구역 거부를 확인했다.") : FString::Printf(TEXT("지형 검사 실패: hit=%d actor=%s point=%s origin=%s path=%d partial=%d protection=%d error=%s"), int32(bHit), *GetNameSafe(Hit.GetActor()), *Hit.ImpactPoint.ToString(), *Origin.ToString(), int32(Path && Path->IsValid()), int32(Path && Path->IsPartial()), int32(bProtected), *Error));
}

FString ACCLExperimentDirector::GenerationRoot() const
{
	return FPaths::ProjectSavedDir() / TEXT("EnvironmentGenerations") / SlotName();
}

bool ACCLExperimentDirector::SaveTerrainWorld(FString& Error)
{
	if (!TerrainRegion->IsTerrainReady() || TerrainRegion->IsPreparing())
	{
		Error = TEXT("지형 확정 후 저장할 수 있다.");
		return false;
	}

	TArray<uint8> WorldBytes;
	FCCLWorldSnapshot WorldState;
	if (!GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>()->Save(WorldBytes)
		|| !FCCLWorldSnapshotCodec::Decode(WorldBytes, WorldState, Error))
	{
		return false;
	}

	FCCLWorldGenerationBundle Previous;
	FString Ignore;
	if (FCCLWorldGenerationStore::Recover(GenerationRoot(), WorldState.Identity.Domain, Previous, Ignore)
		&& Previous.Context.WorldGeneration >= WorldState.Identity.Generation)
	{
		if (Previous.Context.WorldGeneration == MAX_uint64)
		{
			Error = TEXT("저장 세대 번호가 소진됐다.");
			return false;
		}

		WorldState.Identity.Generation = Previous.Context.WorldGeneration + 1;
		if (!FCCLWorldSnapshotCodec::Encode(WorldState, WorldBytes, Error))
		{
			return false;
		}
	}

	TMap<FGuid, TArray<uint8>> Terrain;
	const auto& Definition = TerrainRegion->GetTerrainStore().GetSnapshot().Definition;
	if (!TerrainRegion->GetTerrainStore().Capture(FCCLWorldGenerationStore::ContextFor(WorldState), Terrain.FindOrAdd(Definition.RegionId), Error))
	{
		return false;
	}

	FString Directory;
	if (!FCCLWorldGenerationStore::Write(GenerationRoot(), WorldBytes, Terrain, Directory, Error))
	{
		return false;
	}

	SavedGameSeconds = WorldState.Clock.GameSeconds;
	SavedWorldSeconds = WorldState.Clock.WorldSeconds;
	SavedTerrainRevision = TerrainRegion->GetTerrainStore().GetRevision();
	Error = TEXT("저장 완료. 시계·천체 설정·문 상태·Agent·지형·눈·물을 함께 저장했다.");
	return true;
}

bool ACCLExperimentDirector::QueueWorldRestore(const TArray<uint8>& TerrainBytes, const FCCLTerrainSaveContext& Context,
	const TArray<uint8>& WorldBytes, FString& Error)
{
	if (!TerrainRegion || !MoveToSafety(nullptr, 0, Error))
	{
		return false;
	}

	FCCLWorldSnapshot WorldState;
	if (!FCCLWorldSnapshotCodec::Decode(WorldBytes, WorldState, Error))
	{
		return false;
	}
	TerrainRegion->CancelPendingEdit();
	TWeakObjectPtr<ACCLExperimentDirector> Weak(this);
	if (!TerrainRegion->RequestRestore(TerrainBytes, Context, Error, [Weak, WorldBytes](FString& CommitError)
	{
		if (!Weak.IsValid() || !Weak->GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>()->Restore(WorldBytes, CommitError))
		{
			return false;
		}

		auto* Director = Weak.Get();
		Director->GetWorldTimerManager().ClearTimer(Director->RunTimer);
		Director->Generation = FGuid::NewGuid();
		Director->ActiveCase = NAME_None;
		Director->TerrainCaseStep = 0;
		Director->WaterCaseStep = 0;
		for (auto& Result : Director->Results)
		{
			Result.Generation = Director->Generation;
			Result.RunId.Invalidate();
			Result.Status = Director->FindDefinition(Result.CaseId)->IsImplemented() ? ECCLExperimentStatus::Ready : ECCLExperimentStatus::NotImplemented;
			Result.Detail = TEXT("세계와 지형을 같은 세대로 복원했다. 검사를 다시 실행할 수 있다.");
		}

		Director->ForceNetUpdate();
		return true;
	}, &WorldState))
	{
		return false;
	}

	bTerrainRestorePending = TerrainRegion->IsPreparing() ? 1 : 0;
	Error = TEXT("저장 지형의 충돌을 준비한 뒤 세계와 함께 복원한다.");
	return true;
}

bool ACCLExperimentDirector::LoadTerrainWorld(FString& Error)
{
	FCCLWorldGenerationBundle Bundle;
	if (!TerrainRegion->IsTerrainReady() || TerrainRegion->IsPreparing()
		|| !FCCLWorldGenerationStore::Recover(GenerationRoot(), UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()), Bundle, Error))
	{
		return false;
	}

	const FGuid RegionId = TerrainRegion->GetTerrainStore().GetSnapshot().Definition.RegionId;
	const auto* Bytes = Bundle.Terrain.Find(RegionId);
	if (!Bytes || Bundle.Terrain.Num() != 1)
	{
		Error = TEXT("저장된 지형 지역 목록이 실험장과 다르다.");
		return false;
	}

	FCCLWorldSnapshot Saved;
	FCCLTerrainSnapshot SavedTerrain;
	FCCLTerrainSaveContext SavedContext;
	if (!FCCLWorldSnapshotCodec::Decode(Bundle.World, Saved, Error)
		|| !FCCLTerrainCodec::Decode(*Bytes, SavedTerrain, SavedContext, Error)
		|| !QueueWorldRestore(*Bytes, Bundle.Context, Bundle.World, Error))
	{
		return false;
	}

	SavedGameSeconds = Saved.Clock.GameSeconds;
	SavedWorldSeconds = Saved.Clock.WorldSeconds;
	SavedTerrainRevision = SavedTerrain.Revision;
	Error = bTerrainRestorePending ? TEXT("복원 중: 저장된 지형 충돌과 세계 상태를 준비하고 있다.")
		: TEXT("복원 완료. 안전 지점으로 이동했다. 확인할 구역으로 다시 이동해 줘.");
	return true;
}
