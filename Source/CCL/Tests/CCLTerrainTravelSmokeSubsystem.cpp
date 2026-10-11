#include "CCLTerrainTravelSmokeSubsystem.h"
#include "Items/CCLInventoryPersistenceSubsystem.h"
#include "Session/CCLPlayerPersistenceSubsystem.h"

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
			|| !FCCLWorldSnapshotCodec::Capture(Runtime->GetIdentity(), Runtime->GetClock(), Agents->GetSimulation(), Actual, Error, &Runtime->GetEnvironmentInputs(), &Runtime->GetSurfaceSimulation(), &Runtime->GetEcologySimulation()))
		{
			return false;
		}

        FKSIWorldCaptureContext CaptureContext;
        CaptureContext.WorldId = Actual.Identity.WorldId;
        CaptureContext.WorldSeconds = Actual.Clock.WorldSeconds;
        auto* Players = World->GetSubsystem<UCCLPlayerPersistenceSubsystem>();
        auto* Inventories = World->GetSubsystem<UCCLInventoryPersistenceSubsystem>();
        TArray<uint8> PlayerSection, InventorySection;
        if (!Players || !Inventories || !Players->CaptureWorldSection(CaptureContext, PlayerSection, Error)
            || !Inventories->CaptureWorldSection(CaptureContext, InventorySection, Error))
        {
            return false;
        }
        Actual.SetSection(Players->GetWorldSection(), MoveTemp(PlayerSection));
        Actual.SetSection(Inventories->GetWorldSection(), MoveTemp(InventorySection));

        // A returning PlayerState may bind its saved account after opening a temporary one.
        // Permit only player accounts observed at creation, with every opening field unchanged.
        for (const auto& Pair : PlayerOpeningAccounts)
        {
            if (ExpectedLife.Economy.Accounts.Contains(Pair.Key))
            {
                continue;
            }
            const auto* Opened = ActualLife.Economy.Accounts.Find(Pair.Key);
            if (Opened)
            {
                if (!FCCLAccountRecord::StaticStruct()->CompareScriptStruct(&Pair.Value, Opened, PPF_None))
                {
                    Error = TEXT("new player account changed after its observed opening");
                    return false;
                }
                ActualLife.Economy.Accounts.Remove(Pair.Key);
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

            for (TFieldIterator<FProperty> Property(FCCLEconomyState::StaticStruct()); Property; ++Property)
            {
                if (!Property->Identical_InContainer(&ExpectedLife.Economy, &ActualLife.Economy, PPF_None))
                {
                    UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_TRAVEL economy difference: %s"), *Property->GetName());
                }
            }
            for (const auto& Pair : ExpectedLife.Economy.Accounts)
            {
                const auto* Found = ActualLife.Economy.Accounts.Find(Pair.Key);
                if (!Found || !FCCLAccountRecord::StaticStruct()->CompareScriptStruct(&Pair.Value, Found, PPF_None))
                {
                    UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_TRAVEL expected account %s balance=%lld opening=%lld actual=%lld/%lld"),
                        *Pair.Key.ToString(), Pair.Value.Balance, Pair.Value.OpeningBalance,
                        Found ? Found->Balance : -1, Found ? Found->OpeningBalance : -1);
                }
            }
            for (const auto& Pair : ActualLife.Economy.Accounts)
            {
                if (!ExpectedLife.Economy.Accounts.Contains(Pair.Key))
                {
                    UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_TRAVEL extra account %s balance=%lld opening=%lld"),
                        *Pair.Key.ToString(), Pair.Value.Balance, Pair.Value.OpeningBalance);
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
			Error = FString::Printf(TEXT("world differs after travel: schema=%u/%u sections=%d/%d surface=%d ecology=%d game=%.17g/%.17g world=%.17g/%.17g"),
				Saved.Schema, Actual.Schema, Saved.Sections.Num(), Actual.Sections.Num(),
				Saved.Surface == Actual.Surface, Saved.Ecology == Actual.Ecology,
				Saved.Clock.GameSeconds, Actual.Clock.GameSeconds, Saved.Clock.WorldSeconds, Actual.Clock.WorldSeconds);
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
            const auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
            if (Agents && Agents->IsRunning())
            {
                for (TActorIterator<APlayerState> It(World); It; ++It)
                {
                    const auto* Account = It->FindComponentByClass<UCCLAccountComponent>();
                    if (Account && Account->GetProvisionalAccountId().IsValid()
                        && !PlayerOpeningAccounts.Contains(Account->GetProvisionalAccountId()))
                    {
                        FCCLAccountRecord Opening;
                        Opening.Id = Account->GetProvisionalAccountId();
                        Opening.OwnerId = Opening.Id;
                        Opening.Balance = Account->GetProvisionalOpeningBalance();
                        Opening.OpeningBalance = Opening.Balance;
                        Opening.bHasOpeningBalance = 1;
                        PlayerOpeningAccounts.Add(Opening.Id, Opening);
                        UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_TRAVEL observed player opening %s balance=%lld"),
                            *Opening.Id.ToString(), Opening.Balance);
                    }
                }
            }
		}
	}
}

void UCCLTerrainTravelSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_TRAVEL %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	bComplete = 1;
	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 2);
}
