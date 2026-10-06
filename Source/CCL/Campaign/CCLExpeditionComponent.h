#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CCLExpeditionComponent.generated.h"
class ACCLVillageSteward;
UENUM()
enum class ECCLQuestStatus : uint8 { Available, Accepted, Rewarded };
UCLASS()
class CCL_API UCCLExpeditionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCCLExpeditionComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool Talk(ACCLVillageSteward* Steward);
	bool Buy(ACCLVillageSteward* Steward);
	int32 GetCoins() const { return Coins; }
	ECCLQuestStatus GetQuest() const { return Quest; }
	const FString& GetNotice() const { return Notice; }
	FString GetTutorial() const;
private:
	bool CanInteract(ACCLVillageSteward* Steward) const;
	bool Report(bool bSuccess, const TCHAR* Message);
	UPROPERTY(Replicated)
	int32 Coins = 30;
	UPROPERTY(Replicated)
	ECCLQuestStatus Quest = ECCLQuestStatus::Available;
	UPROPERTY(Replicated)
	FString Notice;
};
