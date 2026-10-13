#include "CCLWorldEnvironmentConfig.h"

#include "Net/UnrealNetwork.h"

ACCLWorldEnvironmentConfig::ACCLWorldEnvironmentConfig()
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

void ACCLWorldEnvironmentConfig::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLWorldEnvironmentConfig, LaboratoryZone);
	DOREPLIFETIME(ACCLWorldEnvironmentConfig, LaboratorySubject);
	DOREPLIFETIME(ACCLWorldEnvironmentConfig, LaboratoryVariant);
	DOREPLIFETIME(ACCLWorldEnvironmentConfig, LaboratoryCount);
}

bool ACCLWorldEnvironmentConfig::IsLaboratory(const UWorld* World)
{
	const auto* C = Find(World);
	return C && C->LaboratoryZone >= 0;
}

bool ACCLWorldEnvironmentConfig::AllowsLaboratory(const UWorld* World, int32 Zone, int32 Subject)
{
	const auto* C = Find(World);
	return !C || C->LaboratoryZone < 0 || (C->LaboratoryZone == Zone && (Subject < 0 || C->LaboratorySubject == Subject));
}

FString ACCLWorldEnvironmentConfig::LaboratoryMap(int32 Zone, int32 Subject)
{
	const TCHAR* Names[] = {TEXT("Clock"), TEXT("Celestial"), TEXT("Weather"), TEXT("Snow"), TEXT("Water"), TEXT("Terrain"), TEXT("Ecology"), TEXT("Vegetation"), TEXT("Persistence"), TEXT("Cycle"), TEXT("Fire"), TEXT("Shelter")};
	if (Zone < 0 || Zone > 11 || Subject < 0 || Subject > 2) { return {}; }
	const TCHAR* Name = Zone == 7 && Subject > 0 ? (Subject == 1 ? TEXT("Life") : TEXT("Decay")) : Names[Zone];
	return FString(TEXT("/Game/Maps/Tests/")) + Name;
}

FString ACCLWorldEnvironmentConfig::LaboratoryTitle(int32 Zone, int32 Subject)
{
	const TCHAR* Names[] = {TEXT("시간"), TEXT("천체"), TEXT("날씨"), TEXT("눈"), TEXT("물"), TEXT("지형"), TEXT("생태계·개체군"), TEXT("식생"), TEXT("저장·복원"), TEXT("환경 순환"), TEXT("화재"), TEXT("차폐·환기")};
	return Zone >= 0 && Zone <= 11 ? (Zone == 7 && Subject > 0 ? (Subject == 1 ? TEXT("생애") : TEXT("부패")) : Names[Zone]) : TEXT("종합 실험");
}

TArray<FString> ACCLWorldEnvironmentConfig::LaboratoryVariants(int32 Zone, int32 Subject)
{
	if (Zone == 6) { return {TEXT("지상 생물"), TEXT("물고기"), TEXT("새"), TEXT("곤충")}; }
	if (Zone == 7 && Subject == 0) { return {TEXT("나무"), TEXT("풀"), TEXT("꽃"), TEXT("관목"), TEXT("이끼"), TEXT("양치"), TEXT("조류"), TEXT("수생"), TEXT("버섯")}; }
	if (Zone == 7 && Subject == 1) { return {TEXT("자연 노화"), TEXT("노화 보호"), TEXT("외부 피해")}; }
	if (Zone == 7 && Subject == 2) { return {TEXT("노출 보관"), TEXT("냉각 보관"), TEXT("밀폐 보관")}; }
	return {TEXT("기본 시험")};
}
