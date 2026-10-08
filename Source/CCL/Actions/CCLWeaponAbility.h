#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "NativeGameplayTags.h"
#include "CCLWeaponAbility.generated.h"

class UCCLCombatDefinition;
class UCCLItemDefinition;
class ACCLProjectile;

namespace CCLActionTags
{
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fire);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reload);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aim);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aiming);
}

UCLASS(BlueprintType)
class CCL_API UCCLProjectileProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UCCLCombatDefinition> Combat;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ACCLProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "1"))
	float Speed = 6000;

	UPROPERTY(EditAnywhere)
	float Gravity = 1;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.05"))
	float FireInterval = 0.5f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.05"))
	float ReloadSeconds = 3;

	// Zero supports a projectile source without a magazine, such as a magical turret.
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "1000"))
	int32 MagazineSize = 0;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCCLItemDefinition> Ammunition;
};

UCLASS()
class CCL_API UCCLProjectileAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCCLProjectileAbility();
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* EventData) override;

private:
	UFUNCTION()
	void Finish();
};

UCLASS()
class CCL_API UCCLReloadAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCCLReloadAbility();
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* EventData) override;

private:
	UFUNCTION()
	void CompleteReload();
};

UCLASS()
class CCL_API UCCLAimAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCCLAimAbility();
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* EventData) override;
};
