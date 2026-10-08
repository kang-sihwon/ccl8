#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/Core/CCLUITypes.h"
#include "CCLHUD.generated.h"

class UCCLHUDContext;

UCLASS()
class CCL_API ACCLHUD : public AHUD
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	ACCLHUD();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UCCLHUDContext* GetHUDContext() const { return Context; }
	FCCLUIViewHandle GetVitalsHandle() const { return VitalsHandle; }

private:
	void RefreshContent();

private:
	UPROPERTY(Transient)
	TObjectPtr<UCCLHUDContext> Context;

	FCCLUIViewHandle VitalsHandle;
	FCCLUIViewHandle FieldHandle;
};
