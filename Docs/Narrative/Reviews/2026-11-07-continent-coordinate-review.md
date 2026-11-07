# 이그니스 대륙 좌표 정합 검토

판정은 `PASS`다. 작성자 Codex와 별도의 읽기 전용 검토자가 [36곳 좌표 정합 후보](../../ArtDirection/EtherWorldMap/2026-11-07-continent-coordinate-map.md), 기준 좌표와 기존 지리·현재 상황을 대조하고 최종 PNG 두 장을 직접 확인했다. 수정 요청은 없다. 판정은 서사 배경과 좌표·표현의 정합성에 한정한다.

이 보고서에서 BLOCKER는 요청 범위나 기준 좌표를 성립시키지 못하는 결함, WARNING은 문서·데이터·표현의 수정 가능한 불일치, OBS는 개선 제안이나 검증 한계다.

## 대조한 기준

| 기준 자료 | 확인한 관계 |
|---|---|
| [세계지도 v3 해안 JSON](../../ArtDirection/EtherWorldMap/Data/ether-world-map-land35-precise-v3.json) | 이그니스 해안 6,156절점과 투영 변환, 원본 외형 보존 |
| [중앙 지역 기준 JSON](../../ArtDirection/EtherWorldMap/Data/worldtree-settlement-basis-v1.json) | 중앙 9곳·환적장·기존 강선 17절점의 보존 |
| [통합 지리 후보](../Drafts/2026-10-31-ether-taiga-dawn/2026-11-06-opening-and-continent/geography.md) | 외곽 27곳의 상대 지형, 하르덴 동향 유역과 서향 고개, 북부 거주 계곡과 계절 지선, 항구·평야 관계 |
| [현재 상황 후보](../Drafts/2026-10-31-ether-taiga-dawn/2026-11-06-opening-and-continent/present-world.md) | 기존 소속·협약·생활권, 인간·몬스터 갈등과 통로의 연결, 확정 국경과의 구분 |
| [새 좌표 JSON](../../ArtDirection/EtherWorldMap/Data/ignis-continent-georeference-v1.json) · [계산 검증](../../ArtDirection/EtherWorldMap/Data/ignis-continent-georeference-v1-validation.json) | 지명·종류·표식, 14개 수계와 24개 노선, 실제 SVG 좌표와 최종 파일 해시 |

## 결과

| 항목 | 판정과 근거 |
|---|---|
| 정착지 구성 | PASS. 고유 정착지 36곳의 대도시 8·도시 12·마을 16 분류와 지명을 확인했다. 여름뜰은 계절 집결지다. |
| 중앙 보존 | PASS. 중앙 9곳·환적장·기존 강선 17절점이 선행 자료와 같다. 전역 원점 추가가 지역 좌표를 이동시키지 않았다. |
| 해안·좌표 표현 | PASS. SVG 좌표를 독립 계산으로 대조했다. 정착지 최대 오차 약 2.26×10⁻⁷km, 해안 약 2.37×10⁻⁷km로 작성자의 기록과 일치한다. |
| 외곽 지리·노선 | PASS. 기존 상대 지리와 수계·육해로 구분이 맞는다. 에스칼은 B15로 R7에 합류하는 지선 관계를 유지한다. |
| 지도 가독성 | PASS. [전체도](../../ArtDirection/EtherWorldMap/Images/ignis-continent-coordinate-map-v1.png)와 [세계지도 연결도](../../ArtDirection/EtherWorldMap/Images/ignis-world-registration-v1.png)를 직접 열었다. 메르사 표식의 이름 배경 가림이 해소된 최종본을 확인했다. |
| 후보·확정 구분 | PASS. 새 원점과 외곽 좌표가 설계 후보이며 색이 확정 국경이 아니라는 설명이 있다. 기존 후보 보존과 정본 승인 경계를 유지한다. |
| 검증 주장 | PASS. 투영 왜곡과 원점 거리의 성질, 고도 모델·수심·통행 미검증을 명시한다. 최종 파일 해시가 기록과 일치한다. |

남은 지적은 없다.

## 판정의 한계

수계·분수계·통행 선형은 설계된 주요 회랑이다. 독립 검토는 실제 지형 고도, 수문·항행·물류·기후 시뮬레이션, 교량·고개 경사, 첫 장의 카메라·보행·전투 실행을 검증하지 않았다. SVG의 내부 좌표 오차가 작다는 사실은 제안한 도시 위치의 측량 정확도를 뜻하지 않는다.

신규 세계수 전역 원점과 외곽 27곳의 위치, 하천 고도와 지형 선형은 사용자 정본 승인 전이다. 검토 PASS를 고유 설정 채택이나 제작 범위 확대로 취급하지 않는다.
