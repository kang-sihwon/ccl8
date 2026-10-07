#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Items/CCLItemFragments.h"
#include "CCLPlayerController.generated.h"

struct FInputActionValue;

class UInputAction;
class UInputMappingContext;
class UCCLSkillDefinition;
class SCCLInventoryWidget;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class ACCLVillageSteward;

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

	UFUNCTION(Server, Reliable)
	void ServerTalkToSteward();
	UFUNCTION(Server, Reliable)
	void ServerBuyPotion();
	UFUNCTION(Client, Reliable)
	void ClientContentTestStep(int32 Step);
	void ToggleInventory();
	void CloseInventory();
	void SelectInventorySlot(int32 Slot);
	void SelectEquipmentSlot(FGameplayTag Slot);
	void CloseDialogue();
	bool IsDialogueVisible() const;
	const FString& GetDialogueName() const { return DialogueName; }
	const FString& GetDialogueText() const { return DialogueText; }

	UFUNCTION(Server, Reliable)
	void ServerMoveInventoryItem(FGuid Id, int32 Slot);
	UFUNCTION(Server, Reliable)
	void ServerEquipToSlot(FGuid Id, FGameplayTag Slot);
	UFUNCTION(Server, Reliable)
	void ServerUnequipToBag(FGuid Id, int32 BagSlot = INDEX_NONE);
	void HandInput(FGameplayTag Hand, bool bPressed);
	void HandPressed(FGameplayTag Hand);
	void HandReleased(FGameplayTag Hand);
	void UpdateEquipmentPreview();
	UTextureRenderTarget2D* GetEquipmentPreview() const { return EquipmentPreview; }
	UFUNCTION(Exec)
	void CCLEquipmentDemo();

	UFUNCTION(Client, Reliable)
	void ClientShowDialogue(ACCLVillageSteward* Speaker, const FString& Name, const FString& Text);
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
	TSharedPtr<SCCLInventoryWidget> GetInventoryWidget() const { return InventoryWidget; }
	int32 GetSelectedItem() const { return SelectedItem; }
	FGuid GetSelectedEquipment() const { return SelectedEquipment; }

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

	UPROPERTY(Transient)

	TObjectPtr<USceneCaptureComponent2D> EquipmentCamera;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> EquipmentPreview;

	FGameplayTag HeldLeftAction;
	FGameplayTag HeldRightAction;
	TSharedPtr<SCCLInventoryWidget> InventoryWidget;
	TWeakObjectPtr<ACCLVillageSteward> DialogueSpeaker;
	FString DialogueName;
	FString DialogueText;

	FGuid SelectedEquipment;
	int32 SelectedItem = 0;
	uint8 bInventoryOpen = 0;
};
