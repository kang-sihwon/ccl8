#include "CCLInventoryScreen.h"

#include "CCLCharacter.h"
#include "CCLPlayerController.h"
#include "SCCLInventoryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/NativeWidgetHost.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Widgets/SNullWidget.h"

void UCCLInventoryContext::BeginDestroy()
{
	Release();
	Super::BeginDestroy();
}

void UCCLInventoryContext::Initialize(ACCLPlayerController* InController)
{
	Controller = InController;
	Pawn = InController ? Cast<ACCLCharacter>(InController->GetPawn()) : nullptr;
	SelectedItem = 0;
	SelectedEquipment.Invalidate();
}

void UCCLInventoryContext::UpdatePreview()
{
	if (!IsUsable())
	{
		return;
	}

	if (!Preview)
	{
		Preview = NewObject<UTextureRenderTarget2D>(this);
		Preview->ClearColor = FLinearColor(0.025f, 0.04f, 0.065f, 1.f);
		Preview->InitAutoFormat(512, 768);
	}

	if (!Camera)
	{
		Camera = NewObject<USceneCaptureComponent2D>(Controller.Get());
		Camera->RegisterComponent();
		Camera->TextureTarget = Preview;
		Camera->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Camera->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		Camera->FOVAngle = 32.f;
		Camera->bCaptureEveryFrame = true;
		Camera->ShowFlags.SetAtmosphere(false);
		Camera->ShowFlags.SetFog(false);
	}

	const FVector Forward = Pawn->GetActorForwardVector();
	Camera->SetWorldLocation(Pawn->GetActorLocation() + Forward * 340.f);
	Camera->SetWorldRotation((-Forward).Rotation());
	Camera->ShowOnlyActors.Reset();
	Camera->ShowOnlyComponents.Reset();
	Camera->ShowOnlyActorComponents(Pawn.Get(), true);
}

void UCCLInventoryContext::Release()
{
	if (Camera)
	{
		Camera->DestroyComponent();
		Camera = nullptr;
	}

	Preview = nullptr;
	Controller.Reset();
	Pawn.Reset();
}

bool UCCLInventoryContext::IsUsable() const
{
	return Controller.IsValid() && Controller->IsLocalController() && Pawn.IsValid() &&
		Controller->GetPawn() == Pawn.Get() && !Pawn->IsDead();
}

ACCLPlayerController* UCCLInventoryContext::GetController() const
{
	return Controller.Get();
}

void UCCLInventoryScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Host = WidgetTree->ConstructWidget<UNativeWidgetHost>();
	WidgetTree->RootWidget = Host;
}

void UCCLInventoryScreen::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	auto* Context = Cast<UCCLInventoryContext>(GetContext());
	if (!Context || !Context->IsUsable())
	{
		RequestClose();
		return;
	}

	Context->UpdatePreview();
}

FReply UCCLInventoryScreen::NativeOnFocusReceived(const FGeometry& Geometry, const FFocusEvent& Event)
{
	return Body ? FReply::Handled().SetUserFocus(Body.ToSharedRef(), Event.GetCause()) : Super::NativeOnFocusReceived(Geometry, Event);
}

void UCCLInventoryScreen::OnContextBound()
{
	if (auto* Context = Cast<UCCLInventoryContext>(GetContext()))
	{
		Context->UpdatePreview();
		Body = SNew(SCCLInventoryWidget).Controller(Context->GetController());
		Host->SetContent(Body.ToSharedRef());
	}
}

void UCCLInventoryScreen::OnContextReleased()
{
	if (Host)
	{
		Host->SetContent(SNullWidget::NullWidget);
	}

	Body.Reset();
	if (auto* Context = Cast<UCCLInventoryContext>(GetContext()))
	{
		if (auto* PC = Context->GetController())
		{
			SCCLInventoryWidget::CancelOwnedDrag(PC);
			PC->FlushPressedKeys();
		}

		Context->Release();
	}
}
