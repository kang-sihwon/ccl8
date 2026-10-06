#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CCLCampaignState.generated.h"

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
class CCL_API ACCLCampaignState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void SetProgress(ECCLCampaignPhase Phase, int32 RemainingGuards);
	ECCLCampaignPhase GetPhase() const { return Progress.Phase; }
	int32 GetRemainingGuards() const { return Progress.RemainingGuards; }
	FString GetObjective() const;

private:
	UPROPERTY(Replicated)
	FCCLCampaignProgress Progress;
};
