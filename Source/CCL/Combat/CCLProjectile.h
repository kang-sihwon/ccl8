#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UCCLCombatDefinition;
class UAbilitySystemComponent;

UCLASS()
class CCL_API ACCLProjectile : public AActor
{
	GENERATED_BODY()

public:
	ACCLProjectile();
	virtual void BeginPlay() override;

public:
	void Launch(UAbilitySystemComponent* ASC, const UCCLCombatDefinition* Definition, FVector Velocity, float Gravity);

private:
	UFUNCTION()
	void Impact(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComponent,
		FVector Impulse, const FHitResult& Hit);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY()
	TObjectPtr<UCCLCombatDefinition> ShotDefinition;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> SourceASC;

	uint8 bResolved = 0;
};
