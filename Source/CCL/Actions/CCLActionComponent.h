#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "CCLActionComponent.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct FCCLActionGrant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag Action;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayAbility> Ability;
};

USTRUCT()
struct FCCLActionSource
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
class CCL_API UCCLActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<FCCLActionGrant> Actions;
};

// Sources may be items, innate abilities or external interactions. GAS owns execution lifetime.
UCLASS(ClassGroup = Actions, meta = (BlueprintSpawnableComponent))
class CCL_API UCCLActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCLActionComponent();
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

public:
	bool RegisterSource(FGuid Id, UObject* Definition, const TArray<FCCLActionGrant>& Grants);
	void RemoveSource(FGuid Id);
	bool Execute(FGuid Source, FGameplayTag Action);
	void Cancel(FGuid Source);
	void Release(FGuid Source, FGameplayTag Action);
	bool HasSource(FGuid Id) const;
	const FCCLActionSource* FindSource(FGameplayAbilitySpecHandle Ability) const;
	UAbilitySystemComponent* GetASC() const;

private:
	UPROPERTY()
	TArray<FCCLActionSource> Sources;
};
