#include "CCLMapSystem.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

bool UCCLMapMarkerComponent::IsRevealedTo(const APlayerController* Observer) const
{
	if (!Observer || !Observer->GetPawn() || !IsValid(GetOwner()) || GetOwner()->IsHidden())
	{
		return false;
	}
	if (GetOwner() == Observer->GetPawn())
	{
		return true;
	}
	return RevealDistance > 0 && FVector::DistSquared(Observer->GetPawn()->GetActorLocation(), GetOwner()->GetActorLocation()) <= FMath::Square(RevealDistance) &&
		(!bRequireLineOfSight || Observer->LineOfSightTo(GetOwner()));
}

void UCCLMapSubsystem::SetRegion(UCCLMapRegionDefinition* Definition)
{
	Region = Definition;
	CachedWorld.Reset();
}

void UCCLMapSubsystem::Refresh(APlayerController* Observer)
{
	if (!Observer || !Observer->GetPawn())
	{
		Markers.Reset();
		return;
	}
	UWorld* World = Observer->GetWorld();
	if (CachedWorld != World)
	{
		CachedWorld = World;
		Terrain.Reset();
		Bounds = FBox2D(EForceInit::ForceInit);
		// The authored level's static geometry is public terrain, shared by both map views.
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			const auto* Mesh = It->GetStaticMeshComponent();
			if (!Mesh || !Mesh->IsCollisionEnabled())
			{
				continue;
			}
			const FBox Box = Mesh->Bounds.GetBox();
			const FVector Extent = Box.GetExtent();
			if (Extent.X < 25 || Extent.Y < 25 || Extent.X > 20000 || Extent.Y > 20000)
			{
				continue;
			}
			FCCLMapTerrain Tile;
			Tile.Min = FVector2D(Box.Min);
			Tile.Max = FVector2D(Box.Max);
			Tile.Color = Extent.Z < 100 ? FLinearColor(0.12f, 0.19f, 0.19f) : FLinearColor(0.3f, 0.34f, 0.32f);
			Terrain.Add(Tile);
			Bounds += Tile.Min;
			Bounds += Tile.Max;
		}
		if (Region && Region->Max.X > Region->Min.X && Region->Max.Y > Region->Min.Y)
		{
			Bounds = FBox2D(Region->Min, Region->Max);
			Terrain = Region->Terrain;
		}
		Terrain.Sort([](const FCCLMapTerrain& A, const FCCLMapTerrain& B)
		{
			const FVector2D AS = A.Max - A.Min;
			const FVector2D BS = B.Max - B.Min;
			return AS.X * AS.Y > BS.X * BS.Y;
		});
		if (!Bounds.bIsValid || Bounds.GetSize().GetMin() < 1)
		{
			Bounds = FBox2D(FVector2D(-2000, -1600), FVector2D(4000, 1600));
		}
	}
	Markers.Reset();
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		const auto* Marker = It->FindComponentByClass<UCCLMapMarkerComponent>();
		if (!Marker || !Marker->IsRevealedTo(Observer))
		{
			continue;
		}
		auto& View = Markers.AddDefaulted_GetRef();
		View.Position = It->GetActorLocation();
		View.Heading = It->GetActorRotation().Yaw;
		View.Kind = Marker->Kind;
		View.Label = Marker->Label.IsEmpty() ? (It->IsA<APawn>() && *It == Observer->GetPawn() ? TEXT("You") : TEXT("")) : Marker->Label.ToString();
	}
}

FVector2D UCCLMapSubsystem::Project(FVector World, const FBox2D& InBounds, FVector2D Size)
{
	const FVector2D Extent = InBounds.GetSize();
	return FVector2D((World.X - InBounds.Min.X) / FMath::Max(1., Extent.X) * Size.X,
		(1 - (World.Y - InBounds.Min.Y) / FMath::Max(1., Extent.Y)) * Size.Y);
}
