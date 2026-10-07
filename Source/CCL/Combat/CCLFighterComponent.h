#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystemComponent.h"
#include "CCLFighterComponent.generated.h"

class UCCLAbilitySystemComponent;
class UCCLItemDefinition;
class UCCLAttachmentProfile;
class UCCLCombatDefinition;
class UAnimMontage;
class UMeshComponent;

UENUM()
enum class ECCLCombatAction : uint8 { None, Attack, Dodge, Guard, Parry };

UCLASS(ClassGroup = Combat, meta = (BlueprintSpawnableComponent))
class CCL_API UCCLFighterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCLFighterComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void Initialize(UCCLAbilitySystemComponent* InASC);
	void EndLife();
	UFUNCTION()
	void RefreshEquipmentVisuals();
	void SetAction(ECCLCombatAction NewAction);
	void LockAttackDirection();
	void NotifyHit(FGameplayTag Outcome);
	UCCLAbilitySystemComponent* GetASC() const { return ASC; }
	const UCCLCombatDefinition* GetAttack() const;
	ECCLCombatAction GetAction() const { return Action; }
	const FString& GetFeedback() const { return Feedback; }
	bool IsDead() const;

private:
	void OnStaminaChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void OnRep_Action();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFeedback(FGameplayTag Outcome);

public:
	UPROPERTY(EditAnywhere, Category = "Presentation")
	TSoftObjectPtr<UCCLAttachmentProfile> AttachmentProfile;

	UPROPERTY(EditAnywhere, Replicated, Category = "Definition")
	TObjectPtr<UCCLItemDefinition> Item;
	UPROPERTY(ReplicatedUsing = RefreshEquipmentVisuals)
	TObjectPtr<UCCLItemDefinition> LeftHandItem;

	UPROPERTY(ReplicatedUsing = RefreshEquipmentVisuals)
	TObjectPtr<UCCLItemDefinition> RightHandItem;

	UPROPERTY(Replicated)
	TObjectPtr<UCCLCombatDefinition> AttackOverride;

	UPROPERTY(EditAnywhere, Category = "Definition")
	int32 Team = 0;

	UPROPERTY(EditAnywhere, Category = "Resources")
	float InitialHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "Resources")
	float InitialStamina = 100.f;

	UPROPERTY(EditAnywhere, Category = "Resources")
	float RegenRate = 20.f;

	UPROPERTY(EditAnywhere, Category = "Resources")
	float RegenDelay = 1.f;

	UPROPERTY(EditAnywhere, Category = "Defense")
	float DefenseAngle = 120.f;

	UPROPERTY(EditAnywhere, Category = "Defense")
	float GuardCost = 30.f;

	UPROPERTY(EditAnywhere, Category = "Defense")
	float GuardBreakDuration = 1.f;

	UPROPERTY(EditAnywhere, Category = "Defense")
	float ParryStaggerDuration = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	float DodgeCost = 25.f;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	float DodgeDistance = 400.f;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	float DodgeMoveDuration = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	float DodgeDuration = 0.55f;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	float InvulnerableStart = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Dodge")
	float InvulnerableEnd = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Parry")
	float ParryCost = 15.f;

	UPROPERTY(EditAnywhere, Category = "Parry")
	float ParryStart = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Parry")
	float ParryEnd = 0.28f;

	UPROPERTY(EditAnywhere, Category = "Parry")
	float ParryDuration = 0.65f;

	UPROPERTY(EditAnywhere, Category = "Presentation")
	TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditAnywhere, Category = "Presentation")
	TObjectPtr<UAnimMontage> DefenseMontage;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCCLAbilitySystemComponent> ASC;

	UPROPERTY(Transient)

	TArray<TObjectPtr<UMeshComponent>> EquipmentVisuals;

	UPROPERTY(ReplicatedUsing = OnRep_Action)
	ECCLCombatAction Action = ECCLCombatAction::None;

	FDelegateHandle StaminaChanged;
	FString Feedback;
	double FeedbackExpires = 0.;
	uint8 bAttackDirectionLocked = 0;
};
