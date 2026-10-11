#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "KSIUITypes.h"
#include "KSIUIPresentation.h"
#include "KSIScreen.h"
#include "CCLUIFoundationSmoke.generated.h"

class UKSIUIContext;
class UKSIUIRegistry;

UCLASS()
class UCCLUICloseOnHideProbe : public UKSIScreen
{
	GENERATED_BODY()

protected:
	virtual void OnPresentationChanged() override;
};

UCLASS()
class CCL_API UCCLUIFoundationSmoke : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
	virtual bool IsTickable() const override { return !IsTemplate() && !bComplete; }
	virtual bool IsTickableWhenPaused() const override { return true; }

private:
	bool Check(bool bCondition, const TCHAR* Message);
	void RequestFinished(FKSIUIRequestHandle Request, FKSIUIViewHandle View, bool bSuccess);
	bool ConfigureExample(bool bStrategy);

private:
	UPROPERTY()
	TObjectPtr<UKSIUIRegistry> Registry;

	UPROPERTY()
	TObjectPtr<UKSIUIContext> ContextA;

	UPROPERTY()
	TObjectPtr<UKSIUIContext> ContextB;

	UPROPERTY()
	TObjectPtr<UKSIUIContext> ContextC;

	TWeakObjectPtr<UKSIScreen> RecycledScreen;
	TWeakObjectPtr<UWorld> PreviousWorld;
	TWeakObjectPtr<AActor> FeatureOwner;
	FKSIUIViewHandle PanelA;
	FKSIUIViewHandle PanelB;
	FKSIUIViewHandle StackA;
	FKSIUIViewHandle StackB;
	FKSIUIViewHandle QueueA;
	FKSIUIViewHandle QueueB;
	FKSIUIViewHandle OwnedView;
	FKSIUIViewHandle PersistentView;
	FKSIUIViewHandle PersistentStackA;
	FKSIUIViewHandle PersistentStackB;
	FKSIUIViewHandle WorldView;
	FKSIUIViewHandle LoadedView;
	FKSIUIRegistrationHandle OwnedRegistration;
	FKSIUIRequestHandle CancelledRequest;
	FKSIUIRequestHandle LoadingRequest;
	FKSIUIPresentationHandle PersistentPresentation;
	int32 CompletedRequests = 0;
	int32 CancelledRequests = 0;
	int32 Step = 0;
	double Next = 0.;
	double Started = 0.;
	uint8 bComplete = 0;
};
