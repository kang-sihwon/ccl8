#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "CCLPlayerState.generated.h"

class UCCLAbilitySystemComponent;
class UCCLHealthSet;
class UCCLStaminaSet;

UCLASS()
class CCL_API ACCLPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACCLPlayerState();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	UCCLAbilitySystemComponent* GetCCLAbilitySystem() const { return AbilitySystem; }

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UCCLHealthSet> Health;

	UPROPERTY()
	TObjectPtr<UCCLStaminaSet> Stamina;
};
