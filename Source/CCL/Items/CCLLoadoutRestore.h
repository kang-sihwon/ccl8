#pragma once

#include "CoreMinimal.h"
#include "CCLInventoryComponent.h"
#include "CCLItemFragments.h"

class UCCLLoadoutComponent;
class UCCLSkillDefinition;

struct FCCLLoadoutRestoreRequest
{
	UCCLLoadoutComponent* Loadout = nullptr;
	TArray<FCCLInventoryEntry> Entries;
	TArray<FCCLEquippedSlot> Equipment;
	TArray<UCCLSkillDefinition*> Skills;
	int32 Points = 0;
	uint8 bResetHealth = 1;
};

// Synchronous game-thread transaction. Callbacks must not destroy participants,
// change their candidates/effects, or run gameplay/GC. CommitWorld preserves its
// own state on failure; FinalizeProfiles cannot fail and runs before notifications.
class CCL_API FCCLLoadoutRestoreBatch
{
public:
	static bool Execute(TConstArrayView<FCCLLoadoutRestoreRequest> Requests, TFunctionRef<bool()> CommitWorld,
						TFunctionRef<void()> FinalizeProfiles);
};
