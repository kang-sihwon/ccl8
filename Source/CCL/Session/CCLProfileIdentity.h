#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/SaveGame.h"
#include "CCLProfileIdentity.generated.h"

struct FURL;

// Server-issued identity for remote players.
// The server gives each new remote player a random secret. The client keeps it per server address and presents it
// as a login option. The world save stores only a hash of the secret as the profile identity key, so a save file
// cannot be used to reconnect as someone else.
namespace CCLProfileIdentity
{
	// Login option name, for example "?CCLProfile=<secret>".
	CCL_API const TCHAR* LoginOption();
	CCL_API FString IssueSecret();
	CCL_API bool IsValidSecret(const FString& Secret);
	CCL_API FString IdentityKeyFor(const FString& Secret);
	CCL_API FString ServerAddress(const FURL& Url);
}

// Client-side store of reconnect secrets, keyed by server address.
UCLASS()
class CCL_API UCCLProfileKeySave : public USaveGame
{
	GENERATED_BODY()

	// 프로퍼티
public:
	UPROPERTY()
	TMap<FString, FString> Secrets;
};

UCLASS()
class CCL_API UCCLLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	// Adds the reconnect secret for the server this player is joining.
	virtual FString GetGameLoginOptions() const override;
};
