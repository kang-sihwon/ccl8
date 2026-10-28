#pragma once

#include "CoreMinimal.h"

// Optional immutable facts belong to the caller, and outlive section validation.
class KSICORE_API IKSIWorldSectionFacts
{
public:
    virtual ~IKSIWorldSectionFacts() = default;
    virtual FName GetTypeName() const = 0;
};

// Facts a world section validator may check against. Core state is already decoded and consistent.
struct KSICORE_API FKSIWorldSectionContext
{
    FGuid WorldId;
    double WorldSeconds = 0.;
    const IKSIWorldSectionFacts* Facts = nullptr;

    template <typename T> const T* FindFacts() const
    {
        return Facts && Facts->GetTypeName() == T::TypeName() ? static_cast<const T*>(Facts) : nullptr;
    }

    // Item identities already claimed by earlier sections. A section adds its own and rejects duplicates.
    TSet<FGuid>* ClaimedItems = nullptr;
};

using FKSIWorldSectionValidator = TFunction<bool(const FKSIWorldSectionContext&, const TArray<uint8>&, FString&)>;

// Validators for the named sections that game systems add to a world snapshot.
// The world codec owns the envelope; each system owns and validates the format inside its section.
class KSICORE_API FKSIWorldSectionRegistry
{
public:
    struct FEntry
    {
        FName Name;
        int32 Order = 0;
        FKSIWorldSectionValidator Validator;
    };

    static void Register(FName Name, int32 Order, FKSIWorldSectionValidator Validator);
    static void Unregister(FName Name);
    static const FEntry* Find(FName Name);

    // Registered sections in validation order.
    static TArray<const FEntry*> GetOrdered();

private:
    static TArray<FEntry>& Entries();
};

// Registers a section validator during static initialization of the owning system's translation unit.
struct KSICORE_API FKSIWorldSectionRegistrar
{
    FKSIWorldSectionRegistrar(FName Name, int32 Order, FKSIWorldSectionValidator Validator)
        : SectionName(Name)
    {
        FKSIWorldSectionRegistry::Register(Name, Order, MoveTemp(Validator));
    }
    ~FKSIWorldSectionRegistrar();
    FKSIWorldSectionRegistrar(const FKSIWorldSectionRegistrar&) = delete;
    FKSIWorldSectionRegistrar& operator=(const FKSIWorldSectionRegistrar&) = delete;
private:
    FName SectionName;
};

struct KSICORE_API FKSIWorldCaptureContext
{
    FGuid WorldId;
    double WorldSeconds = 0.;

    // Owner saved by a personal session record. World participants leave it out of the world capture.
    const UObject* SessionContainer = nullptr;

    // Containers that an earlier participant already captured or will restore.
    TArray<const UObject*> ClaimedContainers;
};

struct KSICORE_API FKSIWorldRestoreContext
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
class KSICORE_API IKSIWorldParticipant
{
public:
    virtual ~IKSIWorldParticipant() = default;

    virtual FName GetWorldSection() const = 0;
    virtual int32 GetWorldOrder() const = 0;

    // Adds this participant's section. It may claim containers so later participants exclude them.
    virtual bool CaptureWorldSection(FKSIWorldCaptureContext& Context, TArray<uint8>& OutBytes, FString& Error) = 0;

    // Validates and stages a restore without changing visible state. Section is null when the snapshot has none,
    // which restores an empty section. Every successful prepare is followed by Commit or Cancel.
    virtual bool PrepareWorldRestore(FKSIWorldRestoreContext& Context, const TArray<uint8>* Section, FString& Error) = 0;

    // Calls CommitInner exactly once and then commits its own staged state. If CommitInner fails, the participant
    // keeps its previous state and returns false. Notifications are allowed only after both commits succeeded.
    virtual bool CommitWorldRestore(TFunctionRef<bool()> CommitInner) = 0;

    // Sends notifications after every participant and the core world committed.
    virtual void PublishWorldRestore() = 0;
    virtual void CancelWorldRestore() = 0;
};
