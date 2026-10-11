#include "CCLCampaignState.h"

#include "Agents/CCLLifeSimulation.h"
#include "Net/UnrealNetwork.h"

// 생성자

ACCLCampaignState::ACCLCampaignState()
{
	PopulationScenario = TSoftObjectPtr<UCCLPopulationScenario>(FSoftObjectPath(TEXT("/Game/Progression/DA_MerchantLifeScenario.DA_MerchantLifeScenario")));
}

// 부모 인터페이스 함수

void ACCLCampaignState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLCampaignState, Progress);
}

bool ACCLCampaignState::BuildInitialPopulation(FCCLSimulationSnapshot& OutInitial, FString& Error) const
{
	const auto* Scenario = PopulationScenario.LoadSynchronous();
	if (!Scenario)
	{
		Error = TEXT("Campaign population scenario is missing.");
		return false;
	}

	OutInitial = Scenario->InitialState;
	return true;
}

// 내 클래스 함수

void ACCLCampaignState::SetProgress(ECCLCampaignPhase Phase, int32 RemainingGuards)
{
	if (!HasAuthority())
	{
		return;
	}

	Progress.Phase = Phase;
	Progress.RemainingGuards = FMath::Max(0, RemainingGuards);
	ForceNetUpdate();
}

FString ACCLCampaignState::GetObjective() const
{
	switch (Progress.Phase)
	{
	case ECCLCampaignPhase::Village:
		return TEXT("마을 · 동쪽 길을 따라 무너진 성문으로 향하자.");
	case ECCLCampaignPhase::Road:
		return FString::Printf(TEXT("길목 · 성문 경비병을 처치하자. 남은 적: %d명"), Progress.RemainingGuards);
	case ECCLCampaignPhase::Boss:
		return TEXT("우두머리 · 동쪽 안뜰의 성문 수호자를 처치하자.");
	case ECCLCampaignPhase::Victory:
		return TEXT("승리 · 길이 열렸다. 원정을 완료했다.");
	default:
		return TEXT("전투를 시작할 수 없다. 세션을 다시 시작해 줘.");
	}
}
