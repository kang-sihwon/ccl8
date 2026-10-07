#include "CCLUISmokeSubsystem.h"

#include "CCLCharacter.h"
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
#include "UnrealClient.h"
#include "Widgets/SWindow.h"

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
		FPlatformMisc::RequestExitWithStatus(false, 1);
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

	if (Now - Started >= 90.)
	{
		Check(false, TEXT("UI timeout"));
		return;
	}

	if (Now < Next)
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
		if (!Check(FCCLSessionCodec::Encode(Legacy, Bytes) && FCCLSessionCodec::Decode(Bytes, Decoded) && Decoded.Version == 4 &&
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

		PC->ToggleInventory();
	}
	else if (Step == 1)
	{
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

		UE_LOG(LogTemp, Display, TEXT("CCL_UI PASS drag swap cancel dialogue"));
		bComplete = 1;
		FPlatformMisc::RequestExitWithStatus(false, 0);
	}

	++Step;
	Next = Now + 2.;
}
