#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CCLEquipmentPresenter.generated.h"

class UCCLItemDefinition;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UCCLEquipmentPresenter : public UInterface
{
	GENERATED_BODY()
};

// Implemented by a body component that presents equipment and runs hand actions.
// The equipment owner talks to this contract instead of a concrete combat component.
class CCL_API ICCLEquipmentPresenter
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	// Shows the hand items on the current body and selects the active weapon from them.
	virtual void PresentEquipment(UCCLItemDefinition* LeftHand, UCCLItemDefinition* RightHand) = 0;

	// Selects the item whose action the next hand input performs. Null selects the body's fallback item.
	virtual void SelectActiveItem(UCCLItemDefinition* Item) = 0;
};
