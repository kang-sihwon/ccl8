#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "CCLEnemyCharacter.generated.h"

class ACCLEnemyCharacter;
class UCCLAbilitySystemComponent;
class UCCLHealthSet;
class UCCLFighterComponent;
class UKSICombatComponent;
class UCCLEnemyDefinition;

DECLARE_MULTICAST_DELEGATE_OneParam(FCCLEnemyDefeated, ACCLEnemyCharacter*);

UCLASS()
class CCL_API ACCLEnemyCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	ACCLEnemyCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 내 클래스 함수
public:
	// Applies an enemy type before FinishSpawning or BeginPlay. Instance overrides may follow it.
	void ApplyDefinition(UCCLEnemyDefinition* InDefinition);
	void SelectAttackPattern();

	const UCCLEnemyDefinition* GetDefinition() const { return Definition; }
	bool IsDead() const { return bDead != 0; }
	FVector GetHome() const { return SpawnTransform.GetLocation(); }

private:
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void Respawn();
	void CopyConfigurationTo(ACCLEnemyCharacter& Replacement) const;

	UFUNCTION()
	void OnRep_Dead();

	// 프로퍼티
public:
	UPROPERTY(EditAnywhere, Category = "Encounter")
	uint8 bRespawnEnabled = 1;

	UPROPERTY(EditAnywhere, Replicated, Category = "Encounter")
	FString DisplayName = TEXT("적");

	UPROPERTY(Replicated)
	FString PatternLabel;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float DetectionRadius = 1400.f;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float LeashRadius = 1800.f;

	FCCLEnemyDefeated OnDefeated;

private:
	UPROPERTY(EditAnywhere, Replicated, Category = "Encounter")
	TObjectPtr<UCCLEnemyDefinition> Definition;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UCCLHealthSet> Health;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLFighterComponent> Fighter;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UKSICombatComponent> Combat;

	UPROPERTY(ReplicatedUsing = OnRep_Dead)
	uint8 bDead = 0;

	int32 PatternCursor = INDEX_NONE;
	uint8 bDefinitionApplied = 0;
	FTransform SpawnTransform;
	FTimerHandle RespawnTimer;
	FDelegateHandle HealthChanged;
};
