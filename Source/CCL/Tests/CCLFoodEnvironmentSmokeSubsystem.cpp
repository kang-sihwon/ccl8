#include "CCLFoodEnvironmentSmokeSubsystem.h"

#include "Environment/CCLFoodExperiment.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLWorldSnapshot.h"
#include "Items/CCLInventoryPersistenceSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Environment/CCLExperimentScreen.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Camera/CameraActor.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

bool UCCLFoodEnvironmentSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLFoodEnvironmentSmoke"));
#endif
}

void UCCLFoodEnvironmentSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}
	if (Now - Started > 240.)
	{
		Finish(false, FString::Printf(TEXT("timeout step=%d"), Step));
		return;
	}
	auto* W = GetWorld();
	auto* D = W ? ACCLExperimentDirector::Find(W) : nullptr;
	auto* R = W ? W->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	auto* Food = W ? ACCLFoodExperiment::Find(W) : nullptr;
	if (!D || !D->IsReady() || !R || !Food || Food->GetView().Rows.Num() != 3 || Now < Next)
	{
		return;
	}
	FString Error, Directory;
	FParse::Value(FCommandLine::Get(), TEXT("CCLFoodEnvironmentSmokeDir="), Directory);
	auto Check = [&](bool OK, const TCHAR* Message)
	{
		if (!OK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s %s"), Step, Message, *Error));
		}
		return OK;
	};
	if (!Check(!Directory.IsEmpty(), TEXT("isolated output")))
	{
		return;
	}
	const auto& View = Food->GetView();
	if (W->GetNetMode() == NM_Client)
	{
		if (!FPaths::FileExists(Directory / TEXT("final.txt")))
		{
			if (!Step && View.Rows[0].EnvironmentSource == 0 && View.Rows[0].Quantity == 4)
			{
				Step = 1;
				UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_ENVIRONMENT BASE_OBSERVED"));
			}
			return;
		}
		FString Text;
		TArray<FString> Lines;
		if (!FFileHelper::LoadFileToString(Text, *(Directory / TEXT("final.txt"))))
		{
			return;
		}
		Text.ParseIntoArrayLines(Lines);
		if (!Check(Lines.Num() == 5, TEXT("observation oracle")))
		{
			return;
		}
		if (View.Epoch.ToString() != Lines[1] || !FMath::IsNearlyEqual(View.WorldSeconds, FCString::Atod(*Lines[0]), 1.e-8))
		{
			return;
		}
		for (int32 N = 0; N < 3; ++N)
		{
			TArray<FString> P;
			Lines[N + 2].ParseIntoArray(P, TEXT(","));
			if (!Check(P.Num() == 6, TEXT("observation row")))
			{
				return;
			}
			const auto& Row = View.Rows[N];
			if (Row.EnvironmentSource != FCString::Atoi(*P[0]) || Row.Quantity != FCString::Atoi(*P[1]) ||
				!FMath::IsNearlyEqual(Row.Temperature, FCString::Atod(*P[2]), 1.e-8) ||
				!FMath::IsNearlyEqual(Row.Humidity, FCString::Atod(*P[3]), 1.e-8) ||
				!FMath::IsNearlyEqual(Row.Freshness, FCString::Atod(*P[4]), 1.e-8) ||
				!FMath::IsNearlyEqual(Row.Mold, FCString::Atod(*P[5]), 1.e-8))
			{
				return;
			}
		}
		if (!Check(!Food->Execute(ECCLExperimentAction::FoodWeather, Error) &&
					   Food->GetInventory(0)->GetEntries().IsEmpty(),
				   TEXT("client policy mutation refused and inventory private")))
		{
			return;
		}
		Finish(true, TEXT("environment source temperature humidity and food quality replicated after restore"));
		return;
	}
	auto* Store = W->GetSubsystem<UCCLInventoryPersistenceSubsystem>();
	auto* Agents = W->GetSubsystem<UCCLAgentWorldSubsystem>();
	auto* PC = Cast<ACCLExperimentPlayerController>(W->GetFirstPlayerController());
	auto* Open = Food->GetInventory(0);
	auto* Sealed = Food->GetInventory(2);
	auto Execute = [&](ECCLExperimentAction Action, FName Case = TEXT("Zone_07"))
	{ return D->Execute(PC, Action, Case, D->GetGeneration(), D->FindResult(Case)->RunId, Error); };
	auto Capture = [&](TArray<uint8>& Bytes)
	{
		TArray<FCCLInventorySnapshot> Records;
		return Store->Capture(R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), Records, Error) &&
			   CCLInventorySnapshot::Encode(Records, R->GetIdentity().WorldId, R->GetClock().GetWorldSeconds(), Bytes,
											Error);
	};
	auto Advance = [&]()
	{
		if (!R->GetClock().HasPendingTime() && !Check(R->QueueGameTime(.25, Error), TEXT("queue time once")))
		{
			return false;
		}
		if (!R->AdvancePending(.25, 1., Error))
		{
			if (!Check(Error.Contains(TEXT("awaits")), TEXT("environment advance")))
			{
				return false;
			}
			Next = Now + .25;
			return false;
		}
		return true;
	};
	const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLFoodEnvironmentCapture"));
	auto Screen = [&]() -> UCCLExperimentScreen*
	{
		auto* UI = PC ? CCLGameUI::Get(PC) : nullptr;
		return UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
	};
	const bool bReader = FParse::Param(FCommandLine::Get(), TEXT("CCLFoodEnvironmentRead"));
	if (!Step)
	{
		R->SetTickableTickType(ETickableTickType::Never);
		if (!Check(R->ChangeTimeScale(0., Error), TEXT("pause")))
		{
			return;
		}
		if (bReader)
		{
			TArray<uint8> Bytes;
			if (!Check(FFileHelper::LoadFileToArray(Bytes, *(Directory / TEXT("world.bin"))) &&
						   R->Restore(Agents->GetSimulation(), Bytes, Error) &&
						   FFileHelper::LoadFileToArray(SavedInventory, *(Directory / TEXT("inventory.bin"))),
					   TEXT("new process restore")))
			{
				return;
			}
			TArray<uint8> Actual;
			if (!Check(Capture(Actual) && Actual == SavedInventory, TEXT("restored policy and food bytes")))
			{
				return;
			}
			TargetWorld = R->GetClock().GetWorldSeconds() + 10.;
			if (!Check(R->ChangeTimeScale(4., Error), TEXT("reader resume")))
			{
				return;
			}
			Step = 50;
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_ENVIRONMENT BASE_READY"));
		Step = 1;
		Next = Now + 2.;
		return;
	}
	if (Step == 50)
	{
		if (R->GetClock().GetWorldSeconds() < TargetWorld - 1.e-8)
		{
			Advance();
			return;
		}
		TArray<uint8> Actual, Expected;
		if (!Check(Capture(Actual) && FFileHelper::LoadFileToArray(Expected, *(Directory / TEXT("continued.bin"))) &&
					   Actual == Expected,
				   TEXT("new process continuation matches exact inventory history")))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_ENVIRONMENT CONTINUATION_MATCH"));
		Finish(true, TEXT("fresh process policy and held-sample continuation verified"));
		return;
	}
	if (!PC || !D->CanOperate(PC))
	{
		return;
	}
	if (Step == 1)
	{
		if (W->GetNetMode() != NM_Standalone && !FPaths::FileExists(Directory / TEXT("base-observed.txt")))
		{
			return;
		}
		auto Forcing = R->GetSurfaceSimulation().FindRegion(D->WeatherRegionId())->Forcing;
		Forcing.TemperatureCelsius = -5.;
		if (!Check(R->ChangeSurfaceForcing(D->WeatherRegionId(), Forcing, Error), TEXT("frozen outside forcing")))
		{
			return;
		}
		if (bRendered)
		{
			auto* Camera = W->SpawnActor<ACameraActor>(UCCLExperimentDefinition::ZoneCenter(7) + FVector(0, -1600, 520),
													   FRotator(-22, 90, 0));
			PC->SetViewTarget(Camera);
			auto* UI = CCLGameUI::Get(PC);
			if (!UI->IsViewOpen(PC->GetExperimentView()))
			{
				PC->CCLExperiment();
			}
			if (auto* Current = Screen())
			{
				Current->SelectCase(TEXT("Zone_07"));
				Current->RevealControl(Current->GetFoodWeatherButton());
			}
		}
		Step = 11;
		Next = Now + 2.;
		return;
	}
	if (Step == 11)
	{
		if (bRendered)
		{
			auto* Current = Screen();
			auto Control = Current ? Current->GetFoodWeatherButton() : nullptr;
			if (!Check(Control && Control->IsEnabled() &&
						   !Control->GetCachedGeometry().GetAbsoluteSize().IsNearlyZero(),
					   TEXT("weather binding button visible")))
			{
				return;
			}
			const FVector2D Point = Control->GetCachedGeometry().GetAbsolutePosition() +
									Control->GetCachedGeometry().GetAbsoluteSize() * .5;
			auto& App = FSlateApplication::Get();
			const auto Window = App.FindWidgetWindow(Control.ToSharedRef());
			App.ProcessMouseMoveEvent(
				FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()));
			App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
											FPointerEvent(0, Point, Point, TSet<FKey>{EKeys::LeftMouseButton},
														  EKeys::LeftMouseButton, 0, FModifierKeysState()));
			App.ProcessMouseButtonUpEvent(
				FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
		}
		else if (!Check(Execute(ECCLExperimentAction::FoodWeather), TEXT("bind weather")))
		{
			return;
		}
		Step = 12;
		Next = Now + 1.;
		return;
	}
	if (Step == 12)
	{
		if (!Check(Execute(ECCLExperimentAction::FoodIndoor) &&
					   Open->GetFoodEnvironment().Source == ECCLFoodEnvironment::WeatherRegion &&
					   Open->GetFoodEnvironment().Conditions.TemperatureCelsius == -5. &&
					   Sealed->GetFoodEnvironment().Source == ECCLFoodEnvironment::AirSpace,
				   TEXT("actual weather and room bound")))
		{
			return;
		}
		FrozenSpoilage = Open->GetEntries()[0].Food.Lots[0].SpoilageSeconds;
		InitialRoomTemperature = Sealed->GetFoodEnvironment().Conditions.TemperatureCelsius;
		if (!Check(R->ChangeTimeScale(4., Error), TEXT("frozen world interval")))
		{
			return;
		}
		TargetWorld = R->GetClock().GetWorldSeconds() + 10.;
		Step = 2;
		return;
	}
	if (Step == 2)
	{
		if (R->GetClock().GetWorldSeconds() < TargetWorld - 1.e-8)
		{
			Advance();
			return;
		}
		if (!Check(Open->GetEntries()[0].Food.Lots[0].SpoilageSeconds == FrozenSpoilage,
				   TEXT("frozen exposure preserves prior history")))
		{
			return;
		}
		TArray<uint8> Before, After, SurfaceBefore, SurfaceAfter;
		const double BeforeTime = R->GetClock().GetWorldSeconds();
		if (!Check(Capture(Before) && R->GetSurfaceSimulation().Capture(SurfaceBefore, Error) &&
					   R->QueueGameTime(.25, Error) && !R->AdvancePending(.25, 1., Error, 0) &&
					   R->GetClock().GetWorldSeconds() == BeforeTime && Capture(After) && Before == After &&
					   R->GetSurfaceSimulation().Capture(SurfaceAfter, Error) && SurfaceBefore == SurfaceAfter,
				   TEXT("life rejection cancels prepared food surface and time")))
		{
			return;
		}
		Error.Reset();
		if (!Check(R->AdvancePending(.25, 1., Error), TEXT("same queued input retries")))
		{
			return;
		}
		auto Forcing = R->GetSurfaceSimulation().FindRegion(D->WeatherRegionId())->Forcing;
		Forcing.TemperatureCelsius = 30.;
		if (!Check(R->ChangeSurfaceForcing(D->WeatherRegionId(), Forcing, Error) &&
					   Execute(ECCLExperimentAction::FireIgnite, TEXT("Zone_11")),
				   TEXT("warm outside and heat room")))
		{
			return;
		}
		Store->Unbind(*Open);
		if (!Check(Open->Restore({}), TEXT("unloaded representation released")))
		{
			return;
		}
		TargetWorld = R->GetClock().GetWorldSeconds() + 10.;
		Step = 3;
		return;
	}
	if (Step == 3)
	{
		if (R->GetClock().GetWorldSeconds() < TargetWorld - 1.e-8)
		{
			Advance();
			return;
		}
		if (!Check(Open->BindPersistence(ACCLFoodExperiment::ContainerId(0), Error) &&
					   Open->GetEntries()[0].Food.Lots[0].SpoilageSeconds > FrozenSpoilage &&
					   Open->GetFoodEnvironment().Conditions.TemperatureCelsius == 30.,
				   TEXT("unloaded container continued in warm environment")))
		{
			return;
		}
		const auto* Room = R->GetSurfaceSimulation().GetAirSimulation().FindSpace(D->FireSpaceId());
		if (!Check(Room && Room->TemperatureCelsius() > InitialRoomTemperature + .01 && FMath::IsNearlyEqual(Sealed->GetFoodEnvironment().Conditions.TemperatureCelsius,
												FMath::Clamp(Room->TemperatureCelsius(), -80., 80.), 1.e-8),
				   TEXT("indoor food follows actual room heat")))
		{
			return;
		}
		auto* Owner = W->SpawnActor<AActor>();
		auto* Empty = NewObject<UCCLInventoryComponent>(Owner);
		Empty->RegisterComponent();
		FCCLFoodEnvironmentBinding Binding = Open->GetFoodEnvironment();
		const FGuid OriginalId = Open->GetEntries()[0].Id;
		const int32 Quantity = Open->GetEntries()[0].Quantity;
		if (!Check(Empty->BindPersistence(FGuid(940, 941, 942, 943), Error) &&
					   Empty->ChangeFoodEnvironment(Binding, Error) &&
					   Open->TransferFood(OriginalId, Quantity, Empty) && Empty->GetEntries()[0].Id == OriginalId &&
					   Empty->GetEntries()[0].Food.Storage == Empty->GetFoodEnvironment().Conditions &&
					   Empty->TransferFood(OriginalId, Quantity, Open),
				   TEXT("empty environment target inherits conditions and preserves identity")))
		{
			return;
		}
		Owner->Destroy();
		if (!Check(R->ChangeTimeScale(0., Error), TEXT("pause checkpoint")))
		{
			return;
		}
		Step = 4;
		Next = Now + 1.;
		return;
	}
	if (Step == 4)
	{
		TArray<uint8> Bytes;
		SavedWorldTime = R->GetClock().GetWorldSeconds();
		if (!Check(Capture(SavedInventory) && Execute(ECCLExperimentAction::Save) &&
					   R->Save(Agents->GetSimulation(), Bytes, Error) &&
					   FFileHelper::SaveArrayToFile(Bytes, *(Directory / TEXT("world.bin"))) &&
					   FFileHelper::SaveArrayToFile(SavedInventory, *(Directory / TEXT("inventory.bin"))) &&
					   R->ChangeTimeScale(4., Error),
				   TEXT("save food policy and common world")))
		{
			return;
		}
		TargetWorld = SavedWorldTime + 10.;
		Step = 5;
		return;
	}
	if (Step == 5)
	{
		if (R->GetClock().GetWorldSeconds() < TargetWorld - 1.e-8)
		{
			Advance();
			return;
		}
		TArray<uint8> Continued;
		if (!Check(Capture(Continued) &&
					   FFileHelper::SaveArrayToFile(Continued, *(Directory / TEXT("continued.bin"))) &&
					   Execute(ECCLExperimentAction::FoodWeather) && Execute(ECCLExperimentAction::FoodAdd) &&
					   Execute(ECCLExperimentAction::Load),
				   TEXT("record continuation then alter and load")))
		{
			return;
		}
		Step = 6;
		Next = Now + 2.;
		return;
	}
	if (Step == 6)
	{
		if (D->IsRestoring())
		{
			return;
		}
		TArray<uint8> Actual;
		if (!Check(Capture(Actual) && Actual == SavedInventory && Execute(ECCLExperimentAction::Reset),
				   TEXT("restored exact food and policy then reset")))
		{
			return;
		}
		Step = 7;
		Next = Now + 2.;
		return;
	}
	if (Step == 7)
	{
		if (D->IsRestoring())
		{
			return;
		}
		if (!Check(!Open->GetFoodEnvironment().IsDynamic() && !Sealed->GetFoodEnvironment().IsDynamic() &&
					   Open->GetEntries()[0].Quantity == 4 && Execute(ECCLExperimentAction::Load),
				   TEXT("reset fixed policies then reload")))
		{
			return;
		}
		Step = 8;
		Next = Now + 2.;
		return;
	}
	if (D->IsRestoring())
	{
		return;
	}
	TArray<uint8> Actual;
	if (!Check(Capture(Actual) && Actual == SavedInventory && View.Rows[0].EnvironmentSource == 1 &&
				   View.Rows[2].EnvironmentSource == 2,
			   TEXT("final observations")))
	{
		return;
	}
	if (bRendered && Step == 8)
	{
		if (auto* Current = Screen())
		{
			Current->RevealFoodControls();
		}
		Step = 9;
		Next = Now + 1.;
		return;
	}
	if (bRendered && Step == 9)
	{
		FScreenshotRequest::RequestScreenshot(Directory / TEXT("food-environment-controls.png"), true, false);
		Step = 10;
		Next = Now + 1.;
		return;
	}
	if (bRendered && Step == 10)
	{
		if (Screen())
		{
			PC->CCLExperiment();
		}
		Step = 13;
		Next = Now + 1.;
		return;
	}
	if (bRendered && Step == 13)
	{
		FScreenshotRequest::RequestScreenshot(Directory / TEXT("food-environment-states.png"), true, false);
		Step = 14;
		Next = Now + 1.;
		return;
	}
	FString Oracle = FString::Printf(TEXT("%.12f\n%s\n"), View.WorldSeconds, *View.Epoch.ToString());
	for (const auto& Row : View.Rows)
	{
		Oracle += FString::Printf(TEXT("%d,%d,%.12f,%.12f,%.12f,%.12f\n"), Row.EnvironmentSource, Row.Quantity,
								  Row.Temperature, Row.Humidity, Row.Freshness, Row.Mold);
	}
	if (!Check(FFileHelper::SaveStringToFile(Oracle, *(Directory / TEXT("final.txt"))),
			   TEXT("publish observation oracle")))
	{
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_ENVIRONMENT RESTORE_READY"));
	Finish(true, TEXT("weather indoor held exposure rollback unload transfer save reset verified"));
}

void UCCLFoodEnvironmentSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_ENVIRONMENT %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!bSuccess || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLFoodEnvironmentSmoke"));
	}
}

TStatId UCCLFoodEnvironmentSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLFoodEnvironmentSmokeSubsystem, STATGROUP_Tickables);
}
