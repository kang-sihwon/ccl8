#include "CCLLoadoutRestoreSmokeSubsystem.h"
#include "Items/CCLInventoryFood.h"

#include "CCLPlayerState.h"
#include "Session/CCLPlayerPersistenceSubsystem.h"
#include "Session/CCLSessionRecord.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLExperimentDirector.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Agents/CCLAccountComponent.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLStaminaSet.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLWorldPickup.h"
#include "Items/CCLItemDefinition.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"

namespace
{
TArray<FCCLPlayerSnapshot> WorldProfiles(const FCCLWorldSnapshot& World)
{
	TArray<FCCLPlayerSnapshot> Profiles;
	FString Error;
	CCLPlayerSnapshot::ReadWorld(World, Profiles, Error);
	return Profiles;
}
} // namespace

bool UCCLLoadoutRestoreSmokeSubsystem::CheckProfiles(ACCLPlayerState& First)
{
	auto* W = GetWorld();
	auto* Store = W->GetSubsystem<UCCLPlayerPersistenceSubsystem>();
	auto* R = W->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Agents = W->GetSubsystem<UCCLAgentWorldSubsystem>();
	auto* Second = W->SpawnActor<ACCLPlayerState>();
	auto* Chest = W->SpawnActor<AActor>();
	if (!Store || !R || !Agents || !Second || !Chest)
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		Store->Unbind(First);
		if (IsValid(Second))
		{
			Second->Destroy();
		}
		Chest->Destroy();
	};
	FString Error;
	R->SetTickableTickType(ETickableTickType::Never);
	auto Check = [&](bool Good, const TCHAR* Message)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_LOADOUT_RESTORE %s player-profile %s: %s"),
			   Good ? TEXT("CHECK") : TEXT("FAIL"), Message, *Error);
		Error.Reset();
		return Good;
	};
	auto* A = First.GetCCLAbilitySystem();
	auto* B = Second->GetCCLAbilitySystem();
	B->InitAbilityActorInfo(Second, Second);
	auto* Box = NewObject<UCCLInventoryComponent>(Chest);
	Box->RegisterComponent();
	auto* Bread = FCCLSessionCodec::Item(TEXT("DA_FoodSample"));
	auto* Vitality = FCCLSessionCodec::Skill(TEXT("DA_VitalityTraining"));
	auto* Power = FCCLSessionCodec::Skill(TEXT("DA_PowerTraining"));
	auto* FirstAccount = First.FindComponentByClass<UCCLAccountComponent>();
	auto* SecondAccount = Second->FindComponentByClass<UCCLAccountComponent>();
	if (!Check(First.GetLoadout()->RestoreState({}, {}, {Vitality}, 7) &&
				   Second->GetLoadout()->RestoreState({}, {}, {Vitality, Power}, 9) && FirstAccount->ImportLegacy(79) &&
				   SecondAccount->ImportLegacy(83) &&
				   First.GetExpedition()->Restore(0, ECCLQuestStatus::Accepted, false) &&
				   Second->GetExpedition()->Restore(0, ECCLQuestStatus::Rewarded, false) &&
				   Box->BindPersistence(FGuid::NewGuid(), Error),
			   TEXT("seed progression and box")))
	{
		return false;
	}
	const auto FirstFood = First.GetInventory()->Add(Bread, 3);
	const auto GuestFood = Second->GetInventory()->Add(Bread, 2);
	const auto BoxFood = Box->Add(Bread, 4);
	auto* Pawn = First.GetPawn();
	auto* Drop = Pawn ? ACCLWorldPickup::DropFood(First.GetInventory(), FirstFood, 3,
												  FTransform(Pawn->GetActorLocation() + FVector(60, 0, 0)), Error)
					  : nullptr;
	if (!Check(FirstFood.IsValid() && GuestFood.IsValid() && BoxFood.IsValid() && Drop &&
				   Drop->TryCollect(First.GetInventory(), Pawn),
			   TEXT("actual food drop and collection")))
	{
		return false;
	}
	const auto Staff = First.GetInventory()->Add(FCCLSessionCodec::Item(TEXT("DA_TrainingStaff")), 1);
	if (!Check(Staff.IsValid() && First.GetLoadout()->Equip(Staff), TEXT("two-hand equipment before world save")))
	{
		return false;
	}
	A->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 62.5f);
	A->SetNumericAttributeBase(UCCLStaminaSet::GetStaminaAttribute(), 40.f);
	B->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 93.75f);
	B->SetNumericAttributeBase(UCCLStaminaSet::GetStaminaAttribute(), 25.f);
	if (!Check(Store->Bind(First, TEXT("server:first"), Error) && Store->Bind(*Second, TEXT("server:second"), Error),
			   TEXT("trusted keys register actual PlayerStates")))
	{
		return false;
	}
	if (!Check(!Store->Bind(*Second, TEXT("server:first"), Error), TEXT("duplicate identity refused")))
	{
		return false;
	}
	TArray<uint8> Saved;
	FCCLWorldSnapshot Snapshot;
	if (!Check(Agents->Save(Saved) && FCCLWorldSnapshotCodec::Decode(Saved, Snapshot, Error) &&
				   WorldProfiles(Snapshot).Num() == 2,
			   TEXT("world envelope captures player food and equipment")))
	{
		return false;
	}
	FString ProfileFile;
	if (FParse::Value(FCommandLine::Get(), TEXT("CCLPlayerProfileFile="), ProfileFile) &&
		!Check(FFileHelper::SaveArrayToFile(Saved, *ProfileFile), TEXT("write world profile file for fresh process")))
	{
		return false;
	}
	TArray<uint8> SessionWorld;
	FCCLWorldSnapshot SessionSnapshot;
	if (!Check(Agents->Save(SessionWorld, Error, First.GetInventory()) &&
				   FCCLWorldSnapshotCodec::Decode(SessionWorld, SessionSnapshot, Error) &&
				   WorldProfiles(SessionSnapshot).Num() == 1 &&
				   WorldProfiles(SessionSnapshot)[0].IdentityKey == TEXT("server:second"),
			   TEXT("outer session owner is excluded from world profiles")))
	{
		return false;
	}
	const auto FirstId = FirstAccount->GetAccountId();
	const auto GuestId = SecondAccount->GetAccountId();
	if (!Check(First.GetLoadout()->RestoreState({}, {}, {}, 17) && Second->GetLoadout()->RestoreState({}, {}, {}, 19) &&
				   FirstAccount->ImportLegacy(222) && SecondAccount->ImportLegacy(333) && Box->Remove(BoxFood, 3) &&
				   First.GetExpedition()->Restore(0, ECCLQuestStatus::Available, false) &&
				   Second->GetExpedition()->Restore(0, ECCLQuestStatus::Available, false),
			   TEXT("mutate every saved owner")))
	{
		return false;
	}
	A->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 20.f);
	B->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 30.f);
	A->SetNumericAttributeBase(UCCLStaminaSet::GetStaminaAttribute(), 10.f);
	B->SetNumericAttributeBase(UCCLStaminaSet::GetStaminaAttribute(), 15.f);
	int32 Notify = 0;
	bool Consistent = true;
	auto Restored = [&]
	{
		return First.GetLoadout()->GetPoints() == 7 && Second->GetLoadout()->GetPoints() == 9 &&
			   First.GetLoadout()->GetEquippedId() == Staff &&
			   First.GetLoadout()->GetEquippedId(CCLItemTags::Slot_LeftHand) == Staff &&
			   First.GetExpedition()->GetQuest() == ECCLQuestStatus::Accepted &&
			   Second->GetExpedition()->GetQuest() == ECCLQuestStatus::Rewarded &&
			   FirstAccount->GetAccountId() == FirstId && SecondAccount->GetAccountId() == GuestId &&
			   FirstAccount->GetBalance() == 79 && SecondAccount->GetBalance() == 83 &&
			   A->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 62.5f &&
			   B->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 93.75f &&
			   A->GetNumericAttribute(UCCLStaminaSet::GetStaminaAttribute()) == 40.f &&
			   B->GetNumericAttribute(UCCLStaminaSet::GetStaminaAttribute()) == 25.f && Box->Find(BoxFood) &&
			   Box->Find(BoxFood)->Quantity == 4 && Second->GetInventory()->Find(GuestFood) &&
			   Second->GetInventory()->Find(GuestFood)->Quantity == 2;
	};
	auto Listen = [&]
	{
		++Notify;
		Consistent &= Restored();
	};
	const auto N1 = First.GetInventory()->OnChanged.AddLambda(Listen);
	const auto N2 = Second->GetInventory()->OnChanged.AddLambda(Listen);
	const auto N3 = Box->OnChanged.AddLambda(Listen);
	ON_SCOPE_EXIT
	{
		First.GetInventory()->OnChanged.Remove(N1);
		if (IsValid(Second))
		{
			Second->GetInventory()->OnChanged.Remove(N2);
		}
		Box->OnChanged.Remove(N3);
	};
	const auto Epoch = R->GetEpoch();
	const auto OldA = A->GetActiveEffects(FGameplayEffectQuery());
	const auto OldB = B->GetActiveEffects(FGameplayEffectQuery());
	{
		int32 Attempt = 0;
		const auto Query = FGameplayEffectApplicationQuery::CreateLambda(
			[&](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec&) { return ++Attempt != 2; });
		B->GameplayEffectApplicationQueries.Add(Query);
		ON_SCOPE_EXIT
		{
			B->GameplayEffectApplicationQueries.RemoveAll([&](const auto& V)
														  { return V.GetHandle() == Query.GetHandle(); });
		};
		if (!Check(!Agents->Restore(Saved, Error) && Attempt == 2 && R->GetEpoch() == Epoch && Notify == 0 &&
					   FirstAccount->GetBalance() == 222 && SecondAccount->GetBalance() == 333 &&
					   First.GetInventory()->GetEntries().IsEmpty() && Second->GetInventory()->GetEntries().IsEmpty() &&
					   Box->Find(BoxFood)->Quantity == 1 && First.GetLoadout()->GetPoints() == 17 &&
					   A->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 20.f &&
					   B->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 30.f &&
					   A->GetActiveEffects(FGameplayEffectQuery()).Num() == OldA.Num() &&
					   B->GetActiveEffects(FGameplayEffectQuery()).Num() == OldB.Num(),
				   TEXT("GAS refusal preserves world, accounts, box and both live profiles")))
		{
			return false;
		}
	}
	if (!Check(Agents->Restore(Saved, Error) && Restored() && Notify == 3 && Consistent,
			   TEXT("box and player listeners observe all final profile values")))
	{
		return false;
	}
	First.GetInventory()->OnChanged.Remove(N1);
	Second->GetInventory()->OnChanged.Remove(N2);
	Box->OnChanged.Remove(N3);
	TArray<uint8> After;
	FCCLWorldSnapshot Actual;
	TArray<uint8> ExpectedPlayers, ActualPlayers;
	if (!Check(Agents->Save(After) && FCCLWorldSnapshotCodec::Decode(After, Actual, Error) &&
				   CCLPlayerSnapshot::Encode(WorldProfiles(Snapshot), Snapshot.Identity.WorldId, Snapshot.Clock.WorldSeconds,
											 ExpectedPlayers, Error) &&
				   CCLPlayerSnapshot::Encode(WorldProfiles(Actual), Actual.Identity.WorldId, Actual.Clock.WorldSeconds,
											 ActualPlayers, Error) &&
				   ExpectedPlayers == ActualPlayers,
			   TEXT("actual restored player body matches saved food and all fields")))
	{
		return false;
	}
	// A bound player missing from the saved world joined later and keeps the current profile.
	auto Missing = Snapshot;
	auto MissingProfiles = WorldProfiles(Snapshot);
	MissingProfiles.Pop();
	TArray<uint8> MissingBytes;
	TArray<FCCLPlayerSnapshot> KeptBefore, KeptAfter;
	TArray<UCCLInventoryComponent*> KeptOwners;
	TArray<uint8> KeptBeforeBody, KeptAfterBody;
	if (!Check(CCLPlayerSnapshot::WriteWorld(Missing, MissingProfiles, Error) &&
				   FCCLWorldSnapshotCodec::Encode(Missing, MissingBytes, Error) &&
				   Store->Capture(R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), KeptBefore, KeptOwners, Error) &&
				   Agents->Restore(MissingBytes, Error) &&
				   Store->Capture(R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), KeptAfter, KeptOwners, Error) &&
				   CCLPlayerSnapshot::Encode(KeptBefore, R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), KeptBeforeBody, Error) &&
				   CCLPlayerSnapshot::Encode(KeptAfter, R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), KeptAfterBody, Error) &&
				   KeptBeforeBody == KeptAfterBody,
			   TEXT("player missing from the saved world keeps the current profile")))
	{
		return false;
	}
	Store->Unbind(*Second);
	Second->Destroy();
	Second = W->SpawnActor<ACCLPlayerState>();
	B = Second->GetCCLAbilitySystem();
	B->InitAbilityActorInfo(Second, Second);
	SecondAccount = Second->FindComponentByClass<UCCLAccountComponent>();
	if (!Check(Store->Bind(*Second, TEXT("server:second"), Error) && Restored(),
			   TEXT("unloaded record rebinds inventory, old account and injury to a new PlayerState")))
	{
		return false;
	}
	FCCLFoodEnvironmentBinding Weather;
	Weather.Source = ECCLFoodEnvironment::WeatherRegion;
	Weather.SourceId = ACCLExperimentDirector::Find(W)->WeatherRegionId();
	if (!Check(FCCLInventoryFood::ChangeFoodEnvironment(*First.GetInventory(), Weather, Error) &&
				   FCCLInventoryFood::ChangeFoodEnvironment(*Second->GetInventory(), Weather, Error),
			   TEXT("dynamic player food binding")))
	{
		return false;
	}
	Store->Unbind(*Second);
	Second->Destroy();
	Second = nullptr;
	return true;
}

bool UCCLLoadoutRestoreSmokeSubsystem::CheckPlayerFood(ACCLPlayerState& First)
{
	bProfileRetry = 0;
	auto* R = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Store = GetWorld()->GetSubsystem<UCCLPlayerPersistenceSubsystem>();
	FString Error;
	auto Check = [&](bool Good, const TCHAR* Message)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_LOADOUT_RESTORE %s player-food %s: %s"), Good ? TEXT("CHECK") : TEXT("FAIL"),
			   Message, *Error);
		Error.Reset();
		return Good;
	};
	TArray<FCCLPlayerSnapshot> Profiles;
	TArray<UCCLInventoryComponent*> Owners;
	if (!FoodStep)
	{
		if (!Check(Store->Bind(First, TEXT("server:first"), Error) && !R->GetClock().HasPendingTime() &&
					   R->ChangeTimeScale(60., Error),
				   TEXT("bind live owner and sixty-fold time scale")))
		{
			return false;
		}
		FoodStartTime = R->GetClock().GetWorldSeconds();
		TArray<uint8> Before, After;
		if (!Check(Store->Capture(R->GetIdentity().WorldId, FoodStartTime, Profiles, Owners, Error) &&
					   CCLPlayerSnapshot::Encode(Profiles, R->GetIdentity().WorldId, FoodStartTime, Before, Error) &&
					   R->QueueGameTime(.25, Error) && !R->AdvancePending(.25, 1., Error, 0) &&
					   R->GetClock().GetWorldSeconds() == FoodStartTime &&
					   Store->Capture(R->GetIdentity().WorldId, FoodStartTime, Profiles, Owners, Error) &&
					   CCLPlayerSnapshot::Encode(Profiles, R->GetIdentity().WorldId, FoodStartTime, After, Error) &&
					   Before == After,
				   TEXT("failed world advance preserves live and unloaded player food")))
		{
			return false;
		}
		FoodStep = 1;
		bProfileRetry = 1;
		return false;
	}
	if (R->GetClock().HasPendingTime())
	{
		if (!R->AdvancePending(.25, 1., Error))
		{
			if (Error.Contains(TEXT("awaits")))
			{
				bProfileRetry = 1;
				return false;
			}
			return Check(false, TEXT("retry same pending food interval"));
		}
		if (R->GetClock().HasPendingTime())
		{
			bProfileRetry = 1;
			return false;
		}
	}
	if (!Check(Store->Capture(R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), Profiles, Owners, Error),
			   TEXT("capture live and unloaded dynamic owners")))
	{
		return false;
	}
	for (const auto& Profile : Profiles)
	{
		for (const auto& Item : Profile.Items)
		{
			FCCLFoodState Food;
			if (!CCLFood::Decode(Item.Food, Food, Error) ||
				(Food.IsPresent() && Food.UpdatedWorldSeconds != R->GetClock().GetWorldSeconds()))
			{
				return Check(false, TEXT("all dynamic player food reaches committed time"));
			}
		}
	}
	Store->Unbind(First);
	return Check(R->GetClock().GetWorldSeconds() == FoodStartTime + 15. && Profiles.Num() == 2,
				 TEXT("live and offline food share committed world time"));
}

void UCCLLoadoutRestoreSmokeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (!FParse::Param(FCommandLine::Get(), TEXT("CCLPlayerProfileRead")))
	{
		return;
	}
	auto* Store = Collection.InitializeDependency<UCCLAgentSessionStore>();
	FString Path, Error;
	FCCLWorldSnapshot Snapshot;
	if (!Store || !FParse::Value(FCommandLine::Get(), TEXT("CCLPlayerProfileFile="), Path) ||
		!FFileHelper::LoadFileToArray(ReaderWorld, *Path) ||
		!FCCLWorldSnapshotCodec::Decode(ReaderWorld, Snapshot, Error))
	{
		Finish(false, TEXT("read world profile fixture before world start"));
		return;
	}
	Store->SnapshotFor(Snapshot.Identity.Domain) = ReaderWorld;
}

bool UCCLLoadoutRestoreSmokeSubsystem::CheckFreshProfiles(ACCLPlayerState& First)
{
	FString Error;
	auto* Store = GetWorld()->GetSubsystem<UCCLPlayerPersistenceSubsystem>();
	auto* R = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	FCCLWorldSnapshot Saved;
	TArray<FCCLPlayerSnapshot> Actual;
	TArray<UCCLInventoryComponent*> Owners;
	TArray<uint8> ExpectedBody, ActualBody;
	if (!FCCLWorldSnapshotCodec::Decode(ReaderWorld, Saved, Error) || !R->IsRunning() ||
		R->GetIdentity().WorldId != Saved.Identity.WorldId ||
		!Store->Capture(R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), Actual, Owners, Error) ||
		!CCLPlayerSnapshot::Encode(WorldProfiles(Saved), Saved.Identity.WorldId, Saved.Clock.WorldSeconds, ExpectedBody,
								   Error) ||
		!CCLPlayerSnapshot::Encode(Actual, R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), ActualBody,
								   Error) ||
		ExpectedBody != ActualBody || !Owners.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_LOADOUT_RESTORE fresh world records: %s"), *Error);
		return false;
	}
	auto* Second = GetWorld()->SpawnActor<ACCLPlayerState>();
	if (!Second)
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		Store->Unbind(First);
		Second->Destroy();
	};
	Second->GetCCLAbilitySystem()->InitAbilityActorInfo(Second, Second);
	if (!Store->Bind(First, TEXT("server:first"), Error) || !Store->Bind(*Second, TEXT("server:second"), Error) ||
		!Store->Capture(R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), Actual, Owners, Error) ||
		!CCLPlayerSnapshot::Encode(Actual, R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), ActualBody,
								   Error) ||
		ExpectedBody != ActualBody || Owners.Num() != 2 ||
		First.FindComponentByClass<UCCLAccountComponent>()->GetBalance() != 79 ||
		Second->FindComponentByClass<UCCLAccountComponent>()->GetBalance() != 83)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_LOADOUT_RESTORE fresh bound profiles: %s"), *Error);
		return false;
	}
	UE_LOG(LogTemp, Display,
		   TEXT("CCL_LOADOUT_RESTORE CHECK fresh world start preserves profiles and rebinds both old accounts"));
	return true;
}
