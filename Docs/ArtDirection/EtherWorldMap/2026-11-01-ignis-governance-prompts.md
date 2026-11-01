# 이그니스 통치·생산·교역 지도 제작 기록

기존 주요 도시 지도에 통치권·협약 주변권, 생산·가공·비축 표식과 교역선을 더했다. 공간 배치와 범례의 의미는 [통치·생산·교역 배치](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-ignis-governance-layout.md)가 소유한다. 상위 문서는 [세계지도 시안](../EtherWorldMap.md)이다.

## 파일과 제작 방식

| 항목 | 자료 |
|---|---|
| 입력 | [주요 도시 8곳 지도](Images/ignis-major-cities-v1.png) |
| 출력 | [통치·생산·교역 PNG](Images/ignis-governance-production-trade-v2.png), 1,536×1,024 |
| 도구 | 기본 제공 `image_gen`. 최초 편집 후 누락 경로 두 곳과 중앙 농경지의 P2 표식을 같은 도구로 보정 |
| 저장 | 선택한 생성 원본을 변환 없이 프로젝트에 복사. 원본과 복사본의 SHA-256 일치 확인 |
| 기준 | 기존 지형과 도시의 상대 배치. 정밀 면적·위경도 기준은 세계지도 v3를 유지 |

[이전 v1](Images/ignis-governance-production-trade-v1.png)은 중앙 농경지 표식이 P3로 중복 인쇄된 비교용 이미지다. 현재 배치에는 P2로 보정한 v2를 사용한다.

## 화면 대조

정치권 4곳, 도시 1-8, P1-P8 표식과 범례를 v2에서 확인했다. 첫 생성물에서 끊긴 동부 연안 항로와 빠진 관리도시·환적장 접속 육로를 보정했고, 선택한 이미지에서 연결을 확인했다. 독립 검토가 v1의 중앙 농경지 P3 중복 표기를 발견해 v2에서는 P2로 고쳤다. 북부는 관문도시를 중심으로 사선이 많은 협약권으로 표현됐다. 대륙 북쪽·남서부 등에 관할을 정하지 않은 무색 지역이 남는다.

생성 편집은 해안과 산맥·수계의 픽셀 단위 보존을 보장하지 않는다. 범위의 정확한 면적, 지형상 실제 통행, 항행 조건과 물류량은 검사하지 않았다. 표시한 선은 공간 관계 후보이며 측량 결과나 정식 국경선이 아니다.

## 최초 편집 프롬프트

백분율은 입력 이미지 위의 편집 지시이며 생성 결과를 측정한 좌표가 아니다.

```text
Use case: precise-object-edit / infographic-diagram.
Edit the supplied IGNIS map into a beautiful, legible political-and-economic planning atlas in Korean. The input is the edit target, NOT loose inspiration. Preserve the full existing land silhouette, south bay, northern snowy mountains, western plateaus, eastern ridges, river/lake network, golden World Tree and ALL EIGHT existing numbered city symbols at the same geographic positions. No new cities or changed coastline. Keep the painted terrain clearly visible beneath a restrained cartographic overlay.

LAYOUT: one large high-resolution landscape atlas sheet, approximately 3:2 overall. The existing 2:1 map occupies the upper three quarters at its original aspect, fully uncropped. Add a clean warm-ivory legend band BELOW the map rather than covering terrain or shrinking the continent into an inset. Aim for very sharp readable Korean text. Small heading above map "이그니스 | 통치 · 생산 · 교역". Subtitle "생활권 중심 배치안 · 국경과 운송 조건은 미확정". All positions below refer to the original map rectangle, NOT the added legend.

POLITICAL OVERLAY: FOUR distinct colors on LAND ONLY, mostly transparent. Irregular contours follow valleys, foothills and settlement hinterlands, not straight quadrants. Moderately stronger translucent tint inside thin SOLID contours = core local administration. Sparse outward HATCHING with a dotted outer edge = negotiated/autonomous peripheral cooperation, not fully governed land. Leave northern tundra, most snowy mountain massifs, remote southwestern forests and intervening highlands uncolored: jurisdiction undesigned, NOT uninhabited. Do NOT color the entire continent into four countries. No national sea ownership, no tint over water. The Ether League has no territorial polygon.

A. TEAL "테른 도시연합": a connected but irregular lowland corridor with lobes around cities 1 World Tree administrative city, 2 Roden and 3 Serna, centered on the central basin and main river. World Tree stays visible, river passes EAST of the tree. Core encloses city 1 around x46-54 y54-64, city 2 x60-68 y43-51, and city 3 x51-60 y64-73 and adjacent habitable land, connected through lower river valley, not mountains or sea. Outer sparse hatched fringe on nearby basin farmland. Name in open basin southwest of city 1, avoiding its label.
B. OCHRE "서부 계곡동맹": valley/bench patches and connecting low passes between city 4 Harden at x22 y43 and city 5 western port at x5 y53. Include their immediate farming and workshop hinterlands, but do not annex all western highland peaks or southwestern peninsula. Core jurisdiction exercised by member communities, not a single king. Peripheral hatching at outer plateau valleys. Name on western plateau open space.
C. VIOLET "동부 평야왕국": core mainly southeastern low rolling farmland around city 8 x73 y75 and connecting lower valley corridor to eastern port city 6 x95 y52. Keep far eastern mountains mostly outside core, allow hatched negotiated eastern foothill communities. City 6 gets a thin double ring indicating chartered port autonomy WITHIN the kingdom, not an independent state. Core around city 8 does not absorb Serna. Name in southeastern lowland away from city labels.
D. ROSE "북부 계곡자치회": a limited belt of lower southern foothill valleys around city 7 x42 y35; strong core close to that city, sparse hatching toward neighboring inhabited valley entrances. Never tint the entire northern snow range or north coast. Name below northern foothill belt.
Use the existing city labels exactly ONCE: "1 세계수 관리도시", "2 로덴", "3 세르나", "4 하르덴", "5 서안항", "6 동안항", "7 북부 관문도시", "8 남동 평야도시". Preserve their readable cream halos. City 7 is a living town, not ruins.

PRODUCTION/SUPPLY MARKERS: exactly eight small ivory shield badges P1-P8 with tiny recognizable commodity symbols, at the following land locations; short descriptions belong in footer so map does not become a wall of text.
P1 managed giant-tree/woodland production belt just north/northeast of Roden in lower FOREST, x67 y40, NOT snowy peaks: normal ether and timber; not all continental ether originates here.
P2 central basin cultivated hinterland WEST/SOUTHWEST of World Tree, x38 y56: grain and vegetables.
P3 Roden workshop quarter beside city 2 on LAND, x61 y49: precision parts and repairs.
P4 valley workshops/pastures near Harden, x25 y46: water/wind equipment and livestock.
P5 western port coastal fisheries/processing beside city 5, x8 y55: fish. Do not label locally produced salt.
P6 eastern lower forest/shipbuilding supply district near eastern port, x89 y51: ship timber and shipbuilding.
P7 lower valley hinterland just east of city 7, x47 y37: local livestock and winter storage, NOT a major grain surplus area.
P8 farmland south of city 8, x75 y81: grain and seed reserves.

TRADE ROUTES: thin high-contrast lines with white halo, no giant arrows. Routes are BIDIRECTIONAL proposals, not measured roads. Do not add every possible pair.
BLUE SOLID water route: Roden down the EXISTING main river southwest then south, passing EAST of the World Tree, to Serna. Trace the visible river accurately, NEVER cut across dry land or mountain ridges. Tiny hollow square on west bank nearest city 1 = transshipment landing.
RUSSET SHORT-DASHED land routes:
- city 1 to that nearby river landing, ending on shore, NOT entering tree trunk or continuing across the main river.
- city 1 northward to city 7, skirting west of tree crown/lakes along low terrain.
- city 1 westward to city 4 through the valley/gentler plateau approaches, no ridge-crest shortcut.
- city 4 west to city 5 following valley approaches, use LAND not continuous navigable river.
- city 3 to city 8 along LAND EAST of the southern bay.
- city 8 to city 6 via low eastern valley/pass approaches; no straight line over a mountain massif.
DARK NAVY LONG-DASHED coastal shipping:
- western port 5 to Serna 3: travel on OCEAN outside the west coast, round the SOUTHWEST peninsula's southern tip, then enter the SOUTH bay to Serna; NEVER shortcut across the southwestern peninsula or a neck of land.
- Serna 3 to eastern port 6: exit the SOUTH bay, round the SOUTHEAST peninsula's southernmost tip on ocean, then up the east coast. NEVER cross southeast land or inland farmland.
Keep enough blue ocean margin in the map for these two ocean arcs without changing the land. Add no foreign continents, intercontinental destination arrows, measured distances, scale bars or latitude-longitude grids.

FOOTER, crisp text, generous whitespace, aligned columns:
First row four color swatches with exact polity names.
Second row legend "实" must NOT appear: use KOREAN exact text "실선 테두리: 핵심 통치권" / "사선: 협약·자치 주변권" / "무색: 관할 미설정" / "이중 원: 자치항".
Next show route line samples with exact text "수운" "육로" "연안 해로" and square "환적". Caption "교역 의존은 영토 소속을 뜻하지 않음".
Production/supply list in two neat rows of four:
"P1 정상 에테르·목재" ; "P2 곡물·채소" ; "P3 정밀 부품·수리" ; "P4 설비·목축"
"P5 수산" ; "P6 조선용 목재·조선" ; "P7 목축·겨울 비축" ; "P8 곡물·종자"
Small footer note "에테르 연맹: 국가를 잇는 공급 조직, 독자 영토 없음" and "생산·가공·비축 거점 / 수량·국경·항행 가능성 미검증".
Maintain a professional warm fantasy atlas look with modern clear information hierarchy. No extraneous legends, no fake compass scales, no invented mineral mines or new state names. Retain all eight source-city placements and the whole continent.
```

## 경로 보정 프롬프트

첫 생성물을 입력해 누락된 동부 연안 해로와 관리도시·환적장 육로만 보정하도록 요청했다. 프로젝트에는 보정 후 선택한 결과를 보관한다.

```text
Edit this atlas image with ONLY TWO missing route connections. Preserve all existing terrain, coastline, city positions and numbers, World Tree, color regions, political boundaries, P1-P8 badges, Korean labels and footer legend. Keep the same high quality, readable typography, layout, style and resolution.

1. Complete the NAVY LONG-DASHED coastal shipping route between city "3 세르나" and city "6 동안항". Currently the eastern route runs from port 6 around the southeast outer coast then STOPS at an isolated dot offshore the SOUTHERN TIP of the southeast peninsula near the lower-right edge above the legend. Extend that unfinished end WESTWARD on the OCEAN along the entire southern coast of the southeast peninsula, then NORTHWARD through the SOUTH BAY along the peninsula's western/bay-facing coastline, until it reaches the harbor node at "3 세르나". This must become ONE visibly continuous Serna-to-Eastport ocean route. Every added dash stays on BLUE WATER, outside the coastline. Do NOT cut across the southeast farmland or any land neck. Do NOT confuse the brown dashed road from Serna to city 8 with the ocean route. Keep the existing western-port-to-Serna navy route. Provide a thin extra ocean margin immediately above the footer if required so every navy dash has clear space outside southern headlands and does not collide with the legend border. Do not draw this as a straight cross-land shortcut. Do not end in open ocean.

2. Add a SHORT RUSSET DASHED land road from the node at "1 세계수 관리도시" to the existing square "환적" landing on the west bank of the main river, just EAST of the World Tree. Route around the SOUTH side of the tree/city roofs through visible lowland to reach that square, avoiding tree crown, labels, buildings and water. It must connect city 1 to the west-bank square, then STOP. Do NOT cross the main river. Keep the blue river transport route unchanged and clearly separate.

No other changes. All city numbers 1-8, production badges P1-P8 and political labels remain exactly as they are.
```

## 표식 보정 프롬프트

경로 보정본의 중앙 농경지 배지에 P3가 중복돼 해당 글자만 P2로 수정하도록 요청했다. 출력 v2를 현재 배치 이미지로 사용한다.

```text
Use case: text-localization / precise-object-edit. Correct ONE tiny badge text on this supplied map. Preserve EVERYTHING else including both complete navy coastal routes, russet road from city 1 to river transfer square, all terrain, region outlines/colors, typography, footer, city positions and commodity icons.

Find the ivory shield with a WHEAT symbol in the CENTRAL BASIN, just WEST/LEFT of the golden World Tree and city 1, approximately pixel x620, y450 in this 1536x1024 image (40.4% from left, 44% from top). Its small heading is incorrectly P3. Change ONLY that shield heading to exactly "P2" (uppercase Latin P followed by the digit 2). Make the digit 2 crisp and unambiguous, with diagonal descending stroke and horizontal bottom, NOT 3. Retain the wheat symbol inside this P2 shield.

The separate shield with a GEAR by city "2 로덴", approximately x1017,y411, must stay "P3".
Every map-body shield must occur exactly once: P1 normal ether/wood, P2 grain/vegetables at central basin wheat shield, P3 precision parts at Roden gear shield, P4 equipment/livestock at Harden, P5 fisheries at western port, P6 timber/shipbuilding east, P7 winter provisions north, P8 grain southeast.
The footer key already has correct P2 and P3; do not change it.
No relocation, no new text, no redesigned terrain, no routes deleted or moved. Only this one character correction.
```
