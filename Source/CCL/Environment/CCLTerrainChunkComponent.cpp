#include "CCLTerrainChunkComponent.h"

#include "AI/NavigationSystemHelpers.h"
#include "Chaos/Capsule.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/MeshNormals.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/StrongObjectPtr.h"

UCCLTerrainChunkComponent::UCCLTerrainChunkComponent()
{
	SetMobility(EComponentMobility::Movable);
	SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::Yes;
	SetHiddenInGame(true);
	SetVisibility(false);
	SetDeferredCollisionUpdatesEnabled(true, false);
	bEnableComplexCollision = true;
	CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
	SetTangentsType(EDynamicMeshComponentTangentsMode::NoTangents);
	bUseAsyncCooking = true;
}

void UCCLTerrainChunkComponent::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
{
	check(IsInGameThread());
	if (State != ECCLTerrainChunkState::Cooking || !AsyncBodySetupQueue.Contains(FinishedBodySetup))
	{
		return;
	}

	if (bDiscardResults)
	{
		State = ECCLTerrainChunkState::Cancelled;
	}
	else if (!bRejectCook && bSuccess && FinishedBodySetup && FinishedBodySetup->bCreatedPhysicsMeshes
		&& !FinishedBodySetup->bFailedToCreatePhysicsMeshes && !FinishedBodySetup->TriMeshGeometries.IsEmpty())
	{
		MeshBodySetup = FinishedBodySetup;
		bCollisionUpdatePending = false;
		State = ECCLTerrainChunkState::Ready;
	}
	else
	{
		Failure = TEXT("Terrain candidate collision cooking failed or returned no triangle geometry.");
		State = ECCLTerrainChunkState::Failed;
	}

	AsyncBodySetupQueue.Reset();
	if (bDiscardResults)
	{
		DestroyComponent();
	}
}

bool UCCLTerrainChunkComponent::Prepare(FCCLTerrainMesh&& Mesh, FString& Error)
{
	check(IsInGameThread());
	Error.Reset();
	if (State != ECCLTerrainChunkState::Empty || IsRegistered() || !Mesh.Epoch.IsValid() || Mesh.WorldRevision == 0
		|| Mesh.Triangles.Num() != Mesh.TriangleMaterials.Num() || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		Error = TEXT("Terrain collision preparation requires a new unregistered game-world component and valid mesh.");
		return false;
	}

	UE::Geometry::FDynamicMesh3 DynamicMesh;
	for (const FVector& V : Mesh.VerticesMeters)
	{
		if (V.ContainsNaN())
		{
			Error = TEXT("Terrain vertex is not finite.");
			return false;
		}

		DynamicMesh.AppendVertex(V * 100.);
	}

	for (const FIntVector& T : Mesh.Triangles)
	{
		if (!Mesh.VerticesMeters.IsValidIndex(T.X) || !Mesh.VerticesMeters.IsValidIndex(T.Y) || !Mesh.VerticesMeters.IsValidIndex(T.Z)
			|| DynamicMesh.AppendTriangle(T.X, T.Y, T.Z) < 0)
		{
			Error = TEXT("Terrain surface cannot be represented by a valid DynamicMesh triangle topology.");
			return false;
		}
	}

	DynamicMesh.EnableAttributes();
	UE::Geometry::FMeshNormals::InitializeOverlayToPerVertexNormals(DynamicMesh.Attributes()->PrimaryNormals(), false);
	SetMesh(MoveTemp(DynamicMesh));
	SourceMesh = MoveTemp(Mesh);
	SetRelativeLocation(SourceMesh.OriginMeters * 100.);
	SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
	if (SourceMesh.Triangles.IsEmpty())
	{
		State = bRejectCook ? ECCLTerrainChunkState::Failed : ECCLTerrainChunkState::Ready;
		Failure = bRejectCook ? TEXT("Terrain empty collision rejected by test injection.") : FString();
		return !bRejectCook;
	}

	if (!ContainsPhysicsTriMeshData(false))
	{
		Error = TEXT("Terrain collision exceeds the engine's supported complex collision budget.");
		State = ECCLTerrainChunkState::Failed;
		return false;
	}

	UBodySetup* PreparedBody = CreateBodySetupHelper();
	PreparedBody->bDoubleSidedGeometry = true;
	AsyncBodySetupQueue.Add(PreparedBody);
	State = ECCLTerrainChunkState::Cooking;
	// Cooking owns a strong component reference, including on cancellation or world teardown.
	// The completion only updates this component; it never dereferences its owner or world.
	TStrongObjectPtr<UCCLTerrainChunkComponent> KeepAlive(this);
	PreparedBody->CreatePhysicsMeshesAsync(FOnAsyncPhysicsCookFinished::CreateLambda(
		[KeepAlive = MoveTemp(KeepAlive), PreparedBody](bool bSuccess)
		{
			KeepAlive->FinishPhysicsAsyncCook(bSuccess, PreparedBody);
		}));
	return true;
}

bool UCCLTerrainChunkComponent::ActivatePrepared(FString& Error)
{
	check(IsInGameThread());
	if (State != ECCLTerrainChunkState::Ready || bDiscardResults || !GetWorld())
	{
		Error = TEXT("Terrain collision is not ready for activation.");
		return false;
	}

	SetCollisionEnabled(SourceMesh.Triangles.IsEmpty() ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	RegisterComponent();
	if (!IsRegistered() || (!SourceMesh.Triangles.IsEmpty() && !BodyInstance.IsValidBodyInstance()))
	{
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Error = TEXT("Terrain prepared body could not create a live physics state.");
		return false;
	}

	SetHiddenInGame(false);
	SetVisibility(true);
	State = ECCLTerrainChunkState::Active;
	return true;
}

void UCCLTerrainChunkComponent::Discard()
{
	check(IsInGameThread());
	if (bDiscardResults && State == ECCLTerrainChunkState::Cancelled)
	{
		return;
	}

	bDiscardResults = 1;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetHiddenInGame(true);
	SetVisibility(false);
	if (IsRegistered())
	{
		UnregisterComponent();
	}

	if (State != ECCLTerrainChunkState::Cooking)
	{
		State = ECCLTerrainChunkState::Cancelled;
		DestroyComponent();
	}
}

bool UCCLTerrainChunkComponent::OverlapsCapsule(const FVector& WorldCenter, const FVector& WorldAxis,
	double RadiusCm, double HalfHeightCm) const
{
	check(IsInGameThread());
	if (!MeshBodySetup || SourceMesh.Triangles.IsEmpty())
	{
		return false;
	}

	const double SegmentHalfLength = FMath::Max(0., HalfHeightCm - RadiusCm);
	const FTransform Transform = GetComponentTransform();
	const FVector A = Transform.InverseTransformPosition(WorldCenter - WorldAxis * SegmentHalfLength);
	const FVector B = Transform.InverseTransformPosition(WorldCenter + WorldAxis * SegmentHalfLength);
	const Chaos::FCapsule Capsule(A, B, RadiusCm);
	for (const auto& TriMesh : MeshBodySetup->TriMeshGeometries)
	{
		if (TriMesh && TriMesh->OverlapGeom(Capsule, Chaos::FRigidTransform3::Identity, 0.))
		{
			return true;
		}
	}

	return false;
}

bool UCCLTerrainChunkComponent::DoCustomNavigableGeometryExport(FNavigableGeometryExport& Export) const
{
	if (State != ECCLTerrainChunkState::Active)
	{
		return false;
	}

	TArray<FVector> Vertices;
	Vertices.Reserve(SourceMesh.VerticesMeters.Num());
	for (const FVector& Vertex : SourceMesh.VerticesMeters)
	{
		Vertices.Add(Vertex * 100.);
	}

	TArray<int32> Indices;
	Indices.Reserve(SourceMesh.Triangles.Num() * 3);
	for (const auto& Triangle : SourceMesh.Triangles)
	{
		Indices.Add(Triangle.X);
		Indices.Add(Triangle.Y);
		Indices.Add(Triangle.Z);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainSmoke")))
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_NAV_EXPORT %s verts=%d triangles=%d transform=%s"), *GetName(), Vertices.Num(), SourceMesh.Triangles.Num(), *GetComponentLocation().ToString());
	}

	Export.ExportCustomMesh(Vertices.GetData(), Vertices.Num(), Indices.GetData(), Indices.Num(), GetComponentTransform());
	return false;
}
