# 진행 중인 작업

새 세션은 이 목록에서 이어서 할 작업을 고른다. 한 줄에 작업 하나를 적고, 세부와 다음 조각은 작업별 지도가 소유한다. 끝난 작업은 결과를 소유 사양서로 옮긴 뒤 이 목록과 지도에서 지운다. 근거는 [결정 28](../decisions/0028-agent-workflow-restructure.md)이다.

작업을 시작하면 아래 표에 한 줄을 더한다.

| 작업 | 지도 | 상태 |
|---|---|---|
| 이그니스 연속 지역 지도 | [공통 지형 상세 배경](../ArtDirection/EtherWorldMap/IgnisSharedTerrain.md) | 정착지 36곳·상세 타일 150칸을 보존하고 [결정 50](../decisions/0050-shared-coordinate-map-terrain.md)의 공통 좌표 재질·식생 방식으로 전환했다. 길목–케르딘 B04 연결 구간 시제품을 제작해 재질 격자와 외곽 전환을 보정했다. 전체 대륙 아트 품질 검증은 미완료다. 다음 작업은 양 끝 접합과 지역별 지형·식생 조건 보완, 다른 연결로·해안·산악 표본 검증이다. 벨레스 정밀 정합·실제 치수·통행·UE는 미검증, 에테르 전체는 보류한다. |
| 5대륙 메인 여정과 생태 배경 | [서사 작업 현황](../Narrative/CurrentWork.md) | 네 인물의 동기·관계 사건을 CP01-CP12 추천·PA01-PA06 대안·PH01-PH08 보류로 정리했다. CE01-CE12와 여섯 관계를 대조한 독립 검토 PASS이며 선택표는 DISCUSSION이다. 다음은 사용자가 고른 핵심을 결정·Scenario에 반영하는 일이다. 기존 정본·인물 원문·대안·보류를 보존하며 실제 플레이·독자 반응·동료 AI·UE는 미검증이다. |

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
