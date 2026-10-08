#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLWorkshop.generated.h"
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class CCL_API ACCLWorkshop : public AActor
{
	GENERATED_BODY()

public:
	ACCLWorkshop();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	FGuid OwnershipId;
private:
	UFUNCTION()
	void Refresh();
	UPROPERTY(ReplicatedUsing = Refresh)
	int32 Level = 0;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Store;
	UPROPERTY()
	TObjectPtr<UTextRenderComponent> Label;
};
