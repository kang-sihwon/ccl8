#include "Items/CCLFoodState.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Items/CCLItemDefinition.h"
#include "Misc/AutomationTest.h"
#include "Misc/Base64.h"
#include "Misc/Crc.h"

namespace
{
	FCCLFoodState MakeFood(int32 Count, double Time = 0., int32 Origin = 1)
	{
		FCCLFoodState Food;
		FCCLFoodPolicy Policy;
		Policy.ShelfLifeSeconds = 100.;
		Policy.MoldDelaySeconds = 50.;
		Policy.MoldGrowthSeconds = 50.;
		FCCLFoodStorage Storage;
		Storage.RelativeHumidity = 1.;
		FString Error;
		CCLFood::Create(FGuid(1, 2, 3, 4), FGuid(9, 1, 0, Origin), Count, Time, Policy, Storage, Food, Error);
		return Food;
	}

	FString Encode(const FCCLFoodState& State)
	{
		FString Value, Error;
		CCLFood::Encode(State, Value, Error);
		return Value;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFoodExposureTest, "CCL.Environment.Food.StorageExposure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLFoodExposureTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto Whole = MakeFood(4);
	auto Split = Whole;
	TestTrue(TEXT("advance whole"), CCLFood::Advance(Whole, 75., Error));
	for (int32 I = 1; I <= 75; ++I)
	{
		TestTrue(TEXT("partition"), CCLFood::Advance(Split, I, Error));
	}

	TestEqual(TEXT("partitioned exposure"), Encode(Whole), Encode(Split));
	FCCLFoodQuality Quality;
	TestTrue(TEXT("evaluate"), CCLFood::Evaluate(Whole, 75., Quality, Error));
	TestEqual(TEXT("freshness"), Quality.Freshness, 0.25);
	TestEqual(TEXT("mold"), Quality.Mold, 0.5);
	TestEqual(TEXT("mold quantity"), Quality.MoldyQuantity, 4);
	const FString Before = Encode(Whole);
	TestTrue(TEXT("read only evaluation"), CCLFood::Evaluate(Whole, 100., Quality, Error));
	TestEqual(TEXT("read does not change owner"), Encode(Whole), Before);
	TestEqual(TEXT("exact expiry"), Quality.SpoiledQuantity, 4);
	FCCLFoodStorage Cold = Whole.Storage;
	Cold.TemperatureCelsius = -10.;
	TestTrue(TEXT("cold condition change"), CCLFood::ChangeStorage(Whole, Cold, 75., Error));
	TestTrue(TEXT("frozen query"), CCLFood::Evaluate(Whole, 1000., Quality, Error));
	TestEqual(TEXT("cold preserves prior spoilage"), Quality.Freshness, 0.25);
	TestEqual(TEXT("cold does not erase mold"), Quality.Mold, 0.5);
	auto Hot = MakeFood(1);
	auto HotStorage = Hot.Storage;
	HotStorage.TemperatureCelsius = 30.;
	TestTrue(TEXT("hot storage"), CCLFood::ChangeStorage(Hot, HotStorage, 0., Error));
	TestTrue(TEXT("hot query"), CCLFood::Evaluate(Hot, 50., Quality, Error));
	TestEqual(TEXT("double thermal exposure"), Quality.SpoiledQuantity, 1);
	auto Dry = MakeFood(1);
	auto DryStorage = Dry.Storage;
	DryStorage.RelativeHumidity = 0.;
	TestTrue(TEXT("dry storage"), CCLFood::ChangeStorage(Dry, DryStorage, 0., Error));
	TestTrue(TEXT("dry query"), CCLFood::Evaluate(Dry, 75., Quality, Error));
	TestEqual(TEXT("no dry mold exposure"), Quality.Mold, 0.);
	auto Sealed = MakeFood(1);
	auto SealedStorage = Sealed.Storage;
	SealedStorage.MoldExposure = 0.;
	SealedStorage.SpoilageMultiplier = 0.5;
	TestTrue(TEXT("sealed storage"), CCLFood::ChangeStorage(Sealed, SealedStorage, 0., Error));
	TestTrue(TEXT("sealed query"), CCLFood::Evaluate(Sealed, 100., Quality, Error));
	TestEqual(TEXT("preservation coefficient"), Quality.Freshness, 0.5);
	TestEqual(TEXT("sealed mold prevention"), Quality.Mold, 0.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFoodLotTest, "CCL.Environment.Food.MergeSplitPreservesOrigins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLFoodLotTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto Old = MakeFood(2);
	auto Fresh = MakeFood(3, 100., 2);
	TestTrue(TEXT("merge old and fresh"), CCLFood::Merge(Old, Fresh, 100., Error));
	TestEqual(TEXT("separate histories"), Old.Lots.Num(), 2);
	TestEqual(TEXT("original birth"), Old.Lots[0].CreatedWorldSeconds, 0.);
	FCCLFoodQuality Quality;
	TestTrue(TEXT("merged quality"), CCLFood::Evaluate(Old, 100., Quality, Error));
	TestEqual(TEXT("spoiled quantity retained"), Quality.SpoiledQuantity, 2);
	TestEqual(TEXT("freshness weighted only for view"), Quality.Freshness, 0.6);
	FCCLFoodState Removed;
	TestTrue(TEXT("consume oldest unit"), CCLFood::Split(Old, 1, 100., Removed, Error));
	TestEqual(TEXT("old unit consumed"), Removed.Lots[0].CreatedWorldSeconds, 0.);
	TestEqual(TEXT("one old unit remains"), Old.Lots[0].Quantity, 1);
	TestEqual(TEXT("quantity conserved"), CCLFood::Quantity(Old) + CCLFood::Quantity(Removed), 5);
	TestTrue(TEXT("recombine split lot"), CCLFood::Merge(Old, Removed, 100., Error));
	TestEqual(TEXT("identical origin combines"), Old.Lots.Num(), 2);
	FCCLFoodState All;
	TestTrue(TEXT("remove entire stack"), CCLFood::Split(Old, 5, 100., All, Error));
	TestFalse(TEXT("removed source has no food"), Old.IsPresent());
	TestTrue(TEXT("removed source canonical"), CCLFood::Validate(Old, 0, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFoodAtomicTest, "CCL.Environment.Food.InvalidMutationPreservesState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLFoodAtomicTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto Food = MakeFood(3, 10.);
	const FString Before = Encode(Food);
	TestFalse(TEXT("time reversal"), CCLFood::Advance(Food, 9., Error));
	TestFalse(TEXT("self merge"), CCLFood::Merge(Food, Food, 20., Error));
	auto Other = MakeFood(1);
	Other.WorldId = FGuid::NewGuid();
	TestFalse(TEXT("other world"), CCLFood::Merge(Food, Other, 20., Error));
	Other = MakeFood(1, 0., 1);
	TestFalse(TEXT("same origin cannot change birth"), CCLFood::Merge(Food, Other, 20., Error));
	Other = MakeFood(1);
	Other.Policy.ShelfLifeSeconds = 200.;
	TestFalse(TEXT("changed policy"), CCLFood::Merge(Food, Other, 20., Error));
	auto InvalidStorage = Food.Storage;
	InvalidStorage.RelativeHumidity = 1.1;
	TestFalse(TEXT("invalid storage"), CCLFood::ChangeStorage(Food, InvalidStorage, 20., Error));
	FCCLFoodState Removed = MakeFood(7);
	const FString OutputBefore = Encode(Removed);
	TestFalse(TEXT("excess split"), CCLFood::Split(Food, 4, 20., Removed, Error));
	TestFalse(TEXT("aliased split"), CCLFood::Split(Food, 1, 20., Food, Error));
	TestEqual(TEXT("failed output preserved"), Encode(Removed), OutputBefore);
	TestEqual(TEXT("all failed operations preserve input"), Encode(Food), Before);
	auto Many = MakeFood(1);
	for (int32 I = 2; I <= CCLFood::MaxLots; ++I)
	{
		TestTrue(TEXT("bounded distinct lots"), CCLFood::Merge(Many, MakeFood(1, 0., I), 0., Error));
	}

	const FString LimitBefore = Encode(Many);
	TestFalse(TEXT("lot budget rejects next"), CCLFood::Merge(Many, MakeFood(1, 0., 999), 0., Error));
	TestEqual(TEXT("budget failure preserves state"), Encode(Many), LimitBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFoodSaveTest, "CCL.Environment.Food.SaveResumeAndCorruption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLFoodSaveTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto Food = MakeFood(3);
	TestTrue(TEXT("prior exposure"), CCLFood::Advance(Food, 40., Error));
	FString Saved = Encode(Food);
	FCCLFoodState Restored;
	TestTrue(TEXT("decode"), CCLFood::Decode(Saved, Restored, Error));
	TestEqual(TEXT("identical serialized state"), Encode(Restored), Saved);
	TestTrue(TEXT("continuous"), CCLFood::Advance(Food, 80., Error));
	TestTrue(TEXT("resumed"), CCLFood::Advance(Restored, 80., Error));
	TestEqual(TEXT("resume equals continuous"), Encode(Restored), Encode(Food));
	const FString Before = Encode(Restored);
	TArray<uint8> Bytes;
	FBase64::Decode(Saved, Bytes);
	Bytes[12] ^= 1;
	TestFalse(TEXT("corrupt checksum"), CCLFood::Decode(FBase64::Encode(Bytes), Restored, Error));
	FBase64::Decode(Saved, Bytes);
	Bytes[4] = 2;
	uint32 Crc = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4);
	FMemory::Memcpy(Bytes.GetData() + Bytes.Num() - 4, &Crc, 4);
	TestFalse(TEXT("unknown version with valid CRC"), CCLFood::Decode(FBase64::Encode(Bytes), Restored, Error));
	TestEqual(TEXT("failed decode keeps previous state"), Encode(Restored), Before);
	TestTrue(TEXT("absent old state"), CCLFood::Decode(TEXT(""), Restored, Error));
	TestFalse(TEXT("no invented food"), Restored.IsPresent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFoodDefinitionTest, "CCL.Environment.Food.OptionalDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLFoodDefinitionTest::RunTest(const FString& Parameters)
{
	auto* Definition = NewObject<UCCLItemDefinition>();
	TArray<FText> Errors;
	TestTrue(TEXT("nonfood remains valid"), Definition->ValidateDefinition(Errors));
	TestTrue(TEXT("perishable policy configured"), Definition->ConfigurePerishable(100., 50., 50.));
	TestTrue(TEXT("optional definition valid"), Definition->ValidateDefinition(Errors));
	TestFalse(TEXT("bad policy rejected"), Definition->ConfigurePerishable(-1., 50., 50.));
	const FInstancedStruct Duplicate = Definition->ItemFragments[0];
	Definition->ItemFragments.Add(Duplicate);
	TestFalse(TEXT("duplicate policy rejected"), Definition->ValidateDefinition(Errors));
	return true;
}
#endif
