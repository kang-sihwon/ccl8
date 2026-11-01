# 벨레스 지도 표현 비교

같은 벨레스·첫 장 지역을 세 가지 지도 표현으로 비교한다. 사용자가 UI 제작은 뒤로 미루고 지도만 여러 개 보여 달라고 요청했으므로 상하단 메뉴·범례·조작 안내·현재 위치·목적지 표식을 모두 제외했다. 지명 네 개만 남겼으며 아직 선택된 안은 없다. 상위 문서는 [세계지도 시안](../EtherWorldMap.md)이다.

| 안 | 표현 | 비교할 특징 |
|---|---|---|
| A | 양피지 세밀화 | 크림색 종이와 갈색 선, 옅은 파란 물길. 고전 지도 질감과 세부 묘사 |
| B | 밝은 컬러 지형도 | 밝은 녹색 숲과 청록색 물, 단순한 색면. 지형 종류와 길 구분 |
| C | 먹선 중심 지도 | 흑백 선과 점묘, 흰 물길. 대비와 판화 같은 질감 |

## A: 양피지 세밀화

![A 양피지 세밀화](Images/veles-map-only-a-parchment-v1.png)

## B: 밝은 컬러 지형도

![B 밝은 컬러 지형도](Images/veles-map-only-b-color-v1.png)

## C: 먹선 중심 지도

![C 먹선 중심 지도](Images/veles-map-only-c-ink-v1.png)

## 제작과 확인 범위

기본 제공 `image_gen`으로 [앞선 UI 시안](Images/veles-ingame-map-ui-v1.png)을 공간 참조로 삼아 각각 생성했다. 작성자가 세 화면에서 UI 제거, 지명, 마을·북쪽 동굴·숲길·본류 굽이의 큰 배치를 확인했다. 복사본은 원본과 SHA-256이 일치한다. 지류·등고선·건물·식생의 세부는 생성 과정에서 달라졌으며 픽셀 정합이나 실제 지형·거리를 검증한 자료가 아니다. 지도 바탕 선택용 후보이고 기존 서사·좌표의 변경 근거로 사용하지 않는다.

## 입력문

### A

```text
Use case: style-transfer. Create a MAP ONLY variation of the supplied Veles region map, same exact region, north up, same landscape framing, 16:9 landscape. Treat source as spatial authority. Keep the river's northeast entry, diagonal course along east side of village, southward bend and southwest exit; preserve the oval ridge southwest of village inside the river bend, village footprints and quay near middle-bottom, winding forest walking trail from village north to cave near top center-right, forest masses and separate narrow watercourses. Same settlement size, no new settlements, no bridges. Do not relocate places to create a prettier composition. Vertical top-down cartographic projection, never isometric, never perspective miniatures.
REMOVE ALL UI, top/bottom bars, header, breadcrumb, close button, compass, legend, tooltips, key prompts, marker circles, objective diamond, cyan player triangle. Fill removed bars with continuous map terrain. No frames or decorative border. No glowing routes, pins, combat nodes, numbered labels, scale grids. Only four modest cartographic Korean place names embedded unobtrusively in terrain, exact text: "벨레스", "금빛 안개 삼림", "테른 강", "동굴". Remove all other text. Cave should be a tiny cartographic cave-mouth notation, not a large landmark illustration. Map fills the entire canvas. Preserve geography, change visual language decisively.
STYLE A: archival hand-engraved fantasy survey map on warm pale parchment, sepia brown ink, very restrained faded blue watercolor river wash, fine copperplate hachures for relief, stippled woods with overhead texture not upright tree pictograms, delicate irregular linework, dark small building footprints. Authentic cartographer's crafted map, rich detail but generous pale ground, tactile subtle paper grain, no coffee stains or burnt edges. Predominantly cream and sepia; subdued and elegant.
```

### B

```text
Use case: style-transfer. Create a MAP ONLY variation of the supplied Veles region map, same exact region, north up, same landscape framing, 16:9 landscape. Treat source as spatial authority. Keep the river's northeast entry, diagonal course along east side of village, southward bend and southwest exit; preserve the oval ridge southwest of village inside the river bend, village footprints and quay near middle-bottom, winding forest walking trail from village north to cave near top center-right, forest masses and separate narrow watercourses. Same settlement size, no new settlements, no bridges. Do not relocate places to create a prettier composition. Vertical top-down cartographic projection, never isometric, never perspective miniatures.
REMOVE ALL UI, top/bottom bars, header, breadcrumb, close button, compass, legend, tooltips, key prompts, marker circles, objective diamond, cyan player triangle. Fill removed bars with continuous map terrain. No frames or decorative border. No glowing routes, pins, combat nodes, numbered labels, scale grids. Only four modest cartographic Korean place names embedded unobtrusively in terrain, exact text: "벨레스", "금빛 안개 삼림", "테른 강", "동굴". Remove all other text. Cave should be a tiny cartographic cave-mouth notation, not a large landmark illustration. Map fills the entire canvas. Preserve geography, change visual language decisively.
STYLE B: clean contemporary illustrated topographic atlas, bright light cream background, broad softly bounded sage-green forest areas, light mint terrain terraces, turquoise-blue river, pale sandy pathways, minimal crisp gray-brown building footprints. Simplified harmonious flat color shapes, very subtle low relief shading, a few precise contour lines, strong hierarchy and generous negative space. No dense realistic foliage, no paper distress, no photorealism. Sophisticated clear navigational map with a luminous airy palette, sharply different from antique etching.
```

### C

```text
Use case: style-transfer. Create a MAP ONLY variation of the supplied Veles region map, same exact region, north up, same landscape framing, 16:9 landscape. Treat source as spatial authority. Keep the river's northeast entry, diagonal course along east side of village, southward bend and southwest exit; preserve the oval ridge southwest of village inside the river bend, village footprints and quay near middle-bottom, winding forest walking trail from village north to cave near top center-right, forest masses and separate narrow watercourses. Same settlement size, no new settlements, no bridges. Do not relocate places to create a prettier composition. Vertical top-down cartographic projection, never isometric, never perspective miniatures.
REMOVE ALL UI, top/bottom bars, header, breadcrumb, close button, compass, legend, tooltips, key prompts, marker circles, objective diamond, cyan player triangle. Fill removed bars with continuous map terrain. No frames or decorative border. No glowing routes, pins, combat nodes, numbered labels, scale grids. Only four modest cartographic Korean place names embedded unobtrusively in terrain, exact text: "벨레스", "금빛 안개 삼림", "테른 강", "동굴". Remove all other text. Cave should be a tiny cartographic cave-mouth notation, not a large landmark illustration. Map fills the entire canvas. Preserve geography, change visual language decisively.
STYLE C: striking monochrome charcoal-and-ink cartographic drawing on cool off-white fibrous paper. Black and graphite contour strokes, dry-brush stippling for overhead forest canopy zones, airy white river with thin paired bank lines and sparse flow strokes, sparse dark building footprints, fine gray tracks, expressive economical hand-drawn topographic hachures. Clear readable negative space; no colored areas at all, no sepia, no ornamental Asian calligraphy, no upright pictorial mountains or trees. Mature graphic printmaking aesthetic, terrain stays strictly overhead and geographically faithful.
```
