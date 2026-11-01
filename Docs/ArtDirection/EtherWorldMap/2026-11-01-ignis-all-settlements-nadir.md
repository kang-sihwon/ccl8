# 이그니스 정착망 수직 평면 지도

대도시 8곳, 일반 도시 12곳, 마을 16곳의 이름을 이그니스 대륙도와 중앙 생활권 확대도에 표시한 시안이다. 산맥은 등고선으로, 세계수는 위에서 본 수관 기호로 표현했다. UI는 포함하지 않는다. 상위 문서는 [세계지도 시안](../EtherWorldMap.md)이다.

![이그니스 정착망 수직 평면 지도](Images/ignis-all-settlements-nadir-v1.png)

## 기준과 표시

배치의 근거는 [대도시와 정착망](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-ignis-settlement-network.md)이다. 일반 도시와 마을의 새 이름은 해당 문서의 상세 설정을 따른다. 수직 평면 표현은 [결정 35](../../decisions/0035-nadir-map-presentation.md)를 따른다.

| 표시 | 대상 |
|---|---|
| 큰 이중 원 | 대도시 8곳 |
| 원 | 일반 도시 12곳 |
| 작은 점 | 마을 16곳 중 상설 거처 |
| 삼각형 | 마을 목록에 포함된 계절 거처 여름뜰 |
| 황금 수관 | 세계수의 위치 기호 |

대륙 본도에는 대도시 6곳, 일반 도시 9곳, 마을 12곳의 이름을 표시한다. 하단 확대도에는 세계수 관리도시·로덴, 타르넬·미레바·오르센, 벨레스·솔숨·들보리·숲곁의 이름을 표시한다. 두 도면을 합쳐 대표 정착지 36곳이며 대륙의 전체 정착지 수는 아니다.

중앙의 정착지는 대륙도에서 표식과 글자가 겹치므로 확대도에서 읽도록 구성했다. 본도의 점선 사각형은 확대 영역의 개념적 위치이며 측정한 외곽이 아니다. 외곽 지역의 도시는 기존 해안·계곡·교역로 관계를 바탕으로 그림 안의 위치를 제안했다. 새 위경도나 행정 경계를 확정하지 않았다.

## 확인 범위와 남은 오차

최종 PNG를 화면으로 대조해 36곳의 이름, 큰 남쪽 만과 두 반도, 등고선 중심의 평면 표현을 확인했다. 세계수 줄기와 산봉우리의 측면을 없앴다. 이미지 크기는 1,536×1,024이며 기본 제공 `image_gen`으로 생성·편집했다. CLI/API fallback과 별도 픽셀 편집은 사용하지 않았다.

이 이미지는 배치와 표현을 고르는 시안이다. 아래 오차 때문에 좌표 기준도나 실제 게임 지도 데이터로 사용하지 않는다.

- 확대도는 동서 방향으로 늘어나 기존 지역 좌표의 거리 비율을 충족하지 못한다. 특히 관리도시·미레바와 세계수의 거리, 벨레스 주변 네 마을의 방향·간격을 원래 좌표로 다시 배치해야 한다. 숲곁·들보리 표식은 벨레스 서쪽·남서쪽이라는 설정을 정확히 재현하지 못했다.
- 바람재 근처에 이름 없는 점이 남아 있다. 새 정착지로 해석하지 않는다. 원의 굵기와 마을 점의 테두리도 범례와 완전히 일관되지는 않다.
- 세계수의 수관은 강조된 기호다. 확대도에서도 실제 수관 지름 18km나 주거 제한 반경 12km를 측정할 수 없다.
- 해안선 면적, 산의 고도, 하천의 하강·합류, 도로의 통행 가능성, 항만의 수심은 검증하지 않았다. 수계와 도로를 실제 노선으로 확정하지 않는다.

면적·해안의 기준은 [세계지도 계산 보정본](2026-11-01-precision-validation.md), 중앙 정착지의 거리 기준은 [세계수·정착지 설계도](2026-11-01-worldtree-settlement-plans.md)를 유지한다. 기존 설정의 좌표·정본·첫 장 제작 범위는 변경하지 않았다.

## 생성 입력

전체 대륙의 형태에는 [기존 지형과 세계수](Images/ignis-macro-terrain-worldtree-v1.png)를, 평면 표현에는 [벨레스 수직 지도](Images/veles-wider-region-nadir-v2.png)를 참조했다. 아래 첫 요청 뒤 중간 생성물을 차례로 편집해 최종 PNG를 만들었다. 프롬프트의 백분율과 픽셀은 생성 지시이며 결과를 측정한 좌표가 아니다.

### 최초 요청

```text
Use case: style-transfer / infographic-diagram.
Create a polished high-resolution Korean fantasy CONTINENT ATLAS MAP of IGNIS with all 36 representative settlements. Image 1 is the geographic silhouette and macro-landform reference. Image 2 is the APPROVED strict vertical planimetric visual style. Transform Image 1 completely to Image 2's flat map treatment and add the settlement network below. Preserve Image 1's broad east-west single continent outline, northern inlets, southwestern and southeastern peninsulas and large OPEN south-central sea bay. North up. Full continent, never cropped.

CRITICAL VIEW: absolutely vertical nadir orthographic overhead map. Zero tilted camera, zero isometric terrain, zero visible mountain sides, zero upright trees, zero building facades. Mountains ONLY nested brown contour lines and subtle flat hypsometric fills, snowfields white top-down shapes. Forests irregular stippled dark olive canopy cover. Settlements geometric flat symbols, no illustrated miniature castles. World Tree a SMALL golden overhead rosette/map symbol only, no trunk. Mature restrained atlas craft, warm stone/beige land, dark olive woods, desaturated slate teal sea/rivers, ivory labels with dark hairline halos. Dense fine cartography, beautiful calm legibility.

LAYOUT: landscape 3:2 atlas sheet, request 3072x2048 or highest useful resolution. Upper approximately 72 percent is the uncropped 2:1 entire continent, fills nearly width. Lower approximately 28 percent contains an elegant geographic MAGNIFICATION INSET titled "중앙 생활권 확대" at left/center and a tiny cartographic key at right. This is a MAP sheet, no game UI, no toolbar, quest markers, buttons, interface frames, title banner or giant heading. Thin inset border is fine. Inset purpose: central places are truly close; never spread them across the continent just for labels. Put a small dashed locator box at the World Tree central basin and fine leader to the inset. Map key: double circle "대도시 8", single circle "일반 도시 12", small solid dot "마을 16", outlined triangle "계절 거처". Add small note "대표 정착지 배치 시안". Village count includes seasonal settlement 여름뜰. No scale bar or false coordinate grid.

MAIN MAP geography: northern third irregular snow-covered mountain watershed expressed as contours; western dissected plateaus, central lower basin and lakes, east lower broken ridges and valleys, southern grain lowlands. Main Tern river comes from NE mountain southern slopes into central basin EAST of World Tree, then south to estuary at north shore of big south bay. Other independent catchments drain west and east. All settlements on LAND, ports shore-adjacent, none on snowy peaks or sea. Thin subtle ochre roads follow low valleys; understated blue river transport. No political colored polygons, no production badges; settlements are the focus.

MAIN MAP exact labels (use each listed name once here; central names belong to inset):
Six major double-ring cities, bold larger Korean labels:
"서안항" west coast estuary around x5% y53% of MAIN map.
"하르덴" western plateau inhabited valley bench x22 y43.
"북부 관문도시" southern foothill valley below northern range x42 y35.
"세르나" main river estuary at north shore of south bay x55 y68, shore aligned.
"동안항" eastern coastal valley mouth x95 y52.
"남동 평야도시" southeastern low plains x73 y75, inland.
World Tree main locator near x49 y53, label "세계수"; tiny compact central settlement cluster extends only a few percent northeast. Two other major cities 관리도시 and 로덴 shown within this cluster as tiny double-ring markers, with names ONLY in inset. Do NOT reuse outdated Roden location far east near x64,y47.

Nine regular single-ring towns on main map:
"켈바" x33 y46 western valley between Harden and central basin.
"드라벤" x13 y49 lower western plateau between Harden and western port.
"살레크" x17 y80 southwest peninsula coastal sheltered harbor, NOT inland.
"메르사" x65 y70 plains on LAND road between Serna and southeastern city, road goes round bay's east shore not across water.
"에스칼" x84 y57 lower eastern wooded valley behind east port.
"루세아" x84 y89 sheltered SE outer peninsula coast (adjust to actual shore).
"네르바" x44 y42 valley south of northern gateway.
"케르딘" x43 y28 habitable valley beyond northern gateway, not snowy summit.
"울렌" x33 y34 western branch inhabited pasture valley of northern gateway.

Twelve village dots on main map, small but readable labels:
"윗샘" x32 y40 upstream of Kelba valley.
"누마" x20 y36 gentle plateau hinterland of Harden.
"바람재" x25 y40 windworkshop near Harden.
"갈매" x15 y77 small cove near Salek, LAND/coast.
"씨앗골" x68 y66 farmland behind Mersa.
"물나래" x76 y78 irrigation branch behind SE city.
"긴솔" x82 y51 managed woodland inland of Eskal.
"청그물" x91 y73 coast between Lusea and east port.
"돌곡" x46 y40 low valley by Nerva.
"길목" x41 y31 below approach to Kerdin.
"여름뜰" x29 y30 summer pasture of Ullen; outlined triangle instead of permanent dot.
"새불" x43 y38 between northern gateway and Nerva.

INSET: central basin region with EXACT relative orientation (x east, y north) using these existing km coordinate candidates, but do not print numbers. Draw within rectangular extent x=-30 to190, y=-55 to175. Use UNIFORM scale on both axes. Anchor points must not be spread to make labels fit. Offset labels with fine leader lines where necessary, especially the 4 villages near Veles. Inset is more detailed same planimetric style; its river connects upstream Veles then Tarnel then Roden then east of World Tree then downstream. Golden overhead World Tree symbol at (0,0), physical crown diameter18km on this inset, no enormous crown covering cities. No homes inside12km radius. Its map symbol on main map is exaggerated, not literal.
Two major double-ring cities:
"세계수 관리도시" at(18,-12)
"로덴" at(110,95)
Three single-ring regular towns:
"타르넬" at(137,125)
"미레바" at(37,10)
"오르센" at(45,-35)
Four small village dots:
"벨레스" at(150,145)
"솔숨" at(154,151)
"들보리" at(140,137)
"숲곁" at(143,143)
Also World Tree small crown at(0,0) label "세계수". These nine settlements plus the 27 on main map are ALL36 unique representative places. Put all four Veles-area village labels with separate thin leader lines into clear nearby spaces. No repeated/misspelled settlement names. Korean typography must be sharp and correctly written. Keep village symbols smaller than towns, towns smaller than major cities. Inset river roughly follows (158,157),(150,145),(145,140),(146,130),(137,125),(133,112),(120,110),(118,101),(110,95),(98,88),(83,75),(64,60),(48,42),(36,20),(28,-12),(31,-30),(42,-48).
No extra settlements, no first chapter dungeon labels. Produce the complete beautiful map, not a dashboard, not a textual planning chart.
```

### 평면 표현과 배치 보정 요청

```text
Edit this atlas sheet. Preserve its continent outline, large southern sea bay, 6 main-map city locations, all 9 main-map town locations, atlas framing, palette, inset and key. Two mandatory corrections:

1. COMPLETE ORTHOGRAPHIC PLANIMETRIC CONVERSION. The snowy mountain chain is WRONG: it still shows upright triangular peaks, sloping side faces like an oblique landscape. REPLACE EVERY MOUNTAIN everywhere with flat overhead topographic contour nests and pale elevation color bands. No peak silhouettes, no triangular mountain icons, no vertical cliffs, no shadowed side facades. Every summit is a concentric irregular loop seen from straight above; ridges are elongated nested loops with branching valleys. This includes the entire northern snow chain, western plateaus, eastern ranges. ONLY overhead cartographic contours like a real topographic atlas or flat video game exploration map. Snow as pale surface fill inside highest contours. Do not recreate the old mountains under contour stripes. Remove all visual perspective.
Both golden World Tree drawings currently show upright trunks: REPLACE with round overhead golden canopy rosette, absolutely NO trunk/side branches/side shadow. Its symbol is a diagram, not a giant standing tree.

2. SETTLEMENT AUDIT AND CORRECTIONS. EXACTLY 27 unique labeled settlements in main map and nine in inset. Keep key totals 8/12/16.
Main major names: 서안항, 하르덴, 북부 관문도시, 세르나, 동안항, 남동 평야도시.
Main town names: 켈바, 드라벤, 살레크, 메르사, 에스칼, 루세아, 네르바, 케르딘, 울렌.
Main village names, each ONCE: 윗샘, 누마, 바람재, 갈매, 씨앗골, 물나래, 긴솔, 청그물, 돌곡, 길목, 여름뜰, 새불.
Remove the DUPLICATE lower 윗샘 label near Kelba. Keep just one 윗샘 upstream of Kelba. Any extra unlabeled village dot in the Harden/Kelba area must be removed. Move 새불 north to between 북부 관문도시 and 네르바, not south of Nerva. Keep 돌곡 next to Nerva on a low valley floor. 여름뜰 is the one triangle. 케르딘 and 길목 occupy a snow-free habitable valley through the contours, not a snowy summit.

INSET redraw the central settlement placement so NORTH IS UP and both coordinate axes use the SAME SCALE. Existing lower inset is wide; USE the right half of its interior as the compact north-up settlement network, use left half for surrounding terrain/World Tree label whitespace instead of stretching east-west distances. Fit bounding world-coordinate rectangle (-30,-55) to (190,175) as a nearly SQUARE sub-area within the inset's available height. Extend terrain left and right as needed. Tiny markers with fine leader lines going into empty left/right inset spaces will keep labels readable. Do not move anchors to make room for labels. Coordinates in km only guide placement, not printed:
worldtree(0,0), 世界수 label spelled "세계수".
double-circle "세계수 관리도시"(18,-12), "로덴"(110,95).
single-circle "타르넬"(137,125), "미레바"(37,10), "오르센"(45,-35).
solid-dot "벨레스"(150,145), "솔숨"(154,151), "들보리"(140,137), "숲곁"(143,143).
Therefore from Veles: Solsum very slightly NE, Supgyeot slightly WEST, Deulbori SW. Give four close village anchors FOUR clean distinct fine leader lines to generous text positions. Do not put 숲곁 NE of 벨레스. Inset river flows from Veles through Tarnel and Roden southwest toward management city's east river landing then south past Orsen. World Tree canopy diameter18km, do not visually occupy all150km between tree andVeles. Tiny gold dot on mainmap can remain exaggerated. Keep all exact Korean names legible without duplicated/misspelled names. No additional labels except existing map key and insettitle.
Everything is flat overhead, especially the main map mountain ranges. No UI.
```

### 이름과 기호 보정 요청

```text
Make ONLY these small cartographic label/marker corrections to this 1536x1024 atlas. Preserve ALL terrain, perfectly flat overhead contours, map outline, coloring, other names, inset frame and layout exactly. No mountain perspective changes.

A. Lower-right area of inset around pixels x1000-1170 y670-735: the four village labels must read exactly "벨레스", "솔숨", "숲곁", "들보리", once EACH. Currently 숲곁 is duplicated and 솔숨 is missing. REPLACE the TOPMOST label at approximately x1115,y681 (currently 숲곁) with exactly "솔숨". Solsum anchor around1094,681 remains NE of Veles. Veles anchor remains (1058,709). The SECOND label below Solsum must be exactly "숲곁"; move its marker to (1043,708), WEST of Veles, and use a fine elbow leader to its text at(1140,706). "들보리" marker must be (1041,725), SOUTHWEST of Veles, fine leader to its existing text at(1130,731). Draw those callout leaders so each village is unambiguous. Keep "벨레스" at its current text position with a short leader to (1058,709). If text room is needed, use the open terrain directly below these four village labels; do not change any other settlement. Write 솔숨 correctly, two Hangul syllables 솔 and 숨, not 숲곁.

B. Main map: "새불" village currently around607,326 is incorrectly SOUTH of "네르바" at607,297. Remove old 새불 dot and text and put the dot at(626,268), midway along road between 북부 관문도시(645,240) and 네르바(607,297), label "새불" to its right. Leave 돌곡 intact south of 네르바.

C. Main map "켈바" marker at(510,350) is incorrectly double-ring. Make it a SINGLE-ring town symbol identical to 드라벤/메르사. Major cities alone get double rings.

D. Remove the stray UNLABELED dot at approximately(398,276) north of 바람재, restoring terrain beneath it. Keep labeled 바람재 dot at(411,315), 윗샘 at(491,313) and 누마 at(291,252).

No other changes. Especially preserve the fully flat contour mountains, all36 labels, all worldtree top-down canopy symbols, coastline and roads. Do not add new dots or words.
```

### 최종 누락 표기 보정 요청

```text
This map needs THREE tiny repairs, NOTHING else. Preserve every other pixel as closely as possible, all typography, 36 settlement names, contour terrain, tree canopy, coastline, inset and framing.
1. Restore the missing Korean village name "바람재" immediately above/right of the existing unlabeled small dark village dot at pixel(411,315), just east of 하르덴. Put text around(421,294). Do not remove or move its dot.
2. Restore a SINGLE-RING town marker for "울렌" at pixel(547,245), immediately below-left of its existing name. Its text already exists. Preserve the name.
3. In the lower inset, remove ONLY the stray UNLABELED dot at pixel(1092,726), immediately below the 숲곁 label leader. Keep the four actual village dots and names 벨레스, 솔숨, 숲곁, 들보리. Do not move or retype those labels.
No other corrections or aesthetic changes. Preserve existing flat map style.
```
