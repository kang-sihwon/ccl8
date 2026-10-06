#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "CCLCharacter.generated.h"

class UCCLFighterComponent;
class UCCLCombatComponent;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class CCL_API ACCLCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	ACCLCharacter();

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 내 클래스 함수
public:
	void Die();

	bool IsDead() const { return bDead != 0; }

private:
	UFUNCTION()
	void OnRep_Dead();

	void InitializeAbilitySystem();
	void OnHealthChanged(const FOnAttributeChangeData& Data);

	// 프로퍼티
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLFighterComponent> Fighter;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> FacingMarker;

	UPROPERTY(ReplicatedUsing = OnRep_Dead)
	uint8 bDead = 0;

	FDelegateHandle HealthChanged;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
};
