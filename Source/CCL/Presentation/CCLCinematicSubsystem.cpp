#include "CCLCinematicSubsystem.h"

#include "UI/CCLUIInputData.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/PointLight.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Sound/SoundWaveProcedural.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void UCCLCinematicScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	WidgetTree->RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UCCLCinematicScreen::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, Layer, Style, bParentEnabled);
	const auto* Context = Cast<UCCLCinematicContext>(GetContext());
	if (!Context)
	{
		return Layer;
	}
	const FVector2D Size = Geometry.GetLocalSize();
	const float Scale = Size.Y / 720;
	for (double Y : {0., Size.Y - 95 * Scale})
	{
		FSlateDrawElement::MakeBox(Elements, ++Layer, Geometry.ToPaintGeometry(FVector2D(Size.X, 95 * Scale), FSlateLayoutTransform(FVector2D(0, Y))),
			FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, FLinearColor::Black);
	}
	FSlateDrawElement::MakeText(Elements, ++Layer, Geometry.ToPaintGeometry(FVector2D(1), FSlateLayoutTransform(FVector2D(40, Size.Y - 76 * Scale))),
		Context->Title + FString::Printf(TEXT("   ·   %s: 건너뛰기"), UCCLUIInputData::GetBackKeyLabel(this)), FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FMath::RoundToInt(28 * Scale)),
		ESlateDrawEffect::None, FLinearColor(0.95f, 0.85f, 0.6f));
	return Layer;
}

FGuid UCCLCinematicSubsystem::Play(APlayerController* Controller, FVector Focus, const FString& Title, float Duration)
{
	if (!Controller || !Controller->IsLocalController() || !Controller->GetPawn() || Focus.ContainsNaN() || !FMath::IsFinite(Duration) || Duration <= 0)
	{
		return {};
	}
	Cancel(Active);
	auto* UI = CCLGameUI::Get(Controller);
	if (!UI)
	{
		return {};
	}
	Observer = Controller;
	PreviousTarget = Controller->GetViewTarget();
	InitialPawn = Controller->GetPawn();
	Active = FGuid::NewGuid();
	FocusPoint = Focus;
	Elapsed = 0;
	Seconds = FMath::Clamp(Duration, 0.1f, 15.f);
	Camera = Controller->GetWorld()->SpawnActor<ACameraActor>();
	if (!Camera)
	{
		Active.Invalidate();
		return {};
	}
	Camera->GetCameraComponent()->FieldOfView = 65;
	Camera->GetCameraComponent()->PostProcessSettings.bOverride_VignetteIntensity = true;
	Camera->GetCameraComponent()->PostProcessSettings.VignetteIntensity = 0.5f;
	Camera->SetActorLocation(FocusPoint + FVector(-650, -550, 500));
	Camera->SetActorRotation((FocusPoint - Camera->GetActorLocation()).Rotation());
	Controller->SetViewTargetWithBlend(Camera, 0.4f);
	FCCLUIPresentationDefinition Request;
	Request.Groups.AddTag(CCLUITags::Group_HUD);
	Request.Groups.AddTag(CCLUITags::Group_Menus);
	Request.Groups.AddTag(CCLUITags::Group_Dialogue);
	Request.bHide = 1;
	Request.bBlockGameplay = 1;
	Request.FadeSeconds = 0.25f;
	UIHandle = UI->PushPresentation(Request, this);
	auto* Context = NewObject<UCCLCinematicContext>(this);
	Context->Title = Title;
	Overlay = UI->OpenView(CCLUITags::View_Cinematic, Context, this);
	Light = Controller->GetWorld()->SpawnActor<APointLight>(FocusPoint + FVector(0, 0, 200), FRotator::ZeroRotator);
	if (IsValid(Light))
	{
		Light->SetMobility(EComponentMobility::Movable);
		Light->PointLightComponent->SetLightColor(FLinearColor(1, 0.65f, 0.2f));
		Light->PointLightComponent->SetAttenuationRadius(700);
		Light->PointLightComponent->SetIntensity(18000);
	}
	Sound = NewObject<USoundWaveProcedural>(this);
	Sound->SetSampleRate(22050);
	Sound->NumChannels = 1;
	Sound->Duration = INDEFINITELY_LOOPING_DURATION;
	TArray<int16> Samples;
	Samples.SetNumUninitialized(22050);
	for (int32 I = 0; I < Samples.Num(); ++I)
	{
		const double T = double(I) / 22050;
		Samples[I] = static_cast<int16>((FMath::Sin(T * 2 * PI * 440) + FMath::Sin(T * 2 * PI * 660)) * 1800 * FMath::Sin(PI * T));
	}
	Sound->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
	Audio = NewObject<UAudioComponent>(Camera);
	Audio->bAutoDestroy = false;
	Audio->bIsUISound = true;
	Audio->SetSound(Sound);
	Audio->RegisterComponentWithWorld(Controller->GetWorld());
	Audio->Play();
	return Active;
}

bool UCCLCinematicSubsystem::Cancel(FGuid Handle)
{
	if (!Handle.IsValid() || Handle != Active)
	{
		return false;
	}
	if (auto* Controller = Observer.Get())
	{
		if (auto* UI = Controller->GetLocalPlayer() ? Controller->GetLocalPlayer()->GetSubsystem<UCCLUISubsystem>() : nullptr)
		{
			UI->ReleasePresentation(UIHandle);
			UI->CloseView(Overlay);
		}
		if (Controller->GetViewTarget() == Camera)
		{
			Controller->SetViewTarget(PreviousTarget.IsValid() ? PreviousTarget.Get() : Controller->GetPawn());
		}
	}
	if (IsValid(Audio))
	{
		Audio->Stop();
		Audio->DestroyComponent();
	}
	if (IsValid(Light))
	{
		Light->Destroy();
	}
	if (IsValid(Camera))
	{
		Camera->Destroy();
	}
	Audio = nullptr;
	Sound = nullptr;
	Light = nullptr;
	Camera = nullptr;
	Active.Invalidate();
	return true;
}

void UCCLCinematicSubsystem::Tick(float DeltaTime)
{
	if (!IsActive())
	{
		return;
	}
	const auto* Controller = Observer.Get();
	const auto* ASC = InitialPawn.IsValid() ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InitialPawn.Get()) : nullptr;
	Elapsed += DeltaTime;
	if (!Controller || !IsValid(Camera) || Controller->GetPawn() != InitialPawn.Get() || Elapsed >= Seconds ||
		(ASC && ASC->HasAttributeSetForAttribute(UCCLHealthSet::GetHealthAttribute()) && ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) <= 0))
	{
		Cancel(Active);
		return;
	}
	const float Alpha = Elapsed / Seconds;
	Camera->SetActorLocation(FocusPoint + FVector(-650 + 400 * Alpha, -550, 500 - 100 * Alpha));
	Camera->SetActorRotation((FocusPoint - Camera->GetActorLocation()).Rotation());
	if (IsValid(Light))
	{
		Light->PointLightComponent->SetIntensity(18000 * (1 - Alpha));
	}
}

TStatId UCCLCinematicSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLCinematicSubsystem, STATGROUP_Tickables);
}

void UCCLCinematicSubsystem::Deinitialize()
{
	Cancel(Active);
	Super::Deinitialize();
}
