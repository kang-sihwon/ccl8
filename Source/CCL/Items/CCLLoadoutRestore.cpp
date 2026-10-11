#include "CCLLoadoutRestore.h"

#include "CCLLoadoutComponent.h"

#include "CCLInventoryComponent.h"
#include "CCLItemDefinition.h"
#include "CCLSkillDefinition.h"
#include "KSIActionComponent.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLStaminaSet.h"
#include "GameplayEffectAggregator.h"
#include "GameplayEffectComponent.h"
#include "Misc/ScopeExit.h"

namespace
{
// Restore supports passive numerical effects. Executions, components, cues,
// periodic effects and stacking require their own reversible participant.
bool IsRestorableEffect(TSubclassOf<UGameplayEffect> Class, float Magnitude)
{
	if (!Class || !FMath::IsFinite(Magnitude))
	{
		return false;
	}
	const auto* Effect = Class->GetDefaultObject<UGameplayEffect>();
	if (Effect->DurationPolicy != EGameplayEffectDurationType::Infinite || Effect->Period.GetValueAtLevel(1.f) != 0.f ||
		Effect->GetStackingType() != EGameplayEffectStackingType::None || !Effect->Executions.IsEmpty() ||
		!Effect->GameplayCues.IsEmpty() || Effect->FindComponent<UGameplayEffectComponent>())
	{
		return false;
	}
	for (const auto& Modifier : Effect->Modifiers)
	{
		const auto Calculation = Modifier.ModifierMagnitude.GetMagnitudeCalculationType();
		if (!Modifier.Attribute.IsValid() || Modifier.ModifierOp != EGameplayModOp::Additive ||
			Modifier.Attribute == UCCLHealthSet::GetHealthAttribute() ||
			Modifier.Attribute == UCCLStaminaSet::GetStaminaAttribute() ||
			(Calculation != EGameplayEffectMagnitudeCalculation::ScalableFloat &&
			 Calculation != EGameplayEffectMagnitudeCalculation::SetByCaller))
		{
			return false;
		}
	}
	return true;
}

bool MakeRestoreSpec(UCCLAbilitySystemComponent& ASC, TSubclassOf<UGameplayEffect> Class, float Magnitude,
					 FGameplayEffectSpecHandle& Result)
{
	if (!IsRestorableEffect(Class, Magnitude))
	{
		return false;
	}
	auto Spec = ASC.MakeOutgoingSpec(Class, 1.f, ASC.MakeEffectContext());
	if (!Spec.IsValid())
	{
		return false;
	}
	Spec.Data->SetSetByCallerMagnitude(CCLTags::Data_Magnitude, Magnitude);
	for (const auto& Modifier : Spec.Data->Def->Modifiers)
	{
		float Value = 0.f;
		if (!ASC.HasAttributeSetForAttribute(Modifier.Attribute) ||
			!Modifier.ModifierMagnitude.AttemptCalculateMagnitude(*Spec.Data, Value, false) || !FMath::IsFinite(Value))
		{
			return false;
		}
	}
	Result = MoveTemp(Spec);
	return true;
}
} // namespace

bool UCCLLoadoutComponent::CanRestoreState(const TArray<FKSIInventoryEntry>& Entries,
										   const TArray<FCCLEquippedSlot>& Equipment,
										   const TArray<UCCLSkillDefinition*>& Skills, int32 UnspentPoints) const
{
	const auto* Inventory = GetInventory();
	if (!CanAct() || !Inventory || Inventory->IsTransactionOpen() || !Inventory->CanRestore(Entries) ||
		UnspentPoints < 0 || UnspentPoints > 1000 || Skills.Num() > AvailableSkills.Num() || Equipment.Num() > 8)
	{
		return false;
	}
	TSet<FGameplayTag> Slots;
	TMap<FGuid, TArray<FGameplayTag>> Occupancy;
	for (const auto& Value : Equipment)
	{
		const auto* Entry = Entries.FindByPredicate([&](const auto& Item) { return Item.Id == Value.Id; });
		TArray<FGameplayTag> Required;
		if (Slots.Contains(Value.Slot) || !Entry || Entry->Slot != INDEX_NONE || Entry->Quantity != 1 ||
			!CCLEquipment::GetOccupiedSlots(Entry->Definition, Value.Slot, Required))
		{
			return false;
		}
		Slots.Add(Value.Slot);
		Occupancy.FindOrAdd(Value.Id).Add(Value.Slot);
	}
	for (const auto& Entry : Entries)
	{
		if (Entry.Slot != INDEX_NONE)
		{
			continue;
		}
		const auto* Actual = Occupancy.Find(Entry.Id);
		TArray<FGameplayTag> Required;
		TArray<FText> Errors;
		if (!Actual || !Entry.Definition->ValidateDefinition(Errors) ||
			!CCLEquipment::GetOccupiedSlots(Entry.Definition, (*Actual)[0], Required) ||
			Required.Num() != Actual->Num())
		{
			return false;
		}
		for (const auto Slot : Required)
		{
			if (!Actual->Contains(Slot))
			{
				return false;
			}
		}
		const auto* Equip = Entry.Definition->FindFragment<FCCLItemFragment_Equip>();
		FGameplayEffectSpecHandle Spec;
		if (!Equip || (Equip->Effect && !MakeRestoreSpec(*GetASC(), Equip->Effect, Equip->Magnitude, Spec)))
		{
			return false;
		}
	}
	TSet<UCCLSkillDefinition*> Seen;
	for (auto* Skill : Skills)
	{
		FGameplayEffectSpecHandle Spec;
		if (!Skill || !AvailableSkills.Contains(Skill) || Seen.Contains(Skill) || Skill->PointCost <= 0 ||
			!MakeRestoreSpec(*GetASC(), Skill->Effect, Skill->Magnitude, Spec))
		{
			return false;
		}
		Seen.Add(Skill);
	}
	return true;
}

bool UCCLLoadoutComponent::RestoreState(const TArray<FKSIInventoryEntry>& Entries,
										const TArray<FCCLEquippedSlot>& Equipment,
										const TArray<UCCLSkillDefinition*>& Skills, int32 UnspentPoints,
										bool bResetHealth)
{
	FCCLLoadoutRestoreRequest Request;
	Request.Loadout = this;
	Request.Entries = Entries;
	Request.Equipment = Equipment;
	Request.Skills = Skills;
	Request.Points = UnspentPoints;
	Request.bResetHealth = bResetHealth;
	return FCCLLoadoutRestoreBatch::Execute(MakeArrayView(&Request, 1), [] { return true; }, [] {});
}

bool FCCLLoadoutRestoreBatch::Execute(TConstArrayView<FCCLLoadoutRestoreRequest> Requests,
									  TFunctionRef<bool()> CommitWorld, TFunctionRef<void()> FinalizeProfiles)
{
	// An outer GAS batch would delay max-attribute evaluation beyond SyncAvatar.
	if (!IsInGameThread() || FScopedAggregatorOnDirtyBatch::GlobalBatchCount != 0)
	{
		return false;
	}
	struct FPrepared
	{
		FCCLLoadoutRestoreRequest Request;
		UCCLAbilitySystemComponent* ASC = nullptr;
		UCCLInventoryComponent* Inventory = nullptr;
		TArray<FGameplayEffectSpecHandle> Specs;
		TArray<FGuid> EffectItems;
		TArray<FActiveGameplayEffectHandle> Added;
		bool bLocked = false;
	};
	TArray<FPrepared> Prepared;
	TSet<UCCLLoadoutComponent*> Seen;
	TSet<UCCLAbilitySystemComponent*> SeenASC;
	TSet<UCCLInventoryComponent*> SeenInventory;
	TSet<FGuid> ItemIds;
	for (const auto& Request : Requests)
	{
		auto* Loadout = Request.Loadout;
		if (!IsValid(Loadout) || Seen.Contains(Loadout) ||
			!Loadout->CanRestoreState(Request.Entries, Request.Equipment, Request.Skills, Request.Points))
		{
			return false;
		}
		Seen.Add(Loadout);
		auto& Value = Prepared.AddDefaulted_GetRef();
		Value.Request = Request;
		Value.ASC = Loadout->GetASC();
		Value.Inventory = Loadout->GetInventory();
		if (SeenASC.Contains(Value.ASC) || SeenInventory.Contains(Value.Inventory))
		{
			return false;
		}
		SeenASC.Add(Value.ASC);
		SeenInventory.Add(Value.Inventory);
		for (const auto& Entry : Request.Entries)
		{
			if (ItemIds.Contains(Entry.Id))
			{
				return false;
			}
			ItemIds.Add(Entry.Id);
			const auto* Equip =
				Entry.Slot == INDEX_NONE ? Entry.Definition->FindFragment<FCCLItemFragment_Equip>() : nullptr;
			if (Equip && Equip->Effect)
			{
				FGameplayEffectSpecHandle Spec;
				if (!MakeRestoreSpec(*Value.ASC, Equip->Effect, Equip->Magnitude, Spec))
				{
					return false;
				}
				Value.Specs.Add(MoveTemp(Spec));
				Value.EffectItems.Add(Entry.Id);
			}
		}
		for (const auto* Skill : Request.Skills)
		{
			FGameplayEffectSpecHandle Spec;
			if (!MakeRestoreSpec(*Value.ASC, Skill->Effect, Skill->Magnitude, Spec))
			{
				return false;
			}
			Value.Specs.Add(MoveTemp(Spec));
		}
	}
	bool bPublish = false;
	ON_SCOPE_EXIT
	{
		// Keep every loadout closed until all inventory listeners have returned.
		for (auto& Value : Prepared)
		{
			if (Value.bLocked)
			{
				Value.Inventory->FinishTransaction(bPublish);
			}
		}
		for (auto& Value : Prepared)
		{
			if (Value.bLocked)
			{
				Value.Request.Loadout->bRestoring = 0;
				Value.Inventory->bMutationLocked = 0;
			}
		}
	};
	for (auto& Value : Prepared)
	{
		if (!Value.Inventory->BeginTransaction())
		{
			return false;
		}
		Value.Request.Loadout->bRestoring = 1;
		Value.Inventory->bMutationLocked = 1;
		Value.bLocked = true;
	}
	{
		FScopedAggregatorOnDirtyBatch Batch;
		bool bCommitted = false;
		ON_SCOPE_EXIT
		{
			if (!bCommitted)
			{
				for (auto& Value : Prepared)
				{
					for (const auto Handle : Value.Added)
					{
						Value.ASC->RemoveActiveGameplayEffect(Handle);
					}
				}
			}
		};
		for (auto& Value : Prepared)
		{
			for (const auto& Spec : Value.Specs)
			{
				const auto Handle = Value.ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
				if (!Handle.IsValid())
				{
					return false;
				}
				Value.Added.Add(Handle);
			}
		}
		// A later participant's query may have invalidated an earlier new handle.
		for (const auto& Value : Prepared)
		{
			if (!Value.Inventory->CanRestore(Value.Request.Entries))
			{
				return false;
			}
			for (const auto Handle : Value.Added)
			{
				if (!Value.ASC->GetActiveGameplayEffect(Handle))
				{
					return false;
				}
			}
		}
		if (!CommitWorld())
		{
			return false;
		}
		// All fallible work is complete. Callbacks obey the participant contract.
		for (auto& Value : Prepared)
		{
			Value.Inventory->RestoreValidated(Value.Request.Entries);
		}
		for (auto& Value : Prepared)
		{
			auto& Loadout = *Value.Request.Loadout;
			for (const auto& Pair : Loadout.EquipmentEffects)
			{
				Value.ASC->RemoveActiveGameplayEffect(Pair.Value);
			}
			for (const auto Handle : Loadout.SkillEffects)
			{
				Value.ASC->RemoveActiveGameplayEffect(Handle);
			}
			Loadout.EquipmentEffects.Reset();
			Loadout.SkillEffects.Reset();
			for (int32 Index = 0; Index < Value.Added.Num(); ++Index)
			{
				if (Index < Value.EffectItems.Num())
				{
					Loadout.EquipmentEffects.Add(Value.EffectItems[Index], Value.Added[Index]);
				}
				else
				{
					Loadout.SkillEffects.Add(Value.Added[Index]);
				}
			}
			if (auto* Actions = Loadout.GetOwner()->FindComponentByClass<UKSIActionComponent>())
			{
				const auto PreviousSources = MoveTemp(Loadout.ActionSources);
				Loadout.ActionSources.Reset();
				for (const auto Id : PreviousSources)
				{
					Actions->RemoveSource(Id);
				}
			}
			Loadout.EquipmentSlots = Value.Request.Equipment;
			Loadout.Learned.Reset();
			for (auto* Skill : Value.Request.Skills)
			{
				Loadout.Learned.Add(Skill);
			}
			Loadout.Points = Value.Request.Points;
		}
		bCommitted = true;
	}
	// Every ASC has its final maximum attributes before health or other profile values.
	for (auto& Value : Prepared)
	{
		Value.Request.Loadout->SyncAvatar(Value.Request.bResetHealth != 0);
		Value.Request.Loadout->GetOwner()->ForceNetUpdate();
	}
	FinalizeProfiles();
	bPublish = true;
	return true;
}
