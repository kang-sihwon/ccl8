#include "CCLTerrainNavigation.h"

#include "CCLTerrainRegion.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

ACCLTerrainNavigationBounds::ACCLTerrainNavigationBounds()
{
	GetRootComponent()->SetMobility(EComponentMobility::Movable);
	BoundsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("TerrainNavigationBounds"));
	BoundsComponent->SetupAttachment(GetRootComponent());
	BoundsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsComponent->SetCanEverAffectNavigation(false);
	BoundsComponent->SetBoxExtent(FVector(1600., 1600., 1600.));
}

void ACCLTerrainNavigationBounds::Configure(const FBox& Bounds)
{
	SetActorLocation(Bounds.GetCenter());
	BoundsComponent->SetBoxExtent(Bounds.GetExtent());
	if (auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		Nav->OnNavigationBoundsUpdated(this);
	}
}

bool FCCLTerrainNavigation::IsRouteReady(const UWorld* World, const FVector& Start, const FVector& End)
{
	UNavigationPath* CandidatePath = nullptr;
	for (TActorIterator<ACCLTerrainRegion> It(World); It; ++It)
	{
		if (!It->IsNavigationReady())
		{
			const FBox Bounds = It->GetWorldTerrainBounds().ExpandBy(100.);
			if (Bounds.IsInsideOrOn(Start) || Bounds.IsInsideOrOn(End) || FMath::LineBoxIntersection(Bounds, Start, End, End - Start))
			{
				return false;
			}
            if (!CandidatePath)
            {
                CandidatePath = UNavigationSystemV1::FindPathToLocationSynchronously(const_cast<UWorld*>(World), Start, End);
                if (!CandidatePath || !CandidatePath->IsValid()) { return false; }
            }
            for (int32 Index = 1; Index < CandidatePath->PathPoints.Num(); ++Index)
            {
                const FVector& A = CandidatePath->PathPoints[Index - 1];
                const FVector& B = CandidatePath->PathPoints[Index];
                if (Bounds.IsInsideOrOn(A) || Bounds.IsInsideOrOn(B) || FMath::LineBoxIntersection(Bounds,A,B,B-A)) { return false; }
            }
		}
	}

	return true;
}
