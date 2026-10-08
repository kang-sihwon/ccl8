#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "UI/Core/CCLUITypes.h"
#include "UI/Core/CCLUIPresentation.h"
#include "CCLUIFoundationSmoke.generated.h"

class UCCLUIContext;
class UCCLUIRegistry;
class UCCLScreen;

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

private:
	bool Check(bool bCondition, const TCHAR* Message);
	void RequestFinished(FCCLUIRequestHandle Request, FCCLUIViewHandle View, bool bSuccess);

private:
	UPROPERTY()
	TObjectPtr<UCCLUIRegistry> Registry;

	UPROPERTY()
	TObjectPtr<UCCLUIContext> ContextA;

	UPROPERTY()
	TObjectPtr<UCCLUIContext> ContextB;

	UPROPERTY()
	TObjectPtr<UCCLUIContext> ContextC;

	TWeakObjectPtr<UCCLScreen> RecycledScreen;
	TWeakObjectPtr<UWorld> PreviousWorld;
	TWeakObjectPtr<AActor> FeatureOwner;
	FCCLUIViewHandle PanelA;
	FCCLUIViewHandle PanelB;
	FCCLUIViewHandle StackA;
	FCCLUIViewHandle StackB;
	FCCLUIViewHandle QueueA;
	FCCLUIViewHandle QueueB;
	FCCLUIViewHandle OwnedView;
	FCCLUIViewHandle PersistentView;
	FCCLUIViewHandle PersistentStackA;
	FCCLUIViewHandle PersistentStackB;
	FCCLUIViewHandle WorldView;
	FCCLUIViewHandle LoadedView;
	FCCLUIRegistrationHandle OwnedRegistration;
	FCCLUIRequestHandle CancelledRequest;
	FCCLUIRequestHandle LoadingRequest;
	FCCLUIPresentationHandle PersistentPresentation;
	int32 CompletedRequests = 0;
	int32 CancelledRequests = 0;
	int32 Step = 0;
	double Next = 0.;
	double Started = 0.;
	uint8 bComplete = 0;
};
