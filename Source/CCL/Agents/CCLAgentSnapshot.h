#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KSIAgentTypes.h"
#include "CCLAgentSnapshot.generated.h"

UCLASS()
class CCL_API UCCLAgentSnapshot : public USaveGame
{
	GENERATED_BODY()

public:
	static bool Encode(const FKSIAgentStore& Store, TArray<uint8>& Bytes);
	static bool Decode(const TArray<uint8>& Bytes, FKSIAgentStore& Store, const FKSIFeatureRegistry& Registry, FString& Error);

public:
	UPROPERTY()
	int32 Version = 1;

	UPROPERTY()
	TArray<FKSIAgentRecord> Agents;
};
