#include "CCLSnowMovementComponent.h"

#include "CCLSurfaceReplication.h"
#include "CCLWorldSimulationSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

namespace
{
	FVector Feet(const ACharacter* Character, const FVector& Position)
	{
		return (Position - FVector(0., 0., Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight())) / 100.;
	}

	class FSnowSavedMove : public FSavedMove_Character
	{
	public:
		uint64 Serial = 0;
		uint64 Sequence = 0;
		float Depth = 0.f;
		virtual void Clear() override
		{
			FSavedMove_Character::Clear();
			Serial = Sequence = 0;
			Depth = 0.f;
		}
		virtual void SetMoveFor(ACharacter* C, float Dt, const FVector& Accel, FNetworkPredictionData_Client_Character& Data) override
		{
			auto* M = CastChecked<UCCLSnowMovementComponent>(C->GetCharacterMovement());
			M->PrepareSnowMove(Accel);
			Serial = M->GetSurfaceSerial();
			Sequence = M->GetContactSequence();
			Depth = M->GetSnowDepth();
			FSavedMove_Character::SetMoveFor(C, Dt, Accel, Data);
		}
		virtual void PrepMoveFor(ACharacter* C) override
		{
			FSavedMove_Character::PrepMoveFor(C);
			CastChecked<UCCLSnowMovementComponent>(C->GetCharacterMovement())->RestoreSnowMove(Serial, Sequence, Depth);
		}
		virtual bool CanCombineWith(const FSavedMovePtr& Other, ACharacter* C, float MaxDelta) const override
		{
			const auto* Snow = static_cast<const FSnowSavedMove*>(Other.Get());
			return Depth == 0.f && Snow->Depth == 0.f && Serial == Snow->Serial && FSavedMove_Character::CanCombineWith(Other, C, MaxDelta);
		}
	};

	class FSnowPredictionData : public FNetworkPredictionData_Client_Character
	{
	public:
		explicit FSnowPredictionData(const UCharacterMovementComponent& M) : FNetworkPredictionData_Client_Character(M) {}
		virtual FSavedMovePtr AllocateNewMove() override { return FSavedMovePtr(new FSnowSavedMove()); }
	};
}

void FCCLSnowNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& Move, ENetworkMoveType MoveType)
{
	FCharacterNetworkMoveData::ClientFillNetworkMoveData(Move, MoveType);
	const auto& Snow = static_cast<const FSnowSavedMove&>(Move);
	SurfaceSerial = Snow.Serial;
	ContactSequence = Snow.Sequence;
}

bool FCCLSnowNetworkMoveData::Serialize(UCharacterMovementComponent& M, FArchive& Ar, UPackageMap* Map, ENetworkMoveType Type)
{
	const bool bBase = FCharacterNetworkMoveData::Serialize(M, Ar, Map, Type);
	Ar << SurfaceSerial << ContactSequence;
	return bBase && !Ar.IsError();
}

FCCLSnowMoveDataContainer::FCCLSnowMoveDataContainer()
{
	NewMoveData = &Moves[0];
	PendingMoveData = &Moves[1];
	OldMoveData = &Moves[2];
}

UCCLSnowMovementComponent::UCCLSnowMovementComponent()
{
	SetNetworkMoveDataContainer(MoveData);
}

void UCCLSnowMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		ContactSource = FGuid::NewGuid();
	}
	if (GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		Powder = NewObject<UInstancedStaticMeshComponent>(GetOwner(), TEXT("SnowPowder"));
		Powder->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		Powder->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_SurfaceSnow.M_SurfaceSnow")));
		Powder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Powder->SetCanEverAffectNavigation(false);
		Powder->SetCastShadow(false);
		Powder->RegisterComponent();
	}
}

void UCCLSnowMovementComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* Function)
{
	Super::TickComponent(Dt, TickType, Function);
	if (!Powder)
	{
		return;
	}
	Powder->ClearInstances();
	const double Age = FPlatformTime::Seconds() - PowderTime;
	if (Age >= 0.45)
	{
		return;
	}
	for (int32 I = 0; I < 12; ++I)
	{
		const double Angle = I * 2.4;
		const FVector Offset(FMath::Cos(Angle) * Age * 100., FMath::Sin(Angle) * Age * 100., 75. * Age - 140. * Age * Age);
		Powder->AddInstance(FTransform(FQuat::Identity, PowderOrigin + Offset, FVector(0.02 * (1. - Age / 0.45))), true);
	}
}

void UCCLSnowMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UCCLSnowMovementComponent, ContactSource, COND_OwnerOnly);
}

void UCCLSnowMovementComponent::PrepareSnowMove(const FVector& Direction)
{
	FCCLSnowSample Sample;
	if (const auto* Model = UCCLSurfaceReplication::View(GetWorld()))
	{
		const FVector Heading = (!Direction.IsNearlyZero() ? Direction : (!Acceleration.IsNearlyZero() ? Acceleration : Velocity)).GetSafeNormal2D();
		const FVector Front = UpdatedComponent->GetComponentLocation() + Heading * (CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() + 8.);
		Model->SampleSnow(Feet(CharacterOwner, Front), Sample);
	}
	SurfaceSerial = 0;
	if (const auto* Controller = CharacterOwner->GetController())
	{
		if (const auto* Rep = Controller->FindComponentByClass<UCCLSurfaceReplication>())
		{
			SurfaceSerial = Rep->GetSerial();
		}
	}
	SnowDepthMeters = float(Sample.DepthMeters);
	ActiveSequence = ++NextSequence;
	bPreparedMove = 1;
}

void UCCLSnowMovementComponent::RestoreSnowMove(uint64 Serial, uint64 Sequence, float Depth)
{
	if (CharacterOwner && CharacterOwner->bClientUpdating && Depth > 0.f)
	{
		++SnowReplays;
	}
	SurfaceSerial = Serial;
	ActiveSequence = Sequence;
	SnowDepthMeters = Depth;
	bPreparedMove = 1;
}

float UCCLSnowMovementComponent::GetMaxSpeed() const
{
	const float Base = Super::GetMaxSpeed();
	return IsMovingOnGround() ? Base * FMath::Clamp(1.f / (1.f + 3.f * SnowDepthMeters), 0.25f, 1.f) : Base;
}

FNetworkPredictionData_Client* UCCLSnowMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		const_cast<UCCLSnowMovementComponent*>(this)->ClientPredictionData = new FSnowPredictionData(*this);
	}
	return ClientPredictionData;
}

void UCCLSnowMovementComponent::PerformMovement(float DeltaSeconds)
{
	if (!bPreparedMove && CharacterOwner && UpdatedComponent)
	{
		PrepareSnowMove();
	}
	Super::PerformMovement(DeltaSeconds);
	bPreparedMove = 0;
}

void UCCLSnowMovementComponent::MoveAutonomous(float TimeStamp, float DeltaTime, uint8 Flags, const FVector& Accel)
{
	if (CharacterOwner && CharacterOwner->HasAuthority())
	{
		const auto* Data = static_cast<const FCCLSnowNetworkMoveData*>(GetCurrentNetworkMoveData());
		const auto* Controller = CharacterOwner->GetController();
		const auto* Rep = Controller ? Controller->FindComponentByClass<UCCLSurfaceReplication>() : nullptr;
		const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
		const FVector Heading = (!Accel.IsNearlyZero() ? Accel : Velocity).GetSafeNormal2D();
		const FVector Front = UpdatedComponent->GetComponentLocation() + Heading * (CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() + 8.);
		const FVector At = Feet(CharacterOwner, Front);
		FCCLSnowSample Live, Historical;
		if (Runtime)
		{
			Runtime->GetSurfaceSimulation().SampleSnow(At, Live);
		}
		const bool bHistory = Data && Rep && Rep->SampleHistoricalSnow(Data->SurfaceSerial, At, Historical);
		// Large material changes are always resolved from current authority and corrected.
		const bool bValid = bHistory && Live.DepthMeters <= Historical.DepthMeters + 0.2;
		SnowDepthMeters = float(bValid ? Historical.DepthMeters : Live.DepthMeters);
		SurfaceSerial = Data ? Data->SurfaceSerial : 0;
		ActiveSequence = Data && Data->ContactSequence > 0 && Data->ContactSequence < MAX_uint64 / 16
			? Data->ContactSequence : ++NextSequence;
		bPreparedMove = 1;
		if (!bValid && (Live.DepthMeters > 0. || Historical.DepthMeters > 0.))
		{
			// ForceClientAdjustment only clears the throttle; request a correction as well.
			GetPredictionData_Server_Character()->bForceClientUpdate = true;
			ForceClientAdjustment();
			++SnowCorrections;
		}
	}
	Super::MoveAutonomous(TimeStamp, DeltaTime, Flags, Accel);
}

void UCCLSnowMovementComponent::OnMovementUpdated(float Dt, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(Dt, OldLocation, OldVelocity);
	if (!CharacterOwner || !IsMovingOnGround() || !UpdatedComponent || ActiveSequence == 0)
	{
		return;
	}
	const FVector At = UpdatedComponent->GetComponentLocation();
	if (!bHasContactLocation)
	{
		LastContactLocation = OldLocation;
		bHasContactLocation = 1;
	}
	const double Distance = FVector::Dist2D(LastContactLocation, At);
	if (Distance < 12.)
	{
		return;
	}
	const FVector From = Distance < 200. ? LastContactLocation : At;
	LastContactLocation = At;
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(FVector::Dist2D(From, At) / 12.), 1, 8);
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Controller = CharacterOwner->GetController();
	auto* Rep = Controller ? Controller->FindComponentByClass<UCCLSurfaceReplication>() : nullptr;
	const auto* Model = UCCLSurfaceReplication::View(GetWorld());
	if (!Model)
	{
		return;
	}
	for (int32 I = 1; I <= Steps; ++I)
	{
		const FVector Position = Feet(CharacterOwner, FMath::Lerp(From, At, double(I) / Steps));
		FCCLSnowSample Sample;
		if (!Model->SampleSnow(Position, Sample))
		{
			continue;
		}
		if (!CharacterOwner->bClientUpdating)
		{
			PowderOrigin = FVector(Position.X, Position.Y, Sample.BedMeters + Sample.DepthMeters) * 100.;
			PowderTime = FPlatformTime::Seconds();
		}
		const uint64 Sequence = ActiveSequence * 16 + I;
		if (CharacterOwner->HasAuthority() && Runtime)
		{
			FString Error;
			if (!Runtime->ApplySnowContact(Sample.RegionId, ContactSource, Sequence, Position, 0.27, Error))
			{
				UE_LOG(LogTemp, Warning, TEXT("CCL_SNOW_CONTACT %s"), *Error);
			}
		}
		else if (CharacterOwner->IsLocallyControlled() && Rep && !CharacterOwner->bClientUpdating)
		{
			Rep->PredictSnowContact(ContactSource, Sequence, Position);
		}
	}
}
