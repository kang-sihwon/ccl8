#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CCLAgentFeatures.h"
#include "Combat/CCLHitRule.h"
#include "CCLAgentComponent.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

UCLASS(ClassGroup = Agent, meta = (BlueprintSpawnableComponent))
class CCL_API UCCLAgentComponent : public UActorComponent, public ICCLHitReceiver
{
	GENERATED_BODY()

public:
	UCCLAgentComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
	virtual void ReceiveHit(const FCCLHitContext& Context, FGameplayTag Outcome) override;

public:
	void RecordDamage(AActor* PerceivedSource, float HealthFraction);
	// Clears injury and safety urgency when the same record returns in a new body, for example a respawned enemy.
	void ClearInjury();
	void EnsureRecord();
	void RestoreHealthFromRecord();
	bool ShouldEngage() const;

	UPROPERTY(EditAnywhere)
	uint8 bCreateStandaloneRecord = 0;

	UPROPERTY(EditAnywhere)
	FCCLAgentTraits DefaultTraits;

	UPROPERTY(EditAnywhere, Replicated)
	FGuid AgentId;

private:
	void BindHealth();
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void RecordHealthDeath();
	TWeakObjectPtr<UAbilitySystemComponent> BoundHealthASC;
	FDelegateHandle HealthChanged;
	FGuid HealthEpoch;
	uint8 bHealthRestored = 0;
};
