# 벨레스 주변 생활권 지도 시안

## 수직 시점 수정안

사용자는 이전 시안의 산과 나무가 비스듬하게 보인다고 지적했다. 같은 지역을 수직으로 내려다본 평면 지도로 다시 생성했다. 산의 측면 묘사는 등고선으로 바꾸고 숲과 건물은 수관과 지붕의 윗면으로 표현했다. 기본 제공 `image_gen`으로 기존 확장 지도를 참조했다.

![벨레스 주변 수직 시점 지도](Images/veles-wider-region-nadir-v2.png)

강의 흐름과 정착지의 대략적인 위치는 유지했지만 일부 길과 건물 배치는 달라졌다. 등고선은 실제 고도 자료에서 계산한 것이 아니며 축척과 수계 검증도 하지 않았다. 수직 시점의 표현을 검토하는 시안이다.

### 수정 입력문

```text
Use case: style-transfer. Edit the supplied regional map to correct its projection. The user rejected the oblique mountains, upright trees and visible building walls. Redraw this SAME geographic region as a genuinely FLAT PLANIMETRIC ORTHOGRAPHIC MAP, camera exactly vertically down, 90 degree nadir. This is a flat game cartographic map sheet, not a landscape painting or a diorama.

Preserve river course, village locations, road relationships and label locations from reference. Preserve restrained olive green vegetation, gray beige ground, muted slate blue river, cream labels. Landscape 3:2, edge-to-edge map only.

CRITICAL transformation: completely REMOVE every pictorial mountain peak, rocky cliff face, upright conifer/tree silhouette, front-facing cave entrance and building facade. Replace mountains with continuous nested topographic contour curves and delicate low-contrast relief shading seen vertically from overhead. All landforms are plan-view footprints with branching ridgelines and valley contour shapes. No object extends upwards on page to conceal terrain behind it. Forests are flat textured dark green area fills with small irregular circular canopy crowns viewed directly from above; absolutely no triangular Christmas-tree symbols or trunks. Buildings are tiny rectangular rooftop footprints viewed directly from above, with no visible walls. Far and near buildings equal scale. Cave shown only by tiny dark ground mark adjacent to its label, never a facade. Roads and rivers are flat uniformly cartographic shapes. No cinematic lighting, cast shadows, depth of field, distance haze, horizon, vanishing point, foreshortening, tilted plane, 3D raised relief, isometric perspective, illustrated mountain icons or oblique overhead view. The contours and land cover should visually read like an elegant modern topographic atlas with muted game-art textures.

Keep exact Korean place labels, small readable type: "벨레스", "금빛 안개 삼림", "동굴 입구", "테른 강", "거목돌봄마을", "목재마을", "보리마을". Veles central-right west bank, grove hamlet northeast, lumber village west, barley village southwest. No additional places. No UI, frame, border, compass, scale bar, grid, markers, player arrows, legend, buttons. This is the same expanded region converted to a strict vertical plan view.
```


사용자가 고른 첫 지도 표현을 바탕으로 벨레스 주변까지 범위를 넓힌 시안이다. [기존 지역 지도](Images/veles-map-only-original-style-v1.png)의 올리브색 숲·회갈색 지형·청회색 강을 참조하고 UI 없이 제작했다. 상위 문서는 [세계지도 시안](../EtherWorldMap.md)이다.

![벨레스 주변 생활권 지도](Images/veles-wider-region-original-style-v1.png)

## 확장한 범위

[정착망](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-ignis-settlement-network.md)의 벨레스 V01과 거목돌봄마을 V02·목재마을 V04·보리마을 V03을 연결했다. 북동쪽의 돌봄 마을, 서쪽의 목재 마을, 남서쪽의 농촌이라는 관계와 본류의 상류·하류 방향을 표현한다. 로덴과 중앙 세계수는 이 지도 범위 밖이다. 이웃 마을을 첫 장의 방문 장소로 추가한 것은 아니다.

## 화면 확인과 한계

기본 제공 `image_gen`으로 기존 지도 한 장을 참조해 생성하고 새 PNG로 저장했다. 작성자가 벨레스와 북쪽 숲·동굴, 남서쪽 강 굽이, 이웃 마을 세 곳의 대략적인 방향, UI 부재를 확인했다. 강변 마을과 길의 큰 관계는 읽히지만 생성 결과의 동굴·산지 표현과 거리 비율은 과장되어 있다.

입력의 약 28×18km 범위는 구도 지시였으며 출력에서 해당 축척을 검증하지 않았다. 기존 설계의 동굴까지 1.46km 보행선이나 마을 좌표를 이미지 픽셀로 측정하면 안 된다. 새 지류·도로·등고선·밭·건물은 시각 제안이고 실제 수계·고도·교량·통행 검증은 하지 않았다. 표현 방식을 넓은 지역에 적용한 비교용 결과이며 최종 지형 데이터가 아니다.

## 생성 입력문

```text
Use case: style-transfer and regional map expansion.
Create ONE wider-region MAP ONLY, using the supplied Veles map as BOTH the authoritative visual style reference and the local geography to preserve. Zoom OUT substantially, showing surrounding valleys and three nearby small villages. High-detail landscape image, approximately 3:2 or wide landscape aspect. Full canvas map, no UI whatsoever.

STYLE LOCK: preserve the reference's mature muted olive and deep green forest, gray-beige earth and rocky contours, subdued slate-teal river, detailed fine ink topographic strokes, subtle naturalistic painted top-down canopy texture, pale thin dirt tracks, dark small roof/building footprints, quiet off-white Korean cartographic labels. Same restrained contrast and saturation. NOT bright cartoon, not sepia antique parchment, not monochrome, not 3D isometric mobile-game terrain, no perspective mountains, no dramatic lighting. Strict overhead cartographic terrain illustration.

GEOGRAPHIC COMPOSITION: North up. Expanded design window roughly 28 km east-west by 18 km north-south, relative Veles coordinates x=-16..12km,y=-10..8km (these numbers are composition instructions, NEVER display them on image). Veles at (0,0), about 57% across and44% down the entire frame. Reference map covers only a SMALL LOCAL PATCH around Veles, about one-eighth of the expanded width; render the village with appropriately small footprints, not a giant town. Preserve recognizable local pattern: village on northwest bank of river, small quay, its same-bank northward forest path leading to a cave about1.25km north and0.26km east, golden-mist woodland there, wooded oval ridge about0.65km southwest, broad local river entering from northeast and wrapping east and then south around that ridge before leaving toward southwest. Do not flip or rotate that local patch. Do not make the cave a huge landmark, city or mountain.
Continue Tern river northeast/upstream into a distant high forest valley, then from Veles downstream southwest through (relative km)(-5,-5), then a southerly section toward(-4,-15) beyond the BOTTOM edge. Thus main river leaves bottom near x=-4km, it does not go directly through the barley village. Smooth plausible basin drainage with a few smaller tributaries joining main river downstream, no impossible river splitting over ridges, no lakes required. Surrounding terrain: forested high ridges and tributary valleys north and east, lower managed woodland to west, modest cultivated terraces southwest. Preserve wooded wilderness and coherent continuous landforms.

ONLY THREE NEW SMALL SETTLEMENTS, based on existing design:
1. "거목돌봄마을" at (+4,+6)km, northeast of Veles, upper-right part, a tiny grove-care hamlet at a forest clearing near an ordinary local grove. No gigantic world tree.
2. "목재마을" at (-7,-2)km, west and slightly south of Veles, small sawyard/storage clearings in managed forest with a modest cluster of buildings. Branch dirt road to Veles and downstream valley.
3. "보리마을" at (-10,-8)km, southwest quadrant near lower-left, small farming village among modest barley strips on a low terrace; seasonal stream drainage toward main valley. All hamlets comparable or smaller than Veles. No additional named settlements, no castles, no city walls or urban sprawl.

Readable sparse Korean labels only: "벨레스", "금빛 안개 삼림", "동굴 입구" (very small near local cave), "테른 강", "거목돌봄마을", "목재마을", "보리마을". Omit individual workshop/waterpoint/quay labels at this reduced scale. Maintain hierarchy: place names legible but no oversized headings. No numbers, no region title, no border, no compass, no scalebar, no coordinate grid, no legend, no location pins, no marker circles, no player arrow, no objective, no routes highlighted with glow, no control prompts. Do not show Roden or the central World Tree: they are far outside this window. Broader geography is a coherent extension of the reference, not a different village map.
```
