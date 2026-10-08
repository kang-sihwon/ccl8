#include "CCLIntegrationSmokeSubsystem.h"

#include "CCLPlayerController.h"
#include "Map/CCLMapSystem.h"
#include "Presentation/CCLCinematicSubsystem.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/Paths.h"

bool UCCLIntegrationSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Param(FCommandLine::Get(), TEXT("CCLIntegrationSmoke"));
#endif
}
TStatId UCCLIntegrationSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLIntegrationSmokeSubsystem, STATGROUP_Tickables);
}
bool UCCLIntegrationSmokeSubsystem::Check(bool Condition, const TCHAR* Message)
{
	bPassed = bPassed && Condition;
	UE_LOG(LogTemp, Display, TEXT("CCL_INTEGRATION %s %s"), Condition ? TEXT("CHECK") : TEXT("FAIL"), Message);
	return Condition;
}
void UCCLIntegrationSmokeSubsystem::Capture(const FString& Name)
{
	int32 Width = 0;
	int32 Height = 0;
	GetWorld()->GetFirstPlayerController()->GetViewportSize(Width, Height);
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("Tests/Presentation");
	IFileManager::Get().MakeDirectory(*Directory, true);
	FScreenshotRequest::RequestScreenshot(Directory / FString::Printf(TEXT("%dx%d-%s.png"), Width, Height, *Name), true, false);
}
void UCCLIntegrationSmokeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	auto* PC = Cast<ACCLPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PC || !PC->GetPawn() || !PC->GetLocalPlayer())
	{
		return;
	}
	Elapsed += DeltaTime;
	auto* Map = PC->GetLocalPlayer()->GetSubsystem<UCCLMapSubsystem>();
	auto* Scene = PC->GetLocalPlayer()->GetSubsystem<UCCLCinematicSubsystem>();
	auto* UI = CCLGameUI::Get(PC);
	if (Step == 0 && Elapsed > 2)
	{
		Check(Scene->IsActive(), TEXT("arrival cinematic owns a live presentation"));
		Capture(TEXT("arrival"));
		++Step;
	}
	else if (Step == 1 && Elapsed > 6)
	{
		int32 ExpectedX = 1280;
		int32 ExpectedY = 720;
		FParse::Value(FCommandLine::Get(), TEXT("ResX="), ExpectedX);
		FParse::Value(FCommandLine::Get(), TEXT("ResY="), ExpectedY);
		int32 ActualX = 0;
		int32 ActualY = 0;
		PC->GetViewportSize(ActualX, ActualY);
		Check(ActualX == ExpectedX && ActualY == ExpectedY, TEXT("requested viewport resolution is active"));
		Check(!Scene->IsActive() && UI->GetPresentationCount() == 0, TEXT("completed cinematic releases UI and camera ownership"));
		Check(PC->GetViewTarget() == PC->GetPawn(), TEXT("gameplay camera restored"));
		Map->Refresh(PC);
		Check(!Map->GetTerrain().IsEmpty(), TEXT("both maps have actual level terrain"));
		Check(Map->GetMarkers().ContainsByPredicate([](const FCCLMapMarkerView& M) { return M.Kind == ECCLMapKind::Player; }), TEXT("player appears on map"));
		const FBox2D Bounds(FVector2D(-100, -100), FVector2D(100, 100));
		Check(UCCLMapSubsystem::Project(FVector::ZeroVector, Bounds, FVector2D(200, 100)).Equals(FVector2D(100, 50)), TEXT("map coordinate conversion"));
		Capture(TEXT("minimap"));
		++Step;
	}
	else if (Step == 2 && Elapsed > 7)
	{
		PC->ToggleWorldMap();
		++Step;
	}
	else if (Step == 3 && Elapsed > 8)
	{
		Capture(TEXT("world-map"));
		++Step;
	}
	else if (Step == 4 && Elapsed > 9)
	{
		PC->ToggleWorldMap();
		const FGuid First = Scene->Play(PC, PC->GetPawn()->GetActorLocation(), TEXT("FIRST"));
		const FGuid Second = Scene->Play(PC, PC->GetPawn()->GetActorLocation(), TEXT("SECOND"));
		Check(!Scene->Cancel(First) && Scene->IsActive(), TEXT("stale handle cannot cancel replacement cinematic"));
		Check(Scene->Cancel(Second) && UI->GetPresentationCount() == 0 && PC->GetViewTarget() == PC->GetPawn(),
			TEXT("interrupted cinematic restores UI, input and camera"));
		++Step;
	}
	else if (Step == 5 && Elapsed > 10)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_INTEGRATION %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
		++Step;
		PC->ConsoleCommand(TEXT("quit"));
	}
}
