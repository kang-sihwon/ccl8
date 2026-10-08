#include "UI/CCLCombatViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLUIViewModelTest, "CCL.UI.ViewModelLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLUIViewModelTest::RunTest(const FString& Parameters)
{
	auto* First = NewObject<UAbilitySystemComponent>();
	auto* FirstHealth = NewObject<UCCLHealthSet>(First);
	FirstHealth->InitHealth(100.f);
	FirstHealth->InitMaxHealth(100.f);
	First->AddAttributeSetSubobject(FirstHealth);
	auto* Second = NewObject<UAbilitySystemComponent>();
	auto* SecondHealth = NewObject<UCCLHealthSet>(Second);
	SecondHealth->InitHealth(75.f);
	SecondHealth->InitMaxHealth(150.f);
	Second->AddAttributeSetSubobject(SecondHealth);
	auto* Model = NewObject<UCCLCombatViewModel>();
	int32 Notifications = 0;
	const auto Field = UCCLCombatViewModel::FFieldNotificationClassDescriptor::Health;
	const auto Handle = Model->AddFieldValueChangedDelegate(Field,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda(
			[&Notifications](UObject*, UE::FieldNotification::FFieldId) { ++Notifications; }));
	Model->Bind(First);
	TestEqual(TEXT("initial source snapshot"), Model->Health, 100.f);
	TestEqual(TEXT("initial field change delivered"), Notifications, 1);
	Model->Bind(First);
	TestEqual(TEXT("same source has no redundant notification"), Notifications, 1);
	Model->Bind(Second);
	TestEqual(TEXT("replacement source snapshot"), Model->Health, 75.f);
	FOnAttributeChangeData Change;
	Change.Attribute = UCCLHealthSet::GetHealthAttribute();
	Change.NewValue = 5.f;
	First->GetGameplayAttributeValueChangeDelegate(Change.Attribute).Broadcast(Change);
	TestEqual(TEXT("previous pawn no longer drives the model"), Model->Health, 75.f);
	Change.NewValue = 50.f;
	Second->GetGameplayAttributeValueChangeDelegate(Change.Attribute).Broadcast(Change);
	TestEqual(TEXT("new pawn drives the model"), Model->Health, 50.f);
	Model->Bind(nullptr);
	TestEqual(TEXT("unbinding clears stale values"), Model->Health, 0.f);
	const int32 Before = Notifications;
	Second->GetGameplayAttributeValueChangeDelegate(Change.Attribute).Broadcast(Change);
	TestEqual(TEXT("unbound source cannot send new UI notifications"), Notifications, Before);
	Model->RemoveFieldValueChangedDelegate(Field, Handle);
	return true;
}
#endif
