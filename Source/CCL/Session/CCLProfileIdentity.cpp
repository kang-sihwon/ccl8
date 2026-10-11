#include "CCLProfileIdentity.h"

#include "CCLGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "Engine/PendingNetGame.h"
#include "Misc/SecureHash.h"

// 내 클래스 함수

const TCHAR* CCLProfileIdentity::LoginOption()
{
	return TEXT("CCLProfile");
}

FString CCLProfileIdentity::IssueSecret()
{
	// Two GUIDs give 256 random bits from the platform generator.
	return (FGuid::NewGuid().ToString(EGuidFormats::Digits) + FGuid::NewGuid().ToString(EGuidFormats::Digits)).ToLower();
}

bool CCLProfileIdentity::IsValidSecret(const FString& Secret)
{
	if (Secret.Len() != 64)
	{
		return false;
	}

	for (const TCHAR Character : Secret)
	{
		if (!FChar::IsHexDigit(Character) || FChar::IsUpper(Character))
		{
			return false;
		}
	}

	return true;
}

FString CCLProfileIdentity::IdentityKeyFor(const FString& Secret)
{
	const FTCHARToUTF8 Bytes(*Secret);
	FSHAHash Hash;
	FSHA1::HashBuffer(Bytes.Get(), Bytes.Length(), Hash.Hash);
	return TEXT("profile:") + Hash.ToString().ToLower();
}

FString CCLProfileIdentity::ServerAddress(const FURL& Url)
{
	return FString::Printf(TEXT("%s:%d"), *Url.Host.ToLower(), Url.Port);
}

// 부모 인터페이스 함수

FString UCCLLocalPlayer::GetGameLoginOptions() const
{
	const FString Base = Super::GetGameLoginOptions();
	const auto* Instance = Cast<UCCLGameInstance>(GetGameInstance());
	const FWorldContext* Context = Instance ? Instance->GetWorldContext() : nullptr;
	const UPendingNetGame* Pending = Context ? Context->PendingNetGame.Get() : nullptr;
	if (!Pending)
	{
		return Base;
	}

	const FString Secret = Instance->FindProfileSecret(CCLProfileIdentity::ServerAddress(Pending->URL));
	if (Secret.IsEmpty())
	{
		return Base;
	}

	const FString Option = FString::Printf(TEXT("%s=%s"), CCLProfileIdentity::LoginOption(), *Secret);
	return Base.IsEmpty() ? Option : Base + TEXT("?") + Option;
}
