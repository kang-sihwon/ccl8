#pragma once

#include "CoreMinimal.h"
#include "KSICelestialSystem.h"
#include "KSISurfaceQuery.h"

// Small authoritative input state. Large terrain and fluid fields remain in separate generation files.
struct KSIENVIRONMENT_API FKSIEnvironmentInputs
{
	uint32 Schema = 1;
	uint64 Revision = 1;
	FKSICelestialDefinitionData Celestial;
	FKSICelestialObserver Observer;
	uint64 SurfaceRevision = 1;
	TArray<FKSISurfacePatch> Surfaces;
	TArray<FKSISurfaceOpening> Openings;
};

class KSIENVIRONMENT_API FKSIEnvironmentInputsCodec
{
public:
	static bool Encode(const FKSIEnvironmentInputs& Inputs, TArray<uint8>& OutBytes, FString& Error);

	static bool Decode(const TArray<uint8>& Bytes, FKSIEnvironmentInputs& OutInputs, FString& Error);

	// Rebuild both candidates before the caller publishes either one.
	static bool Prepare(const FKSIEnvironmentInputs& Inputs, double WorldSeconds,
		FKSICelestialSystem& OutCelestial, FKSISurfaceScene& OutSurfaces,
		FKSICelestialObservation& OutObservation, FString& Error);

	static bool Validate(const FKSIEnvironmentInputs& Inputs, FString& Error);

	static bool CheckDefinition(const FKSIEnvironmentInputs& Inputs, const FGuid& ExpectedDefinitionId,
		int32 ExpectedVersion, FString& Error);

	static FKSIEnvironmentInputs MakeDefault(int32 Seed);
};
