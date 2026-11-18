# 벨레스 B안 장소별 생성 입력

내장 `image_gen`을 사용했다. B안은 화풍 참조, 지도와 공방 배치선은 위치 참조다. 원본 자료와 생성 장면의 정밀 투영 일치를 입증하는 입력은 아니다. 결과와 검토 범위는 [장소별 콘셉트](../B-scenes.md)를 따른다.

공유본은 PC 전용 경로를 <repository>와 <generated-images> 자리표시자로 바꿨다. 원본 입력 기록은 Git 제외 로컬 백업에 보존했다.

## 공통 입력

```text
Use case: stylized-concept. Create a single 16:9 landscape environment keyframe for CCL8 Veles. Image 1 is the USER-APPROVED B STYLE REFERENCE: preserve its lyrical semi-realistic game environment rendering, substantial but believable timber forms, warm wood, terracotta roofs, teal shutters, mustard canvas, softly edited material detail, golden conifers, natural late-morning sunlight, cool open shadows. Preserve style and architectural vocabulary, NOT its compressed arrangement of places or giant stairs. Do not turn this into photorealism or a toy-like cartoon.
Additional images are GEOGRAPHY / PLACEMENT REFERENCES ONLY, not style, not output layout. Output a ground-level perspective concept, never a map, diagrams, labels, overlays, collage, text, logos or watermarks.
Authoritative local coordinates in meters: east +X, north +Y. Quay marker (0,0), actual bank access (-46,20) with narrow 3m-wide timber jetty to (0,0). Workshop rectangular footprint X51..69 Y114..134, center (60,124), front door (60,114) faces SOUTH, front route node (60,110). Communal water point (-20,90); water storage node (45,175). Forest entrance (90,260). R01 route goes workshop-front (60,110) westward to (-20,90), then northeast through (42.49,186.57), (90,260), (180,440). Main river is separate from drinking water: river runs from NE through (180,160) to (0,0), then (-300,-150) and south; about 55m wide here. Spring channel runs from north (100,480), (20,290), (45,175), (-20,90), (0,20). Village occupies the northwestern bank, NOT an island and NOT both riverbanks. Use modest low banks and gently rising terrain consistent with current blockout; do NOT use the earlier compressed reference's 20m cliff and giant cascading stairs. Do not move buildings or bring distant facilities adjacent to foreground for scenic composition.
Wood, stone, woven cloth and forged iron remain physically credible. Human scale adult extras only, no named character design. Golden conifer woodland, low RED MOSS carpets on rocks rather than masses of red shrubs, subtle localized shaded frost only. Clear walkable space. No visible world tree, no exposed gigantic roots, castle, palace, modern industry, ocean harbor, big ships, fantasy portals, glowing crystals or mechanical pipes. This is a normal environment design frame, not a specific plot-event illustration.
```

## 공방 근경

참조 파일: `<repository>/Docs/ArtDirection/VelesConcepts-2026-11-17/B-lyrical.png`, `<repository>/Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailArt-v5/previews/workshop-access-geometry.png`

공통 입력 뒤에 다음을 붙였다.

```text
SCENE: close exterior view of the WORKSHOP. Camera local (60,90), eye 1.65m above ground, looking NORTH at the south-facing front (60,114), 35mm perspective. Workshop is the main subject, 18m wide by20m deep, approximately6.5m ridge height. Retain B's red simple gable, chunky believable beams, teal wooden shutters and mustard awning; reinterpret it as a standalone substantial workshop on its actual footprint, NOT perched directly over the quay. Central clear door/walkway at least2.2m, awning-covered bench and anvil to one side, practical tools and one partly repaired small wooden boat component to communicate boat repair. Solid forge hearth against masonry with safe clearance from wood, modest lived-in tools, readable uncluttered floor. Show the short 4m entrance connector reaching the east-west front route; the route continues toward screen-left / west toward water point roughly80m away, mostly out of view. No basin immediately outside this workshop door. North forest behind the building; other village buildings west / left in middle distance. Main river may be a very narrow distant right-edge glimpse, more than several dozen meters away, never beside the doorstep. Do not show the quay in this close frame. Second image cyan outlines/yellow lanes indicate geometry only: workshop is the lower-right rectangular building with SOUTH-facing entrance. Do not reproduce the top-down map or colored lines. Prioritize consistent craftsmanship and a usable work apron.
```

## 나루 전경

참조 파일: `<repository>/Docs/ArtDirection/VelesConcepts-2026-11-17/B-lyrical.png`, `<repository>/Docs/ArtDirection/EtherWorldMap/Images/veles-map-ui-unified-v2.png`

공통 입력 뒤에 다음을 붙였다.

```text
SCENE: WIDE QUAY ESTABLISHING VIEW. Camera local (-10,-20), floating observer viewpoint 2.5m above river surface, looking NORTH to NORTH-NORTHEAST toward the northwestern settled bank, 28mm lens. A long narrow practical wooden landing projects diagonally from bank at(-46,20) on the LEFT toward quay marker(0,0) near CENTER/RIGHT foreground. This is about50m of jetty, 3m wide, clear passage, one or two small river boats moored along its side, rope coils and a few crates at bank end, no huge pier buildings. River fills foreground and RIGHT; upstream comes from upper RIGHT/NE, downstream exits lower LEFT/SW. Village bank on LEFT and center background, opposite bank beyond river on RIGHT remains wooded and unbuilt. The red-roof/teal-shutter/mustard-awning workshop lies about125m inland NE of quay and therefore appears SMALL in the far center-right settled-bank background, not adjacent to the boat. Main village houses spread WEST of workshop in the LEFT background; intervening open ground and paths communicate distance. Small low natural bank, no massive cliff or high stair-stack. Forest entrance is farther behind workshop toward north, a subtle distant gap only. Do not put a communal fountain on the quay. Natural still mooring water distinct from the broad river current. No visible world tree or monumental skyline. Use approved B styling while faithfully respecting these shoreline directions and relative distances.
```

## 숲 진입부

참조 파일: `<repository>/Docs/ArtDirection/VelesConcepts-2026-11-17/B-lyrical.png`, `<repository>/Docs/ArtDirection/EtherWorldMap/Images/veles-map-ui-unified-v2.png`

공통 입력 뒤에 다음을 붙였다.

```text
SCENE: FOREST ENTRANCE from the village. Camera local(80,240), eye1.65m, looking NORTH-NORTHEAST along R01 through entrance(90,260) toward(180,440); 32mm ground-level perspective. A4m-wide maintained earth trail enters a golden conifer woodland, gently rises and bends to the RIGHT/northeast deeper in the scene. Entrance means an opening between trees, NOT a built gate, stone arch or portal. Keep adult-scale trunks and readable ground, red MOSS patches hugging roots and gray rocks, warm golden needles overhead, soft layered golden mist in depth, restrained shaded frost on one side. A modest timber maintenance railing or a few repaired planks may mark a damp side area without blocking the dirt trail. The drinking-water spring channel is to the WEST / screen LEFT, roughly50–70m away here, glimpsed only through trees as a thin blue-green line if visible; do not relocate a large stream directly beside the right edge of the path. The navigable main river is farther EAST / screen RIGHT and should be offscreen behind trees; no dock, boat, basin, village building or workshop in front of camera. Village and workshop lie BEHIND the camera south/southwest, do not put roofs at the trail destination. No cave mouth visible at this distance; cave is around1km farther north. Very small anonymous adult walking away provides scale. Same approved B natural light, simplified but convincing bark/stone/cloth rendering and foliage color grouping, inviting exploration with subtle local cold evidence rather than a winter landscape.
```

## 숲 물길 위치 수정

초기 결과의 길 옆 전경 물길을 제거하는 수정이다. 최종 파일은 `forest-entrance.png`다.

```text
Edit the supplied forest concept only. Preserve the camera, golden conifer canopy, lyrical semi-realistic B art style, warm daylight, person, winding dirt trail, rocks, red moss and foliage exactly in character. Correct one GEOGRAPHIC ERROR: the drinking-water channel must be WEST of the route about50–70m away, NOT next to the path. Remove ALL visible water immediately beside the path and behind the foreground timber railing in the lower-left quadrant. Replace that water with continuous dry or slightly damp forest soil, fallen needles, grasses and low moss; keep the railing on a dry shallow edge. No puddle, brook, drainage ditch or shiny water may touch the foreground path. Retain ONLY a tiny distant blue-green water glimpse deep in the far-left background between distant trees, separated from the trail by a broad densely wooded strip. Do not move or widen the trail; do not add buildings, gates, bridges or a new river. One landscape frame, no text.
```
