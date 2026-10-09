#include "CCLUISmokeSubsystem.h"

#include "UI/CCLUIInputData.h"
#include "CCLCharacter.h"
#include "CCLHUD.h"
#include "UI/CCLHUDScreens.h"
#include "UI/CCLCombatViewModel.h"
#include "UI/CCLGameUI.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "CCLPlayerController.h"
#include "CCLPlayerState.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Campaign/CCLVillageSteward.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "AbilitySystem/CCLOffenseSet.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "Items/CCLLoadoutComponent.h"
#include "Misc/Paths.h"
#include "Session/CCLSessionRecord.h"
#include "UI/SCCLInventoryWidget.h"
#include "UI/CCLInventoryScreen.h"
#include "UI/Core/CCLUISubsystem.h"
#include "UI/Core/CCLUIRoot.h"
#include "Session/CCLGameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Input/CommonUIActionRouterBase.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Containers/Ticker.h"
#endif

namespace
{
void FinishUITest(UWorld* World, uint8 ExitCode)
{
#if WITH_EDITOR
	if (World && World->WorldType == EWorldType::PIE && GEditor)
	{
		GEditor->RequestEndPlayMap();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([ExitCode](float)
		{
			if (GEditor && GEditor->PlayWorld) { return true; }
			FPlatformMisc::RequestExitWithStatus(false, ExitCode);
			return false;
		}), 1.f);
		return;
	}
#endif
	FPlatformMisc::RequestExitWithStatus(false, ExitCode);
}
}


bool UCCLUISmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Param(FCommandLine::Get(), TEXT("CCLUISmoke"));
#endif
}

TStatId UCCLUISmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLUISmokeSubsystem, STATGROUP_Tickables);
}

bool UCCLUISmokeSubsystem::Check(bool Condition, const TCHAR* Message)
{
	UE_LOG(LogTemp, Display, TEXT("CCL_UI %s %s"), Condition ? TEXT("CHECK") : TEXT("FAIL"), Message);
	if (!Condition)
	{
		bComplete = 1;
		FinishUITest(GetWorld(), 1);
	}

	return Condition;
}

void UCCLUISmokeSubsystem::Capture(const TCHAR* Name)
{
	FString Directory;
	FParse::Value(FCommandLine::Get(), TEXT("CCLUICapture="), Directory);
	if (Directory.IsEmpty())
	{
		Directory = FPaths::ProjectSavedDir() / TEXT("Tests/UIVisual");
	}

	IFileManager::Get().MakeDirectory(*Directory, true);
	FScreenshotRequest::RequestScreenshot(Directory / FString(Name) + TEXT(".png"), true, false);
}

bool UCCLUISmokeSubsystem::CheckEquipment(ACCLPlayerController* PC)
{
	auto* Player = PC->GetPlayerState<ACCLPlayerState>();
	auto* Bag = Player->GetInventory();
	auto* Loadout = Player->GetLoadout();
	auto* ASC = Player->GetCCLAbilitySystem();
	auto Add = [Bag](const TCHAR* Name) { return Bag->Add(FCCLSessionCodec::Item(Name), 1); };
	auto Bonus = [ASC]() { return ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()); };
	const FGuid Gauntlet = Add(TEXT("DA_IronGauntlets"));
	const FGuid Shield = Add(TEXT("DA_TrainingShield"));
	const FGuid Staff = Add(TEXT("DA_TrainingStaff"));
	if (!Check(Gauntlet.IsValid() && Shield.IsValid() && Staff.IsValid() && Loadout->Equip(Gauntlet, CCLItemTags::Slot_LeftHand) &&
	               Loadout->Equip(Shield),
	           TEXT("either hand and default right hand")))
	{
		return false;
	}

	TArray<FGuid> Fillers;
	while (Bag->CanAdd(FCCLSessionCodec::Item(TEXT("DA_TrainingSword")), 1))
	{
		Fillers.Add(Add(TEXT("DA_TrainingSword")));
	}

	if (!Check(!Fillers.IsEmpty() && !Loadout->Equip(Staff) && Loadout->GetEquippedId(CCLItemTags::Slot_LeftHand) == Gauntlet &&
	               Loadout->GetEquippedId() == Shield && Bag->Find(Staff)->Slot >= 0 && Bonus() == 10.f,
	           TEXT("full bag rejects two-hand replacement without changing gear or effects")))
	{
		return false;
	}

	Bag->Remove(Fillers.Pop(), 1);
	if (!Check(Loadout->Equip(Staff) && Loadout->GetEquippedId() == Staff && Loadout->GetEquippedId(CCLItemTags::Slot_LeftHand) == Staff &&
	               Bag->Find(Gauntlet)->Slot >= 0 && Bag->Find(Shield)->Slot >= 0 && Bag->Find(Staff)->Slot == INDEX_NONE && Bonus() == 0.f,
	           TEXT("two-hand item returns both previous items atomically")))
	{
		return false;
	}

	if (!Check(!Loadout->Unequip(Staff) && Loadout->IsEquipped(Staff), TEXT("full bag rejects unequip without item loss")))
	{
		return false;
	}

	auto* Character = Cast<ACCLCharacter>(PC->GetPawn());
	TArray<UMeshComponent*> Meshes;
	Character->GetComponents(Meshes);
	int32 AttachedVisuals = 0;
	for (const auto* Mesh : Meshes)
	{
		if (Mesh->GetAttachParent() == Character->GetMesh() && Mesh->GetAttachSocketName().ToString().StartsWith(TEXT("CCL_")))
		{
			++AttachedVisuals;
			if (!Check(Mesh->GetAttachSocketName() == TEXT("CCL_Grip_R"), TEXT("two-hand mesh uses right grip socket")))
			{
				return false;
			}
		}
	}

	if (!Check(AttachedVisuals == 1, TEXT("two-hand occupancy creates only one attached mesh")))
	{
		return false;
	}

	FCCLSessionRecord Record, Decoded;
	Record.EquipmentSlots = Loadout->GetEquipment();
	for (const auto& Entry : Bag->GetEntries())
	{
		FCCLSavedItem Saved;
		Saved.Id = Entry.Id;
		Saved.Definition = Entry.Definition->GetFName();
		Saved.Quantity = Entry.Quantity;
		Saved.Slot = Entry.Slot;
		Record.Items.Add(Saved);
	}

	TArray<uint8> Bytes;
	if (!Check(FCCLSessionCodec::Encode(Record, Bytes) && FCCLSessionCodec::Decode(Bytes, Decoded) && Decoded.EquipmentSlots.Num() == 2,
	           TEXT("version 4 saves one two-hand item with two occupied slots")))
	{
		return false;
	}

	Decoded.EquipmentSlots.Pop();
	if (!Check(!FCCLSessionCodec::Validate(Decoded), TEXT("incomplete two-hand saved occupancy rejected")))
	{
		return false;
	}

	Bag->Remove(Fillers.Pop(), 1);
	if (!Check(Loadout->Unequip(Staff) && !Loadout->GetEquippedId().IsValid() &&
	               !Loadout->GetEquippedId(CCLItemTags::Slot_LeftHand).IsValid(),
	           TEXT("unequip clears both hand references")))
	{
		return false;
	}

	if (!Check(Loadout->Equip(Gauntlet, CCLItemTags::Slot_LeftHand) && Loadout->Equip(Gauntlet, CCLItemTags::Slot_RightHand) &&
	               !Loadout->GetEquippedId(CCLItemTags::Slot_LeftHand).IsValid() && Bonus() == 10.f,
	           TEXT("moving same item between hands does not duplicate effect")))
	{
		return false;
	}

	if (!Check(Bag->Restore({}) && Loadout->GetEquipment().IsEmpty() && Bonus() == 0.f,
	           TEXT("removing equipment clears references and effect")))
	{
		return false;
	}

	const FGuid RingA = Add(TEXT("DA_TrainingRing"));
	const FGuid RingB = Add(TEXT("DA_TrainingRing"));
	const FGuid Armor = Add(TEXT("DA_TrainingArmor"));
	if (!Check(RingA != RingB && Loadout->Equip(RingA, CCLItemTags::Slot_RingOne) && Loadout->Equip(RingB, CCLItemTags::Slot_RingTwo) &&
	               !Loadout->Equip(Armor, CCLItemTags::Slot_LeftHand) && Loadout->Equip(Armor),
	           TEXT("independent ring instances and armor slot restriction")))
	{
		return false;
	}

	const FGuid Guard = Add(TEXT("DA_TrainingShield"));
	if (!Check(Loadout->Equip(Guard, CCLItemTags::Slot_LeftHand), TEXT("shield equips left hand")))
	{
		return false;
	}

	PC->HandInput(CCLItemTags::Slot_LeftHand, true);
	if (!Check(ASC->HasMatchingGameplayTag(CCLTags::State_Guard), TEXT("left mouse hand dispatch starts shield guard")))
	{
		return false;
	}

	PC->HandInput(CCLItemTags::Slot_LeftHand, false);
	if (!Check(!ASC->HasMatchingGameplayTag(CCLTags::State_Guard), TEXT("left hand release stops guard")))
	{
		return false;
	}

	const FGuid TwoHand = Add(TEXT("DA_TrainingStaff"));
	if (!Check(Loadout->Equip(TwoHand) && Loadout->GetHandAction(CCLItemTags::Slot_LeftHand) == ECCLHandAction::Attack &&
	               Loadout->GetHandAction(CCLItemTags::Slot_RightHand) == ECCLHandAction::Guard,
	           TEXT("two-hand weapon has distinct left and right actions")))
	{
		return false;
	}

	PC->HandInput(CCLItemTags::Slot_RightHand, true);
	if (!Check(ASC->HasMatchingGameplayTag(CCLTags::State_Guard), TEXT("right hand dispatch uses two-hand secondary action")))
	{
		return false;
	}

	PC->HandInput(CCLItemTags::Slot_RightHand, false);
	return Check(Bag->Restore({}) && Loadout->GetEquipment().IsEmpty(), TEXT("equipment fixtures cleaned up"));
}

void UCCLUISmokeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bComplete)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}

	if (Now - Started >= 150.)
	{
		Check(false, TEXT("UI timeout"));
		return;
	}

	// Let the arrival cinematic finish before testing inventory pointer input.
	if (Now < Next || (Step == 0 && GetWorld()->GetTimeSeconds() < 6.f))
	{
		return;
	}

	auto* PC = Cast<ACCLPlayerController>(GetWorld()->GetFirstPlayerController());
	auto* Player = PC ? PC->GetPlayerState<ACCLPlayerState>() : nullptr;
	auto* Pawn = PC ? Cast<ACCLCharacter>(PC->GetPawn()) : nullptr;
	if (!Player || !Pawn || !PC->IsLocalController() || !PC->HasAuthority())
	{
		return;
	}

	auto* Inventory = Player->GetInventory();
	auto* UIManager = PC->GetLocalPlayer()->GetSubsystem<UCCLUISubsystem>();
	auto* Router = PC->GetLocalPlayer()->GetSubsystem<UCommonUIActionRouterBase>();
	auto* Session = GetWorld()->GetGameInstance<UCCLGameInstance>();

	auto Drag = [PC](int32 From, int32 To, bool bFromEquipment = false, bool bToEquipment = false) {
		const auto UI = PC->GetInventoryWidget();
		if (!UI)
		{
			return false;
		}

		const auto Source = bFromEquipment ? UI->GetEquipmentSlotWidget(From) : UI->GetSlotWidget(From);
		const auto Destination = bToEquipment ? UI->GetEquipmentSlotWidget(To) : UI->GetSlotWidget(To);
		if (!Source || (To >= 0 && !Destination))
		{
			return false;
		}

		const FVector2D Start = Source->GetCachedGeometry().GetAbsolutePosition() + Source->GetCachedGeometry().GetAbsoluteSize() * 0.5f;
		const FVector2D End =
		    Destination ? Destination->GetCachedGeometry().GetAbsolutePosition() + Destination->GetCachedGeometry().GetAbsoluteSize() * 0.5f
		                : UI->GetCachedGeometry().GetAbsolutePosition() + FVector2D(10.f, 10.f);
		auto& App = FSlateApplication::Get();
		const auto Window = App.FindWidgetWindow(Source.ToSharedRef());
		const TSet<FKey> Pressed = {EKeys::LeftMouseButton};
		App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
		                                FPointerEvent(0, Start, Start, Pressed, EKeys::LeftMouseButton, 0.f, FModifierKeysState()));
		App.ProcessMouseMoveEvent(
		    FPointerEvent(0, Start + FVector2D(16.f, 0.f), Start, Pressed, EKeys::Invalid, 0.f, FModifierKeysState()));
		const bool bDragging = App.IsDragDropping();
		App.ProcessMouseMoveEvent(FPointerEvent(0, End, Start + FVector2D(16.f, 0.f), Pressed, EKeys::Invalid, 0.f, FModifierKeysState()));
		App.ProcessMouseButtonUpEvent(FPointerEvent(0, End, End, TSet<FKey>(), EKeys::LeftMouseButton, 0.f, FModifierKeysState()));
		return bDragging;
	};

	if (Step == 0)
	{
		if (!Check(GetWorld()->WorldType == (FParse::Param(FCommandLine::Get(), TEXT("CCLUISmokePIE")) ? EWorldType::PIE : EWorldType::Game),
			TEXT("requested UI test world type is active")))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_UI back key=%s"), UCCLUIInputData::GetBackKeyLabel(this));
		if (!CheckEquipment(PC))
		{
			return;
		}

		Equipment = Inventory->Add(FCCLSessionCodec::Item(TEXT("DA_IronGauntlets")), 1);
		Potion = Inventory->Add(FCCLSessionCodec::Item(TEXT("DA_RecoveryPotion")), 3);
		if (!Check(Equipment.IsValid() && Potion.IsValid(), TEXT("fixture inventory")))
		{
			return;
		}

		FCCLSessionRecord Legacy, Decoded;
		Legacy.Version = 1;
		FCCLSavedItem Item;
		Item.Id = Equipment;
		Item.Definition = TEXT("DA_IronGauntlets");
		Item.Quantity = 1;
		Legacy.Items.Add(Item);
		TArray<uint8> Bytes;
		if (!Check(FCCLSessionCodec::Encode(Legacy, Bytes) && FCCLSessionCodec::Decode(Bytes, Decoded) && Decoded.Version == FCCLSessionRecord().Version &&
		               Decoded.Items[0].Slot == 0,
		           TEXT("legacy save upgrades to slots")))
		{
			return;
		}

		Decoded.Items[0].Slot = 16;
		if (!Check(!FCCLSessionCodec::Validate(Decoded), TEXT("out of range saved slot rejected")))
		{
			return;
		}

		Decoded.Items[0].Slot = 0;
		Item = Decoded.Items[0];
		Item.Id = FGuid::NewGuid();
		Decoded.Items.Add(Item);
		if (!Check(!FCCLSessionCodec::Validate(Decoded), TEXT("duplicate saved slots rejected")))
		{
			return;
		}

		FSlateApplication::Get().SetAllUserFocusToGameViewport();
		PC->ToggleInventory();
	}
	else if (Step == 1)
	{
		const auto* Screen = Cast<UCCLInventoryScreen>(UIManager->FindScreen(PC->GetInventoryHandle()));
		const auto* Context = Screen ? Cast<UCCLInventoryContext>(Screen->GetContext()) : nullptr;
		UE_LOG(LogTemp, Display, TEXT("CCL_UI inventory screen=%d context=%d usable=%d capture=%d preview=%d owner=%d mode=%d"),
			Screen != nullptr, Context != nullptr, Context && Context->IsUsable(), Context && Context->HasCapture(),
			Context && Context->GetPreview(), Screen && Screen->GetOwningLocalPlayer() == PC->GetLocalPlayer(), static_cast<int32>(Router->GetActiveInputMode()));
		if (!Check(Screen && Context && Context->IsUsable() && Context->HasCapture() && Context->GetPreview() &&
			Screen->GetOwningLocalPlayer() == PC->GetLocalPlayer() && Router->GetActiveInputMode() == ECommonInputMode::Menu,
			TEXT("inventory is owned by the player UI manager with a live preview and menu input")))
		{
			return;
		}

		Capture(TEXT("inventory-before"));
	}
	else if (Step == 2)
	{
		if (!Check(Drag(0, 15) && Inventory->Find(Equipment)->Slot == 15 && !Inventory->FindSlot(0),
		           TEXT("Slate mouse drag into empty slot")))
		{
			return;
		}

		Capture(TEXT("inventory-moved"));
	}
	else if (Step == 3)
	{
		if (!Check(Drag(15, 1) && Inventory->Find(Equipment)->Slot == 1 && Inventory->Find(Potion)->Slot == 15,
		           TEXT("Slate occupied slot swap")))
		{
			return;
		}
	}
	else if (Step == 4)
	{
		if (!Check(Drag(1, -1) && Inventory->Find(Equipment)->Slot == 1 && Inventory->GetEntries().Num() == 2,
		           TEXT("outside drop cancels without item loss")))
		{
			return;
		}

		if (!Check(Drag(1, 1, false, true), TEXT("Slate drag bag into right hand")))
		{
			return;
		}

		if (!Check(Player->GetLoadout()->GetEquippedId() == Equipment, TEXT("selected moved item equips")))
		{
			return;
		}

		Capture(TEXT("inventory-equipped"));
	}
	else if (Step == 5)
	{
		if (!Check(Drag(1, 3, true, false) && Inventory->Find(Equipment)->Slot == 3 && !Player->GetLoadout()->IsEquipped(Equipment),
		           TEXT("Slate drag equipment back into bag")))
		{
			return;
		}

		PC->CCLEquipmentDemo();
		const TArray<FCCLInventoryEntry> Demo = Inventory->GetEntries();
		for (const auto& Entry : Demo)
		{
			const FString Name = Entry.Definition->GetName();
			if (!Name.StartsWith(TEXT("DA_Training")) || Name == TEXT("DA_TrainingStaff"))
			{
				continue;
			}

			Player->GetLoadout()->Equip(Entry.Id,
			                            Name == TEXT("DA_TrainingShield") ? CCLItemTags::Slot_LeftHand : FGameplayTag());
		}

		const FGuid SecondRing = Inventory->Add(FCCLSessionCodec::Item(TEXT("DA_TrainingRing")), 1);
		if (!Check(Player->GetLoadout()->Equip(SecondRing, CCLItemTags::Slot_RingTwo) && Player->GetLoadout()->GetEquipment().Num() == 8,
		           TEXT("all eight equipment slots populated")))
		{
			return;
		}
	}
	else if (Step == 6)
	{
		Capture(TEXT("equipment-full"));
	}
	else if (Step == 7)
	{
		FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::I, FModifierKeysState(), 0, false, 0, 0));
		if (!Check(!PC->IsInventoryOpen() && !PC->bShowMouseCursor, TEXT("I closes inventory and restores game input")))
		{
			return;
		}

		TActorIterator<ACCLVillageSteward> Speaker(GetWorld());
		if (!Check(static_cast<bool>(Speaker), TEXT("dialogue NPC exists")))
		{
			return;
		}

		Pawn->SetActorLocation(Speaker->GetActorLocation() + FVector(0.f, -150.f, 0.f));
		PC->ServerTalkToSteward();
		if (!Check(PC->IsDialogueVisible() && !PC->GetDialogueName().IsEmpty() && !PC->GetDialogueText().IsEmpty(),
		           TEXT("NPC name and dialogue delivered")))
		{
			return;
		}
	}
	else if (Step == 8)
	{
		auto* Dialogue = Cast<UCCLDialogueScreen>(UIManager->FindScreen(PC->GetDialogueHandle()));
		if (!Check(Dialogue && Dialogue->GetDisplayedBody() == PC->GetDialogueText(),
			TEXT("independent dialogue widget renders server-delivered content")))
		{
			return;
		}

		auto* Context = Cast<UCCLDialogueContext>(Dialogue->GetContext());
		Context->Update(PC->GetDialogueName(), PC->GetDialogueText() + TEXT("\n대사 변경 알림을 확인하는 중이야."));
		if (!Check(Dialogue->GetDisplayedBody() == Context->Body, TEXT("dialogue change notification updates without recreating screen")))
		{
			return;
		}

		Context->Update(PC->GetDialogueName(), PC->GetDialogueText());
		Capture(TEXT("npc-dialogue"));
	}
	else if (Step == 9)
	{
		PC->ToggleInventory();
		if (!Check(!PC->IsDialogueVisible() && PC->IsInventoryOpen(), TEXT("inventory replaces dialogue without overlap")))
		{
			return;
		}

		PC->CloseInventory();
		PC->ServerTalkToSteward();
		Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(0.f, -800.f, 0.f));
		if (!Check(!PC->IsDialogueVisible(), TEXT("leaving interaction range closes dialogue")))
		{
			return;
		}

	}
	else if (Step == 10)
	{
		if (!Check(!UIManager->IsViewOpen(PC->GetDialogueHandle()), TEXT("controller closes out-of-range dialogue independently of painting")))
		{
			return;
		}

		PC->ToggleInventory();
		PooledScreen = Cast<UCCLInventoryScreen>(UIManager->FindScreen(PC->GetInventoryHandle()));
		RetainedContext = PooledScreen.IsValid() ? Cast<UCCLInventoryContext>(PooledScreen->GetContext()) : nullptr;
		if (!Check(RetainedContext && RetainedContext->HasCapture(), TEXT("reopened inventory owns a fresh capture")))
		{
			return;
		}
	}
	else if (Step == 11)
	{
		const auto Body = PC->GetInventoryWidget();
		const auto Source = Body ? Body->GetSlotWidget(Inventory->Find(Potion)->Slot) : nullptr;
		if (!Check(Source.IsValid(), TEXT("drag cancellation source is present")))
		{
			return;
		}

		auto& App = FSlateApplication::Get();
		const FVector2D Start = Source->GetCachedGeometry().GetAbsolutePosition() + Source->GetCachedGeometry().GetAbsoluteSize() * 0.5f;
		const auto Window = App.FindWidgetWindow(Source.ToSharedRef());
		const TSet<FKey> Pressed = {EKeys::LeftMouseButton};
		App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
			FPointerEvent(0, Start, Start, Pressed, EKeys::LeftMouseButton, 0.f, FModifierKeysState()));
		App.ProcessMouseMoveEvent(FPointerEvent(0, Start + FVector2D(16.f, 0.f), Start, Pressed, EKeys::Invalid, 0.f, FModifierKeysState()));
		if (!Check(App.IsDragDropping(), TEXT("real pointer drag remains active before manager close")))
		{
			return;
		}

		UIManager->CloseView(PC->GetInventoryHandle());
		App.ProcessMouseButtonUpEvent(FPointerEvent(0, Start, Start, TSet<FKey>(), EKeys::LeftMouseButton, 0.f, FModifierKeysState()));
		if (!Check(!PC->IsInventoryOpen() && !App.IsDragDropping() && !RetainedContext->HasCapture() &&
			!RetainedContext->GetPreview() && !RetainedContext->GetController(),
			TEXT("manager close cancels owned drag and releases capture, texture and controller context")))
		{
			return;
		}
	}
	else if (Step == 12)
	{
		PC->ToggleInventory();
		auto* Screen = Cast<UCCLInventoryScreen>(UIManager->FindScreen(PC->GetInventoryHandle()));
		auto* Context = Screen ? Cast<UCCLInventoryContext>(Screen->GetContext()) : nullptr;
		if (!Check(Screen == PooledScreen.Get() && Context && Context != RetainedContext && Context->HasCapture() &&
			Context->GetController() == PC && PC->GetSelectedItem() == 0, TEXT("pooled inventory screen binds a fresh selection and preview context")))
		{
			return;
		}

		RetainedContext = Context;
		TActorIterator<ACCLVillageSteward> Speaker(GetWorld());
		if (!Check(static_cast<bool>(Speaker), TEXT("NPC is available for inventory reply check")))
		{
			return;
		}

		const FVector PreviousLocation = Pawn->GetActorLocation();
		Pawn->SetActorLocation(Speaker->GetActorLocation() + FVector(0.f, -150.f, 0.f));
		PC->ServerTalkToSteward();
		if (!Check(PC->IsInventoryOpen() && !PC->IsDialogueVisible() && Context->HasCapture(),
			TEXT("late NPC response preserves the newer inventory and preview")))
		{
			return;
		}

		Pawn->SetActorLocation(PreviousLocation);
	}
	else if (Step == 13)
	{
		Session->ShowMenu();
		if (!Check(Session->IsMenuVisible() && !PC->IsInventoryOpen() && !RetainedContext->HasCapture(),
			TEXT("session menu replaces inventory through the same manager")))
		{
			return;
		}

		Capture(TEXT("managed-session-menu"));
		TActorIterator<ACCLVillageSteward> Speaker(GetWorld());
		if (!Check(static_cast<bool>(Speaker), TEXT("NPC is available for delayed reply check")))
		{
			return;
		}

		const FVector PreviousLocation = Pawn->GetActorLocation();
		Pawn->SetActorLocation(Speaker->GetActorLocation() + FVector(0.f, -150.f, 0.f));
		PC->ServerTalkToSteward();
		if (!Check(!PC->IsDialogueVisible() && Session->IsMenuVisible(),
			TEXT("server dialogue reply cannot open a hidden interaction behind the menu")))
		{
			return;
		}

		Pawn->SetActorLocation(PreviousLocation);
	}
	else if (Step == 14)
	{
		if (!Check(PC->bShowMouseCursor && PC->IsMoveInputIgnored() && PC->IsLookInputIgnored() &&
			Router->GetActiveInputMode() == ECommonInputMode::Menu, TEXT("session menu owns cursor and movement input policy")))
		{
			return;
		}

		FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(UCCLUIInputData::GetBackKey(this), FModifierKeysState(), 0, false, 0, 0));
		FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(UCCLUIInputData::GetBackKey(this), FModifierKeysState(), 0, false, 0, 0));
		if (!Check(!Session->IsMenuVisible(), TEXT("Back key closes the managed session menu")))
		{
			return;
		}
	}
	else if (Step == 15)
	{
		if (!Check(!PC->bShowMouseCursor && !PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored() &&
			Router->GetActiveInputMode() == ECommonInputMode::Game, TEXT("closing menu restores gameplay input without a second input owner")))
		{
			return;
		}

		FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(UCCLUIInputData::GetBackKey(this), FModifierKeysState(), 0, false, 0, 0));
	}
	else if (Step == 16)
	{
		FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(UCCLUIInputData::GetBackKey(this), FModifierKeysState(), 0, false, 0, 0));
		if (!Check(Session->IsMenuVisible(), TEXT("Back key from gameplay still opens the menu with UI mappings installed")))
		{
			return;
		}

		Session->HideMenu();
		PC->ToggleInventory();
		PreviousPawn = Pawn;
		Pawn->Die();
	}
	else if (Step == 17)
	{
		if (!Check(!PC->IsInventoryOpen() && !PC->GetEquipmentPreview(), TEXT("death releases the inventory screen and preview")))
		{
			return;
		}

		PC->CCLRetry();
	}
	else if (Step == 18)
	{
		if (Pawn == PreviousPawn.Get() || Pawn->IsDead())
		{
			return;
		}

		PC->ToggleInventory();
	}
	else if (Step == 19)
	{
		const auto* Screen = Cast<UCCLInventoryScreen>(UIManager->FindScreen(PC->GetInventoryHandle()));
		const auto* Context = Screen ? Cast<UCCLInventoryContext>(Screen->GetContext()) : nullptr;
		if (!Check(Context && Context->IsUsable() && Context->HasCapture() && Pawn != PreviousPawn.Get(),
			TEXT("respawn opens inventory against the new pawn without retaining the old preview")))
		{
			return;
		}

	}
	else if (Step == 20)
	{
		FString Error;
		OtherLocal = Session->CreateLocalPlayer(1, Error, true);
		auto* OtherPC = OtherLocal.IsValid() ? OtherLocal->GetPlayerController(GetWorld()) : nullptr;
		if (!Check(OtherPC && OtherPC != PC, TEXT("second local player created for feature isolation")))
		{
			return;
		}

		Session->ShowMenuForPlayer(OtherPC);
		if (!Check(Session->IsMenuVisibleForPlayer(OtherPC) && !Session->IsMenuVisibleForPlayer(PC) && PC->IsInventoryOpen(),
			TEXT("second player menu does not replace the first player's inventory")))
		{
			return;
		}
	}
	else if (Step == 21)
	{
		auto* OtherPC = OtherLocal.IsValid() ? OtherLocal->GetPlayerController(GetWorld()) : nullptr;
		Session->HideMenuForPlayer(OtherPC);
		if (!Check(!Session->IsMenuVisibleForPlayer(OtherPC) && PC->IsInventoryOpen(), TEXT("closing another player's menu preserves the first view")))
		{
			return;
		}

		Session->RemoveLocalPlayer(OtherLocal.Get());
		PC->CloseInventory();
		RetainedContext = nullptr;
	}
	else if (Step == 22)
	{
		auto* HUD = Cast<ACCLHUD>(PC->GetHUD());
		auto* Context = HUD ? HUD->GetHUDContext() : nullptr;
		auto* Screen = HUD ? Cast<UCCLVitalsScreen>(UIManager->FindScreen(HUD->GetVitalsHandle())) : nullptr;
		if (!Check(Context && Context->Vitals && Screen, TEXT("independent vitals widget survives respawn and local-player removal")))
		{
			return;
		}

		auto* ASC = Pawn->GetAbilitySystemComponent();
		const float Health = ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
		const int32 Before = Screen->GetRefreshCount();
		ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), Health - 10.f);
		if (!Check(Context->Vitals->Health == Health - 10.f && Screen->GetRefreshCount() > Before &&
			Screen->GetDisplayedText().Contains(FString::Printf(TEXT("체력 %.0f /"), Health - 10.f)),
			TEXT("GAS change publishes MVVM FieldNotify and updates displayed HP synchronously")))
		{
			return;
		}

		const int32 After = Screen->GetRefreshCount();
		Context->Vitals->Bind(ASC);
		if (!Check(Screen->GetRefreshCount() == After, TEXT("unchanged source binding does not poll the rendered values")))
		{
			return;
		}

		ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), Health);
		Capture(TEXT("managed-hud"));
	}
	else if (Step == 23)
	{
		auto* HUD = CastChecked<ACCLHUD>(PC->GetHUD());
		auto* Vitals = CastChecked<UCCLVitalsScreen>(UIManager->FindScreen(HUD->GetVitalsHandle()));
		FCCLUIPresentationDefinition Cutscene;
		Cutscene.Groups.AddTag(CCLUITags::Group_HUD);
		Cutscene.bHide = 1;
		Cutscene.bSuspendUpdates = 1;
		Cutscene.bBlockGameplay = 1;
		Cutscene.FadeSeconds = 0.3f;
		PresentationA = UIManager->PushPresentation(Cutscene, this);
		const int32 Before = Vitals->GetRefreshCount();
		auto* ASC = Pawn->GetAbilitySystemComponent();
		ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 70.f);
		if (!Check(PresentationA.IsValid() && Vitals->IsActivated() && !Vitals->IsPresentationInteractive() &&
			Vitals->GetRefreshCount() == Before && HUD->GetHUDContext()->Vitals->Health == 70.f,
			TEXT("cutscene suspends presentation updates but preserves live data and screen activation")))
		{
			return;
		}

		TActorIterator<ACCLVillageSteward> Speaker(GetWorld());
		Pawn->SetActorLocation(Speaker->GetActorLocation() + FVector(0.f, -150.f, 0.f));
		PC->ServerTalkToSteward();
	}
	else if (Step == 24)
	{
		auto* HUD = CastChecked<ACCLHUD>(PC->GetHUD());
		auto* Vitals = UIManager->FindScreen(HUD->GetVitalsHandle());
		auto* Dialogue = UIManager->FindScreen(PC->GetDialogueHandle());
		if (!Check(Vitals && Vitals->GetVisibility() == ESlateVisibility::Hidden && Dialogue && Dialogue->IsVisible() &&
			PC->IsMoveInputIgnored() && PC->IsLookInputIgnored() && !Router->CanProcessNormalGameInput(),
			TEXT("dialogue remains visible while the HUD group fades out and gameplay input is blocked")))
		{
			return;
		}

		Capture(TEXT("presentation-dialogue-only"));
		FCCLUIPresentationDefinition Nested;
		Nested.Groups.AddTag(CCLUITags::Group_HUD);
		Nested.bHide = 1;
		Nested.bSuspendUpdates = 1;
		Nested.bBlockGameplay = 1;
		PresentationB = UIManager->PushPresentation(Nested, this);
		UIManager->ReleasePresentation(PresentationA);
		UIManager->ReleasePresentation(PresentationA);
	}
	else if (Step == 25)
	{
		auto* HUD = CastChecked<ACCLHUD>(PC->GetHUD());
		auto* Vitals = UIManager->FindScreen(HUD->GetVitalsHandle());
		if (!Check(UIManager->GetPresentationCount() == 1 && Vitals->GetVisibility() == ESlateVisibility::Hidden &&
			UIManager->IsGameplayInputBlocked(), TEXT("releasing the first request twice preserves the overlapping request")))
		{
			return;
		}

		FCCLUIViewPresentation Base;
		Base.Opacity = 0.4f;
		UIManager->SetBasePresentation(HUD->GetVitalsHandle(), Base);
		UIManager->ReleasePresentation(PresentationB);
	}
	else if (Step == 26)
	{
		auto* HUD = CastChecked<ACCLHUD>(PC->GetHUD());
		auto* Vitals = CastChecked<UCCLVitalsScreen>(UIManager->FindScreen(HUD->GetVitalsHandle()));
		if (!Check(FMath::IsNearlyEqual(Vitals->GetRenderOpacity(), 0.4f) && Vitals->GetDisplayedText().Contains(TEXT("체력 70 /")) &&
			!UIManager->IsGameplayInputBlocked() && !PC->IsMoveInputIgnored(),
			TEXT("last release restores the current base opacity and latest data rather than an old snapshot")))
		{
			return;
		}

		FCCLUIPresentationDefinition Hide;
		Hide.Groups.AddTag(CCLUITags::Group_HUD);
		Hide.bHide = 1;
		Hide.FadeSeconds = 1.f;
		PresentationA = UIManager->PushPresentation(Hide, this);
	}
	else if (Step == 27)
	{
		auto* HUD = CastChecked<ACCLHUD>(PC->GetHUD());
		auto* Vitals = UIManager->FindScreen(HUD->GetVitalsHandle());
		FadeSample = Vitals->GetRenderOpacity();
		if (!Check(FadeSample > 0.f && FadeSample < 0.4f, TEXT("fade advances between the current and target opacity")))
		{
			return;
		}

		UIManager->ReleasePresentation(PresentationA);
	}
	else if (Step == 28)
	{
		auto* HUD = CastChecked<ACCLHUD>(PC->GetHUD());
		auto* Vitals = UIManager->FindScreen(HUD->GetVitalsHandle());
		if (!Check(Vitals->GetRenderOpacity() > FadeSample && Vitals->GetRenderOpacity() <= 0.4f &&
			!Vitals->IsPresentationInteractive(), TEXT("reversing an unfinished fade is continuous and keeps input disabled until completion")))
		{
			return;
		}
	}
	else if (Step == 29)
	{
		PC->CloseDialogue();
		PC->ToggleInventory();
		auto* Screen = CastChecked<UCCLInventoryScreen>(UIManager->FindScreen(PC->GetInventoryHandle()));
		RetainedContext = CastChecked<UCCLInventoryContext>(Screen->GetContext());
		FCCLUIPresentationDefinition HiddenInventory;
		HiddenInventory.Groups.AddTag(CCLUITags::Group_Menus);
		HiddenInventory.bHide = 1;
		HiddenInventory.bSuspendUpdates = 1;
		HiddenInventory.bBlockGameplay = 1;
		PresentationA = UIManager->PushPresentation(HiddenInventory, this);
		if (!Check(Screen->IsActivated() && PC->IsInventoryOpen() && RetainedContext->HasCapture() && !RetainedContext->IsPreviewRunning(),
			TEXT("temporary inventory hide preserves its stack entry and stops capture")))
		{
			return;
		}

		PreviousPawn = Pawn;
		Pawn->Die();
	}
	else if (Step == 30)
	{
		if (!Check(!PC->IsInventoryOpen() && !RetainedContext->HasCapture(), TEXT("hidden inventory still detects death and releases its resources")))
		{
			return;
		}

		UIManager->ReleasePresentation(PresentationA);
		PC->CCLRetry();
	}
	else if (Step == 31)
	{
		if (Pawn == PreviousPawn.Get() || Pawn->IsDead())
		{
			return;
		}

		FCCLUIPresentationDefinition Disable;
		Disable.Groups.AddTag(CCLUITags::Group_Menus);
		Disable.bDisableInput = 1;
		Disable.bBlockGameplay = 1;
		PresentationA = UIManager->PushPresentation(Disable, this);
		PC->ToggleInventory();
	}
	else if (Step == 32)
	{
		auto* Screen = UIManager->FindScreen(PC->GetInventoryHandle());
		if (!Check(Screen && Screen->IsVisible() && !Screen->IsPresentationInteractive(), TEXT("newly opened views inherit outstanding input requests")))
		{
			return;
		}

		FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(UCCLUIInputData::GetBackKey(this), FModifierKeysState(), 0, false, 0, 0));
		FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(UCCLUIInputData::GetBackKey(this), FModifierKeysState(), 0, false, 0, 0));
		if (!Check(PC->IsInventoryOpen(), TEXT("suppressed UI does not consume Back or leak it into gameplay")))
		{
			return;
		}

		UIManager->ReleasePresentation(PresentationA);
	}
	else if (Step == 33)
	{
		auto* Screen = UIManager->FindScreen(PC->GetInventoryHandle());
		if (!Check(Screen && Screen->IsPresentationInteractive() && Router->GetLeafmostActivatableWidget() == Screen &&
			FSlateApplication::Get().GetUserFocusedWidget(0) == PC->GetInventoryWidget(),
			TEXT("release restores the existing screen's input and Slate focus without popping it")))
		{
			return;
		}

		PC->CloseInventory();
		PresentationOwner = GetWorld()->SpawnActor<AActor>();
		FCCLUIPresentationDefinition Owned;
		Owned.bAllViews = 1;
		Owned.bHide = 1;
		Owned.bBlockGameplay = 1;
		PresentationA = UIManager->PushPresentation(Owned, PresentationOwner.Get());
		PresentationOwner->Destroy();
	}
	else if (Step == 34)
	{
		if (!Check(UIManager->GetPresentationCount() == 0 && !UIManager->IsGameplayInputBlocked(),
			TEXT("destroyed request owner releases visibility and gameplay restrictions")))
		{
			return;
		}

		RetainedContext = nullptr;
		FCCLUIPresentationDefinition Overlap;
		Overlap.bAllViews = 1;
		Overlap.bHide = 1;
		Overlap.bBlockGameplay = 1;
		PresentationA = UIManager->PushPresentation(Overlap, this);
		PresentationB = UIManager->PushPresentation(Overlap, this);
		UIManager->ReleasePresentation(PresentationB);
		if (!Check(UIManager->GetPresentationCount() == 1 && UIManager->IsGameplayInputBlocked(),
			TEXT("reverse release order also preserves the earlier request")))
		{
			return;
		}

		UIManager->ReleasePresentation(PresentationA);
		UE_LOG(LogTemp, Display, TEXT("CCL_UI PASS drag dialogue menu pooling respawn FieldNotify presentation overlap fade input owners"));
		bComplete = 1;
		FinishUITest(GetWorld(), 0);
	}

	++Step;
	Next = Now + ((Step == 27 || Step == 28) ? 0.2 : 2.);
}
