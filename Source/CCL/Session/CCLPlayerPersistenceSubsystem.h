#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLPlayerSnapshot.h"
#include "KSIWorldParticipant.h"
#include "Environment/CCLWorldAdvanceParticipant.h"
#include "Items/CCLLoadoutRestore.h"
#include "CCLPlayerPersistenceSubsystem.generated.h"

class ACCLPlayerState;
class FKSISurfaceSimulation;
struct FKSIWorldStep;

// Owns player profiles of a server world and stores them in the "Players" world section.
// Registered players keep equipment, food, account and quest across world saves, restores and reconnection.
UCLASS()
class CCL_API UCCLPlayerPersistenceSubsystem : public UWorldSubsystem, public IKSIWorldParticipant, public ICCLWorldAdvanceParticipant
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual FName GetWorldSection() const override;
	virtual int32 GetWorldOrder() const override { return 0; }
	virtual bool CaptureWorldSection(FKSIWorldCaptureContext& Context, TArray<uint8>& OutBytes, FString& Error) override;
	virtual bool PrepareWorldRestore(FKSIWorldRestoreContext& Context, const TArray<uint8>* Section, FString& Error) override;
	virtual bool CommitWorldRestore(TFunctionRef<bool()> CommitInner) override;
	virtual void PublishWorldRestore() override;
	virtual void CancelWorldRestore() override;
	virtual int32 GetWorldAdvanceOrder() const override { return GetWorldOrder(); }
	virtual bool NeedsWorldAdvance() const override;
	virtual bool PrepareWorldAdvance(const FKSIWorldStep& Step, const FKSISurfaceSimulation& Surface, FString& Error) override;
	virtual void CommitWorldAdvance() override;
	virtual void PublishWorldAdvance() override;
	virtual void CancelWorldAdvance() override;

	// 내 클래스 함수
public:
	// Trusted server identity binding, never an RPC or client identity assertion.
	bool Bind(ACCLPlayerState& Player, const FString& IdentityKey, FString& Error);
	void Unbind(ACCLPlayerState& Player);
	bool Capture(FGuid WorldId, double WorldSeconds, TArray<FCCLPlayerSnapshot>& Output,
		TArray<UCCLInventoryComponent*>& Participants, FString& Error,
		const UCCLInventoryComponent* SessionInventory = nullptr) const;

	// Adds the host's personal session to the next world restore so both commit in one effect batch.
	// Finalize runs after every state committed and before notifications. Server only.
	bool StageSessionRestore(const FCCLLoadoutRestoreRequest& Request, TFunction<void()> Finalize, FString& Error);
	void ClearSessionRestore();

	bool OwnsInventory(const UCCLInventoryComponent& Inventory) const;
	bool IsBound(const ACCLPlayerState& Player) const;
	bool HasProfile(const FString& IdentityKey) const { return Records.Contains(IdentityKey); }

private:
	bool CapturePlayer(const FString& Key, ACCLPlayerState& Player, FCCLPlayerSnapshot& Output, FString& Error) const;
	bool Resolve(const FCCLPlayerSnapshot& Profile, ACCLPlayerState& Player, FCCLLoadoutRestoreRequest& Request,
		FString& Error) const;
	void Finalize(ACCLPlayerState& Player, const FCCLPlayerSnapshot& Profile);
	void ResetRestore();

	// 프로퍼티
private:
	TMap<FString, FCCLPlayerSnapshot> Records;
	TMap<FString, TWeakObjectPtr<ACCLPlayerState>> Live;
	TMap<FString, FCCLPlayerSnapshot> Prepared;
	TMap<TWeakObjectPtr<UCCLInventoryComponent>, TArray<FKSIInventoryEntry>> Pending;
	TArray<TWeakObjectPtr<UCCLInventoryComponent>> Locked;

	// Staged world restore: profiles to commit, live players to restore and the optional host session.
	TMap<FString, FCCLPlayerSnapshot> RestoreCandidate;
	TArray<FCCLLoadoutRestoreRequest> RestoreRequests;
	TArray<TPair<TWeakObjectPtr<ACCLPlayerState>, FCCLPlayerSnapshot>> RestoreResolved;
	TOptional<FCCLLoadoutRestoreRequest> SessionRequest;
	TFunction<void()> SessionFinalize;

	uint8 bPreparing = 0;
	uint8 bCaptureFailed = 0;
	uint8 bRestoreStaged = 0;
	uint8 bRestoreStarting = 0;
};
