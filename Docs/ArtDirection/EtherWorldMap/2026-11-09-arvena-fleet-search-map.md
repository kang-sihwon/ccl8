# 아르베나 수색과 G3 연결 도면

[세계지도 안내](../EtherWorldMap.md)에 연결한 동선·생활권 검토 자료다. [수색 후보](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-09-arvena-fleet-search.md)의 항만 배후·낮은 숲·두 난파선·돛갈퀴 대응과 귀환을 그렸다. 현행 지도 화풍·UI 제작에 넘길 공간 근거이며 새로운 인게임 표현 기준은 아니다.

## 전체 연결

![아르베나 수색·구조·항로 확보 전체 연결](Images/arvena-search-overview-v1.png)

[SVG](Images/arvena-search-overview-v1.svg). 기존 도착항 26장소는 좌표를 유지하고 Q 5·W 8·C 15, 총 28장소를 추가했다. 축소도에서 이름을 생략한 도착항 장소는 [이전 확대도](2026-11-09-serna-arvena-passage-map.md)에서 읽는다. 같은 AR-P01 국소 좌표의 확장이며 세계 해안·실제 방위·T2 해거리를 새로 확정하지 않는다. 도면의 해안 직선은 기존 후보 창을 확장한 가정이다.

## 항만 배후와 숲

![항만 창고와 낮은 숲의 생활권·왕복](Images/arvena-search-land-v1.png)

[SVG](Images/arvena-search-land-v1.svg). 원은 생활 중심 반경이고 초록 선은 일부 무리의 활동 예다. 모든 원이 필수 적대 영역은 아니다. Q04-Q05의 다른 귀환, W03-W06-W05의 수액 숲 우회, W05-W06-W07의 운송 사본 회수와 W08 선택 강적을 구분한다. SC1·SM1은 구조체·인간조의 별도 위치다. 역할은 [생활권 표](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-09-arvena-fleet-search/populations-and-leveling.md)가 소유한다.

## 구조·해수 대응·귀환

![난파선 구조와 돛갈퀴 대응 및 육상 귀환](Images/arvena-search-coast-v1.png)

[SVG](Images/arvena-search-coast-v1.svg). 파란선은 배 이동이다. 난파선·대기 수역·회수선 사이를 걸어 다니는 길이 아니다. C04 3명·C05 4명을 C06에서 인수한 뒤 북쪽 안쪽 수로로 A13에 보내고, 7명 인수 확인 뒤 최종 대응을 준비한다. C11 갑판 확대는 60×24m·작업 위치 두 곳의 공간 후보이며 실제 전투 통과 판정이 아니다.

C11-C10-C06 후퇴와 C11-C13-C14 회수는 목적·사용 상태가 다르다. 남쪽 접안 뒤 상단 길로 귀환하거나 북쪽 조사선과 연결한다. G3는 구조·조건부 통행 확인·물자 행선지 대조·선주회 배편과 안내를 모두 갖춘 상태다. T2의 연안 환승·오르다스 외항 연결은 비축척 관계로 남기고 이 국소 도면에 대양 직선을 추가하지 않는다.

## 데이터와 검증

[배치·경로 JSON](Data/arvena-fleet-search-v1.json)과 [계산 검사](Data/arvena-fleet-search-v1-validation.json), [독립 검토](../../Narrative/Reviews/2026-11-09-arvena-fleet-search-review.md)를 함께 읽는다. 원본 도착항과 세계 기준을 보존하고 수량·지역 범위·성장 몫·육상/배 이동·왕복·SVG 좌표를 대조한다. 실제 지형·조석·접안·들것·갑판·전투·AI·보급·항해 시간·UE 실행은 미검증이다.
