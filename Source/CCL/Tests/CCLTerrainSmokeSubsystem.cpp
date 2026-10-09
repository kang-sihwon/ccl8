#include "CCLTerrainSmokeSubsystem.h"

#include "Environment/CCLTerrainChunkComponent.h"
#include "Environment/CCLTerrainRegion.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/GarbageCollection.h"

class FCCLTerrainSmokeParticipant final : public ICCLTerrainEditParticipant
{
public:
	virtual bool Prepare(const FCCLTerrainCandidate& Candidate, FString& Error) override
	{
		Ticket = Candidate.GetTicket();
		if (!bAllowPrepare)
		{
			Error = TEXT("Terrain test participant preparation refused.");
			return false;
		}

		return true;
	}

	virtual bool ValidateCommit(const FCCLTerrainCandidate& Candidate, FString& Error) const override
	{
		if (!bAllowCommit || Ticket != Candidate.GetTicket())
		{
			Error = TEXT("Terrain test participant changed before commit.");
			return false;
		}

		return true;
	}

	virtual void Commit(const FCCLTerrainCandidate& Candidate) override
	{
		++Commits;
		Ticket.Invalidate();
	}

	virtual void Abort(const FCCLTerrainCandidate& Candidate) override
	{
		++Aborts;
		Ticket.Invalidate();
	}

	FGuid Ticket;
	int32 Commits = 0;
	int32 Aborts = 0;
	uint8 bAllowPrepare = 1;
	uint8 bAllowCommit = 1;
};

bool UCCLTerrainSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainSmoke"));
#endif
}

void UCCLTerrainSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (Started == 0.)
	{
		Started = Now;
	}

	if (!Check(Now - Started < 180., TEXT("terrain watchdog")))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->HasBegunPlay() || Now < Next)
	{
		return;
	}

	FString Error;
	FString ScreenshotPath;
	FParse::Value(FCommandLine::Get(), TEXT("CCLTerrainScreenshot="), ScreenshotPath);
	if (Step == 0)
	{
		Region = World->SpawnActor<ACCLTerrainRegion>();
		if (!Check(Region != nullptr, TEXT("spawn terrain region")))
		{
			return;
		}

		Region->SetActorLocation(FVector(40000., 0., 15000.));
		FCCLTerrainDefinition Definition;
		Definition.WorldId = FGuid(1, 2, 3, 4);
		Definition.RegionId = FGuid(5, 6, 7, 8);
		Definition.DefinitionId = FGuid(9, 10, 11, 12);
		Definition.MinimumCell = FIntVector(-32, -32, -16);
		Definition.MaximumCell = FIntVector(32, 32, 16);
		if (!Check(Region->InitializeTerrain(Definition, Error), TEXT("initialize asynchronous terrain")))
		{
			return;
		}

		Step = 1;
		return;
	}

	if (Step == 1)
	{
		if (Region->IsPreparing())
		{
			return;
		}

		if (!Check(Region->IsTerrainReady() && Region->DidLastRequestSucceed() && Region->GetActiveChunkCount() == 8,
			TEXT("initial eight chunks published with cooked collision")) || !CheckHeight(0., 0., 0.))
		{
			return;
		}

		FCCLTerrainAuthority Authority;
		Authority.PrincipalId = Principal;
		Authority.AllowedBoundsMeters = FCCLTerrainStore::EditableBoundsMeters(Region->GetTerrainStore().GetSnapshot().Definition);
		Authority.bCanExcavate = 1;
		Authority.bCanDeposit = 1;
		Participant = MakeShared<FCCLTerrainSmokeParticipant>();
		if (!Check(Region->SetEditAuthority(Authority, Error) && Region->AddParticipant(Participant.ToSharedRef(), Error), TEXT("server policy and dependent state registered"))
			|| !Queue(false, 0., 1) || !CheckHeight(0., 0., 0.))
		{
			return;
		}

		Step = 2;
		return;
	}

	if (Step == 2)
	{
		if (Region->IsPreparing())
		{
			Check(Region->GetTerrainStore().GetRevision() == 1, TEXT("pending data retains committed revision"));
			CheckHeight(0., 0., 0.);
			return;
		}

		if (!Check(Region->DidLastRequestSucceed() && Region->GetTerrainStore().GetRevision() == 2, TEXT("excavation published once"))
			|| !CheckHeight(0., 0., -2.3))
		{
			return;
		}

		for (double X : {-0.5, 0.5})
		{
			for (double Y : {-0.5, 0.5})
			{
				if (!CheckHeight(X, Y, -FMath::Sqrt(2.3 * 2.3 - X * X - Y * Y)))
				{
					return;
				}
			}
		}

		FHitResult Hit;
		const FVector Origin = Region->GetActorLocation();
		const bool bHit = World->SweepSingleByChannel(Hit, Origin + FVector(0., 0., 500.), Origin - FVector(0., 0., 800.),
			FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(20.f, 80.f));
		if (!Check(bHit && Hit.GetActor() == Region && Hit.Location.Z < Origin.Z - 100., TEXT("real capsule sweep enters the excavated floor")))
		{
			return;
		}

#if WITH_DEV_AUTOMATION_TESTS
		Region->RejectNextCookForTesting();
#endif
		if (!Queue(false, 8., 2))
		{
			return;
		}

		Step = 3;
		return;
	}

	if (Region->IsPreparing())
	{
		Check(Region->GetTerrainStore().GetRevision() == 2, TEXT("pending requests leave committed terrain at revision two"));
		return;
	}

	if (Step == 3)
	{
		if (!Check(!Region->DidLastRequestSucceed() && Region->GetLastError().Contains(TEXT("cooking failed")), TEXT("injected cook failure discards complete batch"))
			|| !CheckHeight(8., 0., 0.) || !CheckHeight(0., 0., -2.3) || !Queue(false, 8., 2))
		{
			return;
		}

		FCCLTerrainAuthority Revoked;
		Revoked.PrincipalId = Principal;
		Revoked.PolicyRevision = 2;
		Revoked.AllowedBoundsMeters = FCCLTerrainStore::EditableBoundsMeters(Region->GetTerrainStore().GetSnapshot().Definition);
		if (!Check(Region->SetEditAuthority(Revoked, Error), TEXT("revoke policy while collision is pending")))
		{
			return;
		}

		Step = 4;
		return;
	}

	if (Step == 4)
	{
		if (!Check(!Region->DidLastRequestSucceed() && Region->GetTerrainStore().GetRevision() == 2, TEXT("revoked policy rejected before publication"))
			|| !CheckHeight(8., 0., 0.))
		{
			return;
		}

		FCCLTerrainAuthority Allowed;
		Allowed.PrincipalId = Principal;
		Allowed.PolicyRevision = 3;
		Allowed.AllowedBoundsMeters = FCCLTerrainStore::EditableBoundsMeters(Region->GetTerrainStore().GetSnapshot().Definition);
		Allowed.bCanExcavate = 1;
		Allowed.bCanDeposit = 1;
		if (!Check(Region->SetEditAuthority(Allowed, Error), TEXT("restore policy at a newer revision")))
		{
			return;
		}

		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Occupant = World->SpawnActor<ACharacter>(ACharacter::StaticClass(), Region->GetActorLocation() + FVector(800., 0., 500.), FRotator::ZeroRotator, Spawn);
		if (!Check(Occupant != nullptr, TEXT("spawn actual character capsule")))
		{
			return;
		}

		Occupant->GetCharacterMovement()->DisableMovement();
		if (!Queue(true, 8., 2))
		{
			return;
		}

		Occupant->SetActorLocation(Region->GetActorLocation() + FVector(800., 0., 100.), false, nullptr, ETeleportType::TeleportPhysics);
		Step = 5;
		return;
	}

	if (Step == 5)
	{
		if (!Check(!Region->DidLastRequestSucceed() && Region->GetLastError().Contains(TEXT("bury")), TEXT("character entering during preparation prevents complete burial"))
			|| !CheckHeight(8., 0., 0.))
		{
			return;
		}

		Occupant->SetActorLocation(Region->GetActorLocation() + FVector(800., 0., 230.), false, nullptr, ETeleportType::TeleportPhysics);
		if (!Queue(true, 8., 2))
		{
			return;
		}

		Step = 6;
		return;
	}

	if (Step == 6)
	{
		if (!Check(!Region->DidLastRequestSucceed() && Region->GetLastError().Contains(TEXT("overlap")), TEXT("candidate triangle collision catches partial capsule penetration"))
			|| !CheckHeight(8., 0., 0.))
		{
			return;
		}

		Occupant->Destroy();
		Occupant = nullptr;
		Participant->bAllowCommit = 0;
		if (!Queue(false, 8., 2))
		{
			return;
		}

		Step = 7;
		return;
	}

	if (Step == 7)
	{
		if (!Check(!Region->DidLastRequestSucceed() && Region->GetLastError().Contains(TEXT("participant")), TEXT("dependent state invalidation rejects complete batch"))
			|| !CheckHeight(8., 0., 0.))
		{
			return;
		}

		Participant->bAllowCommit = 1;
		Participant->bAllowPrepare = 0;
		FCCLTerrainEdit Request;
		Request.PrincipalId = Principal;
		Request.Sequence = 2;
		Request.Epoch = Region->GetTerrainStore().GetEpoch();
		Request.ExpectedRevision = Region->GetTerrainStore().GetRevision();
		Request.CenterMeters.X = 8.;
		if (!Check(Region->RequestEdit(Request, Error) == ECCLTerrainPrepareResult::Failed && !Region->IsPreparing(), TEXT("dependent preparation failure keeps terrain idle")))
		{
			return;
		}

		Participant->bAllowPrepare = 1;
		if (!Queue(false, 8., 2))
		{
			return;
		}

		const FGuid CancelledTicket = Region->GetPendingTicket();
		Region->CancelPendingEdit();
		if (!Check(!Region->IsPreparing() && Region->GetTerrainStore().GetRevision() == 2, TEXT("cancel preserves committed state")) || !Queue(true, -8., 2)
			|| !Check(Region->GetPendingTicket() != CancelledTicket, TEXT("replacement edit has a different work ticket")))
		{
			return;
		}

		Step = 8;
		return;
	}

	if (Step == 8)
	{
		if (!Check(Region->DidLastRequestSucceed() && Region->GetTerrainStore().GetRevision() == 3 && Participant->Commits == 2 && Participant->Aborts >= 7,
			TEXT("only successful edit batches commit dependent state")) || !CheckHeight(-8., 0., 2.) || !CheckHeight(8., 0., 0.) || !CheckHeight(0., 0., -2.3))
		{
			return;
		}

		FCCLTerrainMesh Mesh;
		if (!Check(FCCLTerrainMesher::BuildChunk(Region->GetTerrainStore().GetSnapshot(), FIntVector(-1, 0, -1), Region->GetTerrainStore().GetEpoch(), Mesh, Error), TEXT("prepare cancellation collision probe")))
		{
			return;
		}

		auto* Component = NewObject<UCCLTerrainChunkComponent>(Region);
		Component->SetupAttachment(Region->GetRootComponent());
		CancelledChunk = Component;
		if (!Check(Component->Prepare(MoveTemp(Mesh), Error) && Component->GetPreparationState() == ECCLTerrainChunkState::Cooking, TEXT("actual async cook is in flight before cancellation")))
		{
			return;
		}

		Component->Discard();
		CollectGarbage(RF_NoFlags);
		if (!Check(CancelledChunk.IsValid(), TEXT("in-flight collision retains its component across garbage collection")))
		{
			return;
		}

		Step = 9;
		Next = Now + 1.;
		return;
	}

	if (Step == 9)
	{
		CollectGarbage(RF_NoFlags);
		if (!Check(!CancelledChunk.IsValid(), TEXT("cancelled collision completion releases its component")) || !CheckHeight(-8., 0., 2.)
			|| !Check(Region->GetTerrainStore().GetRevision() == 3, TEXT("late cancelled completion cannot publish")))
		{
			return;
		}

		if (!ScreenshotPath.IsEmpty())
		{
			const FVector Location = Region->GetActorLocation() + FVector(2400., -2700., 2200.);
			const FVector Target = Region->GetActorLocation() + FVector(-350., 0., -20.);
			auto* Camera = World->SpawnActor<ACameraActor>(Location, (Target - Location).Rotation());
			auto* Controller = World->GetFirstPlayerController();
			if (!Check(Camera && Controller, TEXT("rendered terrain camera")))
			{
				return;
			}

			auto* Light = World->SpawnActor<ADirectionalLight>(Region->GetActorLocation(), FRotator(-45., -35., 0.));
			if (!Check(Light != nullptr, TEXT("rendered terrain inspection light")))
			{
				return;
			}

			Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			Light->GetLightComponent()->SetIntensity(50000.f);
			auto* CameraComponent = Camera->GetCameraComponent();
			CameraComponent->SetFieldOfView(55.f);
			CameraComponent->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
			CameraComponent->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
			CameraComponent->PostProcessSettings.AutoExposureMinBrightness = 12.f;
			CameraComponent->PostProcessSettings.AutoExposureMaxBrightness = 12.f;
			Controller->SetCinematicMode(true, true, true, true, true);
			Controller->SetViewTarget(Camera);
			Step = 10;
			Next = Now + 2.;
			return;
		}
	}

	if (Step == 10)
	{
		FScreenshotRequest::RequestScreenshot(ScreenshotPath, false, false);
		Step = 11;
		Next = Now + 2.;
		return;
	}

	if (Step == 11 && !Check(IFileManager::Get().FileSize(*ScreenshotPath) > 1024, TEXT("rendered terrain screenshot written")))
	{
		return;
	}

	if (Step == 9 || Step == 11)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_SMOKE PASS Revision=3 Chunks=%d Commits=%d Aborts=%d"), Region->GetActiveChunkCount(), Participant->Commits, Participant->Aborts);
		bComplete = 1;
		FPlatformMisc::RequestExitWithStatus(false, 0);
	}
}

TStatId UCCLTerrainSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLTerrainSmokeSubsystem, STATGROUP_Tickables);
}

bool UCCLTerrainSmokeSubsystem::Check(bool bCondition, const TCHAR* Message)
{
	if (!bCondition && !bComplete)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_TERRAIN_SMOKE FAIL Step=%d %s: %s"), Step, Message, Region ? *Region->GetLastError() : TEXT("No region"));
		bComplete = 1;
		FPlatformMisc::RequestExitWithStatus(false, 2);
	}

	return bCondition;
}

bool UCCLTerrainSmokeSubsystem::CheckHeight(double X, double Y, double ExpectedMeters, double Tolerance)
{
	const FVector Center = Region->GetActorLocation() + FVector(X * 100., Y * 100., 0.);
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Occupant);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Center + FVector(0., 0., 800.), Center - FVector(0., 0., 800.), ECC_Visibility, Params);
	const double Height = (Hit.ImpactPoint.Z - Center.Z) * 0.01;
	if (!bHit || Hit.GetActor() != Region || FMath::Abs(Height - ExpectedMeters) > Tolerance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Terrain probe X=%.2f Y=%.2f Expected=%.3f Actual=%.3f Hit=%s"), X, Y, ExpectedMeters, Height, *GetNameSafe(Hit.GetActor()));
	}

	return Check(bHit && Hit.GetActor() == Region && FMath::Abs(Height - ExpectedMeters) <= Tolerance, TEXT("live collision height matches committed terrain"));
}

bool UCCLTerrainSmokeSubsystem::Queue(bool bDeposit, double X, uint64 Sequence)
{
	FCCLTerrainEdit Request;
	Request.PrincipalId = Principal;
	Request.Sequence = Sequence;
	Request.Epoch = Region->GetTerrainStore().GetEpoch();
	Request.ExpectedRevision = Region->GetTerrainStore().GetRevision();
	Request.CenterMeters.X = X;
	Request.RadiusMeters = bDeposit ? 2. : 2.3;
	Request.Kind = bDeposit ? ECCLTerrainEditKind::Deposit : ECCLTerrainEditKind::Excavate;
	Request.Material = bDeposit ? 2 : 1;
	FString Error;
	return Check(Region->RequestEdit(Request, Error) == ECCLTerrainPrepareResult::Prepared, *FString::Printf(TEXT("queue terrain edit: %s"), *Error));
}
