#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Items/CCLItemFragments.h"
#include "UI/Core/CCLUITypes.h"
#include "CCLPlayerController.generated.h"

struct FInputActionValue;

class UInputAction;
class UInputMappingContext;
class UCCLSkillDefinition;
class SCCLInventoryWidget;
class UCCLInventoryContext;
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
	UTextureRenderTarget2D* GetEquipmentPreview() const;
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
	bool IsInventoryOpen() const;
	TSharedPtr<SCCLInventoryWidget> GetInventoryWidget() const;
	int32 GetSelectedItem() const;
	FGuid GetSelectedEquipment() const;
	FCCLUIViewHandle GetInventoryHandle() const { return InventoryHandle; }

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump();
	void StopJump();
	void CombatPressed(FGameplayTag Tag);
	void CombatReleased(FGameplayTag Tag);
	FGuid GetSelectedEntryId() const;
	void HandleUIViewClosed(FCCLUIViewHandle View);

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
	TObjectPtr<UCCLInventoryContext> InventoryContext;

	FGameplayTag HeldLeftAction;
	FGameplayTag HeldRightAction;
	FCCLUIViewHandle InventoryHandle;
	TWeakObjectPtr<ACCLVillageSteward> DialogueSpeaker;
	FString DialogueName;
	FString DialogueText;

};
