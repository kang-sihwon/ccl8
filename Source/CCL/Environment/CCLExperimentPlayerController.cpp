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
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

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
	if (IsLocalController() && bTerrainToolActive)
	{
		const auto* Director = ACCLExperimentDirector::Find(GetWorld());
		const auto* Region = Director ? Director->GetTerrainRegion() : nullptr;
		FHitResult Hit;
		if (Region && TraceTerrainCursor(Hit))
		{
			const bool bReady = Hit.GetActor() == Region && Hit.ImpactNormal.Z > 0.4 && !Region->IsPreparing()
				&& (Region->HasAuthority() || Region->IsReplicaReady());
						const float Radius = TerrainTool == ECCLExperimentAction::TerrainChannel ? 80.f : 100.f;
			const FVector BrushCenter = Hit.ImpactPoint + FVector(0., 0., TerrainTool == ECCLExperimentAction::TerrainChannel ? 35. : 0.);
			const FBox Bounds = Region->GetWorldTerrainBounds();
			const FVector Extent(Radius + 250.f);
			bool bCanEditBrush = bReady && Bounds.IsInsideOrOn(BrushCenter - Extent) && Bounds.IsInsideOrOn(BrushCenter + Extent);
			if (Region->GetTerrainStore().IsInitialized())
			{
				const auto& Definition = Region->GetTerrainStore().GetSnapshot().Definition;
				const FVector Local = (BrushCenter - Region->GetActorLocation()) / 100.;
				const FBox Affected(Local - Extent / 100., Local + Extent / 100.);
				for (const auto& Protected : Definition.ProtectedRegions)
				{
					bCanEditBrush &= !Affected.Intersect(Protected.BoundsMeters);
				}
			}
			const FColor Color = bCanEditBrush ? FColor::Green : FColor::Red;
			DrawDebugSphere(GetWorld(), BrushCenter, Radius, 32, Color, false, 0.f, 0, 2.f);
			DrawDebugDirectionalArrow(GetWorld(), Hit.ImpactPoint + FVector(0., 0., 260.),
				Hit.ImpactPoint + FVector(0., 0., 25.), 35.f, Color, false, 0.f, 0, 3.f);
			DrawDebugBox(GetWorld(), Bounds.GetCenter(), Bounds.GetExtent(), FColor::Cyan, false, 0.f);
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
		ExperimentInput->MapKey(ToggleAction, EKeys::F7);
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

void ACCLExperimentPlayerController::SelectTerrainTool(ECCLExperimentAction Action)
{
	if (Action < ECCLExperimentAction::TerrainExcavate || Action > ECCLExperimentAction::TerrainChannel)
	{
		return;
	}

	TerrainTool = Action;
	bTerrainToolActive = 1;
	LastMessage = TEXT("도구 선택됨. 패널 오른쪽의 지형 바닥을 클릭하면 표시 범위에 적용한다. 우클릭은 선택 취소.");
}

void ACCLExperimentPlayerController::CancelTerrainTool()
{
	bTerrainToolActive = 0;
}

bool ACCLExperimentPlayerController::ApplyTerrainTool()
{
	const auto* Director = ACCLExperimentDirector::Find(GetWorld());
	const auto* Region = Director ? Director->GetTerrainRegion() : nullptr;
	FHitResult Hit;
	if (!bTerrainToolActive || !Region || !TraceTerrainCursor(Hit)
		|| Hit.GetActor() != Region || Hit.ImpactNormal.Z <= 0.4 || Region->IsPreparing() || (!Region->HasAuthority() && !Region->IsReplicaReady()))
	{
		LastMessage = TEXT("청록색 테두리 안의 준비된 지형 바닥을 클릭해 줘.");
		return false;
	}

	ServerTerrainEdit(TerrainTool, Hit.ImpactPoint, Director->GetGeneration(), Region->GetPublicationSerial());
	return true;
}

void ACCLExperimentPlayerController::ServerTerrainEdit_Implementation(ECCLExperimentAction Action, FVector Target,
	FGuid Generation, uint64 TerrainSerial)
{
	FString Message;
	auto* Director = ACCLExperimentDirector::Find(GetWorld());
	const bool bActionValid = Action >= ECCLExperimentAction::TerrainExcavate && Action <= ECCLExperimentAction::TerrainChannel;
	const bool bAccepted = bActionValid && Director && Director->Execute(this, Action, TEXT("Zone_05"),
		Generation, FGuid(), Message, TerrainSerial, &Target);
	ClientExperimentResponse(Message.IsEmpty() ? (bAccepted ? TEXT("선택 위치에 지형 편집을 요청했다.") : TEXT("지형 편집 요청을 거부했다.")) : Message);
}

void ACCLExperimentPlayerController::ClientExperimentResponse_Implementation(const FString& Message)
{
	LastMessage = Message;
}

void ACCLExperimentPlayerController::SetTerrainCursor(const FVector2D& ViewportPosition)
{
	TerrainCursor = ViewportPosition;
	bHasTerrainCursor = 1;
}

bool ACCLExperimentPlayerController::TraceTerrainCursor(FHitResult& Hit) const
{
	if (!bHasTerrainCursor)
	{
		return false;
	}
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ExperimentTerrainCursor), true);
	Query.AddIgnoredActor(GetPawn());
	return GetHitResultAtScreenPosition(TerrainCursor, ECC_Visibility, Query, Hit);
}
