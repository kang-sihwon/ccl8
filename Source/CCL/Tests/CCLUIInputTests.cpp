#include "UI/CCLUIInputData.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/World.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLUIBackKeyTest, "CCL.UI.BackKeyByWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLUIBackKeyTest::RunTest(const FString& Parameters)
{
	auto* PIE = NewObject<UWorld>();
	PIE->WorldType = EWorldType::PIE;
	auto* Game = NewObject<UWorld>();
	Game->WorldType = EWorldType::Game;
	const auto* Data = GetDefault<UCCLUIInputData>();
	TestTrue(TEXT("PIE uses F6"), UCCLUIInputData::GetBackKey(PIE) == EKeys::F6);
	TestTrue(TEXT("Standalone uses Escape"), UCCLUIInputData::GetBackKey(Game) == EKeys::Escape);
	TestTrue(TEXT("Missing world uses Escape"), UCCLUIInputData::GetBackKey(nullptr) == EKeys::Escape);
	const auto HasKey = [](const UInputMappingContext* Mapping, const FKey& Key)
	{
		return Mapping->GetMappings().ContainsByPredicate([&](const FEnhancedActionKeyMapping& Entry) { return Entry.Key == Key; });
	};
	TestTrue(TEXT("PIE maps F6 and leaves Escape to the editor"),
		HasKey(Data->GetMapping(PIE), EKeys::F6) && !HasKey(Data->GetMapping(PIE), EKeys::Escape));
	TestTrue(TEXT("Game mappings are independent of PIE"),
		HasKey(Data->GetMapping(Game), EKeys::Escape) && !HasKey(Data->GetMapping(Game), EKeys::F6));
	TestTrue(TEXT("Both worlds keep controller back and keyboard accept"),
		HasKey(Data->GetMapping(PIE), EKeys::Gamepad_FaceButton_Right) && HasKey(Data->GetMapping(Game), EKeys::Gamepad_FaceButton_Right) &&
		HasKey(Data->GetMapping(PIE), EKeys::Enter) && HasKey(Data->GetMapping(Game), EKeys::Enter));
	TestEqual(TEXT("PIE hint"), FString(UCCLUIInputData::GetBackKeyLabel(PIE)), FString(TEXT("F6")));
	TestEqual(TEXT("Game hint"), FString(UCCLUIInputData::GetBackKeyLabel(Game)), FString(TEXT("Esc")));
	return true;
}
#endif
