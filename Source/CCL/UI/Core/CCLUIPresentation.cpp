#include "CCLUIPresentation.h"

bool FCCLUIPresentationDefinition::IsValid() const
{
	if ((!bAllViews && Groups.IsEmpty() && !bBlockGameplay) || !FMath::IsFinite(Opacity) || Opacity < 0.f || Opacity > 1.f ||
		!FMath::IsFinite(FadeSeconds) || FadeSeconds < 0.f)
	{
		return false;
	}

	for (const auto& Group : Groups)
	{
		if (!Group.ToString().StartsWith(TEXT("UI.Group.")))
		{
			return false;
		}
	}

	return true;
}

bool FCCLUIPresentationDefinition::Matches(const FGameplayTagContainer& ViewGroups) const
{
	return bAllViews || ViewGroups.HasAny(Groups);
}
