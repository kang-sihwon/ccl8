#include "CCLHUD.h"

#include "CCLCharacter.h"
#include "GameFramework/PlayerController.h"

// 부모 인터페이스 함수

void ACCLHUD::DrawHUD()
{
	Super::DrawHUD();
	DrawText(TEXT("WASD Move  |  Mouse Look  |  Space Jump"), FLinearColor::White, 30.f, 30.f);
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	DrawText(TEXT("K Test Death"), FLinearColor::Yellow, 30.f, 55.f);
#endif
	const ACCLCharacter* Character = PlayerOwner ? Cast<ACCLCharacter>(PlayerOwner->GetPawn()) : nullptr;
	if (!Character || Character->IsDead())
	{
		DrawText(Character ? TEXT("You died. Press R to retry.") : TEXT("Waiting for spawn. Press R to retry."),
			FLinearColor::Yellow, 30.f, 90.f, nullptr, 1.5f);
	}
}
