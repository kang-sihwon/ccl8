#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "KSIHitRule.h"
#include "Components/ActorComponent.h"
#include "KSICombatProbe.generated.h"

class UKSIAbilitySystemComponent;
class UKSICombatComponent;
class UKSIActionComponent;

UCLASS()
class UKSIProbeHealth : public UAttributeSet
{
    GENERATED_BODY()
public:
    UPROPERTY()
    FGameplayAttributeData Health = FGameplayAttributeData(100.f);
};

UCLASS()
class UKSIProbeDamage : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UKSIProbeDamage();
};

UCLASS()
class UKSIProbeHitRule : public UKSIHitRule
{
    GENERATED_BODY()
public:
    virtual FKSIHitResolution Resolve(const FKSIHitContext& Context) const override;
    virtual void Commit(const FKSIHitContext& Context, const FKSIHitResolution& Resolution) const override;
};

UCLASS()
class UKSIProbeReceiver : public UActorComponent, public IKSIHitReceiver
{
    GENERATED_BODY()
public:
    virtual void ReceiveHit(const FKSIHitContext& Context, FGameplayTag Outcome) override;
public:
    int32 Hits = 0;
};

UCLASS()
class UKSIProbeAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UKSIProbeAbility();
    virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* EventData) override;
};

UCLASS()
class AKSIProbeTarget : public AActor, public IAbilitySystemInterface
{
    GENERATED_BODY()
public:
    AKSIProbeTarget();
    virtual void BeginPlay() override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
public:
    UPROPERTY()
    TObjectPtr<UKSIAbilitySystemComponent> ASC;
    UPROPERTY()
    TObjectPtr<UKSICombatComponent> Combat;
    UPROPERTY()
    TObjectPtr<UKSIActionComponent> Actions;
    UPROPERTY()
    TObjectPtr<UKSIProbeHealth> Health;
    UPROPERTY()
    TObjectPtr<UKSIProbeReceiver> Receiver;
};

bool RunKSICombatProbe(UWorld* World);
