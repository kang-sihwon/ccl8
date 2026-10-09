#include "SCCLCelestialDebugView.h"

#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

void SCCLCelestialDebugView::Construct(const FArguments& Args)
{
	World = Args._World;
	SetClipping(EWidgetClipping::ClipToBounds);
}

void SCCLCelestialDebugView::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
	SLeafWidget::Tick(Geometry, CurrentTime, DeltaTime);
	if (!World.IsValid())
	{
		bReady = false;
		return;
	}
	for (TActorIterator<ACCLWorldEnvironmentState> It(World.Get()); It; ++It)
	{
		Published = It->GetTime();
		const auto& View = Published.Environment;
		FString Error;
		if (!View.bValid)
		{
			bReady = false;
			return;
		}
		if (CachedRevision != View.InputRevision || CachedEpoch != Published.Epoch)
		{
			FCCLCelestialDefinitionData Definition;
			Definition.DefinitionId = View.DefinitionId;
			Definition.Version = View.DefinitionVersion;
			Definition.Seed = View.Seed;
			Definition.EpochWorldSeconds = View.CelestialEpochSeconds;
			Definition.Bodies = View.CelestialBodies;
			bReady = System.Initialize(Definition, Error);
			if (!bReady)
			{
				return;
			}
			CachedRevision = View.InputRevision;
			CachedEpoch = Published.Epoch;
			const auto& Bodies = System.GetDefinition().Bodies;
			Orbits.SetNum(Bodies.Num());
			for (int32 I = 0; I < Bodies.Num(); ++I)
			{
				Orbits[I].Reset();
				const auto& Body = Bodies[I];
				const int32 Parent = Bodies.IndexOfByPredicate([&](const auto& B) { return B.BodyId == Body.ParentBodyId; });
				if (Parent == INDEX_NONE)
				{
					continue;
				}
				for (int32 Sample = 0; Sample <= 96; ++Sample)
				{
					TArray<FCCLCelestialBodyState> At;
					if (System.Evaluate(Definition.EpochWorldSeconds + Body.OrbitalPeriodSeconds * Sample / 96., At, Error))
					{
						Orbits[I].Add((At[I].PositionKm - At[Parent].PositionKm) / Body.SemiMajorAxisKm);
					}
				}
			}
		}
		bReady = System.Evaluate(Published.WorldSeconds, States, Error);
		return;
	}
	bReady = false;
}

int32 SCCLCelestialDebugView::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& CullingRect,
	FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const
{
	const FVector2D Size = G.GetLocalSize();
	const auto* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), White, ESlateDrawEffect::None, FLinearColor(0.012f, 0.022f, 0.04f, 0.97f));
	auto Text = [&](FVector2D Position, const FString& Value, FLinearColor Color, int32 Font = 14)
	{
		FSlateDrawElement::MakeText(Out, Layer + 4, G.ToPaintGeometry(FVector2D(1, 1), FSlateLayoutTransform(Position)),
			Value, FCoreStyle::GetDefaultFontStyle("Regular", Font), ESlateDrawEffect::None, Color);
	};
	Text({16, 12}, TEXT("천체 관측 · 드래그 회전 / 휠 확대"), FLinearColor::White, 18);
	Text({16, 40}, TEXT("거리·반경은 보기 위한 확대 축척 · 각도와 위상은 실제 계산값"), FLinearColor(0.6f, 0.7f, 0.85f), 12);
	if (!bReady)
	{
		Text({16, 76}, TEXT("서버 천체 정의와 확정 시각 수신 대기"), FLinearColor::Yellow);
		return Layer + 4;
	}

	const double Y = FMath::DegreesToRadians(Yaw), P = FMath::DegreesToRadians(Pitch);
	const FVector3d Forward(FMath::Cos(P) * FMath::Cos(Y), FMath::Cos(P) * FMath::Sin(Y), FMath::Sin(P));
	const FVector3d Right(-FMath::Sin(Y), FMath::Cos(Y), 0.);
	const FVector3d Up = FVector3d::CrossProduct(Forward, Right);
	auto Project = [&](const FVector3d& V, FVector2D Center, double Scale)
	{
		return FVector2f(Center + FVector2D(FVector3d::DotProduct(V, Right), -FVector3d::DotProduct(V, Up)) * Scale * Zoom);
	};
	auto Line = [&](FVector2f A, FVector2f B, FLinearColor Color, float Width = 1.f)
	{
		TArray<FVector2f> Points{A, B};
		FSlateDrawElement::MakeLines(Out, Layer + 3, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Width);
	};
	auto Sphere = [&](const FVector3d& Center, double Radius, FVector3d Light, FLinearColor Color,
		FVector2D Origin, double Scale, bool bEmissive)
	{
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		const auto Transform = G.GetAccumulatedRenderTransform();
		for (int32 Lat = 0; Lat < 20; ++Lat)
		{
			for (int32 Lon = 0; Lon < 40; ++Lon)
			{
				FVector3d N[4];
				for (int32 K = 0; K < 4; ++K)
				{
					const double A = -UE_PI / 2. + UE_PI * (Lat + (K / 2)) / 20.;
					const double B = 2. * UE_PI * (Lon + (K % 2)) / 40.;
					N[K] = FVector3d(FMath::Cos(A) * FMath::Cos(B), FMath::Cos(A) * FMath::Sin(B), FMath::Sin(A));
				}
				if (FVector3d::DotProduct(N[0] + N[1] + N[2] + N[3], Forward) <= 0.)
				{
					continue;
				}
				const SlateIndex Base = Vertices.Num();
				for (int32 K = 0; K < 4; ++K)
				{
					const float Brightness = bEmissive ? 1.f : 0.2f + 0.8f * FMath::Max(0., FVector3d::DotProduct(N[K], Light));
					FLinearColor Lit = Color * Brightness;
					Lit.A = 1.f;
					Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform,
						Project(Center + N[K] * Radius, Origin, Scale), FVector2f(0.5f, 0.5f), Lit.ToFColor(true)));
				}
				Indices.Append({Base, SlateIndex(Base + 1), SlateIndex(Base + 2), SlateIndex(Base + 1), SlateIndex(Base + 3), SlateIndex(Base + 2)});
			}
		}
		FSlateDrawElement::MakeCustomVerts(Out, Layer + 2, FSlateApplication::Get().GetRenderer()->GetResourceHandle(*White),
			Vertices, Indices, nullptr, 0, 0);
	};

	const auto& Bodies = System.GetDefinition().Bodies;
	const auto& View = Published.Environment;
	TArray<FVector3d> Positions;
	Positions.Init(FVector3d::ZeroVector, Bodies.Num());
	TArray<double> Spans;
	Spans.Init(0., Bodies.Num());
	const FVector2D SystemCenter(Size.X * 0.5, Size.Y * 0.29);
	const double SystemScale = FMath::Min(Size.X / 8.5, Size.Y * 0.063);
	double MinimumPlanetOrbit = TNumericLimits<double>::Max();
	for (const auto& Body : Bodies)
	{
		if (Body.Kind == ECCLCelestialKind::Planet && Body.SemiMajorAxisKm > 0.)
		{
			MinimumPlanetOrbit = FMath::Min(MinimumPlanetOrbit, Body.SemiMajorAxisKm);
		}
	}
	int32 StarIndex = INDEX_NONE, ObserverIndex = INDEX_NONE;
	for (int32 I = 0; I < Bodies.Num(); ++I)
	{
		if (Bodies[I].BodyId == View.DominantStarId)
		{
			StarIndex = I;
		}
		if (Bodies[I].BodyId == View.Observer.BodyId)
		{
			ObserverIndex = I;
		}
		const int32 Parent = Bodies.IndexOfByPredicate([&](const auto& B) { return B.BodyId == Bodies[I].ParentBodyId; });
		if (Parent != INDEX_NONE)
		{
			Spans[I] = Bodies[Parent].Kind == ECCLCelestialKind::Star
				? FMath::Clamp(2. * FMath::Sqrt(Bodies[I].SemiMajorAxisKm / MinimumPlanetOrbit), 1.5, 3.7) : 0.65;
			Positions[I] = Positions[Parent] + (States[I].PositionKm - States[Parent].PositionKm) * (Spans[I] / Bodies[I].SemiMajorAxisKm);
			for (int32 K = 1; K < Orbits[I].Num(); ++K)
			{
				Line(Project(Positions[Parent] + Orbits[I][K - 1] * Spans[I], SystemCenter, SystemScale),
					Project(Positions[Parent] + Orbits[I][K] * Spans[I], SystemCenter, SystemScale), FLinearColor(0.18f, 0.32f, 0.46f));
			}
		}
	}
	TArray<int32> Order;
	for (int32 I = 0; I < Bodies.Num(); ++I)
	{
		Order.Add(I);
	}
	Order.Sort([&](int32 A, int32 B) { return FVector3d::DotProduct(Positions[A], Forward) < FVector3d::DotProduct(Positions[B], Forward); });
	for (int32 I : Order)
	{
		const bool bStar = Bodies[I].Kind == ECCLCelestialKind::Star;
		const FVector3d Light = StarIndex == INDEX_NONE ? Forward : (States[StarIndex].PositionKm - States[I].PositionKm).GetSafeNormal();
		Sphere(Positions[I], bStar ? 0.38 : (I == ObserverIndex ? 0.21 : 0.13), Light,
			bStar ? FLinearColor(1.f, 0.6f, 0.08f) : FLinearColor(0.24f, 0.6f, 0.9f), SystemCenter, SystemScale, bStar);
		Text(FVector2D(Project(Positions[I], SystemCenter, SystemScale)) + FVector2D(12, 12), Bodies[I].BodyId.ToString(), FLinearColor::White, 12);
	}
	Line(FVector2f(16, Size.Y * 0.48), FVector2f(Size.X - 16, Size.Y * 0.48), FLinearColor(0.12f, 0.22f, 0.32f));
	if (ObserverIndex != INDEX_NONE)
	{
		const auto& S = States[ObserverIndex];
		const FVector3d Light = StarIndex == INDEX_NONE ? Forward : (States[StarIndex].PositionKm - S.PositionKm).GetSafeNormal();
		const FVector2D Center(Size.X * 0.5, Size.Y * 0.75);
		const double Scale = FMath::Min(Size.X * 0.2, Size.Y * 0.15);
		Sphere(FVector3d::ZeroVector, 1., Light, FLinearColor(0.16f, 0.48f, 0.78f), Center, Scale, false);
		const double Lat = FMath::DegreesToRadians(View.Observer.LatitudeDegrees);
		const double Meridian = S.SpinRadians + FMath::DegreesToRadians(View.Observer.LongitudeDegrees);
		const FVector3d Equatorial = FMath::Cos(Meridian) * S.EquatorX + FMath::Sin(Meridian) * S.EquatorY;
		const FVector3d Observer = FMath::Cos(Lat) * Equatorial + FMath::Sin(Lat) * S.SpinAxis;
		for (int32 I = 1; I <= 96; ++I)
		{
			auto RingPoint = [&](double Angle, double Latitude)
			{
				return FMath::Cos(Latitude) * (FMath::Cos(Angle) * S.EquatorX + FMath::Sin(Angle) * S.EquatorY) + FMath::Sin(Latitude) * S.SpinAxis;
			};
			const double A = 2. * UE_PI * (I - 1) / 96., B = 2. * UE_PI * I / 96.;
			for (int32 Ring = 0; Ring < 2; ++Ring)
			{
				const auto V = RingPoint(B, Ring ? Lat : 0.);
				const bool bFront = FVector3d::DotProduct(V, Forward) > 0.;
				if (bFront || I % 3 == 0)
				{
					FLinearColor Color = Ring ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(0.9f, 0.65f, 0.18f);
					Color.A = bFront ? 1.f : 0.22f;
					Line(Project(RingPoint(A, Ring ? Lat : 0.), Center, Scale), Project(V, Center, Scale), Color, 1.5f);
				}
			}
		}
		Line(Project(-S.SpinAxis * 1.3, Center, Scale), Project(S.SpinAxis * 1.4, Center, Scale), FLinearColor(0.1f, 0.85f, 1.f), 2.f);
		Text(FVector2D(Project(S.SpinAxis * 1.4, Center, Scale)), TEXT("자전축"), FLinearColor(0.1f, 0.85f, 1.f), 12);
		Line(Project(FVector3d::ZeroVector, Center, Scale), Project(Observer * 1.25, Center, Scale), FLinearColor::Green, 2.f);
		Text(FVector2D(Project(Observer * 1.25, Center, Scale)), TEXT("관측점"), FLinearColor::Green, 12);
		Line(Project(Light * 1.9, Center, Scale), Project(Light * 1.05, Center, Scale), FLinearColor::Yellow, 3.f);
		Text(FVector2D(Project(Light * 1.9, Center, Scale)), TEXT("빛"), FLinearColor::Yellow, 12);
		Text({16, Size.Y * 0.5}, FString::Printf(TEXT("위도 %.1f° · 축 기울기 %.1f° · 자전 %.1f°"), View.Observer.LatitudeDegrees,
			View.ObliquityDegrees, FMath::RadiansToDegrees(S.SpinRadians)), FLinearColor::White, 13);
		Text({16, Size.Y - 26}, FString::Printf(TEXT("입력 %llu · %02.0f시 %02d분 %02d초 · 밝은 면 / 어두운 면 = 낮 / 밤"),
			View.InputRevision, FMath::FloorToDouble(Published.WorldSeconds / 3600.), int32(FMath::Fmod(Published.WorldSeconds, 3600.) / 60.), int32(FMath::Fmod(Published.WorldSeconds, 60.))), FLinearColor(0.6f, 0.75f, 0.9f), 12);
	}
	return Layer + 4;
}

FReply SCCLCelestialDebugView::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	return Event.GetEffectingButton() == EKeys::LeftMouseButton ? FReply::Handled().CaptureMouse(SharedThis(this)) : FReply::Unhandled();
}

FReply SCCLCelestialDebugView::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	return HasMouseCapture() && Event.GetEffectingButton() == EKeys::LeftMouseButton ? FReply::Handled().ReleaseMouseCapture() : FReply::Unhandled();
}

FReply SCCLCelestialDebugView::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (HasMouseCapture())
	{
		Yaw += Event.GetCursorDelta().X * 0.4;
		Pitch = FMath::Clamp(Pitch + Event.GetCursorDelta().Y * 0.4, -85., 85.);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SCCLCelestialDebugView::OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Zoom = FMath::Clamp(Zoom * FMath::Pow(1.1, Event.GetWheelDelta()), 0.5, 1.6);
	return FReply::Handled();
}
