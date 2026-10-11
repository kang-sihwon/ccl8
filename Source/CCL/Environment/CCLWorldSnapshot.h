#pragma once

#include "CoreMinimal.h"
#include "KSIWorldClock.h"
#include "KSIEnvironmentInputs.h"

class FCCLLifeSimulation;
class FKSISurfaceSimulation;
class FKSIEcologySimulation;

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

// Opaque state of one game system. The system that registered the section owns its format and validation.
struct FCCLWorldSection
{
	FName Name;
	TArray<uint8> Bytes;
};

struct CCL_API FCCLWorldSnapshot
{
	const TArray<uint8>* FindSection(FName Name) const;
	void SetSection(FName Name, TArray<uint8> Bytes);
	void RemoveSection(FName Name);

	uint32 Schema = 7;
	uint8 bEnvironmentMigrated = 0;
	uint8 bSurfaceMigrated = 0;
	uint8 bEcologyMigrated = 0;
	FCCLWorldIdentity Identity;
	FKSIWorldClockSnapshot Clock;
	TArray<uint8> Life;
	TArray<uint8> Surface;
	TArray<uint8> Ecology;

	// Sorted by name. A section missing from an older schema reads as empty.
	TArray<FCCLWorldSection> Sections;
	FKSIEnvironmentInputs Environment = FKSIEnvironmentInputsCodec::MakeDefault(42);
};

// The bounded clock/life/ecology envelope stays separate from future terrain chunk files.
class CCL_API FCCLWorldSnapshotCodec
{
public:
	static bool Capture(const FCCLWorldIdentity& Identity, const FKSIWorldClock& Clock,
		const FCCLLifeSimulation& Life, FCCLWorldSnapshot& Snapshot, FString& Error,
		const FKSIEnvironmentInputs* Environment = nullptr, const FKSISurfaceSimulation* Surface = nullptr,
		const FKSIEcologySimulation* Ecology = nullptr);
	static bool Encode(const FCCLWorldSnapshot& Snapshot, TArray<uint8>& Bytes, FString& Error);
	static bool Decode(const TArray<uint8>& Bytes, FCCLWorldSnapshot& Snapshot, FString& Error);
	static bool MigrateLegacy(const TArray<uint8>& LifeBytes, ECCLWorldDomain Domain,
		FCCLWorldSnapshot& Snapshot, FString& Error);
	static bool Restore(const FCCLWorldSnapshot& Snapshot, ECCLWorldDomain ExpectedDomain,
		FKSIWorldClock& Clock, FCCLLifeSimulation& Life, FString& Error, FKSISurfaceSimulation* Surface = nullptr,
		FKSIEcologySimulation* Ecology = nullptr);
	static bool Validate(const FCCLWorldSnapshot& Snapshot, FString& Error);
};
