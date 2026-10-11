#include "CCLTestBridgeComponent.h"

#include "CCLCampaignSmokeSubsystem.h"
#include "CCLCombatSmokeSubsystem.h"
#include "CCLPlayerController.h"
#include "CCLPlayerState.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Engine/World.h"

// 생성자

UCCLTestBridgeComponent::UCCLTestBridgeComponent()
{
	SetIsReplicatedByDefault(true);
}

// 내 클래스 함수

UCCLTestBridgeComponent* UCCLTestBridgeComponent::Ensure(APlayerController* Controller)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!IsValid(Controller) || !Controller->HasAuthority())
	{
		return nullptr;
	}

	if (auto* Existing = Find(Controller))
	{
		return Existing;
	}

	auto* Bridge = NewObject<UCCLTestBridgeComponent>(Controller, TEXT("CCLTestBridge"));
	Bridge->RegisterComponent();
	Bridge->SetIsReplicated(true);
	return Bridge;
#else
	return nullptr;
#endif
}

UCCLTestBridgeComponent* UCCLTestBridgeComponent::Find(const APlayerController* Controller)
{
	return Controller ? Controller->FindComponentByClass<UCCLTestBridgeComponent>() : nullptr;
}

void UCCLTestBridgeComponent::GrantEquipmentSet(APlayerController* Controller)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	auto* State = Controller && Controller->HasAuthority() ? Controller->GetPlayerState<ACCLPlayerState>() : nullptr;
	if (!State)
	{
		return;
	}

	for (const TCHAR* Name : {TEXT("DA_TrainingSword"), TEXT("DA_TrainingShield"), TEXT("DA_TrainingStaff"), TEXT("DA_TrainingArmor"),
	                          TEXT("DA_TrainingBoots"), TEXT("DA_TrainingCloak"), TEXT("DA_TrainingNecklace"), TEXT("DA_TrainingRing")})
	{
		auto* Definition = LoadObject<UCCLItemDefinition>(nullptr, *FString::Printf(TEXT("/Game/Progression/%s.%s"), Name, Name));
		State->GetInventory()->Add(Definition, 1);
	}
#endif
}

void UCCLTestBridgeComponent::ServerCombatTestReady_Implementation()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (auto* Test = GetWorld()->GetSubsystem<UCCLCombatSmokeSubsystem>())
	{
		Test->RegisterDriver(Cast<ACCLPlayerController>(GetOwner()));
	}
#endif
}

void UCCLTestBridgeComponent::ServerCampaignTestReady_Implementation()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (auto* Test = GetWorld()->GetSubsystem<UCCLCampaignSmokeSubsystem>())
	{
		Test->RegisterDriver(Cast<ACCLPlayerController>(GetOwner()));
	}
#endif
}

void UCCLTestBridgeComponent::ClientCombatTestStep_Implementation(int32 Step)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (auto* Test = GetWorld()->GetSubsystem<UCCLCombatSmokeSubsystem>())
	{
		Test->ExecuteClientStep(Step);
	}
#endif
}

void UCCLTestBridgeComponent::ClientCampaignTestStep_Implementation(int32 Step)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (auto* Test = GetWorld()->GetSubsystem<UCCLCampaignSmokeSubsystem>())
	{
		Test->ExecuteClientStep(Step);
	}
#endif
}

void UCCLTestBridgeComponent::ClientProgressionTestStep_Implementation(int32 Step, FGuid EntryId)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (auto* Test = GetWorld()->GetSubsystem<UCCLCampaignSmokeSubsystem>())
	{
		Test->ExecuteProgressionStep(Step, EntryId);
	}
#endif
}

void UCCLTestBridgeComponent::ClientContentTestStep_Implementation(int32 Step)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	auto* Controller = Cast<ACCLPlayerController>(GetOwner());
	if (Controller && GetWorld()->GetSubsystem<UCCLCampaignSmokeSubsystem>() && FParse::Param(FCommandLine::Get(), TEXT("CCLContentSmoke")))
	{
		if (Step == 0)
		{
			Controller->ServerTalkToSteward();
		}
		else
		{
			Controller->ServerBuyPotion();
		}
	}
#endif
}
