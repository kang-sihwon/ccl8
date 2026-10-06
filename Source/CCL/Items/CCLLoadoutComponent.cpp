#include "CCLLoadoutComponent.h"

#include "CCLInventoryComponent.h"
#include "CCLItemDefinition.h"
#include "CCLSkillDefinition.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystemInterface.h"
#include "Combat/CCLFighterComponent.h"
#include "Net/UnrealNetwork.h"

UCCLLoadoutComponent::UCCLLoadoutComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCCLLoadoutComponent::BeginPlay()
{
	Super::BeginPlay();
	for (const TCHAR* Path : {TEXT("/Game/Progression/DA_PowerTraining.DA_PowerTraining"), TEXT("/Game/Progression/DA_VitalityTraining.DA_VitalityTraining")})
	{
		if (auto* Skill = LoadObject<UCCLSkillDefinition>(nullptr, Path))
		{
			AvailableSkills.Add(Skill);
		}
	}
	if (auto* Inventory = GetInventory())
	{
		Inventory->OnChanged.AddUObject(this, &ThisClass::OnInventoryChanged);
	}
}

void UCCLLoadoutComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* Inventory = GetInventory())
	{
		Inventory->OnChanged.RemoveAll(this);
	}
	if (auto* ASC = GetASC(); GetOwner()->HasAuthority() && IsValid(ASC) && ASC->IsOwnerActorAuthoritative())
	{
		if (EquipmentEffect.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(EquipmentEffect);
		}
		for (const auto& Handle : SkillEffects)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UCCLLoadoutComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UCCLLoadoutComponent, EquippedId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCCLLoadoutComponent, Points, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCCLLoadoutComponent, Learned, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCCLLoadoutComponent, LastResult, COND_OwnerOnly);
}

bool UCCLLoadoutComponent::Equip(FGuid Id)
{
	if (!CanAct())
	{
		return Report(false, TEXT("Cannot change equipment now."));
	}
	if (Id == EquippedId)
	{
		return true;
	}
	auto* ASC = GetASC();
	FActiveGameplayEffectHandle NewEffect;
	if (Id.IsValid())
	{
		const auto* Entry = GetInventory() ? GetInventory()->Find(Id) : nullptr;
		const auto* Fragment = Entry && Entry->Definition ? Cast<UCCLItemFragment_Equipment>(Entry->Definition->FindFragment(UCCLItemFragment_Equipment::StaticClass())) : nullptr;
		if (!Fragment || !Fragment->Effect || !FMath::IsFinite(Fragment->Magnitude) ||
			Fragment->Effect->GetDefaultObject<UGameplayEffect>()->DurationPolicy != EGameplayEffectDurationType::Infinite)
		{
			return Report(false, TEXT("Select owned equipment."));
		}
		NewEffect = ASC->ApplyEffect(Fragment->Effect, Fragment->Magnitude);
		if (!NewEffect.IsValid())
		{
			return Report(false, TEXT("Equipment effect failed."));
		}
	}
	if (EquipmentEffect.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(EquipmentEffect);
	}
	EquipmentEffect = NewEffect;
	EquippedId = Id;
	SyncAvatar();
	return Report(true, Id.IsValid() ? TEXT("Equipment applied.") : TEXT("Equipment removed."));
}

bool UCCLLoadoutComponent::Use(FGuid Id)
{
	if (!CanAct())
	{
		return Report(false, TEXT("Cannot use an item now."));
	}
	auto* Inventory = GetInventory();
	const auto* Entry = Inventory ? Inventory->Find(Id) : nullptr;
	const auto* Fragment = Entry && Entry->Definition ? Cast<UCCLItemFragment_Consumable>(Entry->Definition->FindFragment(UCCLItemFragment_Consumable::StaticClass())) : nullptr;
	auto* ASC = GetASC();
	if (!Fragment || !Fragment->Effect || !FMath::IsFinite(Fragment->Magnitude) || Fragment->Magnitude <= 0.f)
	{
		return Report(false, TEXT("Select a recovery item."));
	}
	if (ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) >= ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()))
	{
		return Report(false, TEXT("Health is already full."));
	}
	if (!ASC->ApplyEffect(Fragment->Effect, Fragment->Magnitude).WasSuccessfullyApplied())
	{
		return Report(false, TEXT("Item effect failed."));
	}
	Inventory->Remove(Id, 1);
	return Report(true, TEXT("Recovery item used."));
}

bool UCCLLoadoutComponent::Learn(UCCLSkillDefinition* Definition)
{
	if (!CanAct() || !Definition || !AvailableSkills.Contains(Definition) || IsLearned(Definition) ||
		Definition->PointCost <= 0 || Points < Definition->PointCost || !Definition->Effect || !FMath::IsFinite(Definition->Magnitude) ||
		Definition->Effect->GetDefaultObject<UGameplayEffect>()->DurationPolicy != EGameplayEffectDurationType::Infinite)
	{
		return Report(false, TEXT("Training unavailable or not enough points."));
	}
	const FActiveGameplayEffectHandle Handle = GetASC()->ApplyEffect(Definition->Effect, Definition->Magnitude);
	if (!Handle.IsValid())
	{
		return Report(false, TEXT("Training effect failed."));
	}
	SkillEffects.Add(Handle);
	Learned.Add(Definition);
	Points -= Definition->PointCost;
	return Report(true, TEXT("Training learned."));
}

void UCCLLoadoutComponent::SyncAvatar(bool bResetHealth)
{
	auto* ASC = GetASC();
	if (!GetOwner()->HasAuthority() || !ASC || !ASC->GetAvatarActor())
	{
		return;
	}
	if (auto* Fighter = ASC->GetAvatarActor()->FindComponentByClass<UCCLFighterComponent>())
	{
		Fighter->Item = GetEquippedItem();
		if (!Fighter->Item || !Fighter->Item->FindFragment(UCCLItemFragment_Combat::StaticClass()))
		{
			Fighter->Item = LoadObject<UCCLItemDefinition>(nullptr, TEXT("/Game/Combat/DA_Unarmed.DA_Unarmed"));
		}
		ASC->GetAvatarActor()->ForceNetUpdate();
	}
	if (bResetHealth)
	{
		ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()));
	}
}

void UCCLLoadoutComponent::GrantPoints(int32 Amount)
{
	if (GetOwner()->HasAuthority() && Amount > 0 && Amount <= 1000 - Points)
	{
		Points += Amount;
	}
}

void UCCLLoadoutComponent::ShowNotice(const FString& Message)
{
	if (GetOwner()->HasAuthority())
	{
		LastResult = Message;
		GetOwner()->ForceNetUpdate();
	}
}

UCCLItemDefinition* UCCLLoadoutComponent::GetEquippedItem() const
{
	const auto* Entry = GetInventory() ? GetInventory()->Find(EquippedId) : nullptr;
	return Entry ? Entry->Definition.Get() : nullptr;
}

bool UCCLLoadoutComponent::IsLearned(const UCCLSkillDefinition* Definition) const
{
	return Learned.Contains(Definition);
}

void UCCLLoadoutComponent::OnInventoryChanged()
{
	if (GetOwner()->HasAuthority() && EquippedId.IsValid() && !GetEquippedItem())
	{
		if (auto* ASC = GetASC(); ASC && EquipmentEffect.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(EquipmentEffect);
		}
		EquipmentEffect.Invalidate();
		EquippedId.Invalidate();
		SyncAvatar();
	}
}

bool UCCLLoadoutComponent::Report(bool bSuccess, const TCHAR* Message)
{
	if (GetOwner()->HasAuthority())
	{
		LastResult = Message;
		GetOwner()->ForceNetUpdate();
	}
	return bSuccess;
}

bool UCCLLoadoutComponent::CanAct() const
{
	const auto* ASC = GetASC();
	return GetOwner()->HasAuthority() && ASC && IsValid(ASC->GetAvatarActor()) &&
		!ASC->HasMatchingGameplayTag(CCLTags::State_Dead) && !ASC->HasMatchingGameplayTag(CCLTags::State_Busy) &&
		!ASC->HasMatchingGameplayTag(CCLTags::State_Stagger);
}

UCCLAbilitySystemComponent* UCCLLoadoutComponent::GetASC() const
{
	const auto* Interface = Cast<IAbilitySystemInterface>(GetOwner());
	return Interface ? Cast<UCCLAbilitySystemComponent>(Interface->GetAbilitySystemComponent()) : nullptr;
}

UCCLInventoryComponent* UCCLLoadoutComponent::GetInventory() const
{
	return GetOwner()->FindComponentByClass<UCCLInventoryComponent>();
}

bool UCCLLoadoutComponent::Restore(FGuid Equipment, const TArray<UCCLSkillDefinition*>& Skills, int32 UnspentPoints)
{
	if (!CanAct() || UnspentPoints < 0 || UnspentPoints > 1000 || Skills.Num() > AvailableSkills.Num()) { return false; }
	TSet<UCCLSkillDefinition*> Seen;
	for (auto* Skill : Skills)
	{
		if (!Skill || !AvailableSkills.Contains(Skill) || Seen.Contains(Skill) || Skill->PointCost <= 0) { return false; }
		Seen.Add(Skill);
	}
	if (!Equip(FGuid())) { return false; }
	for (const auto& Handle : SkillEffects) { GetASC()->RemoveActiveGameplayEffect(Handle); }
	SkillEffects.Reset();
	Learned.Reset();
	Points = 1000;
	for (auto* Skill : Skills) { if (!Learn(Skill)) { return false; } }
	Points = UnspentPoints;
	if (!Equip(Equipment)) { return false; }
	SyncAvatar(true);
	return true;
}
