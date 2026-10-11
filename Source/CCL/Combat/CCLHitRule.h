#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "UObject/Interface.h"
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
	FGameplayEffectSpecHandle CapturedEffect;
	int32 CapturedTeam = INDEX_NONE;
};

namespace CCLHit
{
CCL_API FGameplayTag Apply(const FCCLHitContext& Context);
}

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UCCLHitReceiver : public UInterface
{
	GENERATED_BODY()
};

// Implemented by components that react to hits on their owner, such as memory or quest bookkeeping.
// Combat announces results through this contract instead of calling those systems directly.
class CCL_API ICCLHitReceiver
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	// Called on the authority after a hit against the owner was applied and committed.
	virtual void ReceiveHit(const FCCLHitContext& Context, FGameplayTag Outcome) = 0;
};

// Content policy of a hit. Resolve decides from the current state without changing it.
// Commit applies the decision's side effects only after its damage effect, if any, was applied.
UCLASS(Abstract)
class CCL_API UCCLHitRule : public UObject
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	virtual FCCLHitResolution Resolve(const FCCLHitContext& Context) const;
	virtual void Commit(const FCCLHitContext& Context, const FCCLHitResolution& Resolution) const;
};

UCLASS()
class CCL_API UCCLDuelHitRule : public UCCLHitRule
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	virtual FCCLHitResolution Resolve(const FCCLHitContext& Context) const override;
	virtual void Commit(const FCCLHitContext& Context, const FCCLHitResolution& Resolution) const override;
};
