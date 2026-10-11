#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CCLTestBridgeComponent.generated.h"

class APlayerController;

// Network channel for multi-process smoke tests. A smoke subsystem attaches it to player controllers on the
// server only when its command-line switch is present, so production controllers carry no test RPCs.
UCLASS(NotBlueprintable)
class CCL_API UCCLTestBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	UCCLTestBridgeComponent();

	// 내 클래스 함수
public:
	// Server only. Adds a replicated bridge to the controller if it has none.
	static UCCLTestBridgeComponent* Ensure(APlayerController* Controller);
	static UCCLTestBridgeComponent* Find(const APlayerController* Controller);

	// Server only. Gives the controller's player the training equipment set used by UI checks.
	static void GrantEquipmentSet(APlayerController* Controller);

	UFUNCTION(Server, Reliable)
	void ServerCombatTestReady();

	UFUNCTION(Server, Reliable)
	void ServerCampaignTestReady();

	UFUNCTION(Client, Reliable)
	void ClientCombatTestStep(int32 Step);

	UFUNCTION(Client, Reliable)
	void ClientCampaignTestStep(int32 Step);

	UFUNCTION(Client, Reliable)
	void ClientProgressionTestStep(int32 Step, FGuid EntryId);

	UFUNCTION(Client, Reliable)
	void ClientContentTestStep(int32 Step);
};
