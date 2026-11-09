# 북부 귀환과 중앙 탐색 도면

[S6 상세 후보](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-09-central-exploration.md)의 귀환·공급 준비·뿌리권 생활권·안쪽 설비·다른 귀환길을 네 좌표 도면으로 작성했다. 기준은 [원본 대륙 좌표](Data/ignis-continent-georeference-v1.json), [세계수 규모](Data/worldtree-settlement-basis-v1.json), [북부 접근](Data/north-exploration-v1.json)이다. 36정착지·하천·해안·세계수 위치와 기존 후보를 이동하거나 대체하지 않는다.

이 도면은 공간과 동선을 읽는 설계 자료다. [지도 화풍·UI 방향](../EtherWorldMap.md#지도-용도와-ui-방향)을 대체하는 새 아트 시안이 아니다. 연녹색 지형 영역은 후보 뿌리·식생 윗면이며 측량 등고선이 아니다. 위치는 같은 방위정거도법 투영의 km·m로 연결하고 북쪽은 투영 격자 북쪽이다.

## 귀환과 중앙 연결

![북부 귀환과 중앙 연결](Images/central-corridor-v1.png)

왼쪽은 북부 접근 회랑·B04·B03·R3 역행, 오른쪽은 관리도시와 세계수의 다른 축척이다. 중앙의 원은 노출 뿌리 6km·수관 9km·일반 상주 제한 12km·전환 범위 20km의 기존 설계값이다. 색 박스는 아래 확대도 범위다. 새 도시-뿌리권 10.22km·뿌리권-안쪽 4.94km 회랑은 후보이며 시간·경사·중간 생태는 미검증이다.

## 도시의 준비

![관리도시 준비](Images/central-city-v1.png)

C01-C07은 주거·비축·기록·현장 인계를 다룬다. 원점 M01은 (18, -12)km이며 전체 도시 확대 시설은 반경 20km 밖에 둔다. C06의 제한 작업 허가와 기존 공공 통과를 구분한다.

## 바깥 뿌리권

![바깥 뿌리권 생활권](Images/central-outer-v1.png)

O01-O10은 기준점 (8, -5)km에 붙인다. 갈색 원은 집·활동 중심, 보라색 선은 같은 개체의 먹이 이동 중심선이다. 생활권 범위를 실제 적의 현재 위치로 읽지 않는다. OC1 구조체는 생물 생활권과 별도이며 일반 동물은 전투 수량에 넣지 않는다.

## 안쪽 설비

![안쪽 설비와 최종 접근](Images/central-inner-v1.png)

F01-F10은 기준점 (2, -1)km에 붙인다. F06-F07은 준비·지휘 확보, F07-F08은 방어 격리, F08-F09는 정지 확인 후 안쪽 수동 해제로 열린다. 주황 원은 이 세 통제 연결이다. F08-F09를 밖에서 거꾸로 통과해 방어체를 생략할 수 없고, 최종 전투 도중 F06 방향 철수는 유지한다.

## 자료와 검증 범위

[좌표·경로·상태 JSON](Data/central-exploration-v1.json), [계산 검사](Data/central-exploration-v1-validation.json), [독립 검토](../../Narrative/Reviews/2026-11-09-central-exploration-review.md)를 함께 읽는다. SVG 원본은 [귀환](Images/central-corridor-v1.svg)·[도시](Images/central-city-v1.svg)·[뿌리권](Images/central-outer-v1.svg)·[안쪽](Images/central-inner-v1.svg)이다.

27장소·13생활 단위·34개체, 별도 구조체 5기·인간조 5명은 국소 후보 수량이다. 거리·범위·연결 검사는 실제 지형·보급·공급 용량·무기·AI·전투·UE 실행 검증이 아니다. 이후 좌표를 바꾸면 경로 길이·문 상태·생활권 교차와 확대도 연결을 함께 다시 계산한다.
