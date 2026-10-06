#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CCLHitRule.h"
#include "CCLCombatComponent.generated.h"

class UCCLCombatDefinition;

DECLARE_MULTICAST_DELEGATE_TwoParams(FCCLHitResolved, AActor*, FGameplayTag);

UCLASS(ClassGroup = Combat, meta = (BlueprintSpawnableComponent))
class CCL_API UCCLCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCLCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	uint32 BeginAttack(const UCCLCombatDefinition* Definition, const FVector& Direction);
	void EndAttack();
	FGameplayTag ResolveHit(AActor* Target, const FHitResult& Hit, uint32 AttackId);
	uint32 GetAttackId() const { return CurrentAttackId; }

public:
	FCCLHitResolved OnHitResolved;

private:
	UPROPERTY(Transient)
	TObjectPtr<const UCCLCombatDefinition> ActiveDefinition;

	TSet<TWeakObjectPtr<AActor>> HitActors;
	FVector AttackDirection = FVector::ForwardVector;
	uint32 CurrentAttackId = 0;
};
