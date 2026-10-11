#pragma once

#include "CoreMinimal.h"
#include "CCLFoodState.h"
#include "CCLFoodEnvironment.h"

class UCCLInventoryComponent;
class UKSIItemDefinition;
class UCCLItemDefinition;
struct FKSIInventoryEntry;

// Food commands compose inventory storage with the world environment.
class CCL_API FCCLInventoryFood
{
public:
	static bool FoodContext(const UCCLInventoryComponent& Inventory, FGuid& WorldId, double& WorldSeconds);
	static bool BuildFoodAddition(const UCCLInventoryComponent& Inventory, TArray<FKSIInventoryEntry>& Entries, UKSIItemDefinition* Definition,
	const FCCLFoodState& SourceFood, double WorldSeconds, FGuid& AddedId);
	static FGuid AddFood(UCCLInventoryComponent& Inventory, UKSIItemDefinition* Definition, const FCCLFoodState& Food);
	static bool MergeFood(UCCLInventoryComponent& Inventory, FGuid SourceId, FGuid DestinationId);
	static FGuid SplitFood(UCCLInventoryComponent& Inventory, FGuid SourceId, int32 Count, int32 Slot);
	static bool TransferFood(UCCLInventoryComponent& Inventory, FGuid SourceId, int32 Count, UCCLInventoryComponent* Destination);
	static bool ChangeFoodStorage(UCCLInventoryComponent& Inventory, FGuid Id, const FCCLFoodStorage& Storage);
	static bool GetFoodQuality(const UCCLInventoryComponent& Inventory, FGuid Id, FCCLFoodQuality& Quality);
	static bool ChangeFoodEnvironment(UCCLInventoryComponent& Inventory, const FCCLFoodEnvironmentBinding& Binding, FString& Error);
};
