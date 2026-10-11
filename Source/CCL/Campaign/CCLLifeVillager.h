#pragma once

#include "CoreMinimal.h"
#include "CCLVillageSteward.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Agents/CCLAgentEmbodiment.h"
#include "CCLLifeVillager.generated.h"

class UCCLAgentComponent;
class UCCLAbilitySystemComponent;
class UCCLHealthSet;
class UKSIActionComponent;
class UTextRenderComponent;

UCLASS()
class CCL_API ACCLLifeVillager : public ACCLVillageSteward, public IAbilitySystemInterface, public ICCLAgentEmbodiment
{
	GENERATED_BODY()

public:
	ACCLLifeVillager();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UCCLAgentComponent* GetAgentComponent() const override { return Agent; }
	virtual void SetAgentActivity(FGameplayTag Activity) override { SetActivity(Activity); }
	virtual void SetAgentPublicName(const FString& Name) override;

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
	TObjectPtr<UKSIActionComponent> Actions;

	UPROPERTY()
	TObjectPtr<UCCLHealthSet> Health;
};
