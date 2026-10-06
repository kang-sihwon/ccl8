#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "CCLEnemyCharacter.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FCCLEnemyDefeated, class ACCLEnemyCharacter*);

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
	void SelectAttackPattern();
	UPROPERTY(EditAnywhere, Replicated, Category = "Encounter")
	uint8 Archetype = 0;
	UPROPERTY(Replicated)
	FString PatternLabel;
	bool IsDead() const { return bDead != 0; }
	FVector GetHome() const { return SpawnTransform.GetLocation(); }

private:
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void Respawn();

	UFUNCTION()
	void OnRep_Dead();

public:
	UPROPERTY(EditAnywhere, Category = "Encounter")
	uint8 bRespawnEnabled = 1;

	UPROPERTY(EditAnywhere, Replicated, Category = "Encounter")
	FString DisplayName = TEXT("Enemy");

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float DetectionRadius = 1400.f;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float LeashRadius = 1800.f;

	FCCLEnemyDefeated OnDefeated;

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

	uint8 bAlternateAttack = 0;
	FTransform SpawnTransform;
	FTimerHandle RespawnTimer;
	FDelegateHandle HealthChanged;
};
