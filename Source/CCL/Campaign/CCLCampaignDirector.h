#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLCampaignDirector.generated.h"

class ACCLEnemyCharacter;
class ACCLCampaignState;

UCLASS()
class CCL_API ACCLCampaignDirector : public AActor
{
	GENERATED_BODY()

public:
	ACCLCampaignDirector();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

public:
	void NotifyEnemyDefeated(ACCLEnemyCharacter* Enemy);
	ACCLEnemyCharacter* GetBoss() const { return Boss.Get(); }
	const TArray<TWeakObjectPtr<ACCLEnemyCharacter>>& GetGuards() const { return Guards; }

private:
	ACCLEnemyCharacter* SpawnEnemy(FVector Location, bool bBoss);

public:
	UPROPERTY(EditAnywhere, Category = "Encounter")
	TArray<FVector> GuardLocations;

	UPROPERTY(EditAnywhere, Category = "Encounter")
	FVector BossLocation = FVector(1400.f, 0.f, 96.f);

	UPROPERTY(EditAnywhere, Category = "Encounter")
	float RoadStartX = -650.f;

private:
	TWeakObjectPtr<ACCLCampaignState> State;
	TArray<TWeakObjectPtr<ACCLEnemyCharacter>> Guards;
	TSet<TWeakObjectPtr<ACCLEnemyCharacter>> Defeated;
	TWeakObjectPtr<ACCLEnemyCharacter> Boss;
};
