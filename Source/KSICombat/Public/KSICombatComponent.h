#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KSIHitRule.h"
#include "KSICombatComponent.generated.h"

class UKSICombatDefinition;

DECLARE_MULTICAST_DELEGATE_TwoParams(FKSIHitResolved, AActor*, FGameplayTag);

UCLASS(ClassGroup = Combat, meta = (BlueprintSpawnableComponent))
class KSICOMBAT_API UKSICombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKSICombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	uint32 BeginAttack(const UKSICombatDefinition* Definition, const FVector& Direction);
	void EndAttack();
	FGameplayTag ResolveHit(AActor* Target, const FHitResult& Hit, uint32 AttackId);
	uint32 GetAttackId() const { return CurrentAttackId; }

public:
	FKSIHitResolved OnHitResolved;

private:
	UPROPERTY(Transient)
	TObjectPtr<const UKSICombatDefinition> ActiveDefinition;

	TSet<TWeakObjectPtr<AActor>> HitActors;
	FVector AttackDirection = FVector::ForwardVector;
	uint32 CurrentAttackId = 0;
	uint8 bResolvingHit = 0;
};
