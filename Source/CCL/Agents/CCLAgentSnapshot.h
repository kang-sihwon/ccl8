#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CCLAgentTypes.h"
#include "CCLAgentSnapshot.generated.h"

UCLASS()
class CCL_API UCCLAgentSnapshot : public USaveGame
{
	GENERATED_BODY()

public:
	static bool Encode(const FCCLAgentStore& Store, TArray<uint8>& Bytes);
	static bool Decode(const TArray<uint8>& Bytes, FCCLAgentStore& Store, const FCCLFeatureRegistry& Registry, FString& Error);

public:
	UPROPERTY()
	int32 Version = 1;

	UPROPERTY()
	TArray<FCCLAgentRecord> Agents;
};
