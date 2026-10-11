#include "CCLLoadoutComponent.h"

#include "CCLInventoryComponent.h"
#include "CCLItemDefinition.h"
#include "CCLSkillDefinition.h"
#include "Actions/CCLActionComponent.h"
#include "Actions/CCLWeaponAbility.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystemInterface.h"
#include "CCLEquipmentPresenter.h"
#include "CCLItemCatalog.h"
#include "Net/UnrealNetwork.h"

UCCLLoadoutComponent::UCCLLoadoutComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCCLLoadoutComponent::BeginPlay()
{
	Super::BeginPlay();
	if (const auto* Catalog = UCCLItemCatalog::Get())
	{
		for (UCCLSkillDefinition* Skill : Catalog->TrainingSkills)
		{
			if (Skill)
			{
				AvailableSkills.Add(Skill);
			}
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
		for (const auto& Pair : EquipmentEffects)
		{
			ASC->RemoveActiveGameplayEffect(Pair.Value);
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
	DOREPLIFETIME_CONDITION(UCCLLoadoutComponent, EquipmentSlots, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCCLLoadoutComponent, Points, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCCLLoadoutComponent, Learned, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCCLLoadoutComponent, LastResult, COND_OwnerOnly);
}

bool UCCLLoadoutComponent::Equip(FGuid Id, FGameplayTag Slot)
{
	if (!Id.IsValid())
	{
		return Unequip(GetEquippedId(!Slot.IsValid() ? CCLItemTags::Slot_RightHand : Slot));
	}

	if (!CanAct())
	{
		return Report(false, TEXT("지금은 장비를 바꿀 수 없다."));
	}
	auto* Inventory = GetInventory();
	const auto* Entry = Inventory ? Inventory->Find(Id) : nullptr;
	const auto* Equip = Entry && Entry->Definition ? Entry->Definition->FindFragment<FCCLItemFragment_Equip>() : nullptr;
	if (!Equip || Entry->Quantity != 1)
	{
		return Report(false, TEXT("소지 중인 장비를 선택해 줘."));
	}

	TArray<FText> DefinitionErrors;
	if (!Entry->Definition->ValidateDefinition(DefinitionErrors))
	{
		return Report(false, TEXT("장비 정보에 오류가 있어 장착할 수 없다."));
	}

	if (!Slot.IsValid())
	{
		Slot = Equip->DefaultSlotTag;
	}
	TArray<FGameplayTag> Occupied;
	if (!CCLEquipment::GetOccupiedSlots(Entry->Definition, Slot, Occupied))
	{
		return Report(false, TEXT("이 장착 부위에는 사용할 수 없는 장비다."));
	}

	TArray<FCCLEquippedSlot> NewEquipment = EquipmentSlots;
	TSet<FGuid> Returning;
	for (const auto& Existing : EquipmentSlots)
	{
		if (Existing.Id != Id && Occupied.Contains(Existing.Slot))
		{
			Returning.Add(Existing.Id);
		}
	}

	NewEquipment.RemoveAll([&](const FCCLEquippedSlot& Existing) { return Existing.Id == Id || Returning.Contains(Existing.Id); });
	for (FGameplayTag OccupiedSlot : Occupied)
	{
		FCCLEquippedSlot NewSlot;
		NewSlot.Slot = OccupiedSlot;
		NewSlot.Id = Id;
		NewEquipment.Add(NewSlot);
	}

	TArray<FCCLInventoryEntry> Placement = Inventory->GetEntries();
	for (auto& Value : Placement)
	{
		if (Value.Id == Id)
		{
			Value.Slot = INDEX_NONE;
		}
	}

	for (FGuid ReturnId : Returning)
	{
		auto* Value = Placement.FindByPredicate([ReturnId](const auto& Item) { return Item.Id == ReturnId; });
		if (!Value)
		{
			return Report(false, TEXT("장비 상태를 확인할 수 없다."));
		}

		int32 Free = 0;
		while (Placement.ContainsByPredicate([Free](const auto& Item) { return Item.Slot == Free; }))
		{
			++Free;
		}

		if (Free >= Inventory->Capacity)
		{
			return Report(false, TEXT("장비를 돌려놓을 소지품 공간이 부족하다."));
		}

		Value->Slot = Free;
	}

	FActiveGameplayEffectHandle AddedEffect;
	if (!IsEquipped(Id) && Equip->Effect)
	{
		if (!FMath::IsFinite(Equip->Magnitude) ||
		    Equip->Effect->GetDefaultObject<UGameplayEffect>()->DurationPolicy != EGameplayEffectDurationType::Infinite)
		{
			return Report(false, TEXT("장비 효과에 오류가 있다."));
		}
		AddedEffect = GetASC()->ApplyEffect(Equip->Effect, Equip->Magnitude);
		if (!AddedEffect.IsValid())
		{
			return Report(false, TEXT("장비 효과를 적용하지 못했다."));
		}
	}
	if (!Inventory->Restore(Placement))
	{
		if (AddedEffect.IsValid())
		{
			GetASC()->RemoveActiveGameplayEffect(AddedEffect);
		}

		return Report(false, TEXT("장비를 배치할 수 없다."));
	}

	for (FGuid ReturnId : Returning)
	{
		if (auto* Handle = EquipmentEffects.Find(ReturnId))
		{
			GetASC()->RemoveActiveGameplayEffect(*Handle);
			EquipmentEffects.Remove(ReturnId);
		}
	}

	if (AddedEffect.IsValid())
	{
		EquipmentEffects.Add(Id, AddedEffect);
	}

	EquipmentSlots = MoveTemp(NewEquipment);
	SyncAvatar();
	return Report(true, TEXT("장비를 장착했다."));
}

bool UCCLLoadoutComponent::Unequip(FGuid Id, int32 BagSlot)
{
	if (!CanAct())
	{
		return Report(false, TEXT("지금은 장비를 바꿀 수 없다."));
	}

	if (!Id.IsValid())
	{
		return true;
	}

	if (!IsEquipped(Id))
	{
		return Report(false, TEXT("장착 중인 아이템이 아니다."));
	}

	auto* Inventory = GetInventory();
	if (!Inventory)
	{
		return false;
	}

	if (BagSlot == INDEX_NONE)
	{
		BagSlot = 0;
		while (Inventory->FindSlot(BagSlot))
		{
			++BagSlot;
		}
	}

	if (BagSlot < 0 || BagSlot >= Inventory->Capacity || Inventory->FindSlot(BagSlot))
	{
		return Report(false, TEXT("빈 소지품 칸을 선택해 줘."));
	}

	TArray<FCCLInventoryEntry> Placement = Inventory->GetEntries();
	auto* Item = Placement.FindByPredicate([Id](const auto& Value) { return Value.Id == Id; });
	if (!Item)
	{
		return false;
	}

	Item->Slot = BagSlot;
	if (!Inventory->Restore(Placement))
	{
		return false;
	}

	EquipmentSlots.RemoveAll([Id](const auto& Value) { return Value.Id == Id; });
	if (auto* Handle = EquipmentEffects.Find(Id))
	{
		GetASC()->RemoveActiveGameplayEffect(*Handle);
		EquipmentEffects.Remove(Id);
	}
	SyncAvatar();
	return Report(true, TEXT("장비를 해제해 소지품으로 옮겼다."));
}

bool UCCLLoadoutComponent::Use(FGuid Id)
{
	if (!CanAct())
	{
		return Report(false, TEXT("지금은 아이템을 사용할 수 없다."));
	}
	auto* Inventory = GetInventory();
	const auto* Entry = Inventory ? Inventory->Find(Id) : nullptr;
	const auto* Fragment = Entry && Entry->Definition ? Entry->Definition->FindFragment<FCCLItemFragment_ConsumableData>() : nullptr;
	auto* ASC = GetASC();
	if (!Fragment || !Fragment->Effect || !FMath::IsFinite(Fragment->Magnitude) || Fragment->Magnitude <= 0.f)
	{
		return Report(false, TEXT("회복 아이템을 선택해 줘."));
	}
	if (Entry->Food.IsPresent())
	{
		FCCLFoodQuality Quality;
		if (!Inventory->GetFoodQuality(Id, Quality) || Quality.SpoiledQuantity > 0 || Quality.MoldyQuantity > 0)
		{
			return Report(false, TEXT("부패하거나 곰팡이가 생긴 음식 묶음은 사용할 수 없다."));
		}
	}

	if (ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) >= ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()))
	{
		return Report(false, TEXT("체력이 이미 가득 차 있다."));
	}

	// Consume first and restore the exact entries if the effect is rejected, so one item never heals twice.
	const TArray<FCCLInventoryEntry> Before = Inventory->GetEntries();
	if (!Inventory->BeginTransaction())
	{
		return Report(false, TEXT("지금은 아이템을 사용할 수 없다."));
	}

	if (!Inventory->Remove(Id, 1))
	{
		Inventory->EndTransaction(false);
		return Report(false, TEXT("아이템을 소모하지 못했다."));
	}

	if (!ASC->ApplyEffect(Fragment->Effect, Fragment->Magnitude).WasSuccessfullyApplied())
	{
		verify(Inventory->Restore(Before));
		Inventory->EndTransaction(true);
		return Report(false, TEXT("아이템 효과를 적용하지 못했다."));
	}

	Inventory->EndTransaction(true);
	return Report(true, TEXT("회복 아이템을 사용했다."));
}

bool UCCLLoadoutComponent::Learn(UCCLSkillDefinition* Definition)
{
	if (!CanAct() || !Definition || !AvailableSkills.Contains(Definition) || IsLearned(Definition) ||
		Definition->PointCost <= 0 || Points < Definition->PointCost || !Definition->Effect || !FMath::IsFinite(Definition->Magnitude) ||
		Definition->Effect->GetDefaultObject<UGameplayEffect>()->DurationPolicy != EGameplayEffectDurationType::Infinite)
	{
		return Report(false, TEXT("훈련할 수 없거나 훈련 점수가 부족하다."));
	}
	const FActiveGameplayEffectHandle Handle = GetASC()->ApplyEffect(Definition->Effect, Definition->Magnitude);
	if (!Handle.IsValid())
	{
		return Report(false, TEXT("훈련 효과를 적용하지 못했다."));
	}
	SkillEffects.Add(Handle);
	Learned.Add(Definition);
	Points -= Definition->PointCost;
	return Report(true, TEXT("훈련을 완료했다."));
}

void UCCLLoadoutComponent::SyncAvatar(bool bResetHealth)
{
	auto* ASC = GetASC();
	if (!GetOwner()->HasAuthority() || !ASC || !ASC->GetAvatarActor())
	{
		return;
	}
	if (auto* Actions = GetOwner()->FindComponentByClass<UCCLActionComponent>())
	{
		for (auto It = ActionSources.CreateIterator(); It; ++It)
		{
			if (!IsEquipped(*It))
			{
				Actions->RemoveSource(*It);
				It.RemoveCurrent();
			}
		}
	}

	if (auto* Presenter = FindPresenter(ASC->GetAvatarActor()))
	{
		Presenter->PresentEquipment(GetEquippedItem(CCLItemTags::Slot_LeftHand), GetEquippedItem(CCLItemTags::Slot_RightHand));
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

FGuid UCCLLoadoutComponent::GetEquippedId(FGameplayTag Slot) const
{
	const auto* Value = EquipmentSlots.FindByPredicate([Slot](const auto& Entry) { return Entry.Slot == Slot; });
	return Value ? Value->Id : FGuid();
}

bool UCCLLoadoutComponent::IsEquipped(FGuid Id) const
{
	return Id.IsValid() && EquipmentSlots.ContainsByPredicate([Id](const auto& Entry) { return Entry.Id == Id; });
}

UCCLItemDefinition* UCCLLoadoutComponent::GetEquippedItem(FGameplayTag Slot) const
{
	const auto* Entry = GetInventory() ? GetInventory()->Find(GetEquippedId(Slot)) : nullptr;
	return Entry ? Entry->Definition.Get() : nullptr;
}

ECCLHandAction UCCLLoadoutComponent::GetHandAction(FGameplayTag Hand) const
{
	if (Hand != CCLItemTags::Slot_LeftHand && Hand != CCLItemTags::Slot_RightHand)
	{
		return ECCLHandAction::None;
	}

	const auto* Item = GetEquippedItem(Hand);
	if (!Item)
	{
		return ECCLHandAction::Attack;
	}

	const auto* Weapon = Item->FindFragment<FCCLItemFragment_Weapon>();
	return Weapon ? (Hand == CCLItemTags::Slot_LeftHand ? Weapon->LeftAction : Weapon->RightAction) : ECCLHandAction::None;
}

FGameplayTag UCCLLoadoutComponent::PrepareHandAction(FGameplayTag Hand)
{
	if (Hand != CCLItemTags::Slot_LeftHand && Hand != CCLItemTags::Slot_RightHand)
	{
		return FGameplayTag();
	}

	auto* ASC = GetASC();
	if (!ASC || !ASC->GetAvatarActor() || ASC->HasMatchingGameplayTag(CCLTags::State_Dead) ||
	    ASC->HasMatchingGameplayTag(CCLTags::State_Busy) || ASC->HasMatchingGameplayTag(CCLTags::State_Stagger))
	{
		return FGameplayTag();
	}

	const ECCLHandAction Action = GetHandAction(Hand);
	FGameplayTag Input;
	if (Action == ECCLHandAction::Attack)
	{
		Input = CCLTags::Input_Attack;
	}
	else if (Action == ECCLHandAction::Guard)
	{
		Input = CCLTags::Input_Guard;
	}
	else if (Action == ECCLHandAction::Parry)
	{
		Input = CCLTags::Input_Parry;
	}

	if (!Input.IsValid())
	{
		return Input;
	}

	if (auto* Presenter = FindPresenter(ASC->GetAvatarActor()))
	{
		Presenter->SelectActiveItem(GetEquippedItem(Hand));
	}

	// Loadout and ASC share the PlayerState actor channel. Select the hand before
	// the following predicted GAS activation RPC is processed on the server.
	if (!GetOwner()->HasAuthority())
	{
		ServerPrepareHandAction(Hand);
	}

	return Input;
}

void UCCLLoadoutComponent::ServerPrepareHandAction_Implementation(FGameplayTag Hand)
{
	PrepareHandAction(Hand);
}

bool UCCLLoadoutComponent::IsLearned(const UCCLSkillDefinition* Definition) const
{
	return Learned.Contains(Definition);
}

void UCCLLoadoutComponent::OnInventoryChanged()
{
	if (bRestoring || !GetOwner()->HasAuthority() || !GetInventory())
	{
		return;
	}

	if (auto* Actions = GetOwner()->FindComponentByClass<UCCLActionComponent>())
	{
		for (auto It = ActionSources.CreateIterator(); It; ++It)
		{
			if (!GetInventory()->Find(*It) || !IsEquipped(*It))
			{
				Actions->RemoveSource(*It);
				It.RemoveCurrent();
			}
		}
	}

	TSet<FGuid> Missing;
	for (const auto& Value : EquipmentSlots)
	{
		if (!GetInventory()->Find(Value.Id))
		{
			Missing.Add(Value.Id);
		}
	}

	for (FGuid Id : Missing)
	{
		if (auto* Handle = EquipmentEffects.Find(Id))
		{
			GetASC()->RemoveActiveGameplayEffect(*Handle);
			EquipmentEffects.Remove(Id);
		}
	}

	if (!Missing.IsEmpty())
	{
		EquipmentSlots.RemoveAll([&](const auto& Value) { return Missing.Contains(Value.Id); });
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
	return !bRestoring && GetOwner()->HasAuthority() && ASC && IsValid(ASC->GetAvatarActor()) &&
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

ICCLEquipmentPresenter* UCCLLoadoutComponent::FindPresenter(AActor* Avatar)
{
	if (!Avatar)
	{
		return nullptr;
	}

	for (UActorComponent* Component : Avatar->GetComponents())
	{
		if (auto* Presenter = Cast<ICCLEquipmentPresenter>(Component))
		{
			return Presenter;
		}
	}

	return nullptr;
}

bool UCCLLoadoutComponent::Restore(const TArray<FCCLEquippedSlot>& Equipment, const TArray<UCCLSkillDefinition*>& Skills,
								  int32 UnspentPoints)
{
	const auto* Inventory = GetInventory();
	return Inventory && RestoreState(Inventory->GetEntries(), Equipment, Skills, UnspentPoints);
}

bool UCCLLoadoutComponent::RequestCustomHandAction(FGameplayTag Hand, bool bPressed)
{
	const auto* Item = GetEquippedItem(Hand);
	const auto* Weapon = Item ? Item->FindFragment<FCCLItemFragment_Weapon>() : nullptr;
	const auto* Action = Weapon ? Weapon->HandActions.Find(Hand) : nullptr;
	if (!Weapon || !Weapon->ActionSet || !Action)
	{
		return false;
	}

	if (GetOwner()->HasAuthority())
	{
		ServerCustomHandAction_Implementation(Hand, bPressed);
	}
	else
	{
		ServerCustomHandAction(Hand, bPressed);
	}

	return true;
}

void UCCLLoadoutComponent::ServerCustomHandAction_Implementation(FGameplayTag Hand, bool bPressed)
{
	if (!CCLEquipment::IsHand(Hand))
	{
		return;
	}

	const auto* Item = GetEquippedItem(Hand);
	const auto* Weapon = Item ? Item->FindFragment<FCCLItemFragment_Weapon>() : nullptr;
	const auto* Action = Weapon ? Weapon->HandActions.Find(Hand) : nullptr;
	if (Action)
	{
		ExecuteCustom(GetEquippedId(Hand), *Action, bPressed);
	}
}

bool UCCLLoadoutComponent::ExecuteCustom(FGuid Id, FGameplayTag Action, bool bPressed)
{
	const auto* Entry = GetInventory() ? GetInventory()->Find(Id) : nullptr;
	const auto* Weapon = Entry && Entry->Definition ? Entry->Definition->FindFragment<FCCLItemFragment_Weapon>() : nullptr;
	auto* Actions = GetOwner()->FindComponentByClass<UCCLActionComponent>();
	if (!Actions || !IsEquipped(Id) || !Weapon || !Weapon->ActionSet)
	{
		return false;
	}

	if (!bPressed)
	{
		if (Action == CCLActionTags::Aim)
		{
			Actions->Release(Id, Action);
		}

		return true;
	}

	if (!CanAct())
	{
		return false;
	}

	if (!Actions->HasSource(Id))
	{
		if (!Actions->RegisterSource(Id, Entry->Definition, Weapon->ActionSet->Actions))
		{
			return false;
		}

		ActionSources.Add(Id);
	}

	return Actions->Execute(Id, Action);
}

void UCCLLoadoutComponent::RequestReload()
{
	if (GetOwner()->HasAuthority())
	{
		ServerReload_Implementation();
	}
	else
	{
		ServerReload();
	}
}

void UCCLLoadoutComponent::ServerReload_Implementation()
{
	for (const auto Hand : {FGameplayTag(CCLItemTags::Slot_RightHand), FGameplayTag(CCLItemTags::Slot_LeftHand)})
	{
		const FGuid Id = GetEquippedId(Hand);
		if (GetInventory() && GetInventory()->CanReload(Id) && ExecuteCustom(Id, CCLActionTags::Reload, true))
		{
			return;
		}
	}
}
