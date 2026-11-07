# 벨레스 숲 탐색·몬스터 분포 검토

PASS. 읽기 전용 독립 검토에서 지적 없음.

2026-11-07 작성자의 숲 배치 후보를 별도 검토 에이전트가 원자료·문서·도면과 대조했다. BLOCKER는 승인 범위 위반이나 진행을 막는 모순, WARNING은 수정이 필요한 불일치, OBS는 판정을 막지 않는 관찰이다. 이번 검토에는 해당 지적이 없다.

## 확인 결과

| 대상 | 확인 내용 | 판정 |
|---|---|---|
| [숲 배치 문서](../Drafts/2026-10-31-ether-taiga-dawn/2026-11-06-opening-and-continent/2026-11-07-veles-forest-population.md) | 6구역·11개 무리와 순찰·야외 25개체를 후보로 구분했다. 고정 전투 순서·전멸 조건·자동 재등장을 추가하지 않았고 바르툴 생존·복구·출항 조건을 유지했다. | PASS |
| [탐색 PNG](../../ArtDirection/EtherWorldMap/Images/veles-forest-exploration-v1.png)·[분포 PNG](../../ArtDirection/EtherWorldMap/Images/veles-forest-population-v1.png) | 둘 다 직접 열어 구역·길·무리·평소 활동·추격 한계의 구분과 글자 잘림 여부를 확인했다. | PASS |
| [분포 데이터](../../ArtDirection/EtherWorldMap/Data/veles-forest-ecology-v1.json)·[기존 기준](../../ArtDirection/EtherWorldMap/Data/worldtree-settlement-basis-v1.json) | 기존 15개 장소와 보행선·샘물길·본류가 보존됐다. S 10마리·G 5기·W 10마리의 합계가 일치하며 실내·보스·비적대 예시는 제외했다. | PASS |
| 길과 수로 | 독립 계산에서 18노드·23연결·6개 독립 순환, 경로별 길이, B01-B04를 통한 수로 횡단이 일치했다. | PASS |
| 활동·추격 범위 | 기준점·이동 예시가 평소 활동 범위에 있고, 평소 범위가 추격 한계에 포함된다. 무리의 물길 비횡단과 X1-X3의 중첩 조건을 확인했다. | PASS |
| SVG 좌표 | 분포도의 11개 무리 표식과 지도에 완전히 표시된 여섯 연결 경로가 데이터와 일치했다. R01의 마을 쪽 일부는 표시 범위 밖이며 원자료에 보존했다. | PASS |

작성자가 실행한 [계산 검사](../../ArtDirection/EtherWorldMap/Data/veles-forest-validation-v1.json)도 PASS다. PNG와 SVG는 같은 좌표로 새로 작성했으며 이전 지도 이미지를 수정하지 않았다.

## 미검증과 승인 구분

문서·평면 데이터·도면 정합성을 검토했다. 실제 강안 여유·통행 폭·경사·수로 높이·전투 밀도·감지와 추격 AI·개체 합류·재등장·UE 실행은 검증하지 않았다. 구역 경계·개체 수·이동 범위는 후보이며 H02·H04의 보류를 해제하지 않는다. 검토 PASS가 사용자 정본 승인을 뜻하지 않는다.
