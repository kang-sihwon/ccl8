#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "CCLPlayerController.generated.h"

struct FInputActionValue;

class UInputAction;
class UInputMappingContext;

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
	void ServerCombatTestReady();

	UFUNCTION(Client, Reliable)
	void ClientCombatTestStep(int32 Step);

	UFUNCTION(Server, Reliable)
	void ServerCampaignTestReady();

	UFUNCTION(Client, Reliable)
	void ClientCampaignTestStep(int32 Step);

	const UInputAction* GetMoveAction() const { return MoveAction; }

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump();
	void StopJump();
	void CombatPressed(FGameplayTag Tag);
	void CombatReleased(FGameplayTag Tag);

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
};
