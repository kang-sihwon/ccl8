#pragma once

#include "CoreMinimal.h"
#include "CCLWorldClock.h"
#include "CCLEnvironmentInputs.h"

class FCCLLifeSimulation;

enum class ECCLWorldDomain : uint8
{
	Campaign,
	Playground,
	Scenario
};

struct FCCLWorldIdentity
{
	FGuid WorldId;
	uint32 BaseWorldVersion = 1;
	uint64 Generation = 1;
	ECCLWorldDomain Domain = ECCLWorldDomain::Campaign;
};

struct FCCLWorldSnapshot
{
	uint32 Schema = 2;
	uint8 bEnvironmentMigrated = 0;
	FCCLWorldIdentity Identity;
	FCCLWorldClockSnapshot Clock;
	TArray<uint8> Life;
	FCCLEnvironmentInputs Environment = FCCLEnvironmentInputsCodec::MakeDefault(42);
};

// The bounded clock/life envelope stays separate from future terrain chunk files.
class CCL_API FCCLWorldSnapshotCodec
{
public:
	static bool Capture(const FCCLWorldIdentity& Identity, const FCCLWorldClock& Clock,
		const FCCLLifeSimulation& Life, FCCLWorldSnapshot& Snapshot, FString& Error,
		const FCCLEnvironmentInputs* Environment = nullptr);
	static bool Encode(const FCCLWorldSnapshot& Snapshot, TArray<uint8>& Bytes, FString& Error);
	static bool Decode(const TArray<uint8>& Bytes, FCCLWorldSnapshot& Snapshot, FString& Error);
	static bool MigrateLegacy(const TArray<uint8>& LifeBytes, ECCLWorldDomain Domain,
		FCCLWorldSnapshot& Snapshot, FString& Error);
	static bool Restore(const FCCLWorldSnapshot& Snapshot, ECCLWorldDomain ExpectedDomain,
		FCCLWorldClock& Clock, FCCLLifeSimulation& Life, FString& Error);
	static bool Validate(const FCCLWorldSnapshot& Snapshot, FString& Error);
};
