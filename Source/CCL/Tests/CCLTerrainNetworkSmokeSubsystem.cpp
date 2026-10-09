#include "CCLTerrainNetworkSmokeSubsystem.h"

#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLTerrainRegion.h"
#include "Environment/CCLTerrainReplication.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	bool CheckRepeatedReplica(ACCLTerrainRegion* Region, const TArray<uint8>& Bytes, uint64 Serial, FString& Error)
	{
		const auto Ticket = Region->GetPendingTicket();
		const auto Epoch = Region->GetTerrainStore().GetEpoch();
		FCCLTerrainSnapshot Changed;
		FCCLTerrainSaveContext Context;
		TArray<uint8> ConflictingBytes;
		if (!Region->ApplyReplica(Bytes, Serial, Error) || Region->GetPendingTicket() != Ticket
			|| Region->GetTerrainStore().GetEpoch() != Epoch || !FCCLTerrainCodec::Decode(Bytes, Changed, Context, Error))
		{
			return false;
		}

		++Changed.Revision;
		return FCCLTerrainCodec::Encode(Changed, Context, ConflictingBytes, Error)
			&& !Region->ApplyReplica(ConflictingBytes, Serial, Error)
			&& Region->GetPendingTicket() == Ticket && Region->GetTerrainStore().GetEpoch() == Epoch;
	}
}

bool UCCLTerrainNetworkSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainNetworkSmoke"));
#endif
}

void UCCLTerrainNetworkSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}

	if (Now - Started > 180.)
	{
		Finish(false, FString::Printf(TEXT("watchdog step=%d"), Step));
		return;
	}

	auto* World = GetWorld();
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	auto* Region = Director ? Director->GetTerrainRegion() : nullptr;
	if (!World || !World->HasBegunPlay() || !Director || !Region)
	{
		return;
	}

	if (World->GetNetMode() == NM_Client)
	{
		auto* PC = Cast<ACCLExperimentPlayerController>(World->GetFirstPlayerController());
		auto* Peer = PC ? PC->FindComponentByClass<UCCLTerrainReplication>() : nullptr;
		if (!bCheckedPendingReplica && Peer && Peer->bApplying && Region->IsPreparing())
		{
			FString Error;
			if (!CheckRepeatedReplica(Region, Peer->FullBytes, Peer->TargetSerial, Error))
			{
				Finish(false, TEXT("pending replica retry or conflict rejection: ") + Error);
				return;
			}

			bCheckedPendingReplica = 1;
		}

		if (PC && PC->GetExperimentMessage() == TEXT("TERRAIN_NETWORK_FINISHED"))
		{
			Finish(Step == 1 && bCheckedPendingReplica, TEXT("replica collision, pending/applied retries, conflicting bytes and late-join recovery"));
			return;
		}

		if (Step || !Region->IsReplicaReady() || Region->GetTerrainStore().GetRevision() != 2)
		{
			return;
		}

		FHitResult Hit;
		const FVector Origin = Region->GetActorLocation();
		const bool bHit = World->LineTraceSingleByChannel(Hit, Origin + FVector(0., 0., 500.), Origin - FVector(0., 0., 350.), ECC_Visibility);
		TArray<uint8> Saved;
		FCCLTerrainSaveContext Context;
		Context.WorldId = Region->GetTerrainStore().GetSnapshot().Definition.WorldId;
		Context.WorldGeneration = 1;
		FString Error;
		const bool bSaved = Region->GetTerrainStore().Capture(Context, Saved, Error);
		if (!bHit || Hit.GetActor() != Region || FMath::Abs(Hit.ImpactPoint.Z - (Origin.Z - 150.)) > 10.
			|| !bSaved || !CheckRepeatedReplica(Region, Saved, Region->GetReplicaSerial(), Error)
			|| Region->ApplyReplica(Saved, Region->GetReplicaSerial() - 1, Error))
		{
			Finish(false, FString::Printf(TEXT("client collision or stale snapshot rejection hit=%d actor=%s origin=%s point=%s saved=%d error=%s"), int32(bHit), *GetNameSafe(Hit.GetActor()), *Origin.ToString(), *Hit.ImpactPoint.ToString(), int32(bSaved), *Error));
			return;
		}

		Step = 1;
		UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_NETWORK CLIENT_READY Serial=%llu"), Region->GetReplicaSerial());
		return;
	}

	if (!Region->IsTerrainReady())
	{
		return;
	}

	TArray<ACCLExperimentPlayerController*> Remotes;
	ACCLExperimentPlayerController* OperatorPC = nullptr;
	for (TActorIterator<ACCLExperimentPlayerController> It(World); It; ++It)
	{
		if (Director->CanOperate(*It))
		{
			OperatorPC = *It;
		}

		if (!It->IsLocalController())
		{
			Remotes.Add(*It);
		}
	}

	if (Step == 0)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_NETWORK BASE_READY"));
		Step = 1;
	}

	FString Error;
	if (Step == 1 && OperatorPC && !Remotes.IsEmpty())
	{
		auto* Peer = Remotes[0]->FindComponentByClass<UCCLTerrainReplication>();
		if (!Peer || !Peer->IsClientReady(Region))
		{
			return;
		}

#if WITH_DEV_AUTOMATION_TESTS
		Peer->bDropNextReadyAckForTesting = 1;
#endif
		if (!Director->Execute(OperatorPC, ECCLExperimentAction::TerrainExcavate, TEXT("Zone_05"), Director->GetGeneration(), {}, Error, Region->GetPublicationSerial()))
		{
			Finish(false, TEXT("authorized server edit: ") + Error);
			return;
		}

		Step = 2;
	}

	if (Step == 2 && !Region->IsPreparing())
	{
		if (!Region->DidLastRequestSucceed() || Region->GetTerrainStore().GetRevision() != 2)
		{
			Finish(false, TEXT("edit publication failed"));
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_NETWORK EDIT_READY"));
		Step = 3;
	}

	if (Step == 3 && Remotes.Num() >= 2)
	{
		for (auto* Remote : Remotes)
		{
			const auto* Peer = Remote->FindComponentByClass<UCCLTerrainReplication>();
			if (!Peer || !Peer->IsClientReady(Region))
			{
				return;
			}
		}

		auto* Observer = Remotes.FindByPredicate([Director](const auto* PC) { return !Director->CanOperate(PC); });
		if (!Observer || Director->Execute(*Observer, ECCLExperimentAction::TerrainDeposit, TEXT("Zone_05"), Director->GetGeneration(), {}, Error, Region->GetPublicationSerial())
			|| Director->Execute(OperatorPC, ECCLExperimentAction::TerrainDeposit, TEXT("Zone_05"), Director->GetGeneration(), {}, Error, Region->GetPublicationSerial() - 1)
			|| !Director->Execute(OperatorPC, ECCLExperimentAction::TerrainProtection, TEXT("Zone_05"), Director->GetGeneration(), {}, Error, Region->GetPublicationSerial()))
		{
			Finish(false, TEXT("owner, stale request or protection rejection"));
			return;
		}

		FinishAt = Now + 2.;
		Step = 4;
	}

	if (Step == 4 && Now >= FinishAt)
	{
		for (auto* Remote : Remotes)
		{
			Remote->ClientExperimentResponse(TEXT("TERRAIN_NETWORK_FINISHED"));
		}

		Step = 5;
		FinishAt = Now + 3.;
	}

	if (Step == 5 && Now >= FinishAt)
	{
		Finish(true, TEXT("snapshot, delta, late join, collision and authority"));
	}
}

TStatId UCCLTerrainNetworkSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLTerrainNetworkSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLTerrainNetworkSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_NETWORK %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	bComplete = 1;
	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 2);
}
