# 세계수 규모와 정착지 설계도

세계수의 실제 치수, 중앙 분지의 새 지역 좌표, 벨레스 첫 장의 마을·숲·동굴·출항 배치를 계산 가능한 도면으로 작성했다. 서사 내용과 수치는 [세계수 규모](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-worldtree-scale.md), [정착망](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-ignis-settlement-network.md), [벨레스 지역](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-veles-region-layout.md)이 소유한다. 상위 지도는 [세계지도 시안](../EtherWorldMap.md)이다.

## 도면과 데이터

| 자료 | 읽는 때 |
|---|---|
| [벨레스 숲 탐색·분포 지도](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-06-opening-and-continent/2026-11-07-veles-forest-population.md) | 현재 필드 후보의 6구역·갈림길·11개 무리와 순찰·평소 활동과 추격 범위를 볼 때 |
| [첫 장 공간 연결 후보](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-06-opening-and-continent/2026-11-07-opening-spatial-connections.md) | 이전 순차 전투 도면의 장소·물길·동굴·출항 좌표를 참조할 때. 현재 필드 교전 구조는 [개정 후보](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-06-opening-and-continent/2026-11-07-field-combat-revision.md)를 따른다. |
| [중앙 생활권 좌표 기준도](2026-11-07-central-coordinate-map.md) | 중앙 9곳의 현재 이름·기호·좌표와 벨레스 이웃 마을의 확대 배치를 볼 때 |
| [세계수 치수 PNG](Images/worldtree-scale-and-influence-v1.png), [SVG](Images/worldtree-scale-and-influence-v1.svg) | 나무의 실제 높이·너비와 생활권 이격을 비교할 때 |
| [중앙 분지 PNG](Images/ignis-central-settlement-layout-v1.png), [SVG](Images/ignis-central-settlement-layout-v1.svg) | 세계수·관리도시·로덴·벨레스와 중앙의 일반 도시·마을 위치를 볼 때 |
| [첫 장 지역 PNG](Images/veles-first-chapter-layout-v2.png), [SVG](Images/veles-first-chapter-layout-v2.svg) | 마을·숲·동굴·출항 가림 지형의 동선을 볼 때 |
| [입력·계산 결과 JSON](Data/worldtree-settlement-basis-v1.json) | 지역 km와 마을 m 좌표, 제안 강선·보행선, 구면 가시선 가정을 확인할 때 |

설계 데이터를 Python으로 계산하고 Pillow와 SVG로 새 도면을 그렸다. 기존 생성 PNG를 덧칠하거나 세계지도 해안을 편집한 결과가 아니며 이미지 생성 프롬프트는 없다. SVG와 PNG는 같은 좌표와 글자로 작성했다. SVG의 글꼴은 보는 환경에 따라 달라질 수 있어 PNG를 함께 보관한다.

세계수 도면은 1,800×1,100, 중앙 분지와 첫 장 도면은 각각 1,800×1,250이다. 세계수 그림의 수관은 치수 확인용 외곽 도형이며 새 아트 실루엣을 확정하지 않는다. 중앙 지역도에는 그 지역의 T01-T03과 V01-V04만 표시했고, 다른 정치권의 정착지는 정착망 문서의 상대 위치로 정의했다.

## 계산 범위

행성 반지름은 둘레 40,000km에서 구했다. 지역 원점은 세계수이고 방위정거도법으로 원점과 각 점 사이의 거리를 보존한다. 다른 점 사이의 길이는 구면에 놓은 좌표로 계산한다. 지역 강선은 설계 절점을 이은 길이이며 관측된 실제 하천 길이가 아니다.

첫 장의 보행선·강선은 평면 m 좌표의 선분 길이를 합했다. 나루·출항 후 지점에서 세계수의 고도각을 계산하고 수관 너비 방향 5개 표본이 가상의 가까운 능선과 만나는지 검사했다. 이 시야 검사는 해당 능선의 가림 여부만 다루며, 중간 실제 지형·대기 굴절·안개·UE 원경을 포함하지 않는다.

새 자료의 거리·시야는 앞선 보류 좌표나 생성 이미지의 픽셀에서 가져오지 않았다. 세계지도 v3의 해안·면적은 그대로 유지한다. 대륙 위경도와 실제 수계에 이 지역도를 정합한 지도 제작은 후속 작업이다.
