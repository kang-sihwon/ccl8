# 이그니스 주요 도시 시안 제작 기록

세계수가 표시된 대륙 전체 지형에 주요 도시 8곳과 한글 이름을 추가한 이미지다. 도시의 위치와 역할은 [주요 도시 배치안](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-ignis-major-cities.md)이 소유한다. 상위 지도는 [세계지도 시안](../EtherWorldMap.md)이다.

## 파일과 확인 범위

| 항목 | 자료 |
|---|---|
| 입력 | [전체 지형·세계수 시안](Images/ignis-macro-terrain-worldtree-v1.png) |
| 출력 | [도시 표시 시안](Images/ignis-major-cities-v1.png), 1,774×887 PNG |
| 도구 | 기본 제공 `image_gen`. CLI/API fallback 사용 안 함 |
| 저장 | 생성 원본을 변환 없이 별도 파일로 보관 |
| 확인 | 번호 1-8과 이름, 주요 지형에 대한 상대 위치를 화면에서 대조 |
| 미검증 | 지형 픽셀 보존, 정밀 위경도, 시가지의 실제 규모·고도·방재·항행 조건 |

프롬프트의 백분율은 입력 이미지 위에서 도시 표식을 요청한 대략적인 기준점이다. 생성 결과를 측정한 좌표나 세계지도 위경도가 아니다. 도시와 세계수의 표현 크기는 가독성을 위한 지도 기호다. 요청한 위치와 실제 그림 사이에는 차이가 남을 수 있다.

## 사용한 프롬프트

```text
Edit this existing full-continent IGNIS TAIGA terrain map with the golden World Tree. Add a carefully designed set of EIGHT major-city cartographic symbols and their Korean labels. Keep the full continent's exact framing, the giant golden tree, coastline, mountains, plateaus, lakes, rivers and ocean otherwise unchanged. This is an atlas settlement overlay, not a new terrain design. Keep terrain dominant, no close-up, no inset, no large legend panel, no extra cities or icons. The World Tree must remain the most prominent landmark.

Cities must be SMALL, clear pictorial town symbols: compact clusters of 5-10 tiny roofs and one civic landmark, muted stone and red-brown/teal roofs. City symbol width about 2%-3% of full image width, smaller than the World Tree. They are cartographic symbols, not literal city footprints. No giant castles. Labels in legible dark charcoal Korean text with a thin pale cream halo for contrast. Exactly one numbered label per city. Text must be spelled exactly as provided. Put labels next to their symbols without obscuring the tree, rivers, or other labels. Do not add roads or route lines yet. All cities on land, ports at coastal river mouths, mountain towns on valley terraces rather than snowy peaks.

City placement uses x percent from left and y percent from top of the WHOLE IMAGE as approximate anchors. Follow the visible local terrain within each small anchor neighborhood:
1. Label "1 세계수 관리도시". Build a modest crescent of urban roofs near the foot of the EXISTING golden tree at approximately x49%, y57%, outside its trunk and root area. Surrounding lowland is the administrative and maintenance hub, not a fortified cage around the tree. This is the largest urban symbol but still much smaller than the tree. Label slightly to lower left of the tree; do not cover its crown.
2. Label "2 로덴". Upper main-river trade and artisans' city at x64.5%, y47.2%, NORTHEAST of the World Tree, along the prominent river flowing southwest from the northeastern mountains toward the central basin. Place buildings on the low riverbank beside the channel, not over water or on a ridge. Small riverside wharves.
3. Label "3 세르나". Southern estuary port at x57%, y69.5%, on the eastern landward bank at the mouth of the main river where it enters the large SOUTHERN BAY. Put buildings on solid land beside the estuary and tiny wharves facing the bay; do not put the city in the flooded delta channels.
4. Label "4 하르덴". Western plateau valley and water/wind-powered workshop city at x22.5%, y42%, on a broad bench overlooking an incised river valley. No city on a sharp mountain summit.
5. Label "5 서안항". Western coastal trading port at x4.5%, y50.5%, at the western coast near the west-draining river mouth. City core just inland, docks touching the coastline. Put its label to the RIGHT, safely inside the frame.
6. Label "6 동안항". Eastern coastal trading port at x95%, y51%, at the outlet of an eastern valley. City core just inland with small coastal docks. Put its label to the LEFT, safely inside the frame.
7. Label "7 북부 관문도시". A northern gateway and provisions city at x42%, y36.5%, on the SOUTHERN FOOTHILLS of the main snowy mountain range beside a lower valley and pass approach. This is a living supply city below the snowline, NOT the abandoned winter-basin ruins, NOT a city on the ice cap.
8. Label "8 남동 평야도시". A grain-market and agricultural trade city at x73%, y74.5%, on low rolling southern/southeastern plains near a small river, clearly inland from the southern coast, not on the eastern mountain crest.

Do not add Veles village, the old winter-basin ruins, quests, political boundaries, flags, road networks, ships scattered on the sea, or any unrelated landmarks. Only these eight major cities. Maintain the original atlas painting style, north up, wide aspect ratio. Preserve all existing macro terrain and the World Tree position.
```
