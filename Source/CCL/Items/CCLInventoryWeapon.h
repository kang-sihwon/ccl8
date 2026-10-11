#pragma once

#include "CoreMinimal.h"

class UCCLInventoryComponent;
struct FKSIInventoryEntry;

class CCL_API FCCLInventoryWeapon
{
public:
	static bool ConsumeShot(UCCLInventoryComponent& Inventory, FGuid Id);
	static bool Reload(UCCLInventoryComponent& Inventory, FGuid Id);
	static bool CanFire(const UCCLInventoryComponent& Inventory, FGuid Id);
	static bool CanReload(const UCCLInventoryComponent& Inventory, FGuid Id);
	static bool Validate(const FKSIInventoryEntry& Entry);
};
