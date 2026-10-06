#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLNetworkSmokeSubsystem.generated.h"

class ACCLCharacter;

/** Opt-in process test. Disabled unless -CCLSmoke=driver or witness is supplied. */
UCLASS()
class UCCLNetworkSmokeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// 내 클래스 함수
private:
	void Finish(bool bSuccess, const TCHAR* Reason);

	// 프로퍼티
private:
	TWeakObjectPtr<ACCLCharacter> Original;
	TWeakObjectPtr<ACCLCharacter> Peer;
	TArray<TWeakObjectPtr<AActor>> SpawnBlockers;
	FVector StartLocation = FVector::ZeroVector;
	FVector PeerStartLocation = FVector::ZeroVector;
	double StartedAt = 0.0;
	double PhaseStartedAt = 0.0;
	int32 Phase = 0;
	uint8 bSawPeerDeath = 0;
	uint8 bSawPeerMovement = 0;
	uint8 bFinished = 0;
};
