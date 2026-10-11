#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Agents/CCLAccountComponent.h"
#include "CCLVillageSteward.generated.h"
UCLASS()
class CCL_API ACCLVillageSteward : public ACharacter
{
	GENERATED_BODY()

public:
	ACCLVillageSteward();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool CanReach(const APawn* Visitor) const;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText DisplayName = NSLOCTEXT("CCL", "VillageSteward", "마을 관리인");

	UPROPERTY(EditAnywhere, Replicated, Category = "Shop")
	FCCLShopOffer Offer;
};
