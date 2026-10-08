#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "CCLUIRegistry.h"
#include "Tickable.h"
#include "CCLUISubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FCCLUIRequestFinished, FCCLUIRequestHandle, FCCLUIViewHandle, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FCCLUIViewClosed, FCCLUIViewHandle);

class UCCLUIRoot;
class UCCLScreen;
struct FStreamableHandle;

USTRUCT()
struct FCCLUIRegistration
{
	GENERATED_BODY()

	UPROPERTY()
	FCCLUIViewDefinition Definition;

	UPROPERTY()
	TWeakObjectPtr<UObject> Owner;
};

USTRUCT()
struct FCCLUIOpenView
{
	GENERATED_BODY()

	UPROPERTY()
	FCCLUIViewDefinition Definition;

	UPROPERTY()
	FCCLUIRegistrationHandle Registration;

	UPROPERTY()
	TObjectPtr<UCCLUIContext> Context;

	UPROPERTY()
	TObjectPtr<UCCLScreen> Screen;

	UPROPERTY()
	TWeakObjectPtr<UObject> Owner;

	UPROPERTY()
	TWeakObjectPtr<UWorld> World;

	uint64 OpenOrder = 0;
};

USTRUCT()
struct FCCLUIPendingView
{
	GENERATED_BODY()

	UPROPERTY()
	FCCLUIRegistrationHandle Registration;

	UPROPERTY()
	TObjectPtr<UCCLUIContext> Context;

	UPROPERTY()
	TWeakObjectPtr<UObject> Owner;

	UPROPERTY()
	TWeakObjectPtr<UWorld> World;

	uint8 bReady = 0;
};

UCLASS()
class CCL_API UCCLUISubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
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
	bool ConfigureRegistry(UCCLUIRegistry* InRegistry);
	FCCLUIRegistrationHandle RegisterView(const FCCLUIViewDefinition& Definition, UObject* Owner);
	void UnregisterView(FCCLUIRegistrationHandle Registration);
	FCCLUIViewHandle OpenView(FGameplayTag View, UCCLUIContext* Context, UObject* Owner);
	FCCLUIRequestHandle RequestOpenView(FGameplayTag View, UCCLUIContext* Context, UObject* Owner);
	void CancelRequest(FCCLUIRequestHandle Request);
	void CloseView(FCCLUIViewHandle View);
	void CloseAllViews();
	bool EnsureRoot();
	UCCLScreen* FindScreen(FCCLUIViewHandle View) const;
	FCCLUIRegistrationHandle FindRegistration(FGameplayTag View) const;
	UCCLUIRoot* GetRoot() const { return Root; }
	const UCCLUIRegistry* GetRegistry() const { return Registry; }
	int32 GetViewCount() const { return Views.Num(); }
	int32 GetPendingCount() const { return Pending.Num(); }
	bool IsViewOpen(FCCLUIViewHandle View) const { return Views.Contains(View.Id); }

private:
	bool AttachScreen(FCCLUIViewHandle Handle);
	void ResetRoot();
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void FinishRequest(FCCLUIRequestHandle Request);
	void MarkRequestReady(FGuid Id);
	bool CanOpen(const FCCLUIRegistration* Registration, UCCLUIContext* Context, UObject* Owner) const;

public:
	FCCLUIRequestFinished OnRequestFinished;
	FCCLUIViewClosed OnViewClosed;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCCLUIRegistry> Registry;

	UPROPERTY(Transient)
	TObjectPtr<UCCLUIRoot> Root;

	UPROPERTY(Transient)
	TMap<FGuid, FCCLUIRegistration> Registrations;

	UPROPERTY(Transient)
	TMap<FGuid, FCCLUIOpenView> Views;

	UPROPERTY(Transient)
	TMap<FGuid, FCCLUIPendingView> Pending;

	TMap<FGuid, TSharedPtr<FStreamableHandle>> Loads;
	TSet<FGuid> OpeningRegistrations;
	FDelegateHandle WorldCleanupHandle;
	uint64 NextOpenOrder = 0;
	uint8 bShuttingDown = 0;
	uint8 bCleaningWorld = 0;
	uint8 bClosingAll = 0;
	uint8 bRemovingScreen = 0;
};
