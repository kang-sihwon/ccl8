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

FText UCCLItemDefinition::GetLabel() const
{
	const auto* Display = Cast<UCCLItemFragment_Display>(FindFragment(UCCLItemFragment_Display::StaticClass()));
	return Display ? Display->Label : FText::FromString(GetName());
}

int32 UCCLItemDefinition::GetMaxStack() const
{
	const auto* Stack = Cast<UCCLItemFragment_Stack>(FindFragment(UCCLItemFragment_Stack::StaticClass()));
	return Stack ? FMath::Clamp(Stack->MaxCount, 1, 1000) : 1;
}
