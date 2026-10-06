#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "CCLPlayerController.generated.h"

struct FInputActionValue;

class UInputAction;
class UInputMappingContext;
class UCCLSkillDefinition;

UCLASS()
class CCL_API ACCLPlayerController : public APlayerController
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	virtual void FlushPressedKeys() override;

protected:
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 내 클래스 함수
public:
	UFUNCTION(Exec)
	void CCLRetry();

	UFUNCTION(Exec)
	void CCLDie();

	UFUNCTION(Exec)
	void CCLLeave();

	void ToggleInventory();
	void SelectPreviousItem();
	void SelectNextItem();
	void EquipSelectedItem();
	void UnequipItem();
	void UseSelectedItem();
	void LearnFirstSkill();
	void LearnSecondSkill();

	UFUNCTION(Server, Reliable)
	void ServerCollectNearby();

	UFUNCTION(Server, Reliable)
	void ServerEquipItem(FGuid Id);

	UFUNCTION(Server, Reliable)
	void ServerUseItem(FGuid Id);

	UFUNCTION(Server, Reliable)
	void ServerLearnSkill(UCCLSkillDefinition* Definition);

	UFUNCTION(Server, Reliable)
	void ServerCombatTestReady();

	UFUNCTION(Client, Reliable)
	void ClientCombatTestStep(int32 Step);

	UFUNCTION(Server, Reliable)
	void ServerCampaignTestReady();

	UFUNCTION(Client, Reliable)
	void ClientCampaignTestStep(int32 Step);

	UFUNCTION(Client, Reliable)
	void ClientProgressionTestStep(int32 Step, FGuid EntryId);

	const UInputAction* GetMoveAction() const { return MoveAction; }
	bool IsInventoryOpen() const { return bInventoryOpen != 0; }
	int32 GetSelectedItem() const { return SelectedItem; }

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump();
	void StopJump();
	void CombatPressed(FGameplayTag Tag);
	void CombatReleased(FGameplayTag Tag);
	FGuid GetSelectedEntryId() const;

	UFUNCTION(Server, Reliable)
	void ServerRequestRetry();

	UFUNCTION(Server, Reliable)
	void ServerRequestDebugDeath();

	// 프로퍼티
private:
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RetryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DieAction;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> CombatActions;

	int32 SelectedItem = 0;
	uint8 bInventoryOpen = 0;
};
