#pragma once

#include "CoreMinimal.h"

class IKSIEcologySpace;
class FKSISurfaceSimulation;

enum class EKSIEcologyMovement : uint8 { Ground, Water, Air, Insect };
enum class EKSIEcologyRepresentation : uint8 { Dormant, Actor, Mass };
enum class EKSIEcologyEventKind : uint8 { Birth, Immigration, Death, Capture, Migration, Materialize };
enum class EKSIEcologyLifeState : uint8 { Alive, Dead, Captured };

struct KSIENVIRONMENT_API FKSIEcologyNutrition
{
	double Energy = 1.;
	double StarvedSeconds = 0.;
	double LastStepSeconds = 0.;
	void Charge(double At, double Metabolism);
	void Merge(int64 Count, FKSIEcologyNutrition Other, int64 OtherCount, double At, double Metabolism);
};

struct KSIENVIRONMENT_API FKSIEcologySpecies
{
	FGuid Id;
	EKSIEcologyMovement Movement = EKSIEcologyMovement::Ground;
	double MaturitySeconds = 86400.;
	double LifespanSeconds = 864000.;
	// Zero disables scheduled reproduction. Nonzero intervals are whole world seconds.
	double BirthIntervalSeconds = 0.;
	// Zero preserves legacy populations without aggregate feeding.
	uint32 AggregateDietMask = 0;
	double AggregateMetabolism = .002;
	double AggregateEnergyPerUnit = .2;
	double AggregateStarvationSeconds = 60.;
};

struct KSIENVIRONMENT_API FKSIEcologyHabitat
{
	FGuid Id;
	FVector MinimumMeters = FVector::ZeroVector;
	FVector MaximumMeters = FVector(100.);
	uint8 MovementMask = 15;
	int64 Capacity = 10000;
};

struct KSIENVIRONMENT_API FKSIEcologyCohort
{
	FGuid Id;
	double BirthWorldSeconds = 0.;
	int64 Count = 0;
	FKSIEcologyNutrition Nutrition;
};

struct KSIENVIRONMENT_API FKSIEcologyPopulation
{
	FGuid Id;
	FGuid SpeciesId;
	FGuid HabitatId;
	TMap<FGuid, FKSIEcologyCohort> Cohorts;
	int64 Initial = 0;
	int64 Born = 0;
	int64 Arrived = 0;
	int64 Died = 0;
	int64 Captured = 0;
	int64 Departed = 0;
	double AggregateConsumedUnits = 0.;
};

struct KSIENVIRONMENT_API FKSIEcologyIndividual
{
	FGuid Id;
	FGuid PopulationId;
	FGuid CohortId;
	double BirthWorldSeconds = 0.;
	EKSIEcologyLifeState State = EKSIEcologyLifeState::Alive;
	uint8 bPersistent = 0;
	FVector PositionMeters = FVector::ZeroVector;
	FKSIEcologyNutrition Nutrition;
};

struct KSIENVIRONMENT_API FKSIEcologyEvent
{
	FGuid Id;
	EKSIEcologyEventKind Kind = EKSIEcologyEventKind::Immigration;
	FGuid Source;
	FGuid Target;
	FGuid CohortId;
	FGuid IndividualId;
	int64 Count = 1;
	double BirthWorldSeconds = 0.;
	uint8 bPersistent = 0;
	bool operator==(const FKSIEcologyEvent& Other) const;
};

struct KSIENVIRONMENT_API FKSIEcologyLease
{
	FGuid IndividualId;
	FGuid Writer;
	FGuid Epoch;
	uint64 Sequence = 0;
	EKSIEcologyRepresentation Representation = EKSIEcologyRepresentation::Dormant;
};

enum class EKSIEcologyActivity : uint8 { Roaming, SeekingFood, Feeding, Starving, Inactive, Blocked };

struct KSIENVIRONMENT_API FKSIEcologyGroup
{
	FGuid Id;
	FGuid HabitatId;
	FVector AnchorMeters = FVector::ZeroVector;
	double RadiusMeters = 2.;
};

struct KSIENVIRONMENT_API FKSIEcologyForage
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

struct KSIENVIRONMENT_API FKSIEcologyBehavior
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
	EKSIEcologyActivity Activity = EKSIEcologyActivity::Roaming;
	uint8 bUseWorldSpace = 0;
	double RadiusMeters = .2;
};

class KSIENVIRONMENT_API FKSIEcologySimulation
{
public:
	bool Initialize(FGuid WorldId, double WorldSeconds, FString& Error);
	bool AddSpecies(const FKSIEcologySpecies& Species, FString& Error);
	bool AddHabitat(const FKSIEcologyHabitat& Habitat, FString& Error);
	bool AddPopulation(FKSIEcologyPopulation Population, FString& Error);
	bool AddGroup(const FKSIEcologyGroup& Group, FString& Error);
	bool AddForage(FKSIEcologyForage Forage, FString& Error);
	bool AddBehavior(FKSIEcologyBehavior Behavior, FString& Error);
	const TMap<FGuid, FKSIEcologyGroup>& GetGroups() const { return Groups; }
	const TMap<FGuid, FKSIEcologyForage>& GetForage() const { return ForageSources; }
	const TMap<FGuid, FKSIEcologyBehavior>& GetBehaviors() const { return Behaviors; }
	bool HasBehavior(FGuid Id) const { return Behaviors.Contains(Id); }
	bool Apply(const FKSIEcologyEvent& Event, FString& Error);
	bool Materialize(FGuid RequestId, FGuid PopulationId, FGuid CohortId, FGuid IndividualId, bool bPersistent, FString& Error);
	bool Acquire(FGuid IndividualId, EKSIEcologyRepresentation Representation, FGuid Writer, FKSIEcologyLease& OutLease, FString& Error);
	bool Release(const FKSIEcologyLease& Lease, FString& Error);
	bool Suspend(const FKSIEcologyLease& Lease, FString& Error);
	bool ReturnToAggregate(FGuid IndividualId, FString& Error);
	bool Transfer(const FKSIEcologyLease& Lease, EKSIEcologyRepresentation Representation, FGuid Writer,
		FKSIEcologyLease& OutLease, FString& Error);
	bool UpdatePosition(const FKSIEcologyLease& Lease, const FVector& PositionMeters, FString& Error);
	bool Finish(const FKSIEcologyLease& Lease, const FKSIEcologyEvent& Event, FString& Error);
	bool IsLeaseCurrent(const FKSIEcologyLease& Lease) const;
	bool AdvanceTo(double TargetWorldSeconds, FString& Error, int32 MaxBoundaries = 256,
		const IKSIEcologySpace* Space = nullptr, const FKSISurfaceSimulation* Surface = nullptr);
	bool HasSpatialBehavior() const;
	bool Capture(TArray<uint8>& Bytes, FString& Error) const;
	bool Restore(const TArray<uint8>& Bytes, FString& Error);
	bool Validate(FString& Error) const;
	int64 LivingCount(FGuid PopulationId) const;
	int64 AggregateCount(FGuid PopulationId) const;
	FGuid GetWorldId() const { return WorldId; }
	double GetWorldSeconds() const { return WorldSeconds; }
	const TMap<FGuid, FKSIEcologyPopulation>& GetPopulations() const { return Populations; }
	const TMap<FGuid, FKSIEcologyIndividual>& GetIndividuals() const { return Individuals; }
	const TMap<FGuid, FKSIEcologySpecies>& GetSpecies() const { return Species; }
	const TMap<FGuid, FKSIEcologyHabitat>& GetHabitats() const { return Habitats; }
	const FKSIEcologyIndividual* FindIndividual(FGuid Id) const { return Individuals.Find(Id); }
	int32 ReceiptCount() const { return Receipts.Num(); }
	static constexpr int32 MaximumBytes = 8 * 1024 * 1024;

private:
	bool ApplyCandidate(const FKSIEcologyEvent& Event, FString& Error);
	bool AdvanceCandidate(double TargetWorldSeconds, FString& Error, int32 MaxBoundaries,
		const IKSIEcologySpace* Space, const FKSISurfaceSimulation* Surface);
	bool Serialize(FArchive& Archive);
	bool AdvanceBehavior(double TargetSeconds, FString& Error, const IKSIEcologySpace* Space, const FKSISurfaceSimulation* Surface);
	bool ValidateBehavior(FString& Error) const;
	bool AdvanceAggregate(double At, FString& Error);

private:
	FGuid WorldId;
	double WorldSeconds = 0.;
	TMap<FGuid, FKSIEcologySpecies> Species;
	TMap<FGuid, FKSIEcologyHabitat> Habitats;
	TMap<FGuid, FKSIEcologyPopulation> Populations;
	TMap<FGuid, FKSIEcologyIndividual> Individuals;
	TMap<FGuid, FKSIEcologyEvent> Receipts;
	TSet<FGuid> UsedIndividualIds;
	FGuid Epoch = FGuid::NewGuid();
	uint64 NextLease = 1;
	TMap<FGuid, FKSIEcologyLease> Leases;
	TMap<FGuid, FKSIEcologyGroup> Groups;
	TMap<FGuid, FKSIEcologyForage> ForageSources;
	TMap<FGuid, FKSIEcologyBehavior> Behaviors;
};
