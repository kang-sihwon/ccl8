#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CCLHUD.generated.h"

class UCCLCombatViewModel;

UCLASS()
class CCL_API ACCLHUD : public AHUD
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	virtual void DrawHUD() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCCLCombatViewModel> ViewModel;
};
