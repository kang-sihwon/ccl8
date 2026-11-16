# 진행 중인 작업

새 세션은 이 목록에서 이어갈 작업을 고른다. 작업별 지도는 결과·다음 행동을 소유하며, 완료 결과는 소유 사양서에 둔다. 근거는 [결정 28](../decisions/0028-agent-workflow-restructure.md)이다.

| 작업 | 지도 | 상태 |
|---|---|---|
| 벨레스 블록아웃 | [제작·검증](VelesBlockout.md) | 도로 통합·레벨 제작·주요 순환로 PIE 검증과 v5 화면 대조 완료. 숲 군집·동굴 암벽·길과 강변 접속 보완 후 공방–물터–숲문 실리소스 시험 구간을 제안했다. 최종 아트·전투·지하 동굴은 미완료다. |
| 시나리오·세계관 | [현재 상태](../Narrative/CurrentWork.md) | NG01-NG08의 채택 범위는 완료했다. HO01-HO06·공통 경계는 결정 64·정본 반영과 NG04 검토 PASS·잔여 지적 0으로 CLOSED다. [후속 작업군](../Narrative/WorkGroups/next-workgroups.md)의 우선순위·완료 기준을 정리하고 독립 검토 PASS로 마쳤다. 기본 추천은 NG09이며 새 작업은 미착수다. 서사 통합 GX·WC는 미실행이다. |

## 다음 작업 후보

아직 지도가 없는 작업이다. 시작할 때 지도를 만들고 위 표로 옮긴다. 범위와 근거는 소유 문서에 있다.

| 작업 | 소유 문서 | 시작 조건 |
|---|---|---|
| 사용자의 실제 플레이로 조작감·콘텐츠 분량·아트·밸런스 검토 | [StudyRoadmap](../StudyRoadmap.md) | 사용자가 플레이할 시점을 정한다 |
| 전투의 남은 검사(정확한 방어 구간 경계, 동시 피격 등) | [CombatFoundation](../CombatFoundation/Verification.md#미검증) | 없음 |
| 패키지의 미확인 환경(다른 PC·Shipping·외부 인터넷·NAT) | [Packaging](../Packaging/Records.md#세계-모듈-분리-후-패키지) | 검증할 기기와 네트워크가 있어야 한다 |
| 자연 환경 단계 8 마무리와 단계 9 | [EnvironmentPlan](../EnvironmentPlan.md#단계별-현황) | 없음 |
| GitHub Issues와 CI 도입 | [결정 28](../decisions/0028-agent-workflow-restructure.md) | 보류. 여러 세션이 동시에 일을 가져갈 때 다시 검토한다 |
| 전역 설정과 프로젝트 설정의 graft 훅 중복 제거 | [AgentContextTools](../AgentContextTools.md) | 보류. 세션 시작 문구가 두 번 들어오지만 2026-10-31에 고치지 않기로 했다 |
