#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Agents/CCLAgentEmbodiment.h"
#include "CCLCampaignState.generated.h"

class UCCLPopulationScenario;

UENUM()
enum class ECCLCampaignPhase : uint8 { Village, Road, Boss, Victory, Error };

USTRUCT()
struct FCCLCampaignProgress
{
	GENERATED_BODY()

	UPROPERTY()
	ECCLCampaignPhase Phase = ECCLCampaignPhase::Village;

	UPROPERTY()
	int32 RemainingGuards = 0;
};

UCLASS()
class CCL_API ACCLCampaignState : public AGameStateBase, public ICCLAgentWorldHost
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	ACCLCampaignState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool BuildInitialPopulation(FCCLSimulationSnapshot& OutInitial, FString& Error) const override;

public:
	void SetProgress(ECCLCampaignPhase Phase, int32 RemainingGuards);
	ECCLCampaignPhase GetPhase() const { return Progress.Phase; }
	int32 GetRemainingGuards() const { return Progress.RemainingGuards; }
	FString GetObjective() const;

	// 프로퍼티
public:
	// The village population of a new campaign.
	UPROPERTY(EditAnywhere, Category = "Population")
	TSoftObjectPtr<UCCLPopulationScenario> PopulationScenario;

private:
	UPROPERTY(Replicated)
	FCCLCampaignProgress Progress;
};
