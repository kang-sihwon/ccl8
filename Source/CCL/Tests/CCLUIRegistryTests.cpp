#include "UI/Core/CCLUIRegistry.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "UI/Core/CCLScreen.h"
#include "UI/Core/CCLUIContext.h"
#include "Misc/AutomationTest.h"
#include "NativeGameplayTags.h"

namespace
{
UE_DEFINE_GAMEPLAY_TAG_STATIC(TestOverlay, "UI.Layer.ContractOverlay");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TestStack, "UI.Layer.ContractStack");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TestExtension, "UI.Extension.ContractCorner");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TestView, "UI.View.ContractPanel");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TestSecondView, "UI.View.ContractSecondPanel");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TestGroup, "UI.Group.ContractHUD");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLUIRegistryTest, "CCL.UI.RegistryContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLUIRegistryTest::RunTest(const FString& Parameters)
{
	auto* Registry = NewObject<UCCLUIRegistry>();
	FCCLUILayerDefinition Overlay;
	Overlay.Tag = TestOverlay;
	Registry->Layers.Add(Overlay);
	FCCLUILayerDefinition Stack;
	Stack.Tag = TestStack;
	Stack.Layout = ECCLUILayerLayout::Stack;
	Stack.ZOrder = 10;
	Registry->Layers.Add(Stack);
	FCCLUIExtensionDefinition Extension;
	Extension.Tag = TestExtension;
	Extension.Layer = TestOverlay;
	Extension.Alignment = FVector2D(1., 0.);
	Registry->Extensions.Add(Extension);
	FCCLUIViewDefinition View;
	View.Tag = TestView;
	View.Layer = TestOverlay;
	View.Extension = TestExtension;
	View.Groups.AddTag(TestGroup);
	View.WidgetClass = UCCLScreen::StaticClass();
	View.RequiredContextClass = UCCLUIContext::StaticClass();
	View.InstancePolicy = ECCLUIInstancePolicy::PerContext;
	Registry->Views.Add(View);
	FCCLUIViewDefinition Second = View;
	Second.Tag = TestSecondView;
	Second.Layer = TestStack;
	Second.Extension = {};
	Second.InstancePolicy = ECCLUIInstancePolicy::Multiple;
	Registry->Views.Add(Second);

	TArray<FText> Errors;
	TestTrue(TEXT("overlay extensions and stack views share the registry contract"), Registry->ValidateRegistry(Errors));
	TestNull(TEXT("exact view lookup does not match a group"), Registry->FindView(TestGroup));
	TestNotNull(TEXT("view registration is addressable by tag"), Registry->FindView(TestView));
	auto* Context = NewObject<UCCLUIContext>();
	TestTrue(TEXT("required context accepted"), View.AcceptsContext(Context));
	TestFalse(TEXT("required context cannot be omitted"), View.AcceptsContext(nullptr));

	Registry->Views[1].Extension = TestExtension;
	Errors.Reset();
	TestFalse(TEXT("extension in another layer rejected"), Registry->ValidateRegistry(Errors));
	Registry->Views[1] = Second;
	Registry->Views[1].Tag = TestView;
	Errors.Reset();
	TestFalse(TEXT("duplicate view types rejected before instance creation"), Registry->ValidateRegistry(Errors));
	Registry->Views[1] = Second;
	Registry->Layers[1].ZOrder = 0;
	Errors.Reset();
	TestFalse(TEXT("ambiguous layer order rejected"), Registry->ValidateRegistry(Errors));
	Registry->Layers[1] = Stack;
	Registry->Extensions[0].Layer = TestStack;
	Errors.Reset();
	TestFalse(TEXT("extension cannot bypass stack ownership"), Registry->ValidateRegistry(Errors));
	Registry->Extensions[0] = Extension;
	Registry->Views[0].RequiredContextClass = nullptr;
	Errors.Reset();
	TestFalse(TEXT("context singleton requires a typed context"), Registry->ValidateRegistry(Errors));
	TestFalse(TEXT("context-free view does not accept an accidental payload"), Registry->Views[0].AcceptsContext(Context));
	Registry->Views[0] = View;
	Registry->Views[0].Groups.AddTag(TestOverlay);
	Errors.Reset();
	TestFalse(TEXT("layer tag is not a presentation group"), Registry->ValidateRegistry(Errors));

	auto* Screen = NewObject<UCCLScreen>();
	FCCLUIViewHandle FirstHandle;
	FirstHandle.Id = FGuid::NewGuid();
	Screen->BindContext(Context, FirstHandle, ECCLUIInputPolicy::Menu);
	int32 OldCloseCalls = 0;
	Screen->OnCloseRequested.AddLambda([&OldCloseCalls](FCCLUIViewHandle) { ++OldCloseCalls; });
	auto* NextContext = NewObject<UCCLUIContext>();
	FCCLUIViewHandle NextHandle;
	NextHandle.Id = FGuid::NewGuid();
	Screen->BindContext(NextContext, NextHandle, ECCLUIInputPolicy::GameAndUI);
	Screen->RequestClose();
	TestEqual(TEXT("rebind removes callbacks from the previous use"), OldCloseCalls, 0);
	TestTrue(TEXT("rebind replaces context and instance identity"), Screen->GetContext() == NextContext && Screen->GetViewHandle() == NextHandle);
	Screen->ReleaseContext();
	Screen->ReleaseContext();
	TestTrue(TEXT("release is idempotent and clears all retained context"), !Screen->GetViewHandle().IsValid() && !Screen->GetContext());
	TestFalse(TEXT("released screen does not retain an input override"), Screen->GetDesiredInputConfig().IsSet());
	return true;
}
#endif
