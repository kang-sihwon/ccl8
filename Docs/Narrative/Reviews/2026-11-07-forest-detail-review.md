# 벨레스 연결로·동굴 출구 상세 검토

PASS. 읽기 전용 독립 검토에서 지적 없음.

2026-11-07 Codex가 작성한 [공간 상세 후보](../Drafts/2026-10-31-ether-taiga-dawn/2026-11-06-opening-and-continent/2026-11-07-forest-sightlines-and-clearance.md)를 별도 검토 에이전트가 원본 배치·문서·도면·계산과 대조했다. BLOCKER는 진행을 막는 모순이나 승인 범위 위반, WARNING은 수정이 필요한 불일치, OBS는 판정을 막지 않는 관찰이다. 이번 검토에는 해당 지적이 없다.

## 확인 결과

| 대상 | 독립 확인 내용 | 판정 |
|---|---|---|
| [상세 데이터](../../ArtDirection/EtherWorldMap/Data/veles-forest-detail-v1.json)와 [원본 배치](../../ArtDirection/EtherWorldMap/Data/veles-forest-ecology-v1.json) | 원본 해시와 상세 길 양 끝 기준점이 일치한다. 원본·추격 범위·개체 수 변경이 없다. | PASS |
| 통행·장애물 | 장애물 6개·작업 단 6개와 설계 길폭의 비충돌, 기존 무리 기준점·이동 예시 중심선과 장애물의 비충돌을 독립 계산으로 확인했다. | PASS |
| 시야 | 표본 9개의 거리·가림 결과가 일치한다. 평지와 양쪽 점 높이 1.6m라는 가정, 실제 감지·전신 노출과의 차이를 명시했다. | PASS |
| 거리와 공백 | 상세 길 334.758m·326.213m, 귀환 1,662.206m와 생활권 공백 209.132m·324.217m·433.146m를 재계산해 확인했다. | PASS |
| [연결로 PNG](../../ArtDirection/EtherWorldMap/Images/veles-workshop-link-detail-v1.png)·[출구 PNG](../../ArtDirection/EtherWorldMap/Images/veles-cave-exit-detail-v1.png) | 두 장을 직접 열어 표기·범례·가독성을 확인했다. [연결로 SVG](../../ArtDirection/EtherWorldMap/Images/veles-workshop-link-detail-v1.svg)·[출구 SVG](../../ArtDirection/EtherWorldMap/Images/veles-cave-exit-detail-v1.svg)도 함께 대조했다. | PASS |
| 서사·전투 경계 | 추격 축소는 미적용이며 고정 교전·자동 회복·전투 종료선을 추가하지 않았다. 기존 후보 보존과 실제 실행 미검증을 구분했다. | PASS |

## 미검증

판정은 문서·평면 도면·계산 정합성에 한정한다. 실제 지형 고저·물가·경사·충돌체·무기·회피·카메라·AI 실행은 미검증이다. 이동 예시 중심선이 장애물을 지나지 않는다는 계산은 기체의 실제 몸 폭이 통과함을 뜻하지 않는다. 열린 관찰선은 지정 장애물이 없다는 표본이며 발견 거리나 안전을 보장하지 않는다. 정본 승격이나 H01-H07 해제를 승인한 결과가 아니다.
