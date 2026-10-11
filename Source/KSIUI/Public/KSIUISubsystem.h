#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "KSIUIRegistry.h"
#include "KSIUIPresentation.h"
#include "Tickable.h"
#include "KSIUISubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FKSIUIRequestFinished, FKSIUIRequestHandle, FKSIUIViewHandle, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FKSIUIViewClosed, FKSIUIViewHandle);

class UKSIUIRoot;
class UKSIScreen;
struct FStreamableHandle;

USTRUCT()
struct KSIUI_API FKSIUIRegistration
{
	GENERATED_BODY()

	UPROPERTY()
	FKSIUIViewDefinition Definition;

	UPROPERTY()
	TWeakObjectPtr<UObject> Owner;
};

USTRUCT()
struct KSIUI_API FKSIUIOpenView
{
	GENERATED_BODY()

	UPROPERTY()
	FKSIUIViewDefinition Definition;

	UPROPERTY()
	FKSIUIRegistrationHandle Registration;

	UPROPERTY()
	TObjectPtr<UKSIUIContext> Context;

	UPROPERTY()
	TObjectPtr<UKSIScreen> Screen;

	UPROPERTY()
	TWeakObjectPtr<UObject> Owner;

	UPROPERTY()
	TWeakObjectPtr<UWorld> World;

	uint64 OpenOrder = 0;
	FKSIUIViewPresentation BasePresentation;
};

USTRUCT()
struct KSIUI_API FKSIUIPendingView
{
	GENERATED_BODY()

	UPROPERTY()
	FKSIUIRegistrationHandle Registration;

	UPROPERTY()
	TObjectPtr<UKSIUIContext> Context;

	UPROPERTY()
	TWeakObjectPtr<UObject> Owner;

	UPROPERTY()
	TWeakObjectPtr<UWorld> World;

	uint8 bReady = 0;
};

UCLASS()
class KSIUI_API UKSIUISubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }

public:
	bool ConfigureRegistry(UKSIUIRegistry* InRegistry);
	FKSIUIRegistrationHandle RegisterView(const FKSIUIViewDefinition& Definition, UObject* Owner);
	void UnregisterView(FKSIUIRegistrationHandle Registration);
	FKSIUIViewHandle OpenView(FGameplayTag View, UKSIUIContext* Context, UObject* Owner);
	FKSIUIRequestHandle RequestOpenView(FGameplayTag View, UKSIUIContext* Context, UObject* Owner);
	void CancelRequest(FKSIUIRequestHandle Request);
	void CloseView(FKSIUIViewHandle View);
	void CloseAllViews();
	FKSIUIPresentationHandle PushPresentation(const FKSIUIPresentationDefinition& Definition, UObject* Owner);
	void ReleasePresentation(FKSIUIPresentationHandle Handle);
	bool SetBasePresentation(FKSIUIViewHandle View, const FKSIUIViewPresentation& State);
	void InvalidatePresentationRouting() { bPresentationRoutingDirty = 1; }
	bool IsGameplayInputBlocked() const;
	int32 GetPresentationCount() const { return Presentations.Num(); }
	bool EnsureRoot();
	UKSIScreen* FindScreen(FKSIUIViewHandle View) const;
	FKSIUIRegistrationHandle FindRegistration(FGameplayTag View) const;
	UKSIUIRoot* GetRoot() const { return Root; }
	const UKSIUIRegistry* GetRegistry() const { return Registry; }
	int32 GetViewCount() const { return Views.Num(); }
	int32 GetPendingCount() const { return Pending.Num(); }
	bool IsViewOpen(FKSIUIViewHandle View) const { return Views.Contains(View.Id); }

private:
	bool AttachScreen(FKSIUIViewHandle Handle);
	void ResetRoot();
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void FinishRequest(FKSIUIRequestHandle Request);
	void MarkRequestReady(FGuid Id);
	bool CanOpen(const FKSIUIRegistration* Registration, UKSIUIContext* Context, UObject* Owner) const;
	void RefreshPresentation(float FadeSeconds = 0.f);
	void ApplyPresentation(FKSIUIViewHandle View, float FadeSeconds);

public:
	FKSIUIRequestFinished OnRequestFinished;
	FKSIUIViewClosed OnViewClosed;

private:
	UPROPERTY(Transient)
	TObjectPtr<UKSIUIRegistry> Registry;

	UPROPERTY(Transient)
	TObjectPtr<UKSIUIRoot> Root;

	UPROPERTY(Transient)
	TMap<FGuid, FKSIUIRegistration> Registrations;

	UPROPERTY(Transient)
	TMap<FGuid, FKSIUIOpenView> Views;

	UPROPERTY(Transient)
	TMap<FGuid, FKSIUIPendingView> Pending;

	UPROPERTY(Transient)
	TMap<FGuid, FKSIUIPresentationRequest> Presentations;

	TMap<FGuid, TSharedPtr<FStreamableHandle>> Loads;
	TSet<FGuid> OpeningRegistrations;
	FDelegateHandle WorldCleanupHandle;
	uint64 NextOpenOrder = 0;
	uint8 bShuttingDown = 0;
	uint8 bCleaningWorld = 0;
	uint8 bClosingAll = 0;
	uint8 bRemovingScreen = 0;
	uint8 bPresentationRoutingDirty = 0;
	uint8 bRefreshingPresentation = 0;
	uint8 bPresentationDirty = 0;
};
