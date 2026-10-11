#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLVillagePopulationSubsystem.generated.h"

class UCCLAgentWorldSubsystem;

// Builds the campaign village from the Agent population: workshops, the steward body and resident villagers.
// The Agent simulation only announces population changes; this content layer owns the Actors.
UCLASS()
class CCL_API UCCLVillagePopulationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 내 클래스 함수
private:
	void RetirePopulation(UCCLAgentWorldSubsystem& Agents);
	void BuildPopulation(UCCLAgentWorldSubsystem& Agents);
	bool IsCampaignWorld() const;
};
