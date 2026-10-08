#include "CCLPlayerController.h"
#include "Session/CCLGameInstance.h"
#include "UI/CCLInventoryScreen.h"
#include "UI/CCLGameUI.h"
#include "UI/CCLHUDScreens.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Combat/CCLFighterComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "Campaign/CCLVillageSteward.h"

#include "CCLCharacter.h"
#include "CCLPlayerState.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLWorldPickup.h"
#include "Items/CCLSkillDefinition.h"
#include "EngineUtils.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "CCLGameModeBase.h"
#include "Tests/CCLCombatSmokeSubsystem.h"
#include "Tests/CCLCampaignSmokeSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

// 부모 인터페이스 함수

void ACCLPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;

	if (!Subsystem || InputMapping)
	{
		return;
	}

	InputMapping = NewObject<UInputMappingContext>(this);
	auto MakeAction = [this](EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this);
		Action->ValueType = Type;
		return Action;
	};
	MoveAction = MakeAction(EInputActionValueType::Axis2D);
	MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
	LookAction = MakeAction(EInputActionValueType::Axis2D);
	JumpAction = MakeAction(EInputActionValueType::Boolean);
	RetryAction = MakeAction(EInputActionValueType::Boolean);
	DieAction = MakeAction(EInputActionValueType::Boolean);
	auto MapMove = [this](FKey Key, bool bNegate, bool bSwizzle)
	{
		FEnhancedActionKeyMapping& Mapping = InputMapping->MapKey(MoveAction, Key);

		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(InputMapping));
		}

		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(InputMapping);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
	};
	MapMove(EKeys::W, false, true);
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::D, false, false);
	MapMove(EKeys::A, true, false);
	InputMapping->MapKey(LookAction, EKeys::Mouse2D);
	InputMapping->MapKey(JumpAction, EKeys::SpaceBar);
	InputMapping->MapKey(RetryAction, EKeys::R);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::StartJump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJump);
	Input->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::StopJump);
	Input->BindAction(RetryAction, ETriggerEvent::Started, this, &ThisClass::CCLRetry);
	UInputAction* LeaveAction = MakeAction(EInputActionValueType::Boolean);
	CombatActions.Add(LeaveAction);
	InputMapping->MapKey(LeaveAction, EKeys::Escape);
	Input->BindAction(LeaveAction, ETriggerEvent::Started, this, &ThisClass::CCLLeave);
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	InputMapping->MapKey(DieAction, EKeys::K);
	Input->BindAction(DieAction, ETriggerEvent::Started, this, &ThisClass::CCLDie);
#endif
	auto MapCombat = [this, Input, &MakeAction](FKey Key, FGameplayTag Tag)
	{
		UInputAction* Action = MakeAction(EInputActionValueType::Boolean);
		CombatActions.Add(Action);
		InputMapping->MapKey(Action, Key);
		Input->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::CombatPressed, Tag);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &ThisClass::CombatReleased, Tag);
		Input->BindAction(Action, ETriggerEvent::Canceled, this, &ThisClass::CombatReleased, Tag);
	};
	auto MapHand = [this, Input, &MakeAction](FKey Key, FGameplayTag Hand) {
		auto* Action = MakeAction(EInputActionValueType::Boolean);
		CombatActions.Add(Action);
		InputMapping->MapKey(Action, Key);
		Input->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::HandPressed, Hand);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &ThisClass::HandReleased, Hand);
		Input->BindAction(Action, ETriggerEvent::Canceled, this, &ThisClass::HandReleased, Hand);
	};
	MapHand(EKeys::LeftMouseButton, CCLItemTags::Slot_LeftHand);
	MapHand(EKeys::RightMouseButton, CCLItemTags::Slot_RightHand);
	MapCombat(EKeys::Q, CCLTags::Input_Parry);
	MapCombat(EKeys::LeftShift, CCLTags::Input_Dodge);
	auto MapMenu = [this, Input, &MakeAction](FKey Key, void (ThisClass::*Function)())
	{
		UInputAction* Action = MakeAction(EInputActionValueType::Boolean);
		CombatActions.Add(Action);
		InputMapping->MapKey(Action, Key);
		Input->BindAction(Action, ETriggerEvent::Started, this, Function);
	};
	MapMenu(EKeys::T, &ThisClass::ServerTalkToSteward);
	MapMenu(EKeys::B, &ThisClass::ServerBuyPotion);
	MapMenu(EKeys::I, &ThisClass::ToggleInventory);
	MapMenu(EKeys::Up, &ThisClass::SelectPreviousItem);
	MapMenu(EKeys::Down, &ThisClass::SelectNextItem);
	MapMenu(EKeys::F, &ThisClass::EquipSelectedItem);
	MapMenu(EKeys::G, &ThisClass::UnequipItem);
	MapMenu(EKeys::H, &ThisClass::UseSelectedItem);
	MapMenu(EKeys::One, &ThisClass::LearnFirstSkill);
	MapMenu(EKeys::Two, &ThisClass::LearnSecondSkill);
	MapMenu(EKeys::E, &ThisClass::ServerCollectNearby);
	Subsystem->AddMappingContext(InputMapping, 0);
	CCLGameUI::Get(this);
}

void ACCLPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	// Interaction validity is independent of whether presentation hides the widget.
	if (DialogueHandle.IsValid() && !IsDialogueVisible())
	{
		CloseDialogue();
	}
}

void ACCLPlayerController::FlushPressedKeys()
{
	Super::FlushPressedKeys();
	if (IsLocalController())
	{
		HandInput(CCLItemTags::Slot_LeftHand, false);
		HandInput(CCLItemTags::Slot_RightHand, false);
	}

	if (auto* ControlledPawn = Cast<ACCLCharacter>(GetPawn()))
	{
		if (auto* ASC = Cast<UCCLAbilitySystemComponent>(ControlledPawn->GetAbilitySystemComponent()))
		{
			ASC->ReleaseAllInputs();
		}
	}
}

void ACCLPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseInventory();
	CloseDialogue();
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (auto* UI = LocalPlayer->GetSubsystem<UCCLUISubsystem>())
		{
			UI->OnViewClosed.RemoveAll(this);
		}

		if (auto* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(); Subsystem && InputMapping)
		{
			Subsystem->RemoveMappingContext(InputMapping);
		}
	}

	Super::EndPlay(EndPlayReason);
}

// 내 클래스 함수

void ACCLPlayerController::CCLRetry()
{
	ServerRequestRetry();
}

void ACCLPlayerController::CCLDie()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	ServerRequestDebugDeath();
#endif
}

void ACCLPlayerController::CCLLeave()
{
	if (IsInventoryOpen())
	{
		CloseInventory();
		return;
	}

	if (IsDialogueVisible())
	{
		CloseDialogue();
		return;
	}

	if (auto* Session = GetGameInstance<UCCLGameInstance>()) { Session->ToggleMenuForPlayer(this); }
}

void ACCLPlayerController::ToggleInventory()
{
	if (IsInventoryOpen())
	{
		CloseInventory();
		return;
	}

	const auto* UIPawn = Cast<ACCLCharacter>(GetPawn());
	const auto* Session = GetGameInstance<UCCLGameInstance>();
	if (!IsLocalController() || !UIPawn || UIPawn->IsDead() || (Session && Session->IsMenuVisibleForPlayer(this)))
	{
		return;
	}

	if (auto* UI = CCLGameUI::Get(this))
	{
		CloseDialogue();
		FlushPressedKeys();
		UI->OnViewClosed.RemoveAll(this);
		UI->OnViewClosed.AddUObject(this, &ThisClass::HandleUIViewClosed);
		InventoryContext = NewObject<UCCLInventoryContext>(this);
		InventoryContext->Initialize(this);
		InventoryHandle = UI->OpenView(CCLUITags::View_Inventory, InventoryContext, this);
		if (!InventoryHandle.IsValid())
		{
			InventoryContext->Release();
			InventoryContext = nullptr;
		}
	}
}

void ACCLPlayerController::CloseInventory()
{
	if (const auto* Local = GetLocalPlayer())
	{
		if (auto* UI = Local->GetSubsystem<UCCLUISubsystem>())
		{
			UI->CloseView(InventoryHandle);
		}
	}

	InventoryHandle = {};
	InventoryContext = nullptr;
}

void ACCLPlayerController::SelectInventorySlot(int32 Slot)
{
	const auto* State = GetPlayerState<ACCLPlayerState>();
	if (State && InventoryContext && Slot >= 0 && Slot < State->GetInventory()->Capacity)
	{
		InventoryContext->SelectedItem = Slot;
		InventoryContext->SelectedEquipment.Invalidate();
	}
}

void ACCLPlayerController::ServerMoveInventoryItem_Implementation(FGuid Id, int32 Slot)
{
	auto* State = GetPlayerState<ACCLPlayerState>();
	const auto* UIPawn = Cast<ACCLCharacter>(GetPawn());
	if (State && UIPawn && !UIPawn->IsDead() && !State->GetLoadout()->IsEquipped(Id))
	{
		State->GetLoadout()->ShowNotice(State->GetInventory()->MoveToSlot(Id, Slot) ? TEXT("Item moved.") : TEXT("Cannot move this item."));
	}
}

void ACCLPlayerController::ClientShowDialogue_Implementation(ACCLVillageSteward* Speaker, const FString& Name, const FString& Text)
{
	const auto* UIPawn = Cast<ACCLCharacter>(GetPawn());
	const auto* Session = GetGameInstance<UCCLGameInstance>();
	// A server reply can arrive after death or after another modal screen opens.
	if (!UIPawn || UIPawn->IsDead() || !IsValid(Speaker) || !Speaker->CanReach(UIPawn) || IsInventoryOpen() ||
		(Session && Session->IsMenuVisibleForPlayer(this)))
	{
		return;
	}

	DialogueSpeaker = Speaker;
	DialogueName = Name;
	DialogueText = Text;
	if (auto* UI = CCLGameUI::Get(this))
	{
		UI->OnViewClosed.RemoveAll(this);
		UI->OnViewClosed.AddUObject(this, &ThisClass::HandleUIViewClosed);
		if (!DialogueContext)
		{
			DialogueContext = NewObject<UCCLDialogueContext>(this);
		}

		DialogueContext->Update(Name, Text);
		if (!UI->IsViewOpen(DialogueHandle))
		{
			DialogueHandle = UI->OpenView(CCLUITags::View_Dialogue, DialogueContext, this);
		}
	}

	if (!DialogueHandle.IsValid())
	{
		CloseDialogue();
	}
}

void ACCLPlayerController::CloseDialogue()
{
	if (const auto* Local = GetLocalPlayer())
	{
		if (auto* UI = Local->GetSubsystem<UCCLUISubsystem>())
		{
			UI->CloseView(DialogueHandle);
		}
	}

	DialogueHandle = {};
	DialogueContext = nullptr;
	DialogueSpeaker.Reset();
	DialogueName.Reset();
	DialogueText.Reset();
}

bool ACCLPlayerController::IsDialogueVisible() const
{
	const auto* UIPawn = Cast<ACCLCharacter>(GetPawn());
	const auto* Local = GetLocalPlayer();
	const auto* UI = Local ? Local->GetSubsystem<UCCLUISubsystem>() : nullptr;
	return UI && UI->IsViewOpen(DialogueHandle) && UIPawn && !UIPawn->IsDead() &&
		DialogueSpeaker.IsValid() && DialogueSpeaker->CanReach(UIPawn);
}

void ACCLPlayerController::SelectPreviousItem()
{
	if (IsInventoryOpen())
	{
		SelectInventorySlot(FMath::Max(0, GetSelectedItem() - 1));
	}
}

void ACCLPlayerController::SelectNextItem()
{
	const auto* State = GetPlayerState<ACCLPlayerState>();
	if (IsInventoryOpen() && State)
	{
		SelectInventorySlot(FMath::Clamp(GetSelectedItem() + 1, 0, FMath::Max(0, State->GetInventory()->Capacity - 1)));
	}
}

void ACCLPlayerController::EquipSelectedItem()
{
	if (IsInventoryOpen() && GetSelectedEntryId().IsValid())
	{
		ServerEquipItem(GetSelectedEntryId());
	}
}

void ACCLPlayerController::UnequipItem()
{
	if (IsInventoryOpen() && GetPlayerState<ACCLPlayerState>() && (GetSelectedEquipment().IsValid() || GetSelectedItem() >= 0))
	{
		ServerUnequipToBag(GetSelectedEquipment().IsValid() ? GetSelectedEquipment()
		                                               : GetPlayerState<ACCLPlayerState>()->GetLoadout()->GetEquippedId());
	}
}

void ACCLPlayerController::UseSelectedItem()
{
	if (IsInventoryOpen())
	{
		ServerUseItem(GetSelectedEntryId());
	}
}

void ACCLPlayerController::LearnFirstSkill()
{
	const auto* State = GetPlayerState<ACCLPlayerState>();
	if (IsInventoryOpen() && State && State->GetLoadout()->GetSkills().IsValidIndex(0))
	{
		ServerLearnSkill(State->GetLoadout()->GetSkills()[0]);
	}
}

void ACCLPlayerController::LearnSecondSkill()
{
	const auto* State = GetPlayerState<ACCLPlayerState>();
	if (IsInventoryOpen() && State && State->GetLoadout()->GetSkills().IsValidIndex(1))
	{
		ServerLearnSkill(State->GetLoadout()->GetSkills()[1]);
	}
}

void ACCLPlayerController::ServerCollectNearby_Implementation()
{
	auto* State = GetPlayerState<ACCLPlayerState>();
	const auto* ControlledCharacter = Cast<ACCLCharacter>(GetPawn());
	if (!State || !ControlledCharacter || ControlledCharacter->IsDead())
	{
		return;
	}
	ACCLWorldPickup* Nearest = nullptr;
	float Distance = FMath::Square(225.f);
	for (TActorIterator<ACCLWorldPickup> It(GetWorld()); It; ++It)
	{
		const float Candidate = FVector::DistSquared(ControlledCharacter->GetActorLocation(), It->GetActorLocation());
		if (Candidate < Distance)
		{
			Distance = Candidate;
			Nearest = *It;
		}
	}
	if (Nearest)
	{
		const bool bCollected = Nearest->TryCollect(State->GetInventory(), GetPawn());
		State->GetLoadout()->ShowNotice(bCollected ? TEXT("Supplies collected.") : TEXT("Cannot collect: blocked or inventory full."));
	}
	else
	{
		State->GetLoadout()->ShowNotice(TEXT("No supplies within reach."));
	}
}

void ACCLPlayerController::ServerEquipItem_Implementation(FGuid Id)
{
	if (auto* State = GetPlayerState<ACCLPlayerState>())
	{
		State->GetLoadout()->Equip(Id);
	}
}

void ACCLPlayerController::ServerUseItem_Implementation(FGuid Id)
{
	if (auto* State = GetPlayerState<ACCLPlayerState>())
	{
		State->GetLoadout()->Use(Id);
	}
}

void ACCLPlayerController::ServerLearnSkill_Implementation(UCCLSkillDefinition* Definition)
{
	if (auto* State = GetPlayerState<ACCLPlayerState>())
	{
		State->GetLoadout()->Learn(Definition);
	}
}

FGuid ACCLPlayerController::GetSelectedEntryId() const
{
	if (GetSelectedEquipment().IsValid())
	{
		return GetSelectedEquipment();
	}

	const auto* State = GetPlayerState<ACCLPlayerState>();
	if (State)
	{
		if (const auto* Entry = State->GetInventory()->FindSlot(GetSelectedItem()))
		{
			return Entry->Id;
		}
	}
	return FGuid();
}

void ACCLPlayerController::Move(const FInputActionValue& Value)
{
	if (IsInventoryOpen())
	{
		return;
	}

	ACCLCharacter* ControlledCharacter = Cast<ACCLCharacter>(GetPawn());

	if (!ControlledCharacter || ControlledCharacter->IsDead() || (ControlledCharacter->GetAbilitySystemComponent() && ControlledCharacter->GetAbilitySystemComponent()->HasMatchingGameplayTag(CCLTags::State_Stagger)))
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	ControlledCharacter->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	ControlledCharacter->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void ACCLPlayerController::Look(const FInputActionValue& Value)
{
	if (IsInventoryOpen())
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	AddYawInput(Axis.X);
	AddPitchInput(-Axis.Y);
}

void ACCLPlayerController::StartJump()
{
	if (IsInventoryOpen())
	{
		return;
	}

	if (ACCLCharacter* ControlledCharacter = Cast<ACCLCharacter>(GetPawn()); ControlledCharacter && !ControlledCharacter->IsDead())
	{
		ControlledCharacter->Jump();
	}
}

void ACCLPlayerController::StopJump()
{
	if (ACCLCharacter* ControlledCharacter = Cast<ACCLCharacter>(GetPawn()))
	{
		ControlledCharacter->StopJumping();
	}
}

void ACCLPlayerController::ServerRequestRetry_Implementation()
{
	if (auto* GameMode = GetWorld()->GetAuthGameMode<ACCLGameModeBase>())
	{
		GameMode->RequestRetry(this);
	}
}

void ACCLPlayerController::ServerRequestDebugDeath_Implementation()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (ACCLCharacter* ControlledCharacter = Cast<ACCLCharacter>(GetPawn()))
	{
		ControlledCharacter->Die();
	}
#endif
}

void ACCLPlayerController::CombatPressed(FGameplayTag Tag)
{
	if (IsInventoryOpen())
	{
		return;
	}

	if (auto* ControlledPawn = Cast<ACCLCharacter>(GetPawn()))
	{
		if (auto* ASC = Cast<UCCLAbilitySystemComponent>(ControlledPawn->GetAbilitySystemComponent()))
		{
			ASC->AbilityInputTagPressed(Tag);
		}
	}
}

void ACCLPlayerController::CombatReleased(FGameplayTag Tag)
{
	if (auto* ControlledPawn = Cast<ACCLCharacter>(GetPawn()))
	{
		if (auto* ASC = Cast<UCCLAbilitySystemComponent>(ControlledPawn->GetAbilitySystemComponent()))
		{
			ASC->AbilityInputTagReleased(Tag);
		}
	}
}

void ACCLPlayerController::ServerCombatTestReady_Implementation()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (auto* Test = GetWorld()->GetSubsystem<UCCLCombatSmokeSubsystem>())
	{
		Test->RegisterDriver(this);
	}
#endif
}

void ACCLPlayerController::ClientCombatTestStep_Implementation(int32 Step)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (auto* Test = GetWorld()->GetSubsystem<UCCLCombatSmokeSubsystem>())
	{
		Test->ExecuteClientStep(Step);
	}
#endif
}

void ACCLPlayerController::ServerCampaignTestReady_Implementation()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (auto* Test = GetWorld()->GetSubsystem<UCCLCampaignSmokeSubsystem>())
	{
		Test->RegisterDriver(this);
	}
#endif
}

void ACCLPlayerController::ClientCampaignTestStep_Implementation(int32 Step)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (auto* Test = GetWorld()->GetSubsystem<UCCLCampaignSmokeSubsystem>())
	{
		Test->ExecuteClientStep(Step);
	}
#endif
}

void ACCLPlayerController::ClientProgressionTestStep_Implementation(int32 Step, FGuid EntryId)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (auto* Test = GetWorld()->GetSubsystem<UCCLCampaignSmokeSubsystem>())
	{
		Test->ExecuteProgressionStep(Step, EntryId);
	}
#endif
}

namespace
{
ACCLVillageSteward* NearbySteward(UWorld* World, const APawn* Pawn)
{
	for (TActorIterator<ACCLVillageSteward> It(World); It; ++It) { if (It->CanReach(Pawn)) { return *It; } }
	return nullptr;
}
}
void ACCLPlayerController::ServerTalkToSteward_Implementation()
{
	if (auto* State = GetPlayerState<ACCLPlayerState>())
	{
		auto* Speaker = NearbySteward(GetWorld(), GetPawn());
		State->GetExpedition()->Talk(Speaker);
		if (Speaker)
		{
			ClientShowDialogue(Speaker, Speaker->DisplayName.ToString(), State->GetExpedition()->GetNotice());
		}
	}
}
void ACCLPlayerController::ServerBuyPotion_Implementation()
{
	if (auto* State = GetPlayerState<ACCLPlayerState>())
	{
		auto* Speaker = NearbySteward(GetWorld(), GetPawn());
		State->GetExpedition()->Buy(Speaker);
		if (Speaker)
		{
			ClientShowDialogue(Speaker, Speaker->DisplayName.ToString(), State->GetExpedition()->GetNotice());
		}
	}
}
void ACCLPlayerController::ClientContentTestStep_Implementation(int32 Step)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (GetWorld()->GetSubsystem<UCCLCampaignSmokeSubsystem>() && FParse::Param(FCommandLine::Get(), TEXT("CCLContentSmoke")))
	{
		if (Step == 0) { ServerTalkToSteward(); }
		else { ServerBuyPotion(); }
	}
#endif
}

void ACCLPlayerController::ServerEquipToSlot_Implementation(FGuid Id, FGameplayTag Slot)
{
	if (auto* State = GetPlayerState<ACCLPlayerState>())
	{
		State->GetLoadout()->Equip(Id, Slot);
	}
}

void ACCLPlayerController::ServerUnequipToBag_Implementation(FGuid Id, int32 BagSlot)
{
	if (auto* State = GetPlayerState<ACCLPlayerState>())
	{
		State->GetLoadout()->Unequip(Id, BagSlot);
	}
}

void ACCLPlayerController::HandPressed(FGameplayTag Hand)
{
	if (!IsInventoryOpen())
	{
		HandInput(Hand, true);
	}
}

void ACCLPlayerController::HandReleased(FGameplayTag Hand)
{
	HandInput(Hand, false);
}

void ACCLPlayerController::HandInput(FGameplayTag Hand, bool bPressed)
{
	if (Hand != CCLItemTags::Slot_LeftHand && Hand != CCLItemTags::Slot_RightHand)
	{
		return;
	}

	auto* State = GetPlayerState<ACCLPlayerState>();
	auto* PawnActor = Cast<ACCLCharacter>(GetPawn());
	auto* ASC = State ? State->GetCCLAbilitySystem() : nullptr;
	if (!ASC || !PawnActor)
	{
		return;
	}

	FGameplayTag& Held = Hand == CCLItemTags::Slot_LeftHand ? HeldLeftAction : HeldRightAction;
	if (!bPressed)
	{
		if (Held.IsValid())
		{
			ASC->AbilityInputTagReleased(Held);
			Held = FGameplayTag();
		}

		return;
	}

	if (PawnActor->IsDead() || ASC->HasMatchingGameplayTag(CCLTags::State_Busy) || ASC->HasMatchingGameplayTag(CCLTags::State_Stagger))
	{
		return;
	}

	const FGameplayTag Input = State->GetLoadout()->PrepareHandAction(Hand);
	if (!Input.IsValid())
	{
		return;
	}

	Held = Input;
	ASC->AbilityInputTagPressed(Input);
}

UTextureRenderTarget2D* ACCLPlayerController::GetEquipmentPreview() const
{
	return InventoryContext ? InventoryContext->GetPreview() : nullptr;
}

void ACCLPlayerController::CCLEquipmentDemo()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!HasAuthority())
	{
		return;
	}

	if (auto* State = GetPlayerState<ACCLPlayerState>())
	{
		for (const TCHAR* Name : {TEXT("DA_TrainingSword"), TEXT("DA_TrainingShield"), TEXT("DA_TrainingStaff"), TEXT("DA_TrainingArmor"),
		                          TEXT("DA_TrainingBoots"), TEXT("DA_TrainingCloak"), TEXT("DA_TrainingNecklace"), TEXT("DA_TrainingRing")})
		{
			auto* Definition = LoadObject<UCCLItemDefinition>(nullptr, *FString::Printf(TEXT("/Game/Progression/%s.%s"), Name, Name));
			State->GetInventory()->Add(Definition, 1);
		}
	}
#endif
}

void ACCLPlayerController::SelectEquipmentSlot(FGameplayTag Slot)
{
	if (const auto* State = GetPlayerState<ACCLPlayerState>(); State && InventoryContext)
	{
		InventoryContext->SelectedEquipment = State->GetLoadout()->GetEquippedId(Slot);
		InventoryContext->SelectedItem = INDEX_NONE;
	}
}

void ACCLPlayerController::HandleUIViewClosed(FCCLUIViewHandle View)
{
	if (View == DialogueHandle)
	{
		DialogueHandle = {};
		DialogueContext = nullptr;
		DialogueSpeaker.Reset();
		DialogueName.Reset();
		DialogueText.Reset();
	}

	if (View == InventoryHandle)
	{
		InventoryHandle = {};
		InventoryContext = nullptr;
	}
}

bool ACCLPlayerController::IsInventoryOpen() const
{
	const auto* Local = GetLocalPlayer();
	const auto* UI = Local ? Local->GetSubsystem<UCCLUISubsystem>() : nullptr;
	return UI && UI->IsViewOpen(InventoryHandle);
}

TSharedPtr<SCCLInventoryWidget> ACCLPlayerController::GetInventoryWidget() const
{
	const auto* Local = GetLocalPlayer();
	const auto* UI = Local ? Local->GetSubsystem<UCCLUISubsystem>() : nullptr;
	const auto* Screen = UI ? Cast<UCCLInventoryScreen>(UI->FindScreen(InventoryHandle)) : nullptr;
	return Screen ? Screen->GetBody() : nullptr;
}

int32 ACCLPlayerController::GetSelectedItem() const
{
	return InventoryContext ? InventoryContext->SelectedItem : INDEX_NONE;
}

FGuid ACCLPlayerController::GetSelectedEquipment() const
{
	return InventoryContext ? InventoryContext->SelectedEquipment : FGuid();
}
