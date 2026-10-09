#include "CCLWorldEnvironmentState.h"

#include "Net/UnrealNetwork.h"
#include "UObject/Class.h"

namespace
{
	template<typename T>
	bool SerializeViewArray(FArchive& Ar, TArray<T>& Values, uint8 Maximum)
	{
		if (Ar.IsSaving() && Values.Num() > Maximum)
		{
			return false;
		}

		uint8 Count = uint8(Values.Num());
		Ar << Count;
		if (Ar.IsError() || Count > Maximum)
		{
			return false;
		}

		Values.SetNum(Count);
		for (T& Value : Values)
		{
			T::StaticStruct()->SerializeBin(Ar, &Value);
			if (Ar.IsError())
			{
				return false;
			}
		}

		return true;
	}
}

bool FCCLReplicatedWorldTime::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// One native serializer publishes time and the complete view atomically, including on packet loss.
	FCCLReplicatedWorldTime Candidate = *this;
	uint8 WireVersion = 2;
	Ar << WireVersion << Candidate.WorldId << Candidate.Epoch << Candidate.GameSeconds << Candidate.WorldSeconds;
	Ar << Candidate.TimeScale << Candidate.PendingGameSeconds << Candidate.CompletedStepId << Candidate.bAdvanceFailed;
	auto& View = Candidate.Environment;
	Ar << View.bValid << View.DefinitionId << View.DefinitionVersion << View.Seed;
	Ar << View.InputRevision << View.SurfaceRevision << View.SurfaceEpoch << View.ObliquityDegrees << View.DominantStarId << View.CelestialEpochSeconds;
	FCCLCelestialObserver::StaticStruct()->SerializeBin(Ar, &View.Observer);
	bOutSuccess = !Ar.IsError() && WireVersion == 2 && FMath::IsFinite(Candidate.GameSeconds)
		&& FMath::IsFinite(Candidate.WorldSeconds) && FMath::IsFinite(Candidate.TimeScale)
		&& FMath::IsFinite(Candidate.PendingGameSeconds) && FMath::IsFinite(View.CelestialEpochSeconds)
		&& SerializeViewArray(Ar, View.CelestialBodies, 64)
		&& SerializeViewArray(Ar, View.Stars, 64) && SerializeViewArray(Ar, View.SkyBodies, 64)
		&& SerializeViewArray(Ar, View.Probes, 8) && SerializeViewArray(Ar, View.Openings, 64)
		&& SerializeViewArray(Ar, View.Surfaces, 32);
	if (bOutSuccess && Ar.IsLoading())
	{
		*this = MoveTemp(Candidate);
	}

	return true;
}

ACCLWorldEnvironmentState::ACCLWorldEnvironmentState()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(4);
}

void ACCLWorldEnvironmentState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLWorldEnvironmentState, Time);
}

void ACCLWorldEnvironmentState::Publish(const FCCLReplicatedWorldTime& Value)
{
	if (HasAuthority())
	{
		const bool bInputChanged = Time.Epoch != Value.Epoch || Time.Environment.InputRevision != Value.Environment.InputRevision
			|| Time.TimeScale != Value.TimeScale || Time.bAdvanceFailed != Value.bAdvanceFailed;
		Time = Value;
		// Routine observations follow the 4 Hz net frequency. Forcing every simulation tick
		// would bypass this publication rate as the atomic interest set grows.
		if (bInputChanged)
		{
			ForceNetUpdate();
		}
	}
}
