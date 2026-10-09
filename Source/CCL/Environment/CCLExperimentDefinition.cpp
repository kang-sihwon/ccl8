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
		TEXT("굴착·성토 후 표면·충돌·AI 경로 갱신, 저장 복구와 참가자 충돌 준비. 물 수지 유지, 눈은 후속 구현"),
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
		Index == 3 ? ECCLExperimentKind::Snow : Index == 4 ? ECCLExperimentKind::Water : Index == 5 ? ECCLExperimentKind::Terrain : Index == 8 ? ECCLExperimentKind::Snapshot : Index == 11 ? ECCLExperimentKind::Shelter : ECCLExperimentKind::Reserved;
	const TCHAR* Help = TEXT("후속 구현 예정. 현재 실행할 수 없는 구역이다.");
	switch (Index)
	{
	case 0: Help = TEXT("구역을 고르고 구역 이동을 누른다. F7로 메뉴를 닫으면 WASD로 걸을 수 있다. 자동 검사는 기능 회귀 확인용이다."); break;
	case 1: Help = TEXT("천체 3D 관측을 연 뒤 드래그로 회전하고 휠로 확대한다. 격리 실험장에서 시간을 정지하고 위도·기울기·자전·공전 위상을 하나씩 바꾼다. 녹색 관측점, 청록 자전축, 노란 적도와 낮/밤 경계를 비교한다."); break;
	case 3: Help = TEXT("구역으로 이동한 뒤 F7을 닫고 눈 위를 걷는다. 처음 지나갈 때와 만들어진 눈길의 이동 저항·발자국 깊이를 비교한다. 메뉴에서 새 적설과 융해를 조작할 수 있다."); break;
	case 4: Help = TEXT("구역으로 이동한 뒤 비·결빙·융해·건조를 선택한다. 변화에는 세계 시간이 필요하다. 시간 60배에서 수면·얼음·젖은 흙과 아래 물 수지를 함께 확인한다."); break;
	case 5: Help = TEXT("구역으로 이동하고 굴착·흙 쌓기·수로 중 도구를 고른다. 메뉴 밖 지형에 커서를 대면 화살표와 범위가 보인다. 왼쪽 클릭으로 적용, 오른쪽 클릭으로 해제한다. 수로는 작은 굴착 범위를 겹쳐 클릭해 잇는다. 보호 영역이나 캐릭터와 겹치는 편집은 거부된다."); break;
	case 8: Help = TEXT("실험 저장 → 천체·문·지형·물·눈 변경 → 저장 불러오기 순으로 비교한다. 저장 시각과 복원 완료 메시지를 확인하고 원하는 구역으로 다시 이동한다. 복원 중에는 지형 충돌을 준비하며 캐릭터는 안전 위치로 옮긴다. 자동 검사는 저장 손상 거부와 복원을 따로 검사한다."); break;
	case 11: Help = TEXT("구역으로 이동하고 문을 닫힘·반 열림·열림으로 바꾼다. 노랑은 빛, 파랑은 비, 청록은 바람 경로다. 밝은 화살표와 통과율이 함께 바뀐다. 현재는 방향별 차폐 시험이며 공기 교환량·실내 온도·연기는 후속 구현이다."); break;
	}
	Instructions = FText::FromString(Help);
}

bool UCCLExperimentDefinition::Validate(FString& Error) const
{
	const ECCLExperimentKind ExpectedKind = Zone == 0 ? ECCLExperimentKind::Guide : Zone == 1 ? ECCLExperimentKind::Celestial :
		Zone == 3 ? ECCLExperimentKind::Snow : Zone == 4 ? ECCLExperimentKind::Water : Zone == 5 ? ECCLExperimentKind::Terrain : Zone == 8 ? ECCLExperimentKind::Snapshot : Zone == 11 ? ECCLExperimentKind::Shelter : ECCLExperimentKind::Reserved;
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
