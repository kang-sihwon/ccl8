#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UI/Core/CCLUIPresentation.h"
#include "CCLUISmokeSubsystem.generated.h"

class ACCLPlayerController;
class ACCLCharacter;
class UCCLInventoryScreen;
class UCCLInventoryContext;
class ULocalPlayer;

UCLASS()
class CCL_API UCCLUISmokeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	bool Check(bool Condition, const TCHAR* Message);
	void Capture(const TCHAR* Name);
	bool CheckEquipment(ACCLPlayerController* PC);

private:
	UPROPERTY()
	TObjectPtr<UCCLInventoryContext> RetainedContext;

	TWeakObjectPtr<UCCLInventoryScreen> PooledScreen;
	TWeakObjectPtr<ACCLCharacter> PreviousPawn;
	TWeakObjectPtr<ULocalPlayer> OtherLocal;
	TWeakObjectPtr<AActor> PresentationOwner;
	FCCLUIPresentationHandle PresentationA;
	FCCLUIPresentationHandle PresentationB;
	float FadeSample = 0.f;
	int32 Step = 0;
	FGuid Equipment;
	FGuid Potion;
	double Next = 0.;
	double Started = 0.;
	uint8 bComplete = 0;
};
