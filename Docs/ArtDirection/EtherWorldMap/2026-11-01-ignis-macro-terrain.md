# 이그니스 전체 지형 시안

세계지도에서 이그니스만 확대해 대륙 전체가 화면 대부분을 채우도록 만들고, 산맥·고원·분지·평야·강·호수의 큰 형태를 검토하는 시안이다. 도시와 첫 장 동선보다 이 지형 구성을 먼저 고른다.

2026-11-01 사용자는 “대륙이 거의 100%로 보인다고 할때 대륙의 전체적인 지형들을 먼저 잡는거”라고 작업 순서를 바로잡았다. 앞선 거점·첫 장 배치는 이 단계보다 앞서 나간 제안이므로 보류한다. 기존 파일은 비교용으로 보존하며 새 지형에 맞춘 좌표로 승인된 것은 아니다. 상위 문서는 [세계지도 시안](../EtherWorldMap.md)이다.

## 이미지

![이그니스 대륙 전체 지형 시안](Images/ignis-macro-terrain-concept-v1.png)

기본 제공 `image_gen`으로 제작했다. CLI/API fallback은 사용하지 않았다. 생성 원본은 1,774×887 PNG이며 변환 없이 보관했다. 아래 참조도에서 대륙의 동서남북 끝과 남쪽 만을 유지하도록 요청하고 내부에 지형을 그렸다.

| 파일 | 읽는 때 |
|---|---|
| [전체 지형 PNG](Images/ignis-macro-terrain-concept-v1.png) | 산맥·분지·평야·수계의 큰 모양과 비중을 고를 때 |
| [확대 해안 참조 PNG](Images/ignis-coast-closeup-reference-v1.png) | 생성 요청에 사용한 대륙 외형을 확인할 때 |
| [확대 해안 참조 SVG](Images/ignis-coast-closeup-reference-v1.svg) | 기존 세계지도의 해안과 가로세로 비율을 그대로 확대해 쓸 때 |

확대 참조는 세계지도 v3의 이그니스 좌표를 동일 비율로 옮겼다. 2,400×1,200 화면에서 해안 외곽의 가로 범위는 2,280픽셀, 세로 범위는 약 1,089.8픽셀이다. 이는 지도 영역을 대륙 위주로 채운다는 뜻이며 만과 바다를 메워 육지색을 화면의 100%로 만든다는 뜻은 아니다.

## 세계수 위치 후보

현재 실제 치수는 [세계수 규모](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-worldtree-scale.md)의 높이 8km·수관 너비 18km 작업 후보를 따른다. 아래 이미지의 나무 표식은 과장 기호로 보존하며 실제 치수나 새 관리도시의 주거 위치를 나타내지 않는다.

세계수는 중앙 분지의 호수군 남쪽, 큰 강 본류의 서쪽 육지에 놓는 시안을 제안한다. 북부 산맥 아래의 저지에 자리하며 강은 나무 동쪽으로 흘러 남쪽 만에 닿는다. 주변에 숲과 관리도시 생활권을 펼칠 여유를 남기되 도시 배치는 아직 그리지 않았다.

![세계수 위치를 표시한 전체 지형 시안](Images/ignis-macro-terrain-worldtree-v1.png)

사용자는 세계수가 커서 대륙지도에도 보여야 한다고 요청했다. 기본 제공 `image_gen`으로 지형 원본에 황금빛 수관의 나무 표식을 추가했다. 이 그림의 나무 크기는 지도에서 알아보기 쉽게 강조한 상징 표현이며 수관이 실제로 수백 km에 걸친다는 설정이 아니다. 이후 높이·수관 너비와 원경의 계산 기준은 위의 세계수 규모 문서에 작업 후보로 정리했다. 앞서 보류한 거점 배치의 세계수 좌표와 높이를 확정하거나 재채택한 것은 아니다.

입력 지형의 구도와 주요 산맥·강·해안을 유지하도록 요청했으며 큰 형태는 화면에서 대조했다. 생성 편집이므로 주변 지형의 픽셀 단위 보존이나 해안 면적 일치는 검증하지 않았다. 원래의 지형 PNG와 정확한 해안 SVG를 모두 유지한다.

### 세계수 추가 프롬프트

입력 이미지는 `ignis-macro-terrain-concept-v1.png` 한 장이다.

```text
Edit the provided Ignis Taiga full-continent terrain map. Make ONE localized addition: a recognizable golden World Tree cartographic landmark in the central interior basin. Keep the entire existing continent framing, coastline, northern mountains, western plateaus, eastern ridges, lakes, rivers, southern bay and sea otherwise unchanged. Do not regenerate or rearrange terrain.

LOCATION: root/base anchor at approximately 49% of full image width from the left and 53% of full image height from the top, on land in the central basin south of the chain of small central lakes and WEST (left) of the broad main river flowing into the southern bay. It is an inland basin landmark, NOT at the coastal delta, NOT in a lake, NOT atop the northern snow mountains. The river must continue passing to the east of the World Tree without a rerouted channel or dam.

SYMBOL: one extraordinary ancient living tree with a powerful short-visible trunk and a broad, tiered crown of golden needle-like foliage, darker olive-gold shaded lower branches, subtly bright sunlit upper crown, a believable tree silhouette rather than a glowing magical orb. Use a tasteful small pictorial landmark integrated with the map's painterly shaded-relief atlas style. The full tree symbol should be about 8% of image width and 12% of image height, rooted at the anchor; crown extends upward on the map. This is cartographic exaggeration for readability, NOT a literal hundreds-of-kilometers-wide physical tree. It must be clearly visible at whole-continent viewing size but leave the continent's overall terrain as the dominant subject. Add only a very restrained ground contact shadow. Do not add giant exposed root networks, new lakes, settlements, buildings, roads, labels, text, circles, borders, glow rays, clouds, or other symbols. Keep the surrounding terrain and waterways legible. North stays up. Same wide aspect ratio and map extent as the supplied image.
```

## 먼저 고를 지형

이 지형과 세계수 표시를 바탕으로 사용자가 요청한 [주요 도시 배치안](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-ignis-major-cities.md)을 별도로 만들었다. 이 문서의 원본 지형 PNG와 세계수 PNG는 도시를 추가하기 전 비교 자료로 유지한다.

| 위치 | 이번 시안에서 제안한 형태 |
|---|---|
| 북쪽 해안 | 만과 빙식 골짜기, 낮은 설원·툰드라, 일부 호수 |
| 북부 내륙 | 여러 산괴와 고개가 이어지는 주 산맥. 북쪽 해안과 중앙 분지 사이의 분수계 |
| 서부 | 층을 이룬 고원과 깊게 깎인 계곡. 서해안과 남부 저지로 내려오는 지류 |
| 중앙 | 넓고 완만한 내륙 분지, 낮은 구릉, 호수와 합류하는 물길 |
| 동부 | 주 산맥보다 낮고 끊어진 산줄기와 동쪽으로 열린 골짜기 |
| 남부 | 큰 만 주변의 충적 평야와 낮은 구릉, 남서·남동 반도의 해안 저지 |
| 주요 수계 | 북동부 산지에서 중앙 분지 동쪽을 지나 남쪽 만으로 내려가는 큰 강과 지류. 서해·동해·북해로 향하는 별도 유역 |

화면에서 주 산맥·고원·중앙 저지·남쪽 하구를 구별할 수 있고 도시·항로·퀘스트 표식은 없다. 산의 높이는 가독성을 위해 강조된 표현이며 정확한 고도나 경사가 아니다. 숲과 눈의 색도 시각 구분용 후보이며 기후 검증 결과가 아니다.

## 정확도와 작업 순서

생성 이미지의 해안 세부와 하구 삼각주는 참조에서 달라질 수 있다. 이 PNG의 실제 육지 면적과 모든 하천의 하강·합류 관계는 계산하지 않았다. 면적·위경도·해안의 기준은 계속 세계지도 v3의 SVG·좌표 자료이며 이 그림으로 덮어쓰지 않는다.

검토 순서는 전체 지형의 큰 구성 → 산맥 분수계와 강·호수 연결 → 기후·식생과 세계수 생활권 → 벨레스 등 주요 거점 → 첫 장 지역·동선이다. 선택한 지형 구성은 이후 좌표 도형과 높이 초안으로 옮겨 검증한다. 앞선 [거점 배치안](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-ignis-spatial-layout.md)과 [첫 장 배치안](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-first-chapter-spatial-layout.md)은 그때 재검토할 비교 자료다.

## 사용한 프롬프트

입력 이미지는 위 확대 해안 PNG 한 장이다. 아래 문장을 기본 제공 이미지 생성 도구에 전달했다.

```text
Edit the provided coastline reference into ONE full-continent physical terrain planning map of IGNIS TAIGA. The input image is the coastline and framing authority, not a style reference. KEEP its exact north-up coastline silhouette, bays, headlands, broad east-west aspect and framing: the entire single continent, west tip to east tip and north coast to south coast, fills about 95% of image width and 90% of image height. The deep, wide south-central bay MUST stay open ocean. No other continents. Do not zoom into one region. Keep the surrounding ocean margin narrow, as in the reference. No panels, title bars, inset maps, city symbols, roads, route lines, borders, labels, numbers, text, compass roses or giant tree icons.

Task: establish the continent's complete MACRO landforms before settlements or story routes. Draw a beautiful legible professional shaded-relief physical atlas map, directly overhead/orthographic 2D cartography, not an oblique diorama. Natural continuous relief shading and detailed drainage; readable large landforms, not crude flat colored polygons. Very high resolution fine terrain detail.

Terrain hierarchy inside the reference land silhouette:
1. A cold northern coastal belt with fjord valleys, low tundra plains, some glacial lakes, and icy high headlands. Do not cover all northern land in a single uniform white sheet.
2. A branching, curved, irregular east-west MAIN MOUNTAIN SYSTEM across the northern third, inland south of the northern coast. Several connected massifs with snow on the highest ridges, saddles, north-facing glacial valleys, south-facing foothills. Avoid a ruler-straight uninterrupted wall.
3. Broad WESTERN PLATEAUS across the western quarter, stepped uplands and deeply cut river valleys sloping toward west coast and south-facing lowlands, with a gradual transition from the northern mountains.
4. A wide CENTRAL INTERIOR LOW BASIN beneath the main mountains, ringed loosely by foothills, with rolling golden-green boreal woodland terrain and a few connected glacial lakes. It must read as a basin with traversable low country, not a ring-shaped fortress. No world-tree illustration yet.
5. EASTERN UPLANDS AND SECONDARY RIDGES, lower and more broken than the northern system, separated by broad east-draining valleys and a narrow eastern coastal plain.
6. Extensive SOUTHERN ALLUVIAL PLAINS, gentle rolling land and mixed woodland around the large southern bay; the southwest and southeast peninsulas remain intact and contain low ridges and coastal plains, not ocean.
7. A dendritic MAIN RIVER begins on the south-facing northeastern mountains, flows southwest into the east side of the central basin, then south into the northern shore of the big southern bay, with a clear estuary. Tributaries merge downhill. Also show independent west-coast, east-coast and north-coast drainage basins, small tributaries, a limited number of natural elongated lakes. Never run a river over a mountain crest, never connect opposite oceans with a canal-like river, no decorative evenly spaced river lattice.

Palette: restrained pale blue ocean, ivory and grey alpine peaks, cool muted tundra, olive-gold boreal forest lowlands, warm stone plateaus, muted green and ochre southern lowlands. Clear ridge-and-valley structure with soft directional hillshade. Landform transitions are organic and vary longitudinally, not horizontal striped climate bands. This is an authored fantasy terrain CONCEPT over an existing coordinate coastline, not a satellite photograph, no clouds hiding terrain. Preserve all major geography of the coastline reference.
```
