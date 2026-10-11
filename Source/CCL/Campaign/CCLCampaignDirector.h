#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLCampaignDirector.generated.h"

struct FCCLSessionRecord;
struct FCCLSavedSupply;
class ACCLWorldPickup;
class ACCLEnemyCharacter;
class ACCLCampaignState;
class UCCLEnemyDefinition;
class UKSIItemDefinition;
class UCCLItemDefinition;

USTRUCT(BlueprintType)
struct FCCLEncounterSpawn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Encounter")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	TSoftObjectPtr<UCCLEnemyDefinition> Definition;
};

USTRUCT(BlueprintType)
struct FCCLSupplySpawn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Supply")
	FGuid PlacementId;

	UPROPERTY(EditAnywhere, Category = "Supply")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Supply")
	TSoftObjectPtr<UKSIItemDefinition> Item;

	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "1"))
	int32 Quantity = 1;
};

UCLASS()
class CCL_API ACCLCampaignDirector : public AActor
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	ACCLCampaignDirector();

	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// 내 클래스 함수
public:
	bool CaptureSupplies(TArray<FCCLSavedSupply>& OutSupplies, FString& Error) const;
	bool PrepareSupplyRestore(const FCCLSessionRecord& Record, TArray<FCCLSavedSupply>& OutSupplies, FString& Error) const;
	void ApplySupplies(const TArray<FCCLSavedSupply>& Saved);
	bool RestoreCheckpoint(uint8 Mask, bool bVictory);
	static FGuid MakeGuardAgentId(int32 GuardIndex);
	static FGuid BossAgentId() { return FGuid(0xCC190000, 0, 1, 3); }
	bool ValidateSupplies(FString& Error) const;
	void NotifyEnemyDefeated(ACCLEnemyCharacter* Enemy);

	uint8 GetDefeatedMask() const;
	uint8 GetAllGuardsMask() const;
	ACCLEnemyCharacter* GetBoss() const { return BossEnemy.Get(); }
	const TArray<TWeakObjectPtr<ACCLEnemyCharacter>>& GetGuards() const { return GuardEnemies; }

private:
	ACCLEnemyCharacter* SpawnEnemy(const FCCLEncounterSpawn& Spawn, FGuid AgentId);

	// 에디터 전용
#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif

	// 프로퍼티
public:
	// Defeated guards are saved as a bit mask, so a road holds at most eight guards.
	static constexpr int32 MaxGuards = 8;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	TArray<FCCLEncounterSpawn> Guards;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	FCCLEncounterSpawn Boss;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	TArray<FCCLSupplySpawn> Supplies;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float RoadStartX = -650.f;

private:
	TMap<FGuid, TWeakObjectPtr<ACCLWorldPickup>> SupplyActors;
	TWeakObjectPtr<ACCLCampaignState> State;
	TArray<TWeakObjectPtr<ACCLEnemyCharacter>> GuardEnemies;
	TSet<TWeakObjectPtr<ACCLEnemyCharacter>> Defeated;
	TWeakObjectPtr<ACCLEnemyCharacter> BossEnemy;
};
