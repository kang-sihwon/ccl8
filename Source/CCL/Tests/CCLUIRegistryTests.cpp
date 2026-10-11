#include "KSIUIRegistry.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "KSIScreen.h"
#include "KSIUIContext.h"
#include "KSIUIPresentation.h"
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
	auto* Registry = NewObject<UKSIUIRegistry>();
	FKSIUILayerDefinition Overlay;
	Overlay.Tag = TestOverlay;
	Registry->Layers.Add(Overlay);
	FKSIUILayerDefinition Stack;
	Stack.Tag = TestStack;
	Stack.Layout = EKSIUILayerLayout::Stack;
	Stack.ZOrder = 10;
	Registry->Layers.Add(Stack);
	FKSIUIExtensionDefinition Extension;
	Extension.Tag = TestExtension;
	Extension.Layer = TestOverlay;
	Extension.Alignment = FVector2D(1., 0.);
	Registry->Extensions.Add(Extension);
	FKSIUIViewDefinition View;
	View.Tag = TestView;
	View.Layer = TestOverlay;
	View.Extension = TestExtension;
	View.Groups.AddTag(TestGroup);
	View.WidgetClass = UKSIScreen::StaticClass();
	View.RequiredContextClass = UKSIUIContext::StaticClass();
	View.InstancePolicy = EKSIUIInstancePolicy::PerContext;
	Registry->Views.Add(View);
	FKSIUIViewDefinition Second = View;
	Second.Tag = TestSecondView;
	Second.Layer = TestStack;
	Second.Extension = {};
	Second.InstancePolicy = EKSIUIInstancePolicy::Multiple;
	Registry->Views.Add(Second);

	TArray<FText> Errors;
	TestTrue(TEXT("overlay extensions and stack views share the registry contract"), Registry->ValidateRegistry(Errors));
	TestNull(TEXT("exact view lookup does not match a group"), Registry->FindView(TestGroup));
	TestNotNull(TEXT("view registration is addressable by tag"), Registry->FindView(TestView));
	auto* Context = NewObject<UKSIUIContext>();
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

	auto* Screen = NewObject<UKSIScreen>();
	FKSIUIViewHandle FirstHandle;
	FirstHandle.Id = FGuid::NewGuid();
	Screen->BindContext(Context, FirstHandle, EKSIUIInputPolicy::Menu);
	int32 OldCloseCalls = 0;
	Screen->OnCloseRequested.AddLambda([&OldCloseCalls](FKSIUIViewHandle) { ++OldCloseCalls; });
	auto* NextContext = NewObject<UKSIUIContext>();
	FKSIUIViewHandle NextHandle;
	NextHandle.Id = FGuid::NewGuid();
	Screen->BindContext(NextContext, NextHandle, EKSIUIInputPolicy::GameAndUI);
	Screen->RequestClose();
	TestEqual(TEXT("rebind removes callbacks from the previous use"), OldCloseCalls, 0);
	TestTrue(TEXT("rebind replaces context and instance identity"), Screen->GetContext() == NextContext && Screen->GetViewHandle() == NextHandle);
	Screen->ReleaseContext();
	Screen->ReleaseContext();
	TestTrue(TEXT("release is idempotent and clears all retained context"), !Screen->GetViewHandle().IsValid() && !Screen->GetContext());
	TestFalse(TEXT("released screen does not retain an input override"), Screen->GetDesiredInputConfig().IsSet());
	FKSIUIPresentationDefinition Presentation;
	TestFalse(TEXT("empty selector cannot accidentally affect every screen"), Presentation.IsValid());
	Presentation.Groups.AddTag(TestGroup);
	TestTrue(TEXT("valid presentation group accepted"), Presentation.IsValid());
	Presentation.FadeSeconds = -1.f;
	TestFalse(TEXT("negative fade duration rejected"), Presentation.IsValid());
	Presentation.FadeSeconds = 0.f;
	Presentation.Opacity = 2.f;
	TestFalse(TEXT("out-of-range opacity rejected"), Presentation.IsValid());
	Presentation.Opacity = 0.5f;
	Presentation.Groups.AddTag(TestView);
	TestFalse(TEXT("view tag cannot be mistaken for a group selector"), Presentation.IsValid());
	return true;
}
#endif
