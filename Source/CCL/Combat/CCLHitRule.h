#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "CCLHitRule.generated.h"

class UAbilitySystemComponent;
class UCCLCombatDefinition;

class UGameplayEffect;

struct FCCLHitResolution
{
	FCCLHitResolution() = default;
	FCCLHitResolution(FGameplayTag InOutcome) : Outcome(InOutcome) {}
	FCCLHitResolution(FGameplayTag InOutcome, TSubclassOf<UGameplayEffect> InEffect, FGameplayTag InMagnitudeTag, float InMagnitude)
		: Outcome(InOutcome), Effect(InEffect), MagnitudeTag(InMagnitudeTag), Magnitude(InMagnitude) {}

	FGameplayTag Outcome;
	TSubclassOf<UGameplayEffect> Effect;
	FGameplayTag MagnitudeTag;
	float Magnitude = 0.f;
};

struct FCCLHitContext
{
	AActor* Source = nullptr;
	AActor* Target = nullptr;
	UAbilitySystemComponent* SourceASC = nullptr;
	UAbilitySystemComponent* TargetASC = nullptr;
	const UCCLCombatDefinition* Definition = nullptr;
	FHitResult Hit;
	uint32 AttackId = 0;
	FVector IncomingDirection = FVector::ZeroVector;
	uint8 bDetachedShot = 0;
};

namespace CCLHit
{
CCL_API FGameplayTag Apply(const FCCLHitContext& Context);
}

UCLASS(Abstract)
class CCL_API UCCLHitRule : public UObject
{
	GENERATED_BODY()

public:
	virtual FCCLHitResolution Resolve(const FCCLHitContext& Context) const;
};

UCLASS()
class CCL_API UCCLDuelHitRule : public UCCLHitRule
{
	GENERATED_BODY()

public:
	virtual FCCLHitResolution Resolve(const FCCLHitContext& Context) const override;
};
