# 에테르 세계지도 생성 프롬프트

[세계지도 시안](../EtherWorldMap.md)에 사용한 2026-11-01 기본 제공 `image_gen` 입력 원문이다. 보정 입력은 첫 생성물을 참조했다. 이 기록은 실제 입력문을 보존하며 수치 준수 판정을 뜻하지 않는다.

## 첫 생성

```text
Use case: infographic-diagram / fantasy cartography.
Create ONE new clean 2:1 landscape world map for the fantasy RPG Ether. This is a geographic layout concept, not a decorative illustration. Full rectangular equirectangular Plate Carree map: longitude -180 to +180, latitude +90 to -90, north up. The entire canvas is the map, no margins. Pale blue-gray oceans, warm ivory land, thin crisp muted blue-gray coastlines. Simple flat atlas aesthetic without terrain texture.

CRITICAL COMPOSITION: exactly FIVE separate continents. Place the following coastlines within the following bounding boxes. Coordinates are percentages of full canvas width and height measured from upper-left. Respect BOTH the centers AND sizes; do not recompose for visual balance.
1 IGNIS TAIGA: bounding box x36.92–63.08%, y6.94–31.94%. A LARGE northern continent, about 26% of full image width and 25% of its height; since canvas is 2:1, its bounding box is about twice as wide in pixels as tall. Fixed small rust-red cross at (50.00%,19.44%). Its land target is 20 million square km. Do not draw it as a tiny island! Broad connected irregular landmass, naturally indented northern coast, a conspicuous broad south-facing bay with a small estuary. Occupy about 68% of the available box with land, reaching each bounding edge.
2 ARVENA: bounding box x7.04–34.63%, y21.11–51.11%. Fixed small cross (20.83%,36.11%). A wide western continent with multiple distinct east-facing bays and peninsulas. About 69% occupancy of its box. Target 40 million square km.
3 ORDAS: bounding box x18.21–45.68%, y53.19–85.69%. Fixed small cross (31.94%,69.44%). A southwestern continent broadening inland toward the south. About 63% occupancy of box. Target 35 million square km. ESSENTIAL: in its northeast, an exterior bay leads through a narrow but visibly OPEN strait into a large interior sea enclosed by land on most sides. Make the blue water passage continuous and plainly visible. It is NOT a closed lake, and NOT merely an open coastal bite. Keep the surrounding land connected.
4 SERADION: bounding box x68.14–95.75%, y20.28–85.28%. Fixed small cross (81.94%,52.78%). A large elongated eastern continent spanning the equator with a broad continuous interior. About 61% occupancy of its box, target 73.25 million square km. No neighboring decorative island clouds.
5 NIVER: a SINGLE continuous Antarctic landmass along the ENTIRE BOTTOM of the map. Its irregular coast must remain at y89–92%. Everything below y92% through the bottom y100% is connected land. Pale icy white fill with defined coastline; this is real land under snow and ice, NOT floating sea ice. Target 10 million square km. Left and right ends of its coast both meet y90.5% with almost horizontal tangents so they join smoothly when the map wraps. No polar-center cross. The NORTH pole is OCEAN, with at most faint sea ice tint, not another landmass.

The spherical planet circumference is 40,000km. Actual spherical land area target is 35%, including associated small islands, not a 35% flat pixel occupancy quota. Use the precise spatial layout above as the main instruction. Natural coastlines have readable bays and capes, avoid excessive tiny ragged edges. No bounding rectangles visible. All five continents must remain completely separated by water, including the narrow ocean gap between continents 2 and 3 and the continuous southern ocean separating 3/4 from 5. Do not make land bridges. Omit optional small islands to prioritize readability.

GRID: exactly twelve equal divisions horizontally and twelve vertically: eleven interior vertical longitude lines at x 8.333,16.667,25,33.333,41.667,50,58.333,66.667,75,83.333,91.667%; eleven interior horizontal latitude lines at the same y percentages. Very thin low-contrast lines continue through land and water. The central x50 meridian and y50 equator are a little stronger. Grid cells are 2:1 rectangles, not squares.

ONLY IMAGE TEXT: numerals "1", "2", "3", "4", "5", each exactly once inside its corresponding continent, set in restrained dark sans serif. Numerals are separate from the four small rust-red reference crosses. No continent names, heading, legend, axis numbers, scale bar, compass or annotations inside image.

Draw exactly three thin muted ochre dotted sea connections: 1 SOUTH BAY to 2 EAST COAST; 2 SOUTHEAST COAST to 3 NORTH COAST; 1 SOUTH BAY to 3 NORTHEAST OUTER BAY. Curves travel only through sea and end at shore; route 1–3 east of continent 2 and do not obscure the strait. No routes to 4 or 5. No ship or port symbols.
No mountains, trees, cities, roads, borders, world tree illustrations, buildings, ships, monsters, parchment texture, ornament, shaded relief, 3D, shadows or perspective. Deliver a crisp, restrained, easily evaluated geographic map with the northern continent visibly substantial.
```

## 위치·격자 보정

```text
Edit the attached Ether fantasy world map. Preserve the flat blue-gray ocean, ivory land palette, five continent identities, numerals 1–5, the three trade sea connections, and the northeast inland sea connected by an OPEN strait in continent 3. ONE targeted task: align the map geometry to the exact established layout. Keep canvas exactly 2:1 with the entire canvas being map area.

Replace the incorrect grid: exactly TWELVE equal columns and TWELVE equal rows. Thus ELEVEN internal vertical lines at x=8.333,16.667,25,33.333,41.667,50,58.333,66.667,75,83.333,91.667 percent; and ELEVEN internal horizontal lines at the same y percentages. Current image has too many columns. Central meridian x50 and equator y50 slightly stronger. No numeric coordinate labels.

Align each mainland and its small rust-red cross. All percentages are of the full image from upper-left:
1 mainland x36.92–63.08,y6.94–31.94; cross50.00,19.44. Shift the north coast DOWN away from the top edge to y6.94, and constrain its width within the required x bounds. Preserve the large southern bay. Ignis is still a substantial broad northern continent.
2 mainland x7.04–34.63,y21.11–51.11; cross20.83,36.11. Move current cross slightly right and up to its exact position.
3 mainland x18.21–45.68,y53.19–85.69; cross31.94,69.44. Move current cross slightly left and up. Keep the sea strait visibly open, do not close it into a lake.
4 mainland x68.14–95.75,y20.28–85.28; cross81.94,52.78.
5 Antarctic coastline y89–92 along full width, solid pale icy LAND below y92. Both edge coast heights y90.5 with horizontal tangents for seamless wrapping; do not draw isolated polar island or sea ice as land.

Maintain organic coastlines with extremes near their assigned bounds, not rectangular land. The bounding guides must remain invisible. Do not introduce new islands. Ensure dotted routes stay in water and end at the designated shores; no arrowheads, ships or extra routes. Keep numbers separate from crosses. No other text, decoration, relief, trees or cities. This is a numerical placement correction, not a new stylistic concept.
```
