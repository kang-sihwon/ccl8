#include "CCLTerrainTravelSmokeSubsystem.h"

#include "Agents/CCLAgentWorldSubsystem.h"
#include "Agents/CCLAccountComponent.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLTerrainRegion.h"
#include "Environment/CCLTerrainReplication.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "GameFramework/PlayerState.h"

bool UCCLTerrainTravelSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainTravelSmoke"));
#endif
}

void UCCLTerrainTravelSmokeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FWorldDelegates::OnWorldTickStart.AddUObject(this, &UCCLTerrainTravelSmokeSubsystem::FreezeClock);
}

void UCCLTerrainTravelSmokeSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldTickStart.RemoveAll(this);
	Super::Deinitialize();
}

void UCCLTerrainTravelSmokeSubsystem::Tick(float DeltaTime)
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
	if (!World || World == DepartedWorld.Get() || !World->HasBegunPlay() || !Director || !Director->IsReady()
		|| !Region || !Region->IsTerrainReady() || Region->IsPreparing() || !Region->IsNavigationReady())
	{
		return;
	}

	if (!bAnnounced)
	{
		bAnnounced = 1;
		UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_TRAVEL BASE_READY"));
	}

	ACCLExperimentPlayerController* Operator = nullptr;
	int32 ReadyRemotes = 0;
	for (TActorIterator<ACCLExperimentPlayerController> It(World); It; ++It)
	{
		if (Director->CanOperate(*It))
		{
			Operator = *It;
		}

		const auto* Peer = It->FindComponentByClass<UCCLTerrainReplication>();
		if (!It->IsLocalController() && Peer && Peer->IsClientReady(Region))
		{
			++ReadyRemotes;
		}
	}

	if (!Operator || (World->GetNetMode() != NM_Standalone && ReadyRemotes == 0))
	{
		return;
	}

	auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
	const auto Domain = Runtime->GetIdentity().Domain;
	const auto Travel = Domain == ECCLWorldDomain::Scenario ? ECCLExperimentAction::TravelHub : ECCLExperimentAction::TravelScenario;
	FString Error;
	auto Check = [&](bool bPassed)
	{
		if (!bPassed)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s"), Step, *Error));
		}

		return bPassed;
	};
	auto Execute = [&](ECCLExperimentAction Action)
	{
		return Director->Execute(Operator, Action, NAME_None, Director->GetGeneration(), {}, Error, Region->GetPublicationSerial());
	};
	auto Height = [&](double X, double Y, double Z)
	{
		FHitResult Hit;
		const auto Origin = Region->GetActorLocation() + FVector(X, Y, 0.);
		return World->LineTraceSingleByChannel(Hit, Origin + FVector(0., 0., 500.), Origin - FVector(0., 0., 350.), ECC_Visibility)
			&& Hit.GetActor() == Region && FMath::Abs(Hit.ImpactPoint.Z - Origin.Z - Z) < 10.;
	};
	auto Depart = [&]()
	{
		if (!Check(Execute(Travel)))
		{
			return false;
		}

		Expected.Add(Domain, GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>()->TravelSnapshots[Domain]);
		PreviousEpochs.Add(Domain, Region->GetTerrainStore().GetEpoch());
		DepartedWorld = World;
		++Step;
		return true;
	};
	auto VerifyReturn = [&]()
	{
		const auto* Bundle = Expected.Find(Domain);
		FCCLWorldSnapshot Saved, Actual;
		FCCLLifeSimulation SavedLife;
		TArray<uint8> SavedWorld, ActualWorld, Terrain;
		const FGuid RegionId = Region->GetTerrainStore().GetSnapshot().Definition.RegionId;
		FCCLSimulationSnapshot ExpectedLife, ActualLife;
		if (!Bundle || !Bundle->Terrain.Contains(RegionId)
			|| !FCCLWorldSnapshotCodec::Decode(Bundle->World, Saved, Error)
			|| !SavedLife.Load(Saved.Life, Error) || !SavedLife.Capture(ExpectedLife)
			|| !Agents->GetSimulation().Capture(ActualLife)
			|| !FCCLWorldSnapshotCodec::Capture(Runtime->GetIdentity(), Runtime->GetClock(), Agents->GetSimulation(), Actual, Error, &Runtime->GetEnvironmentInputs(), &Runtime->GetSurfaceSimulation()))
		{
			return false;
		}

		// A new PlayerState opens a new account after world restoration. All saved accounts
		// must still match; allow only the current players' untouched opening balances.
		for (TActorIterator<APlayerState> It(World); It; ++It)
		{
			const auto* Account = It->FindComponentByClass<UCCLAccountComponent>();
			if (Account && !ExpectedLife.Economy.Accounts.Contains(Account->GetAccountId()))
			{
				const auto* Opened = ActualLife.Economy.Accounts.Find(Account->GetAccountId());
				if (!Opened || !Opened->bHasOpeningBalance || Opened->Balance != Opened->OpeningBalance)
				{
					Error = TEXT("new player account is not an untouched opening balance");
					return false;
				}

				ActualLife.Economy.Accounts.Remove(Account->GetAccountId());
			}
		}

		if (!FCCLSimulationSnapshot::StaticStruct()->CompareScriptStruct(&ExpectedLife, &ActualLife, PPF_None))
		{
			for (TFieldIterator<FProperty> Property(FCCLSimulationSnapshot::StaticStruct()); Property; ++Property)
			{
				if (!Property->Identical_InContainer(&ExpectedLife, &ActualLife, PPF_None))
				{
					Error += Property->GetName() + TEXT(" ");
				}
			}

			Error = TEXT("life fields differ: ") + Error;
			return false;
		}

		// Compare UObject SaveGame state semantically, then compare the canonical world envelope.
		Actual.Life = Saved.Life;
		if (!FCCLWorldSnapshotCodec::Encode(Saved, SavedWorld, Error)
			|| !FCCLWorldSnapshotCodec::Encode(Actual, ActualWorld, Error) || SavedWorld != ActualWorld)
		{
			Error = TEXT("world identity, clock or environment differs after travel");
			return false;
		}

		if (!Region->GetTerrainStore().Capture(Bundle->Context, Terrain, Error) || Terrain != Bundle->Terrain[RegionId]
			|| Region->GetTerrainStore().GetEpoch() == PreviousEpochs[Domain])
		{
			Error = TEXT("terrain checkpoint or new epoch mismatch");
			return false;
		}

		if (!(Domain == ECCLWorldDomain::Scenario ? Height(0., 0., -150.) : Height(400., 300., 150.)))
		{
			Error = TEXT("restored collision height mismatch");
			return false;
		}

		return true;
	};

	if (Step == 0)
	{
		if (!Check(Domain == ECCLWorldDomain::Scenario && Region->GetTerrainStore().GetRevision() == 1
			&& Runtime->ApplySnowContact(ACCLExperimentDirector::SnowRegionId(), FGuid(16, 1, 1, 1), 1, UCCLExperimentDefinition::ZoneCenter(3) / 100., 0.27, Error)
			&& Runtime->QueueGameTime(37., Error) && Runtime->AdvancePending(40., 2400., Error)
			&& Runtime->ChangeTimeScale(7., Error) && Runtime->QueueGameTime(1.25, Error)
			&& Execute(ECCLExperimentAction::TerrainExcavate) && Region->IsPreparing() && !Execute(Travel)))
		{
			return;
		}

		++Step;
	}
	else if (Step == 1 || Step == 3 || Step == 5)
	{
		Depart();
	}
	else if (Step == 2)
	{
		if (!Check(Domain == ECCLWorldDomain::Playground && Region->GetTerrainStore().GetRevision() == 1 && Height(0., 0., 0.)
			&& Runtime->GetIdentity().WorldId != Expected[ECCLWorldDomain::Scenario].Context.WorldId
			&& Runtime->ApplySnowContact(ACCLExperimentDirector::SnowRegionId(), FGuid(16, 2, 1, 1), 1, UCCLExperimentDefinition::ZoneCenter(3) / 100. + FVector(1., 0., 0.), 0.27, Error)
			&& Runtime->QueueGameTime(13., Error) && Runtime->AdvancePending(15., 900., Error)
			&& Execute(ECCLExperimentAction::TerrainDeposit)))
		{
			return;
		}

		++Step;
	}
	else if (Step == 4)
	{
		if (!Check(Domain == ECCLWorldDomain::Scenario && VerifyReturn() && Execute(ECCLExperimentAction::TerrainChannel)))
		{
			return;
		}

		++Step;
	}
	else if (Step == 6)
	{
		if (Check(Domain == ECCLWorldDomain::Playground && VerifyReturn()))
		{
			Depart();
		}
	}
	else if (Step == 7 && Check(Domain == ECCLWorldDomain::Scenario && VerifyReturn() && Height(0., 200., -150.)))
	{
		Finish(true, TEXT("four travels preserve separate world/clock/life/terrain checkpoints and collision; pending travel rejected; remote readiness checked"));
	}
}

TStatId UCCLTerrainTravelSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLTerrainTravelSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLTerrainTravelSmokeSubsystem::FreezeClock(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	if (World && World->GetGameInstance() == GetGameInstance())
	{
		if (auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>())
		{
			Runtime->SetTickableTickType(ETickableTickType::Never);
		}
	}
}

void UCCLTerrainTravelSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_TRAVEL %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	bComplete = 1;
	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 2);
}
