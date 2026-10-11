#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "KSIActionComponent.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct KSICOMBAT_API FKSIActionGrant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag Action;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayAbility> Ability;
};

USTRUCT()
struct KSICOMBAT_API FKSIActionSource
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;

	UPROPERTY()
	TObjectPtr<UObject> Definition;

	UPROPERTY()
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> Handles;
};

UCLASS(BlueprintType)
class KSICOMBAT_API UKSIActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<FKSIActionGrant> Actions;
};

// Sources may be items, innate abilities or external interactions. GAS owns execution lifetime.
UCLASS(ClassGroup = Actions, meta = (BlueprintSpawnableComponent))
class KSICOMBAT_API UKSIActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKSIActionComponent();
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

public:
	bool RegisterSource(FGuid Id, UObject* Definition, const TArray<FKSIActionGrant>& Grants);
	void RemoveSource(FGuid Id);
	bool Execute(FGuid Source, FGameplayTag Action);
	void RequestInput(FGameplayTag Input, bool bPressed);
	void Cancel(FGuid Source);
	void Release(FGuid Source, FGameplayTag Action);
	bool HasSource(FGuid Id) const;
	const FKSIActionSource* FindSource(FGameplayAbilitySpecHandle Ability) const;
	UAbilitySystemComponent* GetASC() const;

private:
	UPROPERTY()
	TArray<FKSIActionSource> Sources;
};
