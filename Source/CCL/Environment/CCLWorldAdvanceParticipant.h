#pragma once

#include "CoreMinimal.h"

struct FKSIWorldStep;
class FKSISurfaceSimulation;

// Optional environment-time extension. Save-only participants need not implement it.
class CCL_API ICCLWorldAdvanceParticipant
{
public:
	virtual ~ICCLWorldAdvanceParticipant() = default;
	virtual int32 GetWorldAdvanceOrder() const = 0;
	// Time advance of section state that depends on the environment, such as stored food.
	virtual bool NeedsWorldAdvance() const = 0;
	virtual bool PrepareWorldAdvance(const FKSIWorldStep& Step, const FKSISurfaceSimulation& Surface, FString& Error) = 0;
	virtual void CommitWorldAdvance() = 0;
	virtual void PublishWorldAdvance() = 0;
	virtual void CancelWorldAdvance() = 0;
};
