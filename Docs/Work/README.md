# 진행 중인 작업

새 세션은 이 목록에서 이어서 할 작업을 고른다. 한 줄에 작업 하나를 적고, 세부와 다음 조각은 작업별 지도가 소유한다. 끝난 작업은 결과를 소유 사양서로 옮긴 뒤 이 목록과 지도에서 지운다. 근거는 [결정 28](../decisions/0028-agent-workflow-restructure.md)이다.

작업을 시작하면 아래 표에 한 줄을 더한다.

| 작업 | 지도 | 상태 |
|---|---|---|
| 이그니스 연속 지역 지도 | [연속 지역 지도](../ArtDirection/EtherWorldMap/IgnisConnectedAtlas.md) | 대륙 타일 50장·20,480×10,240px 조립본·뷰어를 제작하고 픽셀 조립과 해안 마스크 경계 85곳을 검사했다. 결정 47로 관리도시 M01 수준의 건물·골목 식별을 상세 도안의 최소 기준으로 정했다. 이 기준을 충족하는 하위 도안과 인접 연결 검증은 미완료다. UE 실행은 미검증. 에테르 전체는 다른 대륙 설정 이후로 보류한다. |
| 5대륙 메인 여정과 생태 배경 | [서사 작업 현황](../Narrative/CurrentWork.md) | [핵심 플롯 선택표](../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-13-world-plot-options.md)에 추천 WP01-WP10·대안 WA01-WA10·보류 WH01-WH08을 정리했다. 종 채택 경계 표현 보정 뒤 독립 검토 PASS, 사용자 채택은 미완료다. 다음은 선택한 핵심만 결정·정본에 반영하고 대표 전투와 인물 선택을 비교하는 일이다. 이전 후보·정본·지도·수치 보존, 실제 제작 범위 확대와 전투·항해·UE 검증은 포함하지 않는다. |

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
