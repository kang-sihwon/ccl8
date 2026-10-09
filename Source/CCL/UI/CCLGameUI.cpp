#include "CCLGameUI.h"

#include "CCLInventoryScreen.h"
#include "CCLSessionMenuScreen.h"
#include "CCLUIInputData.h"
#include "CCLHUDScreens.h"
#include "CCLMapScreen.h"
#include "Presentation/CCLCinematicSubsystem.h"
#include "Core/CCLUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

namespace CCLUITags
{
UE_DEFINE_GAMEPLAY_TAG(View_Cinematic, "UI.View.Cinematic");
UE_DEFINE_GAMEPLAY_TAG(View_Minimap, "UI.View.Minimap");
UE_DEFINE_GAMEPLAY_TAG(View_WorldMap, "UI.View.WorldMap");
UE_DEFINE_GAMEPLAY_TAG(Layer_HUD, "UI.Layer.HUD");
UE_DEFINE_GAMEPLAY_TAG(Layer_Panels, "UI.Layer.Panels");
UE_DEFINE_GAMEPLAY_TAG(Layer_Menu, "UI.Layer.Menu");
UE_DEFINE_GAMEPLAY_TAG(Layer_Notifications, "UI.Layer.Notifications");
UE_DEFINE_GAMEPLAY_TAG(View_Inventory, "UI.View.Inventory");
UE_DEFINE_GAMEPLAY_TAG(View_SessionMenu, "UI.View.SessionMenu");
UE_DEFINE_GAMEPLAY_TAG(View_Vitals, "UI.View.Vitals");
UE_DEFINE_GAMEPLAY_TAG(View_FieldHUD, "UI.View.FieldHUD");
UE_DEFINE_GAMEPLAY_TAG(View_Dialogue, "UI.View.Dialogue");
UE_DEFINE_GAMEPLAY_TAG(Group_HUD, "UI.Group.HUD");
UE_DEFINE_GAMEPLAY_TAG(Group_Dialogue, "UI.Group.Dialogue");
UE_DEFINE_GAMEPLAY_TAG(Group_Menus, "UI.Group.Menus");
}

UCCLUISubsystem* CCLGameUI::Get(APlayerController* Controller)
{
	auto* Local = IsValid(Controller) ? Controller->GetLocalPlayer() : nullptr;
	auto* UI = Local ? Local->GetSubsystem<UCCLUISubsystem>() : nullptr;
	if (!UI)
	{
		return nullptr;
	}

	if (!UI->GetRegistry())
	{
		auto* Registry = NewObject<UCCLUIRegistry>(UI);
		Registry->InputMapping = GetDefault<UCCLUIInputData>()->GetMapping(Controller);
		auto AddLayer = [Registry](FGameplayTag Tag, ECCLUILayerLayout Layout, int32 Order)
		{
			FCCLUILayerDefinition Layer;
			Layer.Tag = Tag;
			Layer.Layout = Layout;
			Layer.ZOrder = Order;
			Registry->Layers.Add(Layer);
		};
		AddLayer(CCLUITags::Layer_HUD, ECCLUILayerLayout::Overlay, 0);
		AddLayer(CCLUITags::Layer_Panels, ECCLUILayerLayout::Stack, 10);
		AddLayer(CCLUITags::Layer_Menu, ECCLUILayerLayout::Stack, 20);
		AddLayer(CCLUITags::Layer_Notifications, ECCLUILayerLayout::Queue, 30);
		FCCLUIViewDefinition View;
		View.Tag = CCLUITags::View_Inventory;
		View.WidgetClass = UCCLInventoryScreen::StaticClass();
		View.RequiredContextClass = UCCLInventoryContext::StaticClass();
		View.Layer = CCLUITags::Layer_Panels;
		View.Groups.AddTag(CCLUITags::Group_Menus);
		View.InputPolicy = ECCLUIInputPolicy::Menu;
		Registry->Views.Add(View);
		View.Tag = CCLUITags::View_SessionMenu;
		View.WidgetClass = UCCLSessionMenuScreen::StaticClass();
		View.RequiredContextClass = UCCLSessionMenuContext::StaticClass();
		View.Layer = CCLUITags::Layer_Menu;
		Registry->Views.Add(View);
		View = FCCLUIViewDefinition();
		View.Tag = CCLUITags::View_Vitals;
		View.WidgetClass = UCCLVitalsScreen::StaticClass();
		View.RequiredContextClass = UCCLHUDContext::StaticClass();
		View.Layer = CCLUITags::Layer_HUD;
		View.Groups.AddTag(CCLUITags::Group_HUD);
		Registry->Views.Add(View);
		View.Tag = CCLUITags::View_FieldHUD;
		View.WidgetClass = UCCLFieldHUDScreen::StaticClass();
		Registry->Views.Add(View);
		View.Tag = CCLUITags::View_Dialogue;
		View.WidgetClass = UCCLDialogueScreen::StaticClass();
		View.RequiredContextClass = UCCLDialogueContext::StaticClass();
		View.Groups.Reset();
		View.Groups.AddTag(CCLUITags::Group_Dialogue);
		Registry->Views.Add(View);
		View = FCCLUIViewDefinition();
		View.Tag = CCLUITags::View_Minimap;
		View.WidgetClass = UCCLMapScreen::StaticClass();
		View.RequiredContextClass = UCCLMapContext::StaticClass();
		View.Layer = CCLUITags::Layer_HUD;
		View.Groups.AddTag(CCLUITags::Group_HUD);
		Registry->Views.Add(View);
		View.Tag = CCLUITags::View_WorldMap;
		View.Layer = CCLUITags::Layer_Panels;
		View.Groups.Reset();
		View.Groups.AddTag(CCLUITags::Group_Menus);
		View.InputPolicy = ECCLUIInputPolicy::Menu;
		Registry->Views.Add(View);
		View = FCCLUIViewDefinition();
		View.Tag = CCLUITags::View_Cinematic;
		View.WidgetClass = UCCLCinematicScreen::StaticClass();
		View.RequiredContextClass = UCCLCinematicContext::StaticClass();
		View.Layer = CCLUITags::Layer_Notifications;
		Registry->Views.Add(View);
		if (!UI->ConfigureRegistry(Registry))
		{
			return nullptr;
		}
	}

	return UI->EnsureRoot() ? UI : nullptr;
}
