#pragma once

#include "CoreMinimal.h"

class ICCLEcologySpace;
class FCCLSurfaceSimulation;

enum class ECCLEcologyMovement : uint8 { Ground, Water, Air, Insect };
enum class ECCLEcologyRepresentation : uint8 { Dormant, Actor, Mass };
enum class ECCLEcologyEventKind : uint8 { Birth, Immigration, Death, Capture, Migration, Materialize };
enum class ECCLEcologyLifeState : uint8 { Alive, Dead, Captured };

struct FCCLEcologySpecies
{
	FGuid Id;
	ECCLEcologyMovement Movement = ECCLEcologyMovement::Ground;
	double MaturitySeconds = 86400.;
	double LifespanSeconds = 864000.;
	// Zero disables scheduled reproduction. Nonzero intervals are whole world seconds.
	double BirthIntervalSeconds = 0.;
};

struct FCCLEcologyHabitat
{
	FGuid Id;
	FVector MinimumMeters = FVector::ZeroVector;
	FVector MaximumMeters = FVector(100.);
	uint8 MovementMask = 15;
	int64 Capacity = 10000;
};

struct FCCLEcologyCohort
{
	FGuid Id;
	double BirthWorldSeconds = 0.;
	int64 Count = 0;
};

struct FCCLEcologyPopulation
{
	FGuid Id;
	FGuid SpeciesId;
	FGuid HabitatId;
	TMap<FGuid, FCCLEcologyCohort> Cohorts;
	int64 Initial = 0;
	int64 Born = 0;
	int64 Arrived = 0;
	int64 Died = 0;
	int64 Captured = 0;
	int64 Departed = 0;
};

struct FCCLEcologyIndividual
{
	FGuid Id;
	FGuid PopulationId;
	FGuid CohortId;
	double BirthWorldSeconds = 0.;
	ECCLEcologyLifeState State = ECCLEcologyLifeState::Alive;
	uint8 bPersistent = 0;
	FVector PositionMeters = FVector::ZeroVector;
};

struct FCCLEcologyEvent
{
	FGuid Id;
	ECCLEcologyEventKind Kind = ECCLEcologyEventKind::Immigration;
	FGuid Source;
	FGuid Target;
	FGuid CohortId;
	FGuid IndividualId;
	int64 Count = 1;
	double BirthWorldSeconds = 0.;
	uint8 bPersistent = 0;
	bool operator==(const FCCLEcologyEvent& Other) const;
};

struct FCCLEcologyLease
{
	FGuid IndividualId;
	FGuid Writer;
	FGuid Epoch;
	uint64 Sequence = 0;
	ECCLEcologyRepresentation Representation = ECCLEcologyRepresentation::Dormant;
};

enum class ECCLEcologyActivity : uint8 { Roaming, SeekingFood, Feeding, Starving, Inactive, Blocked };

struct FCCLEcologyGroup
{
	FGuid Id;
	FGuid HabitatId;
	FVector AnchorMeters = FVector::ZeroVector;
	double RadiusMeters = 2.;
};

struct FCCLEcologyForage
{
	FGuid Id;
	FGuid HabitatId;
	FVector PositionMeters = FVector::ZeroVector;
	uint32 FoodMask = 1;
	double InitialUnits = 10.;
	double AvailableUnits = 10.;
	double CapacityUnits = 10.;
	double RegrowthPerSecond = 0.;
	double GrownUnits = 0.;
	double ConsumedUnits = 0.;
	double LastStepSeconds = 0.;
};

struct FCCLEcologyBehavior
{
	FGuid Id;
	FGuid GroupId;
	FGuid TargetFoodId;
	double SpeedMetersPerSecond = 1.;
	uint32 DietMask = 1;
	double Energy = 1.;
	double MetabolismPerSecond = 0.01;
	double BiteUnitsPerSecond = 1.;
	double EnergyPerUnit = 0.2;
	double HungerThreshold = 0.7;
	double StarvationLimitSeconds = 60.;
	double StarvedSeconds = 0.;
	double ConsumedUnits = 0.;
	double LastStepSeconds = 0.;
	ECCLEcologyActivity Activity = ECCLEcologyActivity::Roaming;
	uint8 bUseWorldSpace = 0;
	double RadiusMeters = .2;
};

class CCL_API FCCLEcologySimulation
{
public:
	bool Initialize(FGuid WorldId, double WorldSeconds, FString& Error);
	bool AddSpecies(const FCCLEcologySpecies& Species, FString& Error);
	bool AddHabitat(const FCCLEcologyHabitat& Habitat, FString& Error);
	bool AddPopulation(FCCLEcologyPopulation Population, FString& Error);
	bool AddGroup(const FCCLEcologyGroup& Group, FString& Error);
	bool AddForage(FCCLEcologyForage Forage, FString& Error);
	bool AddBehavior(FCCLEcologyBehavior Behavior, FString& Error);
	const TMap<FGuid, FCCLEcologyGroup>& GetGroups() const { return Groups; }
	const TMap<FGuid, FCCLEcologyForage>& GetForage() const { return ForageSources; }
	const TMap<FGuid, FCCLEcologyBehavior>& GetBehaviors() const { return Behaviors; }
	bool HasBehavior(FGuid Id) const { return Behaviors.Contains(Id); }
	bool Apply(const FCCLEcologyEvent& Event, FString& Error);
	bool Materialize(FGuid RequestId, FGuid PopulationId, FGuid CohortId, FGuid IndividualId, bool bPersistent, FString& Error);
	bool Acquire(FGuid IndividualId, ECCLEcologyRepresentation Representation, FGuid Writer, FCCLEcologyLease& OutLease, FString& Error);
	bool Release(const FCCLEcologyLease& Lease, FString& Error);
	bool Suspend(const FCCLEcologyLease& Lease, FString& Error);
	bool ReturnToAggregate(FGuid IndividualId, FString& Error);
	bool Transfer(const FCCLEcologyLease& Lease, ECCLEcologyRepresentation Representation, FGuid Writer,
		FCCLEcologyLease& OutLease, FString& Error);
	bool UpdatePosition(const FCCLEcologyLease& Lease, const FVector& PositionMeters, FString& Error);
	bool Finish(const FCCLEcologyLease& Lease, const FCCLEcologyEvent& Event, FString& Error);
	bool IsLeaseCurrent(const FCCLEcologyLease& Lease) const;
	bool AdvanceTo(double TargetWorldSeconds, FString& Error, int32 MaxBoundaries = 256,
		const ICCLEcologySpace* Space = nullptr, const FCCLSurfaceSimulation* Surface = nullptr);
	bool HasSpatialBehavior() const;
	bool Capture(TArray<uint8>& Bytes, FString& Error) const;
	bool Restore(const TArray<uint8>& Bytes, FString& Error);
	bool Validate(FString& Error) const;
	int64 LivingCount(FGuid PopulationId) const;
	int64 AggregateCount(FGuid PopulationId) const;
	FGuid GetWorldId() const { return WorldId; }
	double GetWorldSeconds() const { return WorldSeconds; }
	const TMap<FGuid, FCCLEcologyPopulation>& GetPopulations() const { return Populations; }
	const TMap<FGuid, FCCLEcologyIndividual>& GetIndividuals() const { return Individuals; }
	const TMap<FGuid, FCCLEcologySpecies>& GetSpecies() const { return Species; }
	const TMap<FGuid, FCCLEcologyHabitat>& GetHabitats() const { return Habitats; }
	const FCCLEcologyIndividual* FindIndividual(FGuid Id) const { return Individuals.Find(Id); }
	int32 ReceiptCount() const { return Receipts.Num(); }
	static constexpr int32 MaximumBytes = 8 * 1024 * 1024;

private:
	bool ApplyCandidate(const FCCLEcologyEvent& Event, FString& Error);
	bool AdvanceCandidate(double TargetWorldSeconds, FString& Error, int32 MaxBoundaries,
		const ICCLEcologySpace* Space, const FCCLSurfaceSimulation* Surface);
	bool Serialize(FArchive& Archive);
	bool AdvanceBehavior(double TargetSeconds, FString& Error, const ICCLEcologySpace* Space, const FCCLSurfaceSimulation* Surface);
	bool ValidateBehavior(FString& Error) const;

private:
	FGuid WorldId;
	double WorldSeconds = 0.;
	TMap<FGuid, FCCLEcologySpecies> Species;
	TMap<FGuid, FCCLEcologyHabitat> Habitats;
	TMap<FGuid, FCCLEcologyPopulation> Populations;
	TMap<FGuid, FCCLEcologyIndividual> Individuals;
	TMap<FGuid, FCCLEcologyEvent> Receipts;
	TSet<FGuid> UsedIndividualIds;
	FGuid Epoch = FGuid::NewGuid();
	uint64 NextLease = 1;
	TMap<FGuid, FCCLEcologyLease> Leases;
	TMap<FGuid, FCCLEcologyGroup> Groups;
	TMap<FGuid, FCCLEcologyForage> ForageSources;
	TMap<FGuid, FCCLEcologyBehavior> Behaviors;
};
