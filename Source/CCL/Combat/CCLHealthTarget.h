#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "CCLHealthTarget.generated.h"

class UCCLAbilitySystemComponent;
class UCCLHealthSet;
class UStaticMeshComponent;

UCLASS()
class CCL_API ACCLHealthTarget : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACCLHealthTarget();
	virtual void BeginPlay() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

private:
	void OnHealthChanged(const FOnAttributeChangeData& Data);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UCCLHealthSet> Health;
};
