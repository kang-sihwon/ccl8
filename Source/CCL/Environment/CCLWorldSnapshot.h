#pragma once

#include "CoreMinimal.h"
#include "CCLWorldClock.h"
#include "CCLEnvironmentInputs.h"
#include "Items/CCLInventorySnapshot.h"
#include "Session/CCLPlayerSnapshot.h"

class FCCLLifeSimulation;
class FCCLSurfaceSimulation;
class FCCLEcologySimulation;

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
	uint32 Schema = 6;
	uint8 bEnvironmentMigrated = 0;
	uint8 bSurfaceMigrated = 0;
	uint8 bEcologyMigrated = 0;
	uint8 bInventoriesMigrated = 0;
    uint8 bPlayersMigrated = 0;
	FCCLWorldIdentity Identity;
	FCCLWorldClockSnapshot Clock;
	TArray<uint8> Life;
	TArray<uint8> Surface;
	TArray<uint8> Ecology;
	TArray<FCCLInventorySnapshot> Inventories;
    TArray<FCCLPlayerSnapshot> Players;
	FCCLEnvironmentInputs Environment = FCCLEnvironmentInputsCodec::MakeDefault(42);
};

// The bounded clock/life/ecology envelope stays separate from future terrain chunk files.
class CCL_API FCCLWorldSnapshotCodec
{
public:
	static bool Capture(const FCCLWorldIdentity& Identity, const FCCLWorldClock& Clock,
		const FCCLLifeSimulation& Life, FCCLWorldSnapshot& Snapshot, FString& Error,
		const FCCLEnvironmentInputs* Environment = nullptr, const FCCLSurfaceSimulation* Surface = nullptr,
		const FCCLEcologySimulation* Ecology = nullptr);
	static bool Encode(const FCCLWorldSnapshot& Snapshot, TArray<uint8>& Bytes, FString& Error);
	static bool Decode(const TArray<uint8>& Bytes, FCCLWorldSnapshot& Snapshot, FString& Error);
	static bool MigrateLegacy(const TArray<uint8>& LifeBytes, ECCLWorldDomain Domain,
		FCCLWorldSnapshot& Snapshot, FString& Error);
	static bool Restore(const FCCLWorldSnapshot& Snapshot, ECCLWorldDomain ExpectedDomain,
		FCCLWorldClock& Clock, FCCLLifeSimulation& Life, FString& Error, FCCLSurfaceSimulation* Surface = nullptr,
		FCCLEcologySimulation* Ecology = nullptr);
	static bool Validate(const FCCLWorldSnapshot& Snapshot, FString& Error);
};
