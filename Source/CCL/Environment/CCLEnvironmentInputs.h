#pragma once

#include "CoreMinimal.h"
#include "CCLCelestialSystem.h"
#include "CCLSurfaceQuery.h"

// Small authoritative input state. Large terrain and fluid fields remain in separate generation files.
struct CCL_API FCCLEnvironmentInputs
{
	uint32 Schema = 1;
	uint64 Revision = 1;
	FCCLCelestialDefinitionData Celestial;
	FCCLCelestialObserver Observer;
	uint64 SurfaceRevision = 1;
	TArray<FCCLSurfacePatch> Surfaces;
	TArray<FCCLSurfaceOpening> Openings;
};

class CCL_API FCCLEnvironmentInputsCodec
{
public:
	static bool Encode(const FCCLEnvironmentInputs& Inputs, TArray<uint8>& OutBytes, FString& Error);

	static bool Decode(const TArray<uint8>& Bytes, FCCLEnvironmentInputs& OutInputs, FString& Error);

	// Rebuild both candidates before the caller publishes either one.
	static bool Prepare(const FCCLEnvironmentInputs& Inputs, double WorldSeconds,
		FCCLCelestialSystem& OutCelestial, FCCLSurfaceScene& OutSurfaces,
		FCCLCelestialObservation& OutObservation, FString& Error);

	static bool Validate(const FCCLEnvironmentInputs& Inputs, FString& Error);

	static bool CheckDefinition(const FCCLEnvironmentInputs& Inputs, const FGuid& ExpectedDefinitionId,
		int32 ExpectedVersion, FString& Error);

	static FCCLEnvironmentInputs MakeDefault(int32 Seed);
};
