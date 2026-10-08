#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

class APlayerController;
class UCCLUISubsystem;

namespace CCLUITags
{
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Layer_HUD);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Layer_Panels);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Layer_Menu);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Layer_Notifications);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(View_Inventory);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(View_SessionMenu);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(View_Vitals);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(View_FieldHUD);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(View_Dialogue);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(View_Minimap);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(View_WorldMap);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(View_Cinematic);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Group_HUD);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Group_Dialogue);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Group_Menus);
}

// Content composition belongs outside the reusable UI Core.
namespace CCLGameUI
{
CCL_API UCCLUISubsystem* Get(APlayerController* Controller);
}
