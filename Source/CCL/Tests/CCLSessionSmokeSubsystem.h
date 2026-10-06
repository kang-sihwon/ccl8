#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CCLSessionSmokeSubsystem.generated.h"
UCLASS()
class CCL_API UCCLSessionSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate() && !bComplete; }
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
private:
	bool Check(bool bCondition, const TCHAR* Message);
	void Finish();
	void Capture(const TCHAR* Name);
	int32 Step = 0;
	uint8 bComplete = 0;
	double Started = 0.;
	double Next = 0.;
};
