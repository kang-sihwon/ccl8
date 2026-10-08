#include "CCLMapScreen.h"

#include "CCLGameUI.h"
#include "Core/CCLUISubsystem.h"
#include "Map/CCLMapSystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void UCCLMapScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	WidgetTree->RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCCLMapScreen::OnManagedTick(float DeltaTime)
{
	Super::OnManagedTick(DeltaTime);
	RefreshTime += DeltaTime;
	if (RefreshTime >= 0.2f && GetOwningLocalPlayer())
	{
		GetOwningLocalPlayer()->GetSubsystem<UCCLMapSubsystem>()->Refresh(GetOwningPlayer());
		RefreshTime = 0;
	}
}

int32 UCCLMapScreen::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, Layer, Style, bParentEnabled);
	const auto* Context = Cast<UCCLMapContext>(GetContext());
	const auto* PC = GetOwningPlayer();
	const auto* Map = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UCCLMapSubsystem>() : nullptr;
	if (!Context || !Map || !PC || !PC->GetPawn())
	{
		return Layer;
	}
	const FVector2D Screen = Geometry.GetLocalSize();
	const float Scale = FMath::Min(Screen.X / 1280, Screen.Y / 720);
	const FVector2D Size = (Context->bFullMap ? FVector2D(880, 480) : FVector2D(270, 220)) * Scale;
	const FVector2D Origin = Context->bFullMap ? (Screen - Size) / 2 : FVector2D(Screen.X - Size.X - 24 * Scale, 68 * Scale);
	FBox2D Bounds = Map->GetBounds();
	if (!Context->bFullMap)
	{
		const FVector2D Center(PC->GetPawn()->GetActorLocation());
		Bounds = FBox2D(Center - FVector2D(1400, 1140), Center + FVector2D(1400, 1140));
	}
	auto Box = [&](FVector2D Position, FVector2D Extent, FLinearColor Color)
	{
		FSlateDrawElement::MakeBox(Elements, ++Layer, Geometry.ToPaintGeometry(Extent, FSlateLayoutTransform(Position)),
			FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color * Style.GetColorAndOpacityTint());
	};
	auto Text = [&](FVector2D Position, const FString& Value, FLinearColor Color, int32 FontSize)
	{
		FSlateDrawElement::MakeText(Elements, ++Layer, Geometry.ToPaintGeometry(FVector2D(1), FSlateLayoutTransform(Position)),
			Value, FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::Max(12, FMath::RoundToInt(FontSize * Scale))),
			ESlateDrawEffect::None, Color * Style.GetColorAndOpacityTint());
	};
	Box(Origin - FVector2D(3, 36) * Scale, Size + FVector2D(6, 72) * Scale, FLinearColor(0.02f, 0.035f, 0.05f, 0.96f));
	Text(Origin - FVector2D(0, 31) * Scale, Context->bFullMap ? TEXT("REGION MAP   |   Esc: close") : TEXT("NEARBY   |   M: map"), FLinearColor::White, 20);
	Elements.PushClip(FSlateClippingZone(Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Origin))));
	for (const auto& Terrain : Map->GetTerrain())
	{
		const FVector2D A = UCCLMapSubsystem::Project(FVector(Terrain.Min.X, Terrain.Max.Y, 0), Bounds, Size);
		const FVector2D B = UCCLMapSubsystem::Project(FVector(Terrain.Max.X, Terrain.Min.Y, 0), Bounds, Size);
		Box(Origin + A, B - A, Terrain.Color);
	}
	TArray<FVector2D> Labels;
	for (const auto& Marker : Map->GetMarkers())
	{
		const FVector2D Position = Origin + UCCLMapSubsystem::Project(Marker.Position, Bounds, Size);
		const FLinearColor Color = Marker.Kind == ECCLMapKind::Player ? FLinearColor(0.15f, 0.8f, 1) :
			Marker.Kind == ECCLMapKind::Enemy ? FLinearColor(1, 0.25f, 0.2f) : FLinearColor(0.9f, 0.75f, 0.2f);
		Box(Position - FVector2D(4, 4) * Scale, FVector2D(8, 8) * Scale, Color);
		if (Context->bFullMap && !Marker.Label.IsEmpty() && !Labels.ContainsByPredicate([&](FVector2D Prior)
			{ return FMath::Abs(Prior.X - Position.X) < 130 * Scale && FMath::Abs(Prior.Y - Position.Y) < 25 * Scale; }))
		{
			Text(Position + FVector2D(7, -9) * Scale, Marker.Label, Color, 16);
			Labels.Add(Position);
		}
	}
	Elements.PopClip();
	Text(Origin + FVector2D(0, Size.Y + 5 * Scale), Context->bFullMap ? TEXT("Blue: you   Gold: NPC   Red: visible enemy") : TEXT("N ^   Player / NPC / Enemy"), FLinearColor(0.7f, 0.8f, 0.85f), Context->bFullMap ? 18 : 15);
	return Layer;
}

void UCCLMapScreen::OnContextBound()
{
	Super::OnContextBound();
	const auto* Context = Cast<UCCLMapContext>(GetContext());
	if (Context && Context->bFullMap)
	{
		if (auto* UI = CCLGameUI::Get(GetOwningPlayer()))
		{
			FCCLUIPresentationDefinition Request;
			Request.Groups.AddTag(CCLUITags::Group_HUD);
			Request.bHide = 1;
			HUDHidden = UI->PushPresentation(Request, this);
		}
	}
}
void UCCLMapScreen::OnContextReleased()
{
	if (auto* Local = GetOwningLocalPlayer())
	{
		Local->GetSubsystem<UCCLUISubsystem>()->ReleasePresentation(HUDHidden);
	}
	HUDHidden = {};
	Super::OnContextReleased();
}
