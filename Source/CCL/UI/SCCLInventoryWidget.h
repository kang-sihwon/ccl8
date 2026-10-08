#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"

class ACCLPlayerController;

class SCCLInventoryWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCCLInventoryWidget) {}
	SLATE_ARGUMENT(TWeakObjectPtr<ACCLPlayerController>, Controller)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	static void CancelOwnedDrag(ACCLPlayerController* Owner);
	TSharedPtr<SWidget> GetSlotWidget(int32 Slot) const;
	TSharedPtr<SWidget> GetEquipmentSlotWidget(int32 Slot) const;

private:
	TWeakObjectPtr<ACCLPlayerController> Controller;
	TArray<TSharedPtr<SWidget>> Slots;
	TArray<TSharedPtr<SWidget>> EquipmentSlots;
	FSlateBrush CharacterBrush;
};
