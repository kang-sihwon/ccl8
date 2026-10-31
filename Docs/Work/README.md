# 진행 중인 작업

새 세션은 이 목록에서 이어서 할 작업을 고른다. 한 줄에 작업 하나를 적고, 세부와 다음 조각은 작업별 지도가 소유한다. 끝난 작업은 결과를 소유 사양서로 옮긴 뒤 이 목록과 지도에서 지운다. 근거는 [결정 28](../decisions/0028-agent-workflow-restructure.md)이다.

작업을 시작하면 아래 표에 한 줄을 더한다.

| 작업 | 지도 | 상태 |
|---|---|---|
| 문서 정리와 map+tree 구조 | [docs-map-tree](docs-map-tree.md) | 문서 정리 끝. 마지막 질문의 답을 기다림 |

## 다음 작업 후보

아직 지도가 없는 작업이다. 시작할 때 지도를 만들고 위 표로 옮긴다. 범위와 근거는 소유 문서에 있다.

| 작업 | 소유 문서 | 시작 조건 |
|---|---|---|
| 사용자의 실제 플레이로 조작감·콘텐츠 분량·아트·밸런스 검토 | [StudyRoadmap](../StudyRoadmap.md) | 사용자가 플레이할 시점을 정한다 |
| 전투의 남은 검사(정확한 방어 구간 경계, 동시 피격 등) | [CombatFoundation](../CombatFoundation.md#미검증) | 없음 |
| 패키지의 미확인 환경(다른 PC·Shipping·외부 인터넷·NAT) | [Packaging](../Packaging/Records.md#세계-모듈-분리-후-패키지) | 검증할 기기와 네트워크가 있어야 한다 |
| 자연 환경 단계 8 마무리와 단계 9 | [EnvironmentPlan](../EnvironmentPlan.md#단계별-현황) | 없음 |
| `Docs/Learning/` 시스템별 재작성과 QA 재검증 | [결정 28](../decisions/0028-agent-workflow-restructure.md), [학습 문서의 깨진 링크](../Learning/README.md#알려진-깨진-링크) | 없음. 선행 작업인 C++ 규칙 위반 정리는 2026-10-28에 끝났다([Coding](../Coding/CppLayout.md#기계-검사-규칙)) |
| GitHub Issues와 CI 도입 | [결정 28](../decisions/0028-agent-workflow-restructure.md) | 보류. 여러 세션이 동시에 일을 가져갈 때 다시 검토한다 |
