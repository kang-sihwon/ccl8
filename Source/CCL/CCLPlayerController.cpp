#include "CCLPlayerController.h"
#include "Session/CCLGameInstance.h"
#include "UI/SCCLInventoryWidget.h"
#include "Combat/CCLFighterComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Items/CCLItemDefinition.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
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
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
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
	if (bInventoryOpen)
	{
		CloseInventory();
		return;
	}

	if (IsDialogueVisible())
	{
		CloseDialogue();
		return;
	}

	if (auto* Session = GetGameInstance<UCCLGameInstance>()) { Session->ToggleMenu(); }
}

void ACCLPlayerController::ToggleInventory()
{
	if (bInventoryOpen)
	{
		CloseInventory();
		return;
	}

	const auto* UIPawn = Cast<ACCLCharacter>(GetPawn());
	const auto* Session = GetGameInstance<UCCLGameInstance>();
	if (!IsLocalController() || !UIPawn || UIPawn->IsDead() || (Session && Session->IsMenuVisible()) || !GetWorld()->GetGameViewport())
	{
		return;
	}

	CloseDialogue();
	FlushPressedKeys();
	bInventoryOpen = 1;
	SelectedItem = 0;
	SelectedEquipment.Invalidate();
	UpdateEquipmentPreview();
	InventoryWidget = SNew(SCCLInventoryWidget).Controller(this);
	GetWorld()->GetGameViewport()->AddViewportWidgetContent(InventoryWidget.ToSharedRef(), 20);
	bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(InventoryWidget);
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
}

void ACCLPlayerController::CloseInventory()
{
	if (!bInventoryOpen && !InventoryWidget.IsValid())
	{
		return;
	}

	bInventoryOpen = 0;
	if (EquipmentCamera)
	{
		EquipmentCamera->DestroyComponent();
		EquipmentCamera = nullptr;
	}

	if (InventoryWidget.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
	{
		GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(InventoryWidget.ToSharedRef());
	}

	InventoryWidget.Reset();
	if (IsLocalController())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().CancelDragDrop();
		}

		FlushPressedKeys();
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
}

void ACCLPlayerController::SelectInventorySlot(int32 Slot)
{
	const auto* State = GetPlayerState<ACCLPlayerState>();
	if (State && Slot >= 0 && Slot < State->GetInventory()->Capacity)
	{
		SelectedItem = Slot;
		SelectedEquipment.Invalidate();
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
	if (!IsValid(Speaker) || !Speaker->CanReach(GetPawn()))
	{
		return;
	}

	CloseInventory();
	DialogueSpeaker = Speaker;
	DialogueName = Name;
	DialogueText = Text;
}

void ACCLPlayerController::CloseDialogue()
{
	DialogueSpeaker.Reset();
	DialogueName.Reset();
	DialogueText.Reset();
}

bool ACCLPlayerController::IsDialogueVisible() const
{
	const auto* UIPawn = Cast<ACCLCharacter>(GetPawn());
	return UIPawn && !UIPawn->IsDead() && DialogueSpeaker.IsValid() && DialogueSpeaker->CanReach(UIPawn);
}

void ACCLPlayerController::SelectPreviousItem()
{
	if (bInventoryOpen)
	{
		SelectInventorySlot(FMath::Max(0, SelectedItem - 1));
	}
}

void ACCLPlayerController::SelectNextItem()
{
	const auto* State = GetPlayerState<ACCLPlayerState>();
	if (bInventoryOpen && State)
	{
		SelectInventorySlot(FMath::Clamp(SelectedItem + 1, 0, FMath::Max(0, State->GetInventory()->Capacity - 1)));
	}
}

void ACCLPlayerController::EquipSelectedItem()
{
	if (bInventoryOpen && GetSelectedEntryId().IsValid())
	{
		ServerEquipItem(GetSelectedEntryId());
	}
}

void ACCLPlayerController::UnequipItem()
{
	if (bInventoryOpen && GetPlayerState<ACCLPlayerState>() && (SelectedEquipment.IsValid() || SelectedItem >= 0))
	{
		ServerUnequipToBag(SelectedEquipment.IsValid() ? SelectedEquipment
		                                               : GetPlayerState<ACCLPlayerState>()->GetLoadout()->GetEquippedId());
	}
}

void ACCLPlayerController::UseSelectedItem()
{
	if (bInventoryOpen)
	{
		ServerUseItem(GetSelectedEntryId());
	}
}

void ACCLPlayerController::LearnFirstSkill()
{
	const auto* State = GetPlayerState<ACCLPlayerState>();
	if (bInventoryOpen && State && State->GetLoadout()->GetSkills().IsValidIndex(0))
	{
		ServerLearnSkill(State->GetLoadout()->GetSkills()[0]);
	}
}

void ACCLPlayerController::LearnSecondSkill()
{
	const auto* State = GetPlayerState<ACCLPlayerState>();
	if (bInventoryOpen && State && State->GetLoadout()->GetSkills().IsValidIndex(1))
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
	if (SelectedEquipment.IsValid())
	{
		return SelectedEquipment;
	}

	const auto* State = GetPlayerState<ACCLPlayerState>();
	if (State)
	{
		if (const auto* Entry = State->GetInventory()->FindSlot(SelectedItem))
		{
			return Entry->Id;
		}
	}
	return FGuid();
}

void ACCLPlayerController::Move(const FInputActionValue& Value)
{
	if (bInventoryOpen)
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
	if (bInventoryOpen)
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	AddYawInput(Axis.X);
	AddPitchInput(-Axis.Y);
}

void ACCLPlayerController::StartJump()
{
	if (bInventoryOpen)
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
	if (bInventoryOpen)
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
	if (!bInventoryOpen)
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

void ACCLPlayerController::UpdateEquipmentPreview()
{
	auto* PawnActor = Cast<ACCLCharacter>(GetPawn());
	if (!bInventoryOpen || !PawnActor || !IsLocalController())
	{
		return;
	}

	if (!EquipmentPreview)
	{
		EquipmentPreview = NewObject<UTextureRenderTarget2D>(this);
		EquipmentPreview->ClearColor = FLinearColor(0.025f, 0.04f, 0.065f, 1.f);
		EquipmentPreview->InitAutoFormat(512, 768);
	}

	if (!EquipmentCamera)
	{
		EquipmentCamera = NewObject<USceneCaptureComponent2D>(this);
		EquipmentCamera->RegisterComponent();
		EquipmentCamera->TextureTarget = EquipmentPreview;
		EquipmentCamera->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		EquipmentCamera->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		EquipmentCamera->FOVAngle = 32.f;
		EquipmentCamera->bCaptureEveryFrame = true;
		EquipmentCamera->ShowFlags.SetAtmosphere(false);
		EquipmentCamera->ShowFlags.SetFog(false);
	}

	const FVector Forward = PawnActor->GetActorForwardVector();
	EquipmentCamera->SetWorldLocation(PawnActor->GetActorLocation() + Forward * 340.f);
	EquipmentCamera->SetWorldRotation((-Forward).Rotation());
	EquipmentCamera->ShowOnlyActors.Reset();
	EquipmentCamera->ShowOnlyComponents.Reset();
	EquipmentCamera->ShowOnlyActorComponents(PawnActor, true);
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
	if (const auto* State = GetPlayerState<ACCLPlayerState>())
	{
		SelectedEquipment = State->GetLoadout()->GetEquippedId(Slot);
		SelectedItem = INDEX_NONE;
	}
}
