#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassEntityTypes.h"
#include "Mass/EntityHandle.h"
#include "KSIEcologySimulation.h"
#include "CCLEcologyRepresentationSubsystem.generated.h"

class ACCLCreatureActor;
struct FMassEntityManager;

USTRUCT()
struct FCCLEcologyMassFragment : public FMassFragment
{
	GENERATED_BODY()

	FKSIEcologyLease Lease;
	FVector PositionMeters = FVector::ZeroVector;
	uint8 Medium = 0;
};

struct FCCLEcologyRepresentationEntry
{
	FKSIEcologyLease Lease;
	TWeakObjectPtr<ACCLCreatureActor> Actor;
	FMassEntityHandle Entity;
};

UCLASS()
class CCL_API UCCLEcologyRepresentationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

public:
	bool SetRepresentation(FGuid IndividualId, EKSIEcologyRepresentation Representation, FString& Error);
	bool SetPosition(FGuid IndividualId, const FVector& PositionMeters, FString& Error);
	bool FinishIndividual(FGuid IndividualId, const FKSIEcologyEvent& Event, FString& Error);
	bool Flush(FString& Error);
	bool CanModify(FString& Error) const;
	void DiscardRepresentations();
	void OnActorEnded(const FKSIEcologyLease& Lease);
	const FCCLEcologyRepresentationEntry* Find(FGuid Id) const { return Entries.Find(Id); }
	int32 Count() const { return Entries.Num(); }

private:
	FMassEntityManager* Manager() const;
	void Retire(const FCCLEcologyRepresentationEntry& Entry);

private:
	TMap<FGuid, FCCLEcologyRepresentationEntry> Entries;
	FString LastError;
};
