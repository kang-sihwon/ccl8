#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Mass/EntityHandle.h"
#include "KSIAgentTypes.h"
#include "CCLDecision.h"
#include "CCLMassAgentBridge.generated.h"

struct FMassEntityManager;

// Small compatibility adapter. It intentionally supplies no Mass movement or GAS integration.
USTRUCT()
struct CCL_API FCCLMassAgentFragment : public FMassFragment
{
	GENERATED_BODY()

	FKSIPersistentIntent Intent;
	FKSIAgentLease Lease;
};

namespace CCLMassAgentBridge
{
CCL_API FMassEntityHandle Enter(FMassEntityManager& Manager, FKSIAgentStore& Store, FGuid Id);
CCL_API FCCLDecisionResult Decide(FMassEntityManager& Manager, FMassEntityHandle Entity, const FCCLDecisionInput& Input);
CCL_API bool Leave(FMassEntityManager& Manager, FMassEntityHandle Entity, FKSIAgentStore& Store,
	const FKSIFeatureRegistry& Registry, FString& Error);
}
