#include "KSIUIActionRouter.h"

#include "KSIUISubsystem.h"
#include "Engine/LocalPlayer.h"

bool UKSIUIActionRouter::CanProcessNormalGameInput() const
{
	const auto* UI = GetLocalPlayer()->GetSubsystem<UKSIUISubsystem>();
	return (!UI || !UI->IsGameplayInputBlocked()) && Super::CanProcessNormalGameInput();
}

void UKSIUIActionRouter::RefreshPresentationRouting()
{
	if (auto ActiveRoot = GetActiveRoot().Pin())
	{
		// Re-evaluate the existing tree without deactivating/popping any screen.
		SetActiveRoot(ActiveRoot);
	}
}
