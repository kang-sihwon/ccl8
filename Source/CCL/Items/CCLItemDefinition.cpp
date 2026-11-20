#include "CCLItemDefinition.h"

bool UCCLItemDefinition::ConfigurePerishable(double ShelfLifeSeconds, double MoldDelaySeconds, double MoldGrowthSeconds)
{
	FCCLPerishableFragment Food;
	Food.Policy.ShelfLifeSeconds = ShelfLifeSeconds;
	Food.Policy.MoldDelaySeconds = MoldDelaySeconds;
	Food.Policy.MoldGrowthSeconds = MoldGrowthSeconds;
	if (!CCLFood::ValidatePolicy(Food.Policy))
	{
		return false;
	}

	ItemFragments.RemoveAll([](const FInstancedStruct& Fragment)
							{ return Fragment.GetPtr<FCCLPerishableFragment>() != nullptr; });
	ItemFragments.Add(FInstancedStruct::Make(Food));
	return true;
}
