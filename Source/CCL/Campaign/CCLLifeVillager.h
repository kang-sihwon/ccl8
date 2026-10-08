#pragma once

#include "CoreMinimal.h"
#include "CCLVillageSteward.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "CCLLifeVillager.generated.h"

class UCCLAgentComponent;
class UCCLAbilitySystemComponent;
class UCCLHealthSet;
class UCCLActionComponent;
class UTextRenderComponent;

UCLASS()
class CCL_API ACCLLifeVillager : public ACCLVillageSteward, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACCLLifeVillager();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	void SetActivity(FGameplayTag Activity);
	FString DescribeLife() const;
	UCCLAgentComponent* GetAgent() const { return Agent; }

private:
	UFUNCTION()
	void RefreshLabel();

public:
	UPROPERTY(ReplicatedUsing = RefreshLabel)
	FString PublicName;

private:
	UPROPERTY(ReplicatedUsing = RefreshLabel)
	FGameplayTag PublicActivity;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLAgentComponent> Agent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLActionComponent> Actions;

	UPROPERTY()
	TObjectPtr<UCCLHealthSet> Health;
};
