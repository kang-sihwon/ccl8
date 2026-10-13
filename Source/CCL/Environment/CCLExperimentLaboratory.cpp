#include "CCLExperimentDirector.h"

#include "CCLWorldEnvironmentConfig.h"
#include "CCLExperimentPlayerController.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

bool ACCLExperimentDirector::ConfigureLaboratory(APlayerController* Requester, FGuid ExpectedGeneration, int32 Zone, int32 Subject, int32 Variant, int32 Count, FString& Error)
{
	const FString Map = ACCLWorldEnvironmentConfig::LaboratoryMap(Zone, Subject);
	if (!HasAuthority() || !IsReady() || !CanOperate(Requester) || Generation != ExpectedGeneration || bTravelPending || bTerrainRestorePending || !ActiveCase.IsNone()
		|| Map.IsEmpty() || Variant < 0 || Variant >= ACCLWorldEnvironmentConfig::LaboratoryVariants(Zone, Subject).Num() || Count < 1 || Count > 16)
	{
		Error = TEXT("실험을 중지한 뒤 종류와 수량(1-16)을 확인해 줘.");
		return false;
	}
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!ACCLWorldEnvironmentConfig::IsLaboratory(GetWorld()) && (!Agents || !Agents->SaveSessionForTravel(Error))) { return false; }
	const FString Options = FString::Printf(TEXT("LabVariant=%d?LabCount=%d"), Variant, Count);
	bTravelPending = 1;
	if (GetNetMode() == NM_Standalone)
	{
		UGameplayStatics::OpenLevel(this, FName(*Map), true, Options);
		return true;
	}
	if (GetWorld()->ServerTravel(Map + TEXT("?") + Options)) { return true; }
	bTravelPending = 0;
	if (Agents) { Agents->CancelPreparedTravel(); }
	Error = TEXT("전용 레벨을 열 수 없다.");
	return false;
}

void ACCLExperimentPlayerController::ConfigureLaboratory(int32 Zone, int32 Subject, int32 Variant, int32 Count)
{
	if (const auto* Director = ACCLExperimentDirector::Find(GetWorld())) { ServerConfigureLaboratory(Director->GetGeneration(), Zone, Subject, Variant, Count); }
}

void ACCLExperimentPlayerController::ServerConfigureLaboratory_Implementation(FGuid Generation, int32 Zone, int32 Subject, int32 Variant, int32 Count)
{
	FString Error;
	auto* Director = ACCLExperimentDirector::Find(GetWorld());
	if (!Director || !Director->ConfigureLaboratory(this, Generation, Zone, Subject, Variant, Count, Error)) { ClientExperimentResponse(Error); }
}
