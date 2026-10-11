#pragma once

#include "CoreMinimal.h"

class FCCLLifeSimulation;
class FCCLSurfaceSimulation;
struct FCCLWorldStep;

// Facts a world section validator may check against. Core state is already decoded and consistent.
struct CCL_API FCCLWorldSectionContext
{
	FGuid WorldId;
	double WorldSeconds = 0.;
	const FCCLSurfaceSimulation* Surface = nullptr;
	const FCCLLifeSimulation* Life = nullptr;

	// Item identities already claimed by earlier sections. A section adds its own and rejects duplicates.
	TSet<FGuid>* ClaimedItems = nullptr;
};

using FCCLWorldSectionValidator = TFunction<bool(const FCCLWorldSectionContext&, const TArray<uint8>&, FString&)>;

// Validators for the named sections that game systems add to a world snapshot.
// The world codec owns the envelope; each system owns and validates the format inside its section.
class CCL_API FCCLWorldSectionRegistry
{
public:
	struct FEntry
	{
		FName Name;
		int32 Order = 0;
		FCCLWorldSectionValidator Validator;
	};

	static void Register(FName Name, int32 Order, FCCLWorldSectionValidator Validator);
	static const FEntry* Find(FName Name);

	// Registered sections in validation order.
	static TArray<const FEntry*> GetOrdered();

private:
	static TArray<FEntry>& Entries();
};

// Registers a section validator during static initialization of the owning system's translation unit.
struct CCL_API FCCLWorldSectionRegistrar
{
	FCCLWorldSectionRegistrar(FName Name, int32 Order, FCCLWorldSectionValidator Validator)
	{
		FCCLWorldSectionRegistry::Register(Name, Order, MoveTemp(Validator));
	}
};

struct CCL_API FCCLWorldCaptureContext
{
	FGuid WorldId;
	double WorldSeconds = 0.;

	// Owner saved by a personal session record. World participants leave it out of the world capture.
	const UObject* SessionContainer = nullptr;

	// Containers that an earlier participant already captured or will restore.
	TArray<const UObject*> ClaimedContainers;
};

struct CCL_API FCCLWorldRestoreContext
{
	FGuid WorldId;
	double WorldSeconds = 0.;

	// True when a new or loaded world starts, false when a running world is replaced.
	uint8 bStarting = 0;

	// Containers that an earlier participant will restore. Later participants must not treat them as unbound.
	TArray<const UObject*> ClaimedContainers;

	// Items an earlier participant keeps unchanged through the restore. Later participants must not restore them again.
	TSet<FGuid> KeptItems;
};

// A game system that stores runtime state in one section of the world snapshot.
// Participants run in ascending order. A lower participant wraps the commits of higher ones and the core world,
// which lets it keep an atomic scope, such as a gameplay effect batch, around everything it depends on.
class CCL_API ICCLWorldParticipant
{
public:
	virtual ~ICCLWorldParticipant() = default;

	virtual FName GetWorldSection() const = 0;
	virtual int32 GetWorldOrder() const = 0;

	// Adds this participant's section. It may claim containers so later participants exclude them.
	virtual bool CaptureWorldSection(FCCLWorldCaptureContext& Context, TArray<uint8>& OutBytes, FString& Error) = 0;

	// Validates and stages a restore without changing visible state. Section is null when the snapshot has none,
	// which restores an empty section. Every successful prepare is followed by Commit or Cancel.
	virtual bool PrepareWorldRestore(FCCLWorldRestoreContext& Context, const TArray<uint8>* Section, FString& Error) = 0;

	// Calls CommitInner exactly once and then commits its own staged state. If CommitInner fails, the participant
	// keeps its previous state and returns false. Notifications are allowed only after both commits succeeded.
	virtual bool CommitWorldRestore(TFunctionRef<bool()> CommitInner) = 0;

	// Sends notifications after every participant and the core world committed.
	virtual void PublishWorldRestore() = 0;
	virtual void CancelWorldRestore() = 0;

	// Time advance of section state that depends on the environment, such as stored food.
	virtual bool NeedsWorldAdvance() const = 0;
	virtual bool PrepareWorldAdvance(const FCCLWorldStep& Step, const FCCLSurfaceSimulation& Surface, FString& Error) = 0;
	virtual void CommitWorldAdvance() = 0;
	virtual void PublishWorldAdvance() = 0;
	virtual void CancelWorldAdvance() = 0;
};
