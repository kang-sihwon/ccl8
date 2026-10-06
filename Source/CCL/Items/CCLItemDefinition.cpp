#include "CCLItemDefinition.h"

const UCCLItemFragment* UCCLItemDefinition::FindFragment(TSubclassOf<UCCLItemFragment> Type) const
{
	for (const UCCLItemFragment* Fragment : Fragments)
	{
		if (Fragment && Fragment->IsA(Type))
		{
			return Fragment;
		}
	}

	return nullptr;
}
