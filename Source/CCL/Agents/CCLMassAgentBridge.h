#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Mass/EntityHandle.h"
#include "CCLAgentTypes.h"
#include "CCLDecision.h"
#include "CCLMassAgentBridge.generated.h"

struct FMassEntityManager;

// Small compatibility adapter. It intentionally supplies no Mass movement or GAS integration.
USTRUCT()
struct CCL_API FCCLMassAgentFragment : public FMassFragment
{
	GENERATED_BODY()

	FCCLPersistentIntent Intent;
	FCCLAgentLease Lease;
};

namespace CCLMassAgentBridge
{
CCL_API FMassEntityHandle Enter(FMassEntityManager& Manager, FCCLAgentStore& Store, FGuid Id);
CCL_API FCCLDecisionResult Decide(FMassEntityManager& Manager, FMassEntityHandle Entity, const FCCLDecisionInput& Input);
CCL_API bool Leave(FMassEntityManager& Manager, FMassEntityHandle Entity, FCCLAgentStore& Store,
	const FCCLFeatureRegistry& Registry, FString& Error);
}
