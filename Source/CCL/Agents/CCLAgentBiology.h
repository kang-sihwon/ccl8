#pragma once

#include "CoreMinimal.h"
#include "KSIAgentTypes.h"
#include "CCLAgentBiology.generated.h"

UENUM(BlueprintType)
enum class ECCLAgentDeathCause : uint8
{
	None,
	NaturalAge,
	External
};

USTRUCT(BlueprintType)
struct CCL_API FCCLAgentBiology
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Life")
	double BirthWorldSeconds = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Life")
	double MaturitySeconds = 0.;

	// Zero disables natural lifetime expiry without disabling external death.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Life")
	double LifespanSeconds = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Life")
	uint8 bProtectNaturalAging = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Life")
	ECCLAgentDeathCause DeathCause = ECCLAgentDeathCause::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Life")
	double DeathWorldSeconds = 0.;
};

namespace CCLAgentBiology
{
CCL_API const FCCLAgentBiology* Find(const FKSIAgentRecord& Record);
CCL_API bool ValidateFields(const FCCLAgentBiology& Biology);
CCL_API bool ValidateAt(const FKSIAgentRecord& Record, double WorldSeconds, FString& Error);
CCL_API bool IsAlive(const FKSIAgentRecord& Record);
CCL_API double AgeSeconds(const FCCLAgentBiology& Biology, double WorldSeconds);
}
