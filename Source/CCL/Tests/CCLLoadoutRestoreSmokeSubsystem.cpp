#include "CCLLoadoutRestoreSmokeSubsystem.h"

#include "CCLPlayerState.h"
#include "Actions/CCLActionComponent.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLEffects.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLOffenseSet.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLLoadoutRestore.h"
#include "GameplayEffectAggregator.h"
#include "Items/CCLSkillDefinition.h"
#include "Session/CCLSessionRecord.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"

bool UCCLLoadoutRestoreSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLLoadoutRestoreSmoke"));
#endif
}

void UCCLLoadoutRestoreSmokeSubsystem::Finish(bool bSuccess, const TCHAR* Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_LOADOUT_RESTORE %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), Message);
	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
}

TStatId UCCLLoadoutRestoreSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLLoadoutRestoreSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLLoadoutRestoreSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}
	if (Now - Started > 120.)
	{
		Finish(false, TEXT("startup timeout"));
		return;
	}
	auto* W = GetWorld();
	auto* PC = W ? W->GetFirstPlayerController() : nullptr;
	auto* Player = PC ? PC->GetPlayerState<ACCLPlayerState>() : nullptr;
	auto* Director = W ? ACCLExperimentDirector::Find(W) : nullptr;
	auto* ASC = Player ? Cast<UCCLAbilitySystemComponent>(Player->GetAbilitySystemComponent()) : nullptr;
	if (!Player || !Player->HasAuthority() || !ASC || !ASC->GetAvatarActor() || !Director || !Director->IsReady())
	{
		return;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CCLPlayerProfileRead")))
	{
		Finish(CheckFreshProfiles(*Player), TEXT("fresh world start and player profile binding"));
		return;
	}
	if (bBaseComplete)
	{
		if (!bProfilesComplete)
		{
			auto* R = W->GetSubsystem<UCCLWorldSimulationSubsystem>();
			FString Error;
			if (R->GetClock().HasPendingTime())
			{
				if (!R->AdvancePending(.25, 15., Error) && !Error.Contains(TEXT("awaits")))
				{
					UE_LOG(LogTemp, Error, TEXT("CCL_LOADOUT_RESTORE pending time: %s"), *Error);
					Finish(false, TEXT("finish accepted time before profile test"));
				}
				return;
			}
			if (!CheckProfiles(*Player))
			{
				Finish(false, TEXT("world player profile integration"));
				return;
			}
			bProfilesComplete = 1;
			return;
		}
		const bool bFoodPassed = CheckPlayerFood(*Player);
		if (!bProfileRetry)
		{
			Finish(bFoodPassed, TEXT("loadout, world player profiles and dynamic live/offline food"));
		}
		return;
	}
	auto* Inventory = Player->GetInventory();
	auto* Loadout = Player->GetLoadout();
	auto Check = [&](bool Good, const TCHAR* Message)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_LOADOUT_RESTORE %s %s"), Good ? TEXT("CHECK") : TEXT("FAIL"), Message);
		if (!Good)
		{
			Finish(false, Message);
		}
		return Good;
	};
	TArray<FKSIInventoryEntry> Original;
	auto Add = [](TArray<FKSIInventoryEntry>& Entries, const TCHAR* Definition, int32 Slot)
	{
		auto& Entry = Entries.AddDefaulted_GetRef();
		Entry.Id = FGuid::NewGuid();
		Entry.Definition = FCCLSessionCodec::Item(Definition);
		Entry.Quantity = 1;
		Entry.Slot = Slot;
		return Entry.Id;
	};
	auto EquipmentSlot = [](FGuid Id, FGameplayTag Slot)
	{
		FCCLEquippedSlot Value;
		Value.Id = Id;
		Value.Slot = Slot;
		return Value;
	};
	for (int32 Slot = 0; Slot < Inventory->Capacity; ++Slot)
	{
		Add(Original, TEXT("DA_RecoveryPotion"), Slot);
	}
	const auto Gauntlets = Add(Original, TEXT("DA_IronGauntlets"), INDEX_NONE);
	TArray<FCCLEquippedSlot> OriginalEquipment = {EquipmentSlot(Gauntlets, CCLItemTags::Slot_RightHand)};
	auto* Power = FCCLSessionCodec::Skill(TEXT("DA_PowerTraining"));
	auto* Vitality = FCCLSessionCodec::Skill(TEXT("DA_VitalityTraining"));
	TArray<UCCLSkillDefinition*> Skills = {Power, Vitality};
	if (!Check(Power && Vitality && Loadout->RestoreState(Original, OriginalEquipment, Skills, 7),
			   TEXT("seed full bag, equipment and both skills")))
	{
		return;
	}
	ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 120.f);
	const auto OldHandles = ASC->GetActiveEffects(FGameplayEffectQuery());
	const float OldAttack = ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute());
	const float OldMax = ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute());
	int32 Notifications = 0;
	bool bNotificationConsistent = true;
	FGuid ExpectedEquipment = Gauntlets;
	int32 ExpectedPoints = 7;
	const auto Changed = Inventory->OnChanged.AddLambda(
		[&]()
		{
			++Notifications;
			bNotificationConsistent &= Loadout->GetEquippedId() == ExpectedEquipment &&
									   Loadout->GetPoints() == ExpectedPoints && Inventory->Find(ExpectedEquipment) &&
									   Inventory->Find(ExpectedEquipment)->Slot == INDEX_NONE;
		});
	ON_SCOPE_EXIT
	{
		Inventory->OnChanged.Remove(Changed);
	};
	auto Preserved = [&]()
	{
		if (Notifications || Inventory->IsTransactionOpen() || Loadout->GetPoints() != 7 ||
			!Loadout->IsLearned(Power) || !Loadout->IsLearned(Vitality) || Loadout->GetEquippedId() != Gauntlets ||
			Inventory->GetEntries().Num() != Original.Num() ||
			ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) != 120.f ||
			ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()) != OldMax ||
			ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()) != OldAttack ||
			ASC->GetActiveEffects(FGameplayEffectQuery()).Num() != OldHandles.Num())
		{
			return false;
		}
		for (const auto Handle : OldHandles)
		{
			if (!ASC->GetActiveGameplayEffect(Handle))
			{
				return false;
			}
		}
		for (const auto& Entry : Original)
		{
			const auto* Actual = Inventory->Find(Entry.Id);
			if (!Actual || Actual->Definition != Entry.Definition || Actual->Slot != Entry.Slot ||
				Actual->Quantity != Entry.Quantity)
			{
				return false;
			}
		}
		return true;
	};
	TArray<FKSIInventoryEntry> Candidate = Original;
	Candidate.Pop();
	const FGuid Staff = Add(Candidate, TEXT("DA_TrainingStaff"), INDEX_NONE);
	TArray<FCCLEquippedSlot> Equipment = {EquipmentSlot(Staff, CCLItemTags::Slot_LeftHand),
										  EquipmentSlot(Staff, CCLItemTags::Slot_RightHand)};
	auto Reject = [&](const TArray<FKSIInventoryEntry>& Items, const TArray<FCCLEquippedSlot>& Slots,
					  const TArray<UCCLSkillDefinition*>& Learned, int32 Points, const TCHAR* Message)
	{ return Check(!Loadout->RestoreState(Items, Slots, Learned, Points) && Preserved(), Message); };
	auto BrokenSlots = Equipment;
	BrokenSlots.Pop();
	if (!Reject(Candidate, BrokenSlots, Skills, 7, TEXT("partial two-hand occupancy preserves original")))
	{
		return;
	}
	BrokenSlots = Equipment;
	BrokenSlots.Add(Equipment[0]);
	if (!Reject(Candidate, BrokenSlots, Skills, 7, TEXT("duplicate equipment slot preserves original")))
	{
		return;
	}
	BrokenSlots = Equipment;
	BrokenSlots[1].Id = FGuid::NewGuid();
	if (!Reject(Candidate, BrokenSlots, Skills, 7, TEXT("missing item reference preserves original")))
	{
		return;
	}
	auto BrokenItems = Candidate;
	BrokenItems.Last().Slot = 0;
	if (!Reject(BrokenItems, Equipment, Skills, 7, TEXT("bag and equipment collision preserves original")))
	{
		return;
	}
	BrokenItems = Candidate;
	Add(BrokenItems, TEXT("DA_TrainingArmor"), INDEX_NONE);
	if (!Reject(BrokenItems, Equipment, Skills, 7, TEXT("orphan equipped item preserves original")))
	{
		return;
	}
	if (!Reject(Candidate, Equipment, {Power, Power}, 7, TEXT("duplicate skill preserves original")) ||
		!Reject(Candidate, Equipment, {Power, nullptr}, 7, TEXT("missing last skill preserves original")) ||
		!Reject(Candidate, Equipment, Skills, 1001, TEXT("invalid points preserves original")))
	{
		return;
	}
	{
		TGuardValue<TSubclassOf<UGameplayEffect>> InvalidEffect(Vitality->Effect,
																UCCLStaminaRegenEffect::StaticClass());
		if (!Reject(Candidate, Equipment, Skills, 7, TEXT("periodic last skill rejected before mutation")))
		{
			return;
		}
	}
	if (!Check(Inventory->BeginTransaction(), TEXT("hold inventory transaction")))
	{
		return;
	}
	const bool LockedRejected = !Loadout->RestoreState(Candidate, Equipment, Skills, 7);
	Inventory->EndTransaction(false);
	if (!Check(LockedRejected && Preserved(), TEXT("busy inventory rejected and original preserved")))
	{
		return;
	}
	int32 ApplyCalls = 0;
	bool bReentryRejected = false;
	{
		FGameplayEffectApplicationQuery Query = FGameplayEffectApplicationQuery::CreateLambda(
			[&](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec&)
			{
				++ApplyCalls;
				bReentryRejected = !Loadout->RestoreState(Candidate, Equipment, Skills, 7) && !Loadout->Learn(Power);
				return ApplyCalls != 2;
			});
		const auto QueryHandle = Query.GetHandle();
		ASC->GameplayEffectApplicationQueries.Add(Query);
		ON_SCOPE_EXIT
		{
			ASC->GameplayEffectApplicationQueries.RemoveAll([&](const auto& Value)
															{ return Value.GetHandle() == QueryHandle; });
		};
		if (!Check(!Loadout->RestoreState(Candidate, Equipment, Skills, 7) && ApplyCalls == 2 && bReentryRejected &&
					   Preserved(),
				   TEXT("second GAS application failure removes new effects and retains old handles and injury")))
		{
			return;
		}
	}
	ExpectedEquipment = Staff;
	ExpectedPoints = 3;
	if (!Check(Loadout->RestoreState(Candidate, Equipment, Skills, 3, false) && Notifications == 1 &&
				   bNotificationConsistent && Loadout->GetEquippedId(CCLItemTags::Slot_LeftHand) == Staff &&
				   ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 120.f &&
				   ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()) == 125.f &&
				   ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()) == 5.f &&
				   !Inventory->Find(Gauntlets),
			   TEXT("full bag swaps to two-hand staff with one consistent notification and no duplicated bonuses")))
	{
		return;
	}
	if (!Check(Loadout->Restore(Loadout->GetEquipment(), Skills, 3) && Notifications == 2 && bNotificationConsistent &&
				   ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 125.f &&
				   ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()) == 5.f,
			   TEXT("aliased repeat restore preserves slot arrays and checkpoint full-health policy")))
	{
		return;
	}
	Inventory->OnChanged.Remove(Changed);
	if (!Check(Loadout->RestoreState({}, {}, {}, 0, false) && Inventory->GetEntries().IsEmpty() &&
				   Loadout->GetEquipment().IsEmpty() && !Loadout->IsLearned(Power) && !Loadout->IsLearned(Vitality) &&
				   ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 100.f &&
				   ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()) == 100.f &&
				   ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()) == 0.f,
			   TEXT("empty profile removes tracked effects and clamps health to final maximum")))
	{
		return;
	}
	TArray<FKSIInventoryEntry> WeaponItems;
	const auto Pistol = Add(WeaponItems, TEXT("DA_Pistol"), INDEX_NONE);
	TArray<FCCLEquippedSlot> WeaponSlots = {EquipmentSlot(Pistol, CCLItemTags::Slot_RightHand)};
	auto* Actions = Player->FindComponentByClass<UCCLActionComponent>();
	if (!Check(Actions && Loadout->RestoreState(WeaponItems, WeaponSlots, {}, 0), TEXT("seed empty pistol profile")))
	{
		return;
	}
	Loadout->RequestCustomHandAction(CCLItemTags::Slot_RightHand, true);
	if (!Check(Actions->HasSource(Pistol), TEXT("weapon use registers source grants")))
	{
		return;
	}
	WeaponItems[0].Definition = FCCLSessionCodec::Item(TEXT("DA_TrainingStaff"));
	WeaponSlots.Add(EquipmentSlot(Pistol, CCLItemTags::Slot_LeftHand));
	if (!Check(Loadout->RestoreState(WeaponItems, WeaponSlots, {}, 0) && !Actions->HasSource(Pistol),
			   TEXT("same item ID with restored definition clears prior weapon grants")))
	{
		return;
	}
	if (!CheckBatch(*Player))
	{
		Finish(false, TEXT("multi-player prepare, world failure or publication boundary"));
		return;
	}
	bBaseComplete = 1;
	W->GetSubsystem<UCCLWorldSimulationSubsystem>()->SetTickableTickType(ETickableTickType::Never);
}

bool UCCLLoadoutRestoreSmokeSubsystem::CheckBatch(ACCLPlayerState& First)
{
	auto* Second = GetWorld()->SpawnActor<ACCLPlayerState>();
	if (!Second)
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		Second->Destroy();
	};
	auto* FirstASC = Cast<UCCLAbilitySystemComponent>(First.GetAbilitySystemComponent());
	auto* SecondASC = Cast<UCCLAbilitySystemComponent>(Second->GetAbilitySystemComponent());
	SecondASC->InitAbilityActorInfo(Second, Second);
	auto* Vitality = FCCLSessionCodec::Skill(TEXT("DA_VitalityTraining"));
	auto* Power = FCCLSessionCodec::Skill(TEXT("DA_PowerTraining"));
	auto Check = [](bool Good, const TCHAR* Message)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_LOADOUT_RESTORE %s %s"), Good ? TEXT("CHECK") : TEXT("FAIL"), Message);
		return Good;
	};
	auto MakeRequest = [](UCCLLoadoutComponent* Loadout, const TCHAR* Definition, int32 Points)
	{
		FCCLLoadoutRestoreRequest Request;
		Request.Loadout = Loadout;
		Request.Points = Points;
		auto& Entry = Request.Entries.AddDefaulted_GetRef();
		Entry.Id = FGuid::NewGuid();
		Entry.Definition = FCCLSessionCodec::Item(Definition);
		Entry.Quantity = 1;
		Entry.Slot = 0;
		return Request;
	};
	TArray<FCCLLoadoutRestoreRequest> Originals;
	Originals.Add(MakeRequest(First.GetLoadout(), TEXT("DA_RecoveryPotion"), 7));
	Originals.Add(MakeRequest(Second->GetLoadout(), TEXT("DA_RecoveryPotion"), 9));
	Originals[0].Skills = {Vitality};
	if (!Check(FCCLLoadoutRestoreBatch::Execute(
				   Originals, [] { return true; }, [] {}),
			   TEXT("seed two live PlayerStates in one transaction")))
	{
		return false;
	}
	FirstASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 120.f);
	SecondASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 80.f);
	const auto FirstHandles = FirstASC->GetActiveEffects(FGameplayEffectQuery());
	const auto SecondHandles = SecondASC->GetActiveEffects(FGameplayEffectQuery());
	auto* WorldState = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	TArray<uint8> WorldBytes;
	FString WorldError;
	if (!Check(WorldState && Agents && WorldState->Save(Agents->GetSimulation(), WorldBytes, WorldError),
			   TEXT("capture actual world for prepared-player restore")))
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_LOADOUT_RESTORE world capture: %s"), *WorldError);
		return false;
	}
	const FGuid OriginalEpoch = WorldState->GetEpoch();
	const double OriginalTime = WorldState->GetClock().GetWorldSeconds();
	TArray<uint8> DamagedWorld = WorldBytes;
	DamagedWorld.Last() ^= 1;
	TArray<FCCLLoadoutRestoreRequest> Requests;
	Requests.Add(MakeRequest(First.GetLoadout(), TEXT("DA_RecoveryPotion"), 3));
	Requests.Add(MakeRequest(Second->GetLoadout(), TEXT("DA_RecoveryPotion"), 4));
	Requests[0].Skills = {Power};
	Requests[1].Skills = {Vitality, Power};
	int32 WorldCalls = 0;
	int32 FinalizeCalls = 0;
	int32 Notifications = 0;
	bool bConsistent = true;
	auto FinalState = [&]
	{
		return First.GetLoadout()->GetPoints() == 3 && Second->GetLoadout()->GetPoints() == 4 &&
			   First.GetInventory()->Find(Requests[0].Entries[0].Id) &&
			   Second->GetInventory()->Find(Requests[1].Entries[0].Id) &&
			   FirstASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()) == 100.f &&
			   SecondASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()) == 125.f &&
			   FirstASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 50.f &&
			   SecondASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 62.5f;
	};
	auto Listen = [&]
	{
		++Notifications;
		bConsistent &= FinalizeCalls == 1 && FinalState() && !First.GetLoadout()->RestoreState({}, {}, {}, 0) &&
					   !Second->GetLoadout()->RestoreState({}, {}, {}, 0);
	};
	const auto FirstChanged = First.GetInventory()->OnChanged.AddLambda(Listen);
	const auto SecondChanged = Second->GetInventory()->OnChanged.AddLambda(Listen);
	ON_SCOPE_EXIT
	{
		First.GetInventory()->OnChanged.Remove(FirstChanged);
		Second->GetInventory()->OnChanged.Remove(SecondChanged);
	};
	auto Preserved = [&]
	{
		if (Notifications || FinalizeCalls || WorldState->GetEpoch() != OriginalEpoch ||
			WorldState->GetClock().GetWorldSeconds() != OriginalTime || First.GetInventory()->IsTransactionOpen() ||
			Second->GetInventory()->IsTransactionOpen() || First.GetLoadout()->GetPoints() != 7 ||
			Second->GetLoadout()->GetPoints() != 9 || !First.GetInventory()->Find(Originals[0].Entries[0].Id) ||
			!Second->GetInventory()->Find(Originals[1].Entries[0].Id) ||
			FirstASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) != 120.f ||
			SecondASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) != 80.f ||
			FirstASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()) != 125.f ||
			SecondASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()) != 100.f ||
			FirstASC->GetActiveEffects(FGameplayEffectQuery()).Num() != FirstHandles.Num() ||
			SecondASC->GetActiveEffects(FGameplayEffectQuery()).Num() != SecondHandles.Num())
		{
			return false;
		}
		for (auto Handle : FirstHandles)
		{
			if (!FirstASC->GetActiveGameplayEffect(Handle))
			{
				return false;
			}
		}
		for (auto Handle : SecondHandles)
		{
			if (!SecondASC->GetActiveGameplayEffect(Handle))
			{
				return false;
			}
		}
		return true;
	};
	auto Commit = [&]
	{
		++WorldCalls;
		return Agents->Restore(WorldBytes, WorldError);
	};
	auto Finalize = [&]
	{
		++FinalizeCalls;
		bConsistent &= FScopedAggregatorOnDirtyBatch::GlobalBatchCount == 0 &&
					   FirstASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()) == 100.f &&
					   SecondASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()) == 125.f &&
					   FirstASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 100.f &&
					   SecondASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 125.f;
		FirstASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 50.f);
		SecondASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 62.5f);
	};
	auto Duplicates = Requests;
	Duplicates.Add(Requests[0]);
	if (!Check(!FCCLLoadoutRestoreBatch::Execute(Duplicates, Commit, Finalize) && WorldCalls == 0 && Preserved(),
			   TEXT("duplicate participant rejected without world mutation")))
	{
		return false;
	}
	Duplicates = Requests;
	Duplicates[1].Entries[0].Id = Duplicates[0].Entries[0].Id;
	if (!Check(!FCCLLoadoutRestoreBatch::Execute(Duplicates, Commit, Finalize) && WorldCalls == 0 && Preserved(),
			   TEXT("duplicate item owner rejected across participants")))
	{
		return false;
	}
	{
		FScopedAggregatorOnDirtyBatch OuterBatch;
		if (!Check(!FCCLLoadoutRestoreBatch::Execute(Requests, Commit, Finalize) && WorldCalls == 0 && Preserved(),
				   TEXT("outer GAS batch rejected before stale maximum attributes can be used")))
		{
			return false;
		}
	}
	{
		int32 Attempts = 0;
		bool bBothLocked = true;
		const auto Query = FGameplayEffectApplicationQuery::CreateLambda(
			[&](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec&)
			{
				bBothLocked &= First.GetInventory()->IsTransactionOpen() &&
							   Second->GetInventory()->IsTransactionOpen() &&
							   !First.GetLoadout()->RestoreState({}, {}, {}, 0);
				return ++Attempts != 2;
			});
		SecondASC->GameplayEffectApplicationQueries.Add(Query);
		ON_SCOPE_EXIT
		{
			SecondASC->GameplayEffectApplicationQueries.RemoveAll([&](const auto& Value)
																  { return Value.GetHandle() == Query.GetHandle(); });
		};
		if (!Check(!FCCLLoadoutRestoreBatch::Execute(Requests, Commit, Finalize) && Attempts == 2 && bBothLocked &&
					   WorldCalls == 0 && Preserved(),
				   TEXT("late second-player GAS rejection preserves both old profiles and handles")))
		{
			return false;
		}
	}
	auto RejectWorld = [&]
	{
		++WorldCalls;
		bConsistent &= First.GetInventory()->Find(Originals[0].Entries[0].Id) &&
					   Second->GetInventory()->Find(Originals[1].Entries[0].Id) &&
					   FirstASC->GetActiveEffects(FGameplayEffectQuery()).Num() == FirstHandles.Num() + 1 &&
					   SecondASC->GetActiveEffects(FGameplayEffectQuery()).Num() == SecondHandles.Num() + 2;
		return Agents->Restore(DamagedWorld, WorldError);
	};
	if (!Check(!FCCLLoadoutRestoreBatch::Execute(Requests, RejectWorld, Finalize) && WorldCalls == 1 && bConsistent &&
				   Preserved(),
			   TEXT("world rejection rolls back all prepared player effects")))
	{
		return false;
	}
	if (!Check(FCCLLoadoutRestoreBatch::Execute(Requests, Commit, Finalize) && WorldCalls == 2 && FinalizeCalls == 1 &&
				   Notifications == 2 && bConsistent && FinalState() && WorldState->GetEpoch() != OriginalEpoch &&
				   WorldState->GetClock().GetWorldSeconds() == OriginalTime &&
				   !First.GetInventory()->IsTransactionOpen() &&
				   !Second->GetInventory()->IsTransactionOpen(),
			   TEXT("both profiles and final maximum health precede finalization and both inventory notifications")))
	{
		return false;
	}
	First.GetInventory()->OnChanged.Remove(FirstChanged);
	Second->GetInventory()->OnChanged.Remove(SecondChanged);
	return Check(First.GetInventory()->Restore(First.GetInventory()->GetEntries()) &&
					 First.GetInventory()->Find(Requests[0].Entries[0].Id),
				 TEXT("inventory self-alias restore preserves its entries"));
}
