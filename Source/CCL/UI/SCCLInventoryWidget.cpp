#include "SCCLInventoryWidget.h"

#include "CCLCharacter.h"
#include "CCLPlayerController.h"
#include "CCLPlayerState.h"
#include "Input/DragAndDrop.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLSkillDefinition.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
const FCCLInventoryEntry* InventorySlotEntry(const TWeakObjectPtr<ACCLPlayerController>& PC, int32 Slot)
{
	const auto* State = PC.IsValid() ? PC->GetPlayerState<ACCLPlayerState>() : nullptr;
	return State ? State->GetInventory()->FindSlot(Slot) : nullptr;
}

const TCHAR* EquipmentSlotLabel(int32 Slot)
{
	const TCHAR* Names[] = {TEXT("LEFT HAND"), TEXT("RIGHT HAND"), TEXT("ARMOR"),  TEXT("BOOTS"),
	                        TEXT("CLOAK"),     TEXT("NECKLACE"),   TEXT("RING 1"), TEXT("RING 2")};
	return Slot >= 0 && Slot < 8 ? Names[Slot] : TEXT("EQUIPMENT");
}

class FCCLItemDrag : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FCCLItemDrag, FDragDropOperation)
	TWeakObjectPtr<ACCLPlayerController> Owner;
	FGuid Id;
	FText Label;

	static TSharedRef<FCCLItemDrag> New(ACCLPlayerController* PC, FGuid ItemId, FText Name)
	{
		auto Op = MakeShared<FCCLItemDrag>();
		Op->Owner = PC;
		Op->Id = ItemId;
		Op->Label = Name;
		Op->bCreateNewWindow = false;
		Op->Construct();
		return Op;
	}

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override
	{
		return SNew(SBorder).Padding(12.f)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 18)).Text(Label)];
	}
};

class SCCLItemSlot : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCCLItemSlot) : _Index(0), _Equipment(0) {}
	SLATE_ARGUMENT(TWeakObjectPtr<ACCLPlayerController>, Controller)
	SLATE_ARGUMENT(int32, Index)
	SLATE_ARGUMENT(uint8, Equipment)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		PC = Args._Controller;
		Index = Args._Index;
		bEquipment = Args._Equipment;
		SetCursor(EMouseCursor::Hand);
		ChildSlot[
			SNew(SBox).WidthOverride(120.f).HeightOverride(100.f)
			[SNew(SBorder).Padding(6.f).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor_Lambda([this]() {
				const auto* Entry = GetEntry();
				const bool bSelected = PC.IsValid() && (bEquipment ? Entry && PC->GetSelectedEquipment() == Entry->Id
					: !PC->GetSelectedEquipment().IsValid() && PC->GetSelectedItem() == Index);
				return bDropTarget || bSelected ? FLinearColor(0.24f, 0.32f, 0.18f) : FLinearColor(0.085f, 0.11f, 0.15f);
			})
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				.Text(FText::FromString(bEquipment ? EquipmentSlotLabel(Index) : FString::Printf(TEXT("%02d"), Index + 1)))]
				+ SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Center)
				[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).WrapTextAt(108.f)
				.Text_Lambda([this]() {
					const auto* Entry = GetEntry();
					if (!Entry || !Entry->Definition)
					{
						return bEquipment ? FText::FromString(TEXT("Empty")) : FText::GetEmpty();
					}

					FString Name = Entry->Definition->GetLabel().ToString();
					const int32 Stats = Name.Find(TEXT(" ("));
					if (Stats != INDEX_NONE)
					{
						Name.LeftInline(Stats);
					}

					return FText::FromString(bEquipment ? Name : FString::Printf(TEXT("%s\nx%d"), *Name, Entry->Quantity));
				})]
			]]
		];
	}

	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		if (Event.GetEffectingButton() != EKeys::LeftMouseButton || !PC.IsValid())
		{
			return FReply::Unhandled();
		}

		if (bEquipment)
		{
			PC->SelectEquipmentSlot(CCLEquipment::SlotAt(Index));
		}
		else
		{
			PC->SelectInventorySlot(Index);
		}

		const auto* Entry = GetEntry();
		PressedId = Entry ? Entry->Id : FGuid();
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}

	virtual FReply OnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		const auto* Entry = GetEntry();
		if (!Entry || Entry->Id != PressedId || !Entry->Definition || !PC->IsInventoryOpen())
		{
			return FReply::Unhandled();
		}

		return FReply::Handled().BeginDragDrop(FCCLItemDrag::New(PC.Get(), Entry->Id, Entry->Definition->GetLabel()));
	}

	virtual void OnDragEnter(const FGeometry& Geometry, const FDragDropEvent& Event) override
	{
		const auto Op = Event.GetOperationAs<FCCLItemDrag>();
		bDropTarget = Op && Op->Owner == PC;
	}

	virtual void OnDragLeave(const FDragDropEvent& Event) override { bDropTarget = 0; }
	virtual FReply OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event) override
	{
		bDropTarget = 0;
		const auto Op = Event.GetOperationAs<FCCLItemDrag>();
		if (!Op || Op->Owner != PC || !PC.IsValid() || !PC->IsInventoryOpen())
		{
			return FReply::Unhandled();
		}

		if (bEquipment)
		{
			PC->ServerEquipToSlot(Op->Id, CCLEquipment::SlotAt(Index));
		}
		else if (const auto* State = PC->GetPlayerState<ACCLPlayerState>(); State && State->GetLoadout()->IsEquipped(Op->Id))
		{
			PC->ServerUnequipToBag(Op->Id, Index);
		}
		else
		{
			PC->ServerMoveInventoryItem(Op->Id, Index);
		}

		if (bEquipment)
		{
			PC->SelectEquipmentSlot(CCLEquipment::SlotAt(Index));
		}
		else
		{
			PC->SelectInventorySlot(Index);
		}

		return FReply::Handled();
	}

private:
	const FCCLInventoryEntry* GetEntry() const
	{
		if (!bEquipment)
		{
			return InventorySlotEntry(PC, Index);
		}

		const auto* State = PC.IsValid() ? PC->GetPlayerState<ACCLPlayerState>() : nullptr;
		return State ? State->GetInventory()->Find(State->GetLoadout()->GetEquippedId(CCLEquipment::SlotAt(Index))) : nullptr;
	}

	TWeakObjectPtr<ACCLPlayerController> PC;
	uint8 bEquipment = 0;
	int32 Index = 0;
	FGuid PressedId;
	uint8 bDropTarget = 0;
};
} // namespace

void SCCLInventoryWidget::Construct(const FArguments& Args)
{
	Controller = Args._Controller;
	auto Body = SNew(SVerticalBox);
	Body->AddSlot().AutoHeight().Padding(
	    0.f, 0.f, 0.f,
	    8.f)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 24)).Text(FText::FromString(TEXT("INVENTORY & TRAINING")))];
	Body->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[SNew(STextBlock)
	                                                             .Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
	                                                             .Text(FText::FromString(TEXT("Drag to move / swap | I or Esc: close")))];
	auto Grid = SNew(SUniformGridPanel).SlotPadding(3.f);
	for (int32 Index = 0; Index < 16; ++Index)
	{
		TSharedRef<SWidget> Slot = SNew(SCCLItemSlot).Controller(Controller).Index(Index);
		Slots.Add(Slot);
		Grid->AddSlot(Index % 4, Index / 4)[Slot];
	}

	Body->AddSlot().AutoHeight()[Grid];
	auto Buttons = SNew(SHorizontalBox);
	auto AddButton = [this, &Buttons](const TCHAR* Label, void (ACCLPlayerController::*Action)()) {
		Buttons->AddSlot().FillWidth(1.f).Padding(
		    3.f)[SNew(SButton).IsFocusable(false).ContentPadding(6.f).OnClicked_Lambda([Weak = Controller, Action]() {
			if (Weak.IsValid())
			{
				(Weak.Get()->*Action)();
			}

			return FReply::Handled();
		})[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(FText::FromString(Label))]];
	};
	AddButton(TEXT("F Equip"), &ACCLPlayerController::EquipSelectedItem);
	AddButton(TEXT("G Unequip"), &ACCLPlayerController::UnequipItem);
	AddButton(TEXT("H Use"), &ACCLPlayerController::UseSelectedItem);
	Body->AddSlot().AutoHeight().Padding(0.f, 6.f)[Buttons];
	Body->AddSlot().AutoHeight()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text_Lambda([Weak = Controller]() {
		const auto* State = Weak.IsValid() ? Weak->GetPlayerState<ACCLPlayerState>() : nullptr;
		return FText::FromString(State ? FString::Printf(TEXT("Training points: %d"), State->GetLoadout()->GetPoints())
		                               : TEXT("Loading..."));
	})];
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Body->AddSlot().AutoHeight().Padding(
		    0.f, 3.f)[SNew(SButton).IsFocusable(false).ContentPadding(4.f).OnClicked_Lambda([Weak = Controller, Index]() {
			if (Weak.IsValid())
			{
				if (Index == 0)
				{
					Weak->LearnFirstSkill();
				}
				else
				{
					Weak->LearnSecondSkill();
				}
			}

			return FReply::Handled();
		})[SNew(STextBlock)
		       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
		       .AutoWrapText(true)
		       .Text_Lambda([Weak = Controller, Index]() {
			       const auto* State = Weak.IsValid() ? Weak->GetPlayerState<ACCLPlayerState>() : nullptr;
			       if (!State || !State->GetLoadout()->GetSkills().IsValidIndex(Index))
			       {
				       return FText::FromString(TEXT("Loading..."));
			       }

			       const auto* Skill = State->GetLoadout()->GetSkills()[Index].Get();
			       return FText::FromString(FString::Printf(TEXT("%d: %s%s"), Index + 1, *Skill->Label.ToString(),
			                                                State->GetLoadout()->IsLearned(Skill) ? TEXT(" [learned]") : TEXT(" (1 pt)")));
		       })]];
	}

	Body->AddSlot().AutoHeight().Padding(
	    0.f,
	    6.f)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).AutoWrapText(true).Text_Lambda([Weak = Controller]() {
		const auto* State = Weak.IsValid() ? Weak->GetPlayerState<ACCLPlayerState>() : nullptr;
		return FText::FromString(State ? State->GetLoadout()->GetResult() : FString());
	})];
	CharacterBrush.SetResourceObject(Controller.IsValid() ? Controller->GetEquipmentPreview() : nullptr);
	CharacterBrush.ImageSize = FVector2D(320.f, 480.f);
	CharacterBrush.DrawAs = ESlateBrushDrawType::Image;
	auto Left = SNew(SVerticalBox);
	auto Right = SNew(SVerticalBox);
	EquipmentSlots.SetNum(8);
	const int32 LeftSlots[] = {0, 2, 4, 6};
	const int32 RightSlots[] = {1, 3, 5, 7};
	for (int32 Row = 0; Row < 4; ++Row)
	{
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const int32 Index = Side == 0 ? LeftSlots[Row] : RightSlots[Row];
			TSharedRef<SWidget> Slot = SNew(SCCLItemSlot).Controller(Controller).Index(Index).Equipment(1);
			EquipmentSlots[Index] = Slot;
			(Side == 0 ? Left : Right)->AddSlot().AutoHeight().Padding(3.f, 8.f)[Slot];
		}
	}

	auto Equipment =
	    SNew(SVerticalBox) +
	    SVerticalBox::Slot().AutoHeight().Padding(10.f)
	        [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 24)).Text(FText::FromString(TEXT("CHARACTER & EQUIPMENT")))] +
	    SVerticalBox::Slot().AutoHeight().Padding(
	        10.f, 0.f)[SNew(STextBlock)
	                       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
	                       .AutoWrapText(true)
	                       .Text(FText::FromString(TEXT("Drag gear to a slot. Two-hand gear occupies both hands.")))] +
	    SVerticalBox::Slot().FillHeight(1.f).VAlign(
	        VAlign_Center)[SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Left] +
	                       SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(
	                           VAlign_Center)[SNew(SBox).WidthOverride(320.f).HeightOverride(480.f)[SNew(SImage).Image(&CharacterBrush)]] +
	                       SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Right]] +
	    SVerticalBox::Slot().AutoHeight().Padding(
	        10.f)[SNew(STextBlock)
	                  .Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
	                  .AutoWrapText(true)
	                  .Text(FText::FromString(
	                      TEXT("LMB: left-hand action | RMB: right-hand action\nDrag equipped gear to an empty bag slot to remove it.")))];
	ChildSlot[SNew(SBorder)
	              .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
	              .BorderBackgroundColor(FLinearColor(0.012f, 0.018f, 0.028f, 0.97f))[SNew(SScaleBox).Stretch(
	                  EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1280.f).HeightOverride(
	                  720.f)[SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.f).Padding(16.f)[Equipment] +
	                         SHorizontalBox::Slot().AutoWidth().Padding(0.f, 16.f, 16.f, 16.f)[SNew(SBox).WidthOverride(
	                             548.f)[SNew(SBorder)
	                                        .Padding(12.f)
	                                        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
	                                        .BorderBackgroundColor(FLinearColor(0.025f, 0.04f, 0.065f,
	                                                                            0.97f))[SNew(SScrollBox) + SScrollBox::Slot()[Body]]]]]]]];
}

FReply SCCLInventoryWidget::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (!Controller.IsValid())
	{
		return FReply::Handled();
	}

	const FKey Key = Event.GetKey();
	if (Key == EKeys::I || Key == EKeys::Escape)
	{
		Controller->CloseInventory();
	}
	else if (Key == EKeys::Up)
	{
		Controller->SelectInventorySlot(FMath::Max(0, Controller->GetSelectedItem() - 4));
	}
	else if (Key == EKeys::Down)
	{
		Controller->SelectInventorySlot(FMath::Min(15, Controller->GetSelectedItem() + 4));
	}
	else if (Key == EKeys::Left)
	{
		Controller->SelectPreviousItem();
	}
	else if (Key == EKeys::Right)
	{
		Controller->SelectNextItem();
	}
	else if (Key == EKeys::F)
	{
		Controller->EquipSelectedItem();
	}
	else if (Key == EKeys::G)
	{
		Controller->UnequipItem();
	}
	else if (Key == EKeys::H)
	{
		Controller->UseSelectedItem();
	}
	else if (Key == EKeys::One)
	{
		Controller->LearnFirstSkill();
	}
	else if (Key == EKeys::Two)
	{
		Controller->LearnSecondSkill();
	}

	return FReply::Handled();
}

void SCCLInventoryWidget::Tick(const FGeometry& Geometry, double Time, float Delta)
{
	SCompoundWidget::Tick(Geometry, Time, Delta);
	if (Controller.IsValid())
	{
		Controller->UpdateEquipmentPreview();
	}

	const auto* Pawn = Controller.IsValid() ? Cast<ACCLCharacter>(Controller->GetPawn()) : nullptr;
	if (Controller.IsValid() && (!Pawn || Pawn->IsDead()))
	{
		Controller->CloseInventory();
	}
}

TSharedPtr<SWidget> SCCLInventoryWidget::GetSlotWidget(int32 Slot) const
{
	return Slots.IsValidIndex(Slot) ? Slots[Slot] : nullptr;
}

TSharedPtr<SWidget> SCCLInventoryWidget::GetEquipmentSlotWidget(int32 Slot) const
{
	return EquipmentSlots.IsValidIndex(Slot) ? EquipmentSlots[Slot] : nullptr;
}
