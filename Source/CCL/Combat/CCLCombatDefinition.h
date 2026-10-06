#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "CCLCombatDefinition.generated.h"

class UCCLHitRule;
class UAnimMontage;

UCLASS()
class CCL_API UCCLCombatDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UCCLCombatDefinition();

public:
	UPROPERTY(EditAnywhere, Category = "Attack")
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float Cost = 15.f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float Windup = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float Active = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float Recovery = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Trace")
	float Reach = 150.f;

	UPROPERTY(EditAnywhere, Category = "Trace")
	float Radius = 35.f;

	UPROPERTY(EditAnywhere, Category = "Defense")
	uint8 bGuardable = 1;

	UPROPERTY(EditAnywhere, Category = "Defense")
	uint8 bParryable = 1;

	UPROPERTY(EditAnywhere, Category = "Effect")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, Category = "Effect")
	FGameplayTag MagnitudeTag;

	UPROPERTY(EditAnywhere, Category = "Policy")
	TSubclassOf<UCCLHitRule> HitRule;

	UPROPERTY(EditAnywhere, Category = "Presentation")
	TObjectPtr<UAnimMontage> Montage;
};
