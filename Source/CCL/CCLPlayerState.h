#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "CCLPlayerState.generated.h"

class UCCLAbilitySystemComponent;
class UCCLHealthSet;
class UCCLStaminaSet;
class UCCLOffenseSet;
class UCCLInventoryComponent;
class UCCLLoadoutComponent;
class UCCLExpeditionComponent;
class UCCLActionComponent;

UCLASS()
class CCL_API ACCLPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACCLPlayerState();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	UCCLAbilitySystemComponent* GetCCLAbilitySystem() const { return AbilitySystem; }
	UCCLInventoryComponent* GetInventory() const { return Inventory; }
	UCCLLoadoutComponent* GetLoadout() const { return Loadout; }

public:
	UCCLExpeditionComponent* GetExpedition() const { return Expedition; }
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLActionComponent> Actions;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLExpeditionComponent> Expedition;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UCCLHealthSet> Health;

	UPROPERTY()
	TObjectPtr<UCCLStaminaSet> Stamina;

	UPROPERTY()
	TObjectPtr<UCCLOffenseSet> Offense;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLLoadoutComponent> Loadout;
};
