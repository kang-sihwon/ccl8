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
	bool CanReach(const APawn* Visitor) const;
};
