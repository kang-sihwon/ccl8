#include "CCLExperimentDefinition.h"

void UCCLExperimentDefinition::ConfigureZone(int32 Index)
{
	static const TCHAR* Titles[] = {
		TEXT("안내·설정"), TEXT("천체·공통 시간"), TEXT("자연 날씨"), TEXT("깊은 눈·눈길"),
		TEXT("물·진흙·얼음"), TEXT("영구 지형"), TEXT("생태계·개체군"), TEXT("생애·식생·부패"),
		TEXT("상태 저장·복원"), TEXT("환경 연계 순환"), TEXT("불·열"), TEXT("차폐·환기")};
	static const TCHAR* ExpectedValues[] = {
		TEXT("12개 구역·공통 시계·서버 조작 권한 준비 확인"),
		TEXT("같은 Seed·시각에서 같은 일조. 격리 맵은 공통 시간의 실패·재시도도 검증"),
		TEXT("일조와 계절에 따라 기온·강수·바람 변화"),
		TEXT("무릎 깊이 눈을 통과한 연속 눈길과 질량 보존"),
		TEXT("낮은 곳으로 유출, 젖음·진흙·결빙과 물 총량 보존"),
		TEXT("굴착·성토 후 충돌·물·눈·AI 경로 갱신"),
		TEXT("근거리 개체와 먼 개체군 사이 전환 시 총수 보존"),
		TEXT("공통 시간에 따른 성장·휴면·부패와 핵심 NPC 보호"),
		TEXT("손상 저장 거부, 세계 ID·시계·Agent 상태 동시 복원"),
		TEXT("눈길·융해·유출·진흙·결빙·건조의 반복"),
		TEXT("연료·습도·바람에 따른 연소와 비·눈에 의한 소화"),
		TEXT("지붕·벽·문에 따른 일조·강수·바람 차폐와 위아래 표면. 열·연기는 7단계 예정")};
	if (Index < 0 || Index >= UE_ARRAY_COUNT(Titles))
	{
		return;
	}

	Zone = Index;
	CaseId = FName(*FString::Printf(TEXT("Zone_%02d"), Index));
	Title = FText::FromString(FString::Printf(TEXT("%02d  %s"), Index, Titles[Index]));
	Expected = FText::FromString(ExpectedValues[Index]);
	Kind = Index == 0 ? ECCLExperimentKind::Guide : Index == 1 ? ECCLExperimentKind::Celestial :
		Index == 8 ? ECCLExperimentKind::Snapshot : Index == 11 ? ECCLExperimentKind::Shelter : ECCLExperimentKind::Reserved;
	Instructions = FText::FromString(Index == 1 ?
		TEXT("같은 시각의 천체 재현을 시험한다. 격리 맵에서는 시간 실패·재시도와 위도·기울기·위상 조작도 가능하다.") :
		IsImplemented() ? TEXT("구역 선택 후 시작. 초기화 후 재실행할 수 있다.") :
		TEXT("후속 구현 예정. 현재 실행할 수 없는 구역이다."));
}

bool UCCLExperimentDefinition::Validate(FString& Error) const
{
	const ECCLExperimentKind ExpectedKind = Zone == 0 ? ECCLExperimentKind::Guide : Zone == 1 ? ECCLExperimentKind::Celestial :
		Zone == 8 ? ECCLExperimentKind::Snapshot : Zone == 11 ? ECCLExperimentKind::Shelter : ECCLExperimentKind::Reserved;
	if (Zone < 0 || Zone > 11 || CaseId.IsNone() || Title.IsEmpty() || Instructions.IsEmpty() || Expected.IsEmpty() || Kind != ExpectedKind)
	{
		Error = TEXT("Invalid experiment definition or unsupported implementation claim.");
		return false;
	}

	return true;
}

FVector UCCLExperimentDefinition::ZoneCenter(int32 Index)
{
	return Index >= 0 && Index < 12 ? FVector((Index % 4) * 2500., (Index / 4) * 2500., 0) : FVector::ZeroVector;
}

FString UCCLExperimentDefinition::StatusText(ECCLExperimentStatus Status)
{
	switch (Status)
	{
	case ECCLExperimentStatus::NotImplemented: return TEXT("미구현");
	case ECCLExperimentStatus::Ready: return TEXT("준비");
	case ECCLExperimentStatus::Running: return TEXT("실행 중");
	case ECCLExperimentStatus::Passed: return TEXT("통과");
	case ECCLExperimentStatus::Failed: return TEXT("실패");
	default: return TEXT("화면 검토 필요");
	}
}
