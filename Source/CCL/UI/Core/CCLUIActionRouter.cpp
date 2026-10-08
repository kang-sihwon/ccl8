#include "CCLUIActionRouter.h"

#include "CCLUISubsystem.h"
#include "Engine/LocalPlayer.h"

bool UCCLUIActionRouter::CanProcessNormalGameInput() const
{
	const auto* UI = GetLocalPlayer()->GetSubsystem<UCCLUISubsystem>();
	return (!UI || !UI->IsGameplayInputBlocked()) && Super::CanProcessNormalGameInput();
}

void UCCLUIActionRouter::RefreshPresentationRouting()
{
	if (auto ActiveRoot = GetActiveRoot().Pin())
	{
		// Re-evaluate the existing tree without deactivating/popping any screen.
		SetActiveRoot(ActiveRoot);
	}
}
