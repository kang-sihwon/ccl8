#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "UObject/Interface.h"
#include "StructUtils/InstancedStruct.h"
#include "KSIHitRule.generated.h"

class UAbilitySystemComponent;
class UKSICombatDefinition;

class UGameplayEffect;

struct KSICOMBAT_API FKSIHitResolution
{
	FKSIHitResolution() = default;
	FKSIHitResolution(FGameplayTag InOutcome) : Outcome(InOutcome) {}
	FKSIHitResolution(FGameplayTag InOutcome, TSubclassOf<UGameplayEffect> InEffect, FGameplayTag InMagnitudeTag, float InMagnitude)
		: Outcome(InOutcome), Effect(InEffect), MagnitudeTag(InMagnitudeTag), Magnitude(InMagnitude) {}

	FGameplayTag Outcome;
	TSubclassOf<UGameplayEffect> Effect;
	FGameplayTag MagnitudeTag;
	float Magnitude = 0.f;
};

struct KSICOMBAT_API FKSIHitContext
{
	AActor* Source = nullptr;
	AActor* Target = nullptr;
	UAbilitySystemComponent* SourceASC = nullptr;
	UAbilitySystemComponent* TargetASC = nullptr;
	const UKSICombatDefinition* Definition = nullptr;
	FHitResult Hit;
	uint32 AttackId = 0;
	FVector IncomingDirection = FVector::ZeroVector;
	uint8 bDetachedSource = 0;
	FGameplayEffectSpecHandle CapturedEffect;
	// Optional game-owned facts copied for this hit; the common resolver does not interpret them.
	FInstancedStruct SourceFacts;
};

namespace KSIHit
{
KSICOMBAT_API FGameplayTag Apply(const FKSIHitContext& Context);
}

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UKSIHitReceiver : public UInterface
{
	GENERATED_BODY()
};

// Implemented by components that react to hits on their owner, such as memory or quest bookkeeping.
// Combat announces results through this contract instead of calling those systems directly.
class KSICOMBAT_API IKSIHitReceiver
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	// Called on the authority after a hit against the owner was applied and committed.
	virtual void ReceiveHit(const FKSIHitContext& Context, FGameplayTag Outcome) = 0;
};

// Content policy of a hit. Resolve decides from the current state without changing it.
// Commit applies the decision's side effects only after its damage effect, if any, was applied.
UCLASS(Abstract)
class KSICOMBAT_API UKSIHitRule : public UObject
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	virtual FKSIHitResolution Resolve(const FKSIHitContext& Context) const;
	virtual void Commit(const FKSIHitContext& Context, const FKSIHitResolution& Resolution) const;
};
