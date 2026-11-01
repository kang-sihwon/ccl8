# 벨레스 인게임 지도 UI 시안

[결정 34](../../decisions/0034-concept-and-ingame-map-roles.md)에 따라 벨레스와 첫 장 지역을 평면 도면형 지도 UI로 표현한 첫 시안이다. 현재 위치와 동굴 목적지가 표시된 화면을 가정했다. 최종 아트·탐색 공개 정책·UI 기능을 확정하거나 게임에서 실행한 결과는 아니다. 상위 문서는 [세계지도 시안](../EtherWorldMap.md)이다.

![벨레스 인게임 지도 UI 시안](Images/veles-ingame-map-ui-v1.png)

## 표현과 공간

지형의 윤곽선과 낮은 채도의 숲·물 색을 바탕으로 건물의 평면 윤곽, 흙길, 나루를 구분한다. 상단에는 지역명과 방위·닫기, 하단에는 현재 위치·목적지·주요 장소 범례와 입력 안내를 둔다. 입력 안내는 제안한 외형이며 동작하는 UI가 아니다.

[벨레스 지역 배치](../../Narrative/Drafts/2026-10-31-ether-taiga-dawn/2026-11-01-veles-region-layout.md)와 [제작용 배치도](Images/veles-first-chapter-layout-v2.png)를 공간 참조로 사용했다. 북쪽 숲과 동굴, 남쪽 강변 마을, 남서쪽 능선 바깥을 도는 본류라는 큰 관계를 유지한다. 동굴 내부·전투 지점·보스 이름·출항 원경 공개 지점은 표시하지 않았다.

## 화면 확인과 한계

작성자가 생성 화면에서 다음을 확인했다.

- 벨레스·공방·급수대·나루와 북쪽 숲 입구·동굴 입구의 이름을 구별할 수 있다.
- 마을에서 동굴로 이어지는 주 탐색 길이 본류의 같은 강안에 있다. 본류는 남서쪽 능선 바깥을 돌아 화면 왼쪽 아래로 나간다.
- 청록색 현재 위치, 금색 목적지와 흰색 주요 장소 표식이 지형과 구분된다.
- 입체 도시 모형 중심의 대륙 개념도 대신 위에서 읽는 지형 표현을 사용했다.

생성 도구가 더한 작은 지류·섬·샛길·집 수·밭·등고선은 시각 제안이며 세계관과 실제 레벨의 확정 자료가 아니다. 식수용 별도 물길과 저장·분배 시설의 정확한 접속은 이 화면에서 검증하지 않았다. 지형선은 측정 고도를 나타내지 않으며 기존 설계 좌표·거리의 픽셀 정합도 검사하지 않았다. 최종 제작에서는 실제 지형과 시설 배치에 맞춘 지도 바탕과 UI 표식을 분리해야 한다.

## 제작 입력

2026-11-01 기본 제공 `image_gen` 도구로 생성했다. 참조 이미지는 제작용 배치도 한 장이며 표현 스타일이 아닌 공간 관계의 참조다. 생성 원본을 프로젝트의 새 PNG로 복사했고 SHA-256 일치를 확인했다. 외부 게임의 스크린샷이나 UI 에셋은 입력하지 않았다.

최종 입력문은 다음과 같다.

```text
Use case: ui-mockup.
Create one polished widescreen 16:9 Korean PC fantasy RPG IN-GAME REGION MAP SCREEN, Veles and its first chapter forest region. The supplied image is ONLY a spatial layout reference, NOT a style reference. Replace the technical diagram with a beautiful functional flat top-down cartographic game map. Aim for mature premium open-world RPG map legibility, fine engraved terrain linework and subdued colored landcover. Absolutely no isometric toy towns, tilted perspective, 3D miniature mountains, pictorial tree silhouettes, diorama, mobile strategy game style, satellite photo, or concept landscape painting.

Composition: almost full-screen continuous map, subtle thin dark desaturated charcoal interface bars, map occupies 88% of image. Restrained warm ivory/gray map ground, sage/olive woodland masses, fine brown topographic contour strokes, muted blue-green river, pale ochre walking tracks, crisp small charcoal rectangular building FOOTPRINTS. Terrain is seen vertically overhead, with subtle relief bands, not perspective. Elegant understated cream and brass UI accents, sharp legible Korean typography. No weather effects or cinematic lighting.

Spatial constraints: north is up. Frame covers roughly X -1500 to +1200 m, Y -1200 to +1700 m relative to Veles quay (0,0). Village lies in the lower-middle/right half of map, forest north of it, cave at far northern end of forest trail, and downstream bend southwest. The broad river enters from east/northeast at (800,1000), flows via (500,600),(180,160) to quay (0,0), then bends west to (-300,-150), south to (-320,-900), southwest to (-1000,-1000), and exits lower-left. Draw natural smooth banks along this route. An oval wooded ridge centered (-650,-650) radius220m lies WEST of the south-running river and NORTH of its downstream bend; river stays outside the ridge. Small village housing terraces belong on the NORTHWEST bank near the quay, with workshop (60,110), public water point(-20,90), distribution tank(45,175), and a wooden jetty extending a short distance into the river at(0,0). Do not put buildings in water. No river bridge is needed. Housing footprints and a few garden plots imply a modest repair village, not a sprawling city.

A continuous WALKABLE dirt trail follows the SAME northwest bank from village north through forest gate(90,260), (180,440), (80,740), (220,1040), to cave entrance(260,1250). It must never cross the broad river. Forest is terrain texture masses and fine stippling. Parallel to the forest trail is a thin separate engineered spring watercourse, fed from local tree/spring area(320,1420) and flowing south to village tank and public water point. It is distinct from the broad navigable river. At north end, show cave entrance icon against rocky contour bands and a small overhead circular grove footprint above it. No giant world tree here, it is 209km away. The small local grove is not a new city.

Exact visible Korean text, few clean labels only: top-left small breadcrumb "이그니스 타이가", larger title "벨레스 일대". Map labels "벨레스", "금빛 안개 삼림", "테른 강", "공방", "급수대", "나루", "숲 입구", "동굴 입구". Labels avoid colliding with icons. A cyan triangular player marker at village workshop and ONE muted gold objective diamond at cave entrance. Do NOT draw a glowing GPS quest line, numbered combat nodes, monster names, boss spoilers, departure reveal labels, distance statistics, debugging coordinates, giant world-tree marker, teleport points, collectible spam or unrelated settlements.
Top right a minimal north arrow "N" and close glyph. Bottom unobtrusive legend: cyan triangle "현재 위치", gold diamond "목적지", hollow circle "주요 장소". Bottom-right input hints "휠 확대·축소" and "Esc 닫기". No giant side panels. This must look like an actual opened game map screen with believable cartographic landforms and a sparse readable interface, not a map design report.
```
