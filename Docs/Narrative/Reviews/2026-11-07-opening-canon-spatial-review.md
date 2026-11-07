# 첫 장 정본 반영과 공간 연결 검토

PASS. 읽기 전용 독립 검토에서 지적 없음.

2026-11-07 Codex가 작성한 정본 반영과 공간 후보를 별도 검토 에이전트가 읽기 전용으로 확인했다. BLOCKER는 승인 범위 위반이나 진행을 막는 모순, WARNING은 수정이 필요한 불일치, OBS는 현재 판정을 막지 않는 관찰을 뜻한다. 이번 검토에는 해당 지적이 없다.

## 범위와 근거

| 검토 대상 | 확인 내용 | 판정 |
|---|---|---|
| [선택표](../Drafts/2026-10-31-ether-taiga-dawn/2026-11-06-opening-and-continent/2026-11-07-core-setting-options.md)와 [결정 40](../../decisions/0040-opening-core-settings.md) | 사용자의 K01-K09 채택과 H01-H07 유지 지시를 기록했다. 비교 대안과 기존 원문을 보존했다. | PASS |
| [World](../World.md)·[Scenario](../Scenario.md) | 승인된 세계 법칙, 주인공 동기, 다섯 전투, 바르툴 생존·해방, 무기 선택, 주민 역할, 복구·출항 조건만 반영했다. 린의 최종 이름과 수치를 확정하지 않았다. | PASS |
| [공간 연결 후보](../Drafts/2026-10-31-ether-taiga-dawn/2026-11-06-opening-and-continent/2026-11-07-opening-spatial-connections.md) | 도입·다섯 전투·조작부 접근·사람과 곰의 퇴로·복구·귀환·출항의 인과가 연결된다. 새 시설과 지형은 후보로 구분했다. | PASS |
| [도면 PNG](../../ArtDirection/EtherWorldMap/Images/veles-opening-connections-v1.png)와 [SVG](../../ArtDirection/EtherWorldMap/Images/veles-opening-connections-v1.svg) | PNG를 직접 열어 표기·전투 순서·물길 구분을 확인했다. 40×32m 전투 바닥과 별도 샘 공간은 기존 설명을 구체화하는 새 후보로 표시했다. | PASS |
| [연결 데이터](../../ArtDirection/EtherWorldMap/Data/veles-opening-connections-v1.json)와 [기존 기준](../../ArtDirection/EtherWorldMap/Data/worldtree-settlement-basis-v1.json) | 기존 15개 장소 좌표·도보선·샘물길·본류가 일치한다. 보행선·출항선 길이의 재계산도 일치한다. | PASS |

## 계산 확인

기존 중심선의 보행 거리는 1,464.352m, 출항 공개까지 강선은 1,772.990m다. 수원 전투 기준점과 샘은 평면 36.056m 떨어져 있다. 보행선과 샘물길의 내부 교차점은 `(42.485,186.568)`, `(119.355,621.935)`, `(156.638,904.224)`, `(233.069,1108.614)`m의 네 곳이다. 공유 끝점은 별도 교차점으로 세지 않았다.

이 값은 좌표선의 길이와 교차를 확인한 결과다. 실제 계단과 굽은 통로, 전투 중 이동 거리·시간, 수로 높이와 교량 통과를 검증한 수치는 아니다. 새 도면은 기존 좌표 데이터를 바탕으로 작성했고 기존 래스터 이미지는 수정하지 않았다.

## 미검증과 승인 구분

경사·수문·처리량·곰 이동·전투 성능·원경의 실행 검증은 하지 않았다. 새 후보의 격리 집수부 용량, 운반 설비 형상, 교량 폭과 고도도 미정이다. 검토 PASS는 이 범위의 문서·도면 정합성 판정이며 공간안의 사용자 승인이 아니다. 정본 승인은 결정 40의 K01-K09에만 적용한다.
