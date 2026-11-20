# 진행 중인 작업

새 세션은 이 목록에서 이어갈 작업을 고른다. 작업별 지도는 결과·다음 행동을 소유하며, 완료 결과는 소유 사양서에 둔다. 근거는 [결정 28](../decisions/0028-agent-workflow-restructure.md)이다.

| 작업 | 지도 | 상태 |
|---|---|---|
| 아이템 축 재설계 | [검토와 다음 결정](ItemRedesign.md) | 결정 68-72 기록. 구현 단계 1(편집 객체)과 단계 2(Fragment 계층·정의 재생성) 완료, 빌드·자동화 171/172·스모크 PASS. 다음은 단계 3(장비 컴포넌트·출처 등록부·무브셋·손 입력)이다. |
| 벨레스 건축 키트 | [조사 결과와 다음 시험](VelesBuildingKits.md) | 16개 상세·이미지 비교 후 결정 67로 9개 제외, 검토 대상 7개 유지. K01 우선 시험·K02 대안 제안. 임포트와 최종 채택은 미실행이다. |
| Megascans 상세·이미지 선별 | [우선 후보 전수 검토](MegascansReview.md) | 조사 종료. 2,386개 판정과 사용자 승인 미확인 예외 1개, 공방·나루·숲 우선 배치 30개를 작업 지도에서 확인한다. |
| 벨레스 블록아웃 | [제작·검증](VelesBlockout.md) | 공방–물터–숲문 285m 실리소스 시험 구간의 제작·왕복 PIE·카메라·엔진 CSV 검증 완료. 지면·수면·조명·원경 품질, 전투·NavMesh와 패키지 성능 검증이 다음이다. 원본 지형 충돌의 한계와 시험 맵의 수정을 기록했다. |
| 시나리오·세계관 | [현재 상태](../Narrative/CurrentWork.md) | NG01-NG08의 채택 범위는 완료했다. HO01-HO06·공통 경계는 결정 64·정본 반영과 NG04 검토 PASS·잔여 지적 0으로 CLOSED다. [후속 작업군](../Narrative/WorkGroups/next-workgroups.md)의 우선순위·완료 기준을 정리하고 독립 검토 PASS로 마쳤다. NG09 상호작용·작업 기록 상세는 작성·검토 PASS·잔여 지적 0이며 IR01-IR06·공통 경계는 결정 65·정본 반영과 NG04 검토 PASS·잔여 지적 0으로 CLOSED다. NG10-NG12는 미착수다. 서사 통합 GX·WC는 미실행이다. |

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
