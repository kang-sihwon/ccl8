#include "CCLExperimentPlayerController.h"

#include "CCLExperimentDirector.h"
#include "CCLTerrainReplication.h"
#include "CCLSurfaceReplication.h"
#include "CCLTerrainRegion.h"
#include "CCLExperimentScreen.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ExperimentScreen, "UI.View.EnvironmentExperiment");

ACCLExperimentPlayerController::ACCLExperimentPlayerController()
{
	TerrainReplication = CreateDefaultSubobject<UCCLTerrainReplication>(TEXT("TerrainReplication"));
	SurfaceReplication = CreateDefaultSubobject<UCCLSurfaceReplication>(TEXT("SurfaceReplication"));
}

void ACCLExperimentPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (IsLocalController() && !bOpenedOnce && GetPawn())
	{
		if (const auto* Director = ACCLExperimentDirector::Find(GetWorld()); Director && Director->IsReady())
		{
			CCLExperiment();
			bOpenedOnce = ExperimentView.IsValid();
		}
	}
}

void ACCLExperimentPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	auto* Local = GetLocalPlayer();
	auto* InputSystem = Local ? Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	auto* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (InputSystem && EnhancedInput && !ExperimentInput)
	{
		ExperimentInput = NewObject<UInputMappingContext>(this);
		ToggleAction = NewObject<UInputAction>(this);
		ToggleAction->ValueType = EInputActionValueType::Boolean;
		ExperimentInput->MapKey(ToggleAction, EKeys::F8);
		InputSystem->AddMappingContext(ExperimentInput, 1);
		EnhancedInput->BindAction(ToggleAction, ETriggerEvent::Started, this, &ThisClass::CCLExperiment);
	}
}

void ACCLExperimentPlayerController::EndPlay(EEndPlayReason::Type Reason)
{
	const auto* LocalPlayer = GetLocalPlayer();
	if (auto* UI = LocalPlayer ? LocalPlayer->GetSubsystem<UCCLUISubsystem>() : nullptr)
	{
		UI->CloseView(ExperimentView);
		UI->UnregisterView(Registration);
	}

	if (const auto* Local = GetLocalPlayer())
	{
		if (auto* InputSystem = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSystem->RemoveMappingContext(ExperimentInput);
		}
	}

	Super::EndPlay(Reason);
}

void ACCLExperimentPlayerController::CCLExperiment()
{
	auto* UI = CCLGameUI::Get(this);
	if (!UI)
	{
		return;
	}

	if (UI->IsViewOpen(ExperimentView))
	{
		UI->CloseView(ExperimentView);
		return;
	}

	if (!Registration.IsValid())
	{
		FCCLUIViewDefinition Definition;
		Definition.Tag = TAG_ExperimentScreen;
		Definition.WidgetClass = UCCLExperimentScreen::StaticClass();
		Definition.RequiredContextClass = UCCLExperimentContext::StaticClass();
		Definition.Layer = CCLUITags::Layer_Menu;
		Definition.InputPolicy = ECCLUIInputPolicy::Menu;
		Definition.Groups.AddTag(CCLUITags::Group_Menus);
		Registration = UI->RegisterView(Definition, this);
	}

	auto* Context = NewObject<UCCLExperimentContext>(this);
	Context->Controller = this;
	ExperimentView = UI->OpenView(TAG_ExperimentScreen, Context, this);
}

void ACCLExperimentPlayerController::Submit(ECCLExperimentAction Action, FName CaseId)
{
	if (const auto* Director = ACCLExperimentDirector::Find(GetWorld()))
	{
		const auto* Result = Director->FindResult(CaseId);
		ServerExperiment(Action, CaseId, Director->GetGeneration(), Result ? Result->RunId : FGuid(), Director->GetTerrainRegion() ? Director->GetTerrainRegion()->GetPublicationSerial() : 0);
	}
}

void ACCLExperimentPlayerController::ServerExperiment_Implementation(ECCLExperimentAction Action, FName CaseId,
	FGuid Generation, FGuid RunId, uint64 TerrainSerial)
{
	FString Message;
	auto* Director = ACCLExperimentDirector::Find(GetWorld());
	const bool bAccepted = Director && Director->Execute(this, Action, CaseId, Generation, RunId, Message, TerrainSerial);
	ClientExperimentResponse(Message.IsEmpty() ? (bAccepted ? TEXT("조작 요청을 처리했다.") : TEXT("조작 요청을 거부했다.")) : Message);
}

void ACCLExperimentPlayerController::ClientExperimentResponse_Implementation(const FString& Message)
{
	LastMessage = Message;
}
