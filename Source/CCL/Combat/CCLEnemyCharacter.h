#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "CCLEnemyCharacter.generated.h"

class UCCLAbilitySystemComponent;
class UCCLHealthSet;
class UCCLFighterComponent;
class UCCLCombatComponent;

UCLASS()
class CCL_API ACCLEnemyCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACCLEnemyCharacter();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	bool IsDead() const { return bDead != 0; }
	FVector GetHome() const { return SpawnTransform.GetLocation(); }

private:
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void Respawn();

	UFUNCTION()
	void OnRep_Dead();

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UCCLHealthSet> Health;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLFighterComponent> Fighter;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLCombatComponent> Combat;

	UPROPERTY(ReplicatedUsing = OnRep_Dead)
	uint8 bDead = 0;

	FTransform SpawnTransform;
	FTimerHandle RespawnTimer;
	FDelegateHandle HealthChanged;
};
