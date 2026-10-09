#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CCLVillageSteward.generated.h"
UCLASS()
class CCL_API ACCLVillageSteward : public ACharacter
{
	GENERATED_BODY()

public:
	ACCLVillageSteward();
	virtual void Tick(float DeltaSeconds) override;
	bool CanReach(const APawn* Visitor) const;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText DisplayName = NSLOCTEXT("CCL", "VillageSteward", "마을 관리인");
};
