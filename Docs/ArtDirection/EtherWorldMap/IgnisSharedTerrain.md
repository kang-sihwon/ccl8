# 이그니스 공통 지형 상세 배경

**이미지 제작은 종료하고 기존 결과를 실제 월드 제작용 참고 도안으로 보관한다.** [결정 55](../../decisions/0055-map-art-reference-handoff.md)에 따라 새 이미지 생성·보정과 이웃 구역 확장을 중단했다. 대륙 전체 상세 아트나 실제 UE 월드를 완성했다는 뜻은 아니다. 아래 제작본별 설명과 검사 기록은 보존 자료이며 추가 제작 지시가 아니다.

## 제작 도안 보관과 사용

벨레스 v5는 마지막 상세 제작본이고, 통일 v2 세 장은 채택한 화풍·UI 기준이다. v5의 모든 건물 형태·길 폭·지형을 확정 배치로 채택한 것은 아니다. 자료는 다음 순서로 읽는다.

| 자료 | 보관본 | 실제 월드 제작에서 쓸 부분 |
|---|---|---|
| 화풍·UI | [벨레스·이그니스·에테르 통일 v2](MapPresentation.md#최종-시안-v2) | 지표 색감, 시점, 글꼴·아이콘·지명 연결선의 표현 기준 |
| 대륙 지형 초안 | [이그니스 공통 원본 규격](Atlas/IgnisTerrainMaster-v1/manifest.json), [고도 설계안](Atlas/IgnisTerrainMaster-v1/terrain/height16.png) | 기존 해안·도시 위치와 수계·도로 배치의 기준. 수치 고도는 설계 후보이며 통행 가능한 지형으로 검증되지 않았다 |
| 벨레스 배치 초안 | [규격](Atlas/VelesDetailTerrain-v1/manifest.json), [건물·시설](Atlas/VelesDetailTerrain-v1/data/placements.json), [도로·하천·외곽 접속](Atlas/VelesDetailTerrain-v1/data/connections.json) | 블록아웃의 기준점, 건물 외곽과 출입구, 이동로와 인접 구역 연결을 검토하는 입력 |
| 벨레스 전체 상세 도안 | [v5 전체본](Atlas/VelesDetailArt-v5/previews/composite-1m.png), [경계 표시본](Atlas/VelesDetailArt-v5/previews/assembled-grid.png), [확대 뷰어](Atlas/VelesDetailArt-v5/index.html) | 마을·숲·물가·주변 접근 구간의 배치 관계와 항공사진풍 표현을 참고한다 |
| 건물·골목 확대 | [공방·창고](Atlas/VelesDetailArt-v5/assets/workshop-access.png), [서쪽 골목](Atlas/VelesDetailArt-v5/assets/west-access.png), [공방 배치선](Atlas/VelesDetailArt-v5/previews/workshop-access-geometry.png), [골목 배치선](Atlas/VelesDetailArt-v5/previews/west-access-geometry.png) | 건물 사이 공간·진입로의 시각적 참고. 그림과 좌표가 다르면 그림에 맞춰 좌표를 이동하지 않는다 |
| 다른 정착지 | [정착지 상세 도안 목록](IgnisSettlementDetails.md) | 지역별 구성 비교. 독립 생성 그림을 연속 타일이나 실제 월드로 취급하지 않는다 |

벨레스 전체본은 [v5 규격](Atlas/VelesDetailArt-v5/manifest.json)의 로컬 4,096×6,144m 범위를 담는다. 이 범위는 도안의 작업 사각형이며 실제 플레이 영역·이동 시간·최종 제작 규모를 확정하지 않는다. 확대 뷰어와 개별 확대 이미지로 작은 길과 건물 관계를 확인하고, 전체본만으로 모든 세부를 판단하지 않는다.

기존 [검사 기록](Atlas/VelesDetailArt-v5/records/verification.json)의 45개 통과 항목은 원본 보존·좌표 범위·타일 재조립과 경계 픽셀 일치에 관한 것이다. 실제 통행, 충돌, 건물 치수, 경사, 하천 횡단, UE 수입·실행은 검증하지 않았다. [육안 검토](Atlas/VelesDetailArt-v5/records/visual-review.json)에는 처마·길 가장자리의 불일치가 남아 있고, [정적 검사](Atlas/VelesDetailArt-v5/records/static-verification.json)는 v5 브라우저 조작을 검증하지 않는다. 이전 버전과 기록은 삭제하거나 재생성하지 않는다.

## 실제 월드 제작을 시작할 때

후속 제작의 출발점은 벨레스와 필요한 주변 연결 구간의 UE 블록아웃이다. 아직 착수하지 않았으며, 이번 정리로 구현 완료나 제작 착수를 뜻하지 않는다.

1. 보존한 좌표·고도·배치 초안을 읽고 실제 제작 범위를 정한다. 지형·건물·길을 배치한 뒤 캐릭터 크기, 출입구와 길 폭, 경사·이동 시간·전투 동선을 확인한다.
2. 그림의 지붕이나 수관 형태를 그대로 복제할 의무를 두지 않는다. 배치 변경이 필요하면 기존 해안·도시 기준 및 서사와의 영향을 검토하고, 원본을 보존한 별도 수정안으로 다룬다.
3. 실제 월드가 마련되면 그 지형·건물·길에서 지도용 바탕을 추출·가공한다. 채택한 화풍·UI 기준을 적용하되 구현한 공간과 일치하는지 먼저 확인한다.
4. 지명·아이콘·플레이어 위치를 배경 그림과 분리해 실제 좌표에 연결한다. 게임 화면에서 확대 가독성과 위치 일치를 검증한다. 추출 방식·타일 해상도·갱신 주기는 후속 구현에서 정한다.

## 보존한 제작 결과

이그니스의 기존 좌표를 유지하면서 공통 지형 데이터와 재질·식생으로 상세 배경을 만드는 방식을 사용한다. 사용자가 선택한 근거는 [결정 50](../../decisions/0050-shared-coordinate-map-terrain.md)다. **대륙 전체의 수치 고도 원본과 도로·하천·정착지 배치 데이터를 제작했다.** 광역 지형 설계 후보이며 건물·골목 수준의 전체 상세 도안이나 최종 아트 승인을 뜻하지 않는다. 길목 V14와 케르딘 T11 사이의 재질 시제품도 별도 비교 자료로 유지한다.

## 벨레스와 주변 연결 구간

### 중앙 외곽과 출입구 확대 보정 v5

**중앙 동·서쪽의 낮은 상세도 구간을 채우고 공방·창고 진입로와 서쪽 골목을 확대 보정했다.** [서쪽 외곽 비교](Atlas/VelesDetailArt-v5/previews/west-fill-comparison.png), [동쪽 외곽 비교](Atlas/VelesDetailArt-v5/previews/east-fill-comparison.png), [공방·창고 비교](Atlas/VelesDetailArt-v5/previews/workshop-access-comparison.png), [서쪽 골목 비교](Atlas/VelesDetailArt-v5/previews/west-access-comparison.png)는 왼쪽 v4, 오른쪽 v5다. [확대용 전체본](Atlas/VelesDetailArt-v5/previews/composite-1m.png)과 [경계 표시 축소본](Atlas/VelesDetailArt-v5/previews/assembled-grid.png)을 함께 둔다.

상세 이미지 사각형의 합집합이 벨레스 작업 영역 **4,096×6,144m, 약 25.17km² 전체**를 덮는다. 이그니스 대륙 전체를 완성했다는 뜻은 아니다. 접합을 위해 보존한 바깥 띠와 이미지 혼합 구간이 포함되므로 모든 픽셀의 상세도가 같지는 않다. [규격](Atlas/VelesDetailArt-v5/manifest.json) 기준 새 외곽 이미지 두 장은 약 0.78m/px, 공방·창고와 서쪽 골목은 각각 약 0.136m/px와 0.174m/px다. 확대 구역의 세부는 새로 생성한 제작 후보이며 측량 정보의 복원 결과가 아니다. 1m/px 전체 출력보다 세밀한 부분은 개별 확대 이미지나 뷰어로 확인한다.

공방의 기존 4m 길이·2.2m 폭 연결로와 창고의 대각선 진입로가 확대 화면에서 이어진다. [공방 배치선](Atlas/VelesDetailArt-v5/previews/workshop-access-geometry.png)과 [골목 배치선](Atlas/VelesDetailArt-v5/previews/west-access-geometry.png)으로 기존 외곽·도로와 대조한다. 일부 처마와 길 가장자리의 차이는 남아 있어 [육안 검토](Atlas/VelesDetailArt-v5/records/visual-review.json)는 `PARTIAL_PIXEL_REGISTRATION`이다. 그림에 맞춰 원본 건물·도로 좌표를 이동하지 않았다.

[독립 검사](Atlas/VelesDetailArt-v5/records/verification.json) **45개를 통과했다.** v1 원본 파일 5개와 v4 참조 이미지 23개의 해시가 유지됐다. 수정한 네 구역 바깥은 샘플링 여백 2px를 제외하고 기존 픽셀과 같으며, 전체본의 외곽 6px와 축소본의 외곽 3px도 동일하다. [24개 타일](Atlas/VelesDetailArt-v5/tiles.json) 본체의 재조립과 인접 여백 38곳은 정확히 일치한다.

[뷰어](Atlas/VelesDetailArt-v5/index.html)의 `상세 구역`에서 공방·창고, 서쪽 골목, 동·서쪽 외곽으로 이동하도록 연결했다. 정적 구문·이미지 참조 검사는 통과했으며 실제 브라우저 조작과 UE는 미검증이다. [생성 입력문](Atlas/VelesDetailArt-v5/records/generation.json)을 보관한다. 재출력은 `python Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailArt-v5/build.py`, 검사는 같은 폴더의 `verify.py`를 실행한다. Pillow와 보존된 v4 조립 스크립트·원본을 사용한다.

### 북쪽·남쪽 확장과 길 윤곽 보정 v4

**북쪽 6개·남쪽 9개 상세 이미지와 마을·숲길 보정본을 기존 좌표에 조립했다.** [북쪽 비교](Atlas/VelesDetailArt-v4/previews/north-comparison.png), [남쪽 비교](Atlas/VelesDetailArt-v4/previews/south-comparison.png), [마을 길 비교](Atlas/VelesDetailArt-v4/previews/village-comparison.png), [숲길 비교](Atlas/VelesDetailArt-v4/previews/forest-roads-comparison.png)는 왼쪽 v3, 오른쪽 v4다. [확대용 전체 조립본](Atlas/VelesDetailArt-v4/previews/composite-1m.png)은 **4,096×6,144px, 1m/px**이며, [경계 표시 축소본](Atlas/VelesDetailArt-v4/previews/assembled-grid.png)은 2m/px다. 이전 시안은 보존한다.

새 지형 15장의 실제 이미지 간격은 [규격](Atlas/VelesDetailArt-v4/manifest.json) 기준 약 **1.01m/px부터 1.13m/px**로, v3 숲 양옆의 1.03m/px에 가까운 상세도다. 1m/px 전체 출력이 모든 구역에 새로운 1m급 정보를 보장하지는 않는다. 상세 이미지 사각형의 합집합은 4.64km²에서 약 **24.18km²**로 늘었다. 혼합 영역과 보존한 외곽 띠를 포함하며, 중앙 동쪽·서쪽의 바깥 띠에는 광역 배경이 남아 있다.

도로 중심선·폭과 건물 외곽을 가이드로 사용했다. [도로 비교 1](Atlas/VelesDetailArt-v4/previews/roads-audit-1.png), [2](Atlas/VelesDetailArt-v4/previews/roads-audit-2.png), [3](Atlas/VelesDetailArt-v4/previews/roads-audit-3.png), [4](Atlas/VelesDetailArt-v4/previews/roads-audit-4.png), [5](Atlas/VelesDetailArt-v4/previews/roads-audit-5.png), [6](Atlas/VelesDetailArt-v4/previews/roads-audit-6.png)에 기존 103개 도로 항목을 담았다. 출입구 연결로가 더 분명해졌고 숲길도 별도로 보정했다. 공방 앞의 짧은 연결로처럼 희미한 곳과 일부 길 가장자리·지붕 윤곽 차이는 남아 있다. [육안 검토](Atlas/VelesDetailArt-v4/records/visual-review.json)는 그림의 정합을 `PARTIAL`로 판정하며 모든 길 폭의 수치 일치를 주장하지 않는다.

[독립 검사](Atlas/VelesDetailArt-v4/records/verification.json) **43개를 통과했다.** 원본 좌표·고도 파일 5개의 해시를 유지했고, 전체 범위와 기준점도 같다. 2m/px 축소본의 바깥 3px 띠는 v3와 일치한다. [24개 아트 타일](Atlas/VelesDetailArt-v4/tiles.json)은 각 1,024m 구역의 1,024×1,024px 본체와 최대 사방 8px 여백을 가진다. 저장된 타일의 본체를 재조립하면 확대용 전체본과 정확히 같고, 인접 여백 38곳도 동일하다. 이는 공통 합성 화면의 분할 검사이며 이웃 마을의 신규 상세 지형이나 모든 그림 요소의 측량 검증은 아니다.

[생성 입력문](Atlas/VelesDetailArt-v4/records/generation.json)과 [숲길 추가 보정](Atlas/VelesDetailArt-v4/records/forest-road-generation.json)을 보관한다. 자연 암석을 유적처럼 그린 첫 남서쪽 시안은 제외했다. 기존에 없던 길·밭이 생긴 동남쪽 시안도 [수정 기록](Atlas/VelesDetailArt-v4/records/correction.json)에 따라 교체했다. 실제 선택 파일은 `manifest.json`을 따른다.

[뷰어](Atlas/VelesDetailArt-v4/index.html)에 북쪽·남쪽 이동 버튼과 기존 배치선·경계 표시를 연결했다. `좌표 원본`은 v1을 보여 준다. JavaScript 구문과 참조 이미지 검사는 수행했지만 v4의 실제 브라우저 조작과 UE 실행은 미검증이다. 재출력은 `python Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailArt-v4/assemble.py`, 검사는 같은 폴더의 `verify.py`를 실행한다. Pillow가 필요하며 저장된 생성 이미지를 사용한다. 기존 도로·하천·정착지 좌표를 변경하지 않는다.

### 건물 외곽 보정과 숲 양옆 확장 v3

**벨레스의 건물 외곽을 기준으로 지붕 위치를 보정하고, 기존 상세 숲의 서쪽·동쪽을 같은 항공사진 화풍으로 확장했다.** [마을 비교](Atlas/VelesDetailArt-v3/previews/village-comparison.png)와 [숲 비교](Atlas/VelesDetailArt-v3/previews/forest-comparison.png)는 왼쪽 v2, 오른쪽 v3다. [마을 확대](Atlas/VelesDetailArt-v3/previews/village.png), [경계 표시 조립본](Atlas/VelesDetailArt-v3/previews/assembled-grid.png), [뷰어](Atlas/VelesDetailArt-v3/index.html)를 함께 둔다. 기존 시안과 수치 원본은 보존한다.

건물 86개의 외곽선과 도로 중심선을 생성 입력에 표시했다. [외곽 비교 1](Atlas/VelesDetailArt-v3/previews/roof-audit-1.png), [2](Atlas/VelesDetailArt-v3/previews/roof-audit-2.png), [3](Atlas/VelesDetailArt-v3/previews/roof-audit-3.png), [4](Atlas/VelesDetailArt-v3/previews/roof-audit-4.png)에서 각 건물의 v2·v3와 청록색 기준선을 대조했다. 86개 위치의 지붕을 육안으로 확인했고 외곽 정합은 전반적으로 개선됐다. 일부 처마·그림자·길 가장자리의 차이는 남아 있어 시각적 정합은 계속 `PARTIAL`이다. 도로 103개 전체의 그림 정합이나 생성 그림의 총 건물 수를 자동 검증한 결과는 아니다.

새 서쪽 영역은 로컬 좌표 `[-1500,150,-300,1550]m`, 동쪽은 `[300,150,1500,1550]m`다. 각각 1,161×1,355px로 약 1.03m/px의 실제 이미지 정보를 가진다. [규격](Atlas/VelesDetailArt-v3/manifest.json)의 상세 이미지 사각형 합집합은 **2.12km²에서 4.64km²**로 늘었다. 가장자리 혼합 구간을 포함한 면적이며 모든 픽셀의 선명도가 같다는 뜻은 아니다. 북쪽·남쪽과 나머지 외곽에는 4m/px 광역 배경이 남아 있다. [생성 입력문](Atlas/VelesDetailArt-v3/records/generation.json)을 보관한다.

[독립 검사](Atlas/VelesDetailArt-v3/records/verification.json) 18개를 통과했다. 이전에 기록한 원본 파일 5개의 해시와 좌표·고도를 유지했고, 바깥 3px 띠가 기존 원본과 같다. 이미 저장된 [24개 아트 타일](Atlas/VelesDetailArt-v3/tiles.json)을 다시 읽어 조립하면 공통 합성 화면과 일치하며, 인접 여백 38곳도 픽셀 단위로 같다. 동일 합성 화면에서 자른 타일의 연속성 검사이며, 그림 속 모든 도로·건물이 수치 외곽과 일치한다는 증거는 아니다.

v3 뷰어의 `좌표 원본`은 절차적으로 렌더링한 v1과 비교한다. v2와의 비교는 위 비교 이미지를 사용한다. v3는 정적 JavaScript 검사와 브라우저를 쓰지 않는 이미지 조립·검사를 수행했다. v3의 실제 브라우저 조작과 UE 실행은 미검증이다.

재출력은 `node Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailArt-v3/prepare.cjs`, `python Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailArt-v3/assemble.py`, `python Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailArt-v3/verify.py` 순서다. Python에는 Pillow가 필요하다. 새 이미지를 생성하지 않고 저장된 아트 원본을 고정 좌표에 조립한다. 다음 작업은 남은 길 윤곽과 북쪽·남쪽의 상세도 차이를 줄이는 일이다.

### 항공사진 화풍 보정 v2

사용자가 v1의 반복 지붕·수관 표현을 지적해, 채택한 벨레스 v2를 참조한 **항공사진 화풍 보정 후보**를 만들었다. [보정 뷰어](Atlas/VelesDetailArt-v2/index.html)의 `수정 전`, `배치선`, `경계`로 원본과 대조한다. [마을 확대](Atlas/VelesDetailArt-v2/previews/village.png), [숲·동굴 연결로](Atlas/VelesDetailArt-v2/previews/forest.png), [전체 조립본](Atlas/VelesDetailArt-v2/previews/assembled-grid.png)을 함께 둔다. v1의 수치 원본과 타일은 보존한다.

지붕의 마모·수리 흔적과 마당·텃밭·흙길을 개별적으로 표현하고, 식생은 겹치는 수관과 밀도가 다른 군락으로 보정했다. 내장 `image_gen`을 사용한 [생성 입력문](Atlas/VelesDetailArt-v2/records/generation.json)과 [광역 수관 축척 보정](Atlas/VelesDetailArt-v2/records/region-refinement.json)을 보관한다. 마을·숲의 확대 보정본과 광역 배경을 동일한 전역 좌표에 놓고 겹치는 가장자리를 혼합한다. UI·표식은 기존 좌표에서 별도로 그린다.

**보존한 것은 수치 좌표·고도·분할 경계이며, 생성된 모든 지붕 윤곽의 정확한 일치까지 완료한 것은 아니다.** [배치선 비교](Atlas/VelesDetailArt-v2/previews/geometry.png)에서 일부 지붕 윤곽과 길 가장자리의 어긋남이 남아 있다. [검사 기록](Atlas/VelesDetailArt-v2/records/verification.json)의 수치·조립 상태는 `PASS`, 시각적 좌표 정합은 `PARTIAL`이다. 건물 배치의 기준은 계속 v1의 외곽·출입구 데이터다. 생성 그림으로 좌표나 건물 수를 다시 정의하지 않는다.

[규격](Atlas/VelesDetailArt-v2/manifest.json)에 기록한 원본 파일 5개의 해시가 유지됐고, 합성 화면의 바깥 3px 띠는 기존 조립본과 동일하다. [아트 타일 24개](Atlas/VelesDetailArt-v2/tiles.json)는 하나의 공통 합성 화면을 같은 1,024m 구역으로 잘랐다. 38개 중첩 여백은 픽셀 단위로 일치하고, 타일을 다시 붙이면 합성 화면과 정확히 같다. 이 검사는 상세 그림과 원본 건물 외곽의 일치를 대신하지 않는다. 수치·조립 검사 9개와 실제 확대·이동·비교 버튼을 포함한 [뷰어 검사 7개](Atlas/VelesDetailArt-v2/records/browser-verification.json)를 통과했다.

보정 이미지의 실제 해상도는 광역 1,024×1,536px, 마을 1,573×1,000px, 숲 1,254×1,254px다. 각각 약 4m/px, 0.7m/px, 0.96m/px이며, 2m/px의 전체 조립 출력은 검토용이다. 확대할수록 광역 배경과 상세 보정 영역 사이의 선명도 차이가 보인다. v1의 전체 0.5m/px 렌더링을 같은 밀도의 사진 정보로 교체한 결과는 아니다. 다음 보정은 일부 지붕·길 윤곽의 정합과 상세 영역 밖의 해상도 차이를 줄이는 일이다.

재출력은 `node Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailArt-v2/prepare.cjs`, `node Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailArt-v2/capture.cjs`, `python Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailArt-v2/verify.py` 순서다. 캡처는 포트 9448의 별도 Chromium 디버깅 세션을 사용하며 Python에는 Pillow가 필요하다. 생성된 아트 원본을 다시 만들지는 않는다.

### 좌표 기반 상세 원본 v1

**벨레스 중심부와 숲·동굴 접근로, 서쪽·북쪽 연결 구간을 공통 원본의 상세 자식 지형으로 제작했다.** [상세 뷰어](Atlas/VelesDetailTerrain-v1/index.html)에서 건물·골목, 숲·동굴, 서쪽 연결, 북쪽 연결을 선택하고 휠로 확대한다. 건물 외곽 버튼은 실제 배치 데이터의 외곽과 출입구를 보여 준다. 확정 아트가 아닌 상세 제작 후보이며, 이웃 마을 중심부는 이번 범위에 포함하지 않는다.

| 자료 | 확인할 것 |
|---|---|
| [벨레스 확대 화면](Atlas/VelesDetailTerrain-v1/previews/village.png), [건물 상세 출력](Atlas/VelesDetailTerrain-v1/previews/village-detail.png) | 지붕, 골목, 출입구, 공방·취수 지점·나루를 확인한다 |
| [숲·동굴 접근로](Atlas/VelesDetailTerrain-v1/previews/forest.png), [서쪽](Atlas/VelesDetailTerrain-v1/previews/west.png), [북쪽](Atlas/VelesDetailTerrain-v1/previews/north.png) | 기존 숲길과 신규 지역 연결로를 구분한다 |
| [24개 타일 조립본](Atlas/VelesDetailTerrain-v1/previews/assembled.png), [경계 표시 조립본](Atlas/VelesDetailTerrain-v1/previews/assembled-grid.png) | 실제 출력 타일의 여백을 제거하고 위치대로 조립한 축소본이다 |
| [규격과 경계](Atlas/VelesDetailTerrain-v1/manifest.json), [2m 고도 원본](Atlas/VelesDetailTerrain-v1/height/master16.png) | 좌표, 높이 저장 규칙, 분할 파일 목록을 읽는다 |
| [건물·시설 배치](Atlas/VelesDetailTerrain-v1/data/placements.json), [도로·하천 연결](Atlas/VelesDetailTerrain-v1/data/connections.json) | 건물 외곽·출입구·치수·방향과 도로 폭, 하천, 다음 구역의 접속 좌표를 읽는다 |
| [수치·접합 검사](Atlas/VelesDetailTerrain-v1/records/verification.json), [뷰어 검사](Atlas/VelesDetailTerrain-v1/records/browser-verification.json) | 배치·경계와 실제 휠·드래그 검사 결과를 읽는다 |

### 상세도와 배치

벨레스 기준점은 기존 투영 좌표 `[150000, 145000]m`다. 작업 범위는 이 기준점에서 X −2,048부터 2,048m, Y −2,560부터 3,584m로, **4,096×6,144m**다. 고도는 2m 간격의 2,049×3,073개 샘플이며, 높이 저장 단위는 부모와 같은 0.125m다. 1,024m 사각형을 4열×6행으로 나눠 24개 타일을 만든다. 각 고도 타일은 513×513개 샘플로 이웃과 한 줄을 공유한다.

컬러 타일은 0.5m/px로 직접 렌더링한 2,112×2,112px PNG다. 사방 32px 여백을 제외한 2,048×2,048px가 작업 영역이다. 조립은 이 영역만 써야 한다. 축소 조립본은 타일당 512px이며 원본 타일의 상세도를 대신하지 않는다. 컬러 픽셀 밀도와 2m 고도 간격은 별개다.

[배치 데이터](Atlas/VelesDetailTerrain-v1/data/placements.json)에는 건물 86개, 교량 7개와 기존 장소 기준점 15개를 담았다. 도로 자료의 103개 항목에는 건물 출입구 연결 86개가 포함된다. 건물 외곽·골목·밭·지역 진입로의 형태와 치수는 새 후보이며, 이전 생성 이미지의 건물을 추출한 결과가 아니다. 기존 숲길 중심선과 교량 위치 4곳, 공방·급수·나루·동굴 등의 기준점은 보존했다. 지하의 옛 작업장과 보스 위치는 지상 건물로 바꾸지 않았다.

지붕·수관은 [내장 이미지 도구의 재질 생성 기록](Atlas/VelesDetailTerrain-v1/records/generation.json)을 바탕으로 좌표에 배치했다. 재질 원본은 실제 출력 크기 1,254×1,254px이며 기존 지도 그림을 확대하지 않았다. 131,740개의 수관 배치를 저장했고 지형·도로·건물에서 떨어지도록 배제 영역을 적용했다. 반복되는 지붕·식생과 각진 기존 숲길이 남아 있어, 채택한 항공사진 화풍의 최종 품질 승인을 뜻하지 않는다.

### 경계를 맞춘 범위

부모 격자의 내부 제어점 9개를 보존하고, 바깥 256m 구간에서 상세 고도의 추가 변화를 줄여 **외곽 고도를 부모 보간값과 일치**시켰다. 테른 강은 기존 벨레스 지역 중심선을 따르며 외곽의 두 접점에서 부모 H01의 중심선과 폭 180m에 맞춘다. 마을 부근의 강 폭 55m는 기존 시설 위치를 살리기 위한 상세 후보이고, 대륙 원본의 강 폭을 수정한 것은 아니다.

신규 도로는 숲곁·들보리·솔숨 방향의 접근 구간이다. [연결 데이터](Atlas/VelesDetailTerrain-v1/data/connections.json)의 `boundaries`에 도로 출구 3곳의 전역 좌표·폭·방향을 저장했다. 다음 자식 지형은 이 값을 그대로 이어받아야 한다. 타르넬 기준점은 주변 맥락으로 유지하지만 그 중심부까지 육로를 완성한 것은 아니다. 인접 마을 상세 지형이나 이전 독립 생성 그림과의 시각적 접합은 아직 검증하지 않았다.

독립 수치 검사 21개와 브라우저 검사 9개를 통과했다. 24개 고도 타일이 상세 원본의 해당 영역과 같고, 내부 경계 38곳의 고도 샘플은 모두 일치한다. 컬러 타일의 38개 중첩 구간은 최대 채널 차이 9/255, 구간별 평균 차이의 최댓값 약 0.0164/255로 검사 허용 범위 안이다. **고도 경계의 정확한 일치와 렌더링 픽셀의 미세한 차이를 구분한다.** 건물끼리 겹치지 않고 도로가 다른 건물을 관통하지 않으며, 모든 출입구가 골목에 연결됨을 확인했다. 게임 내 통행·충돌·교량 구조·배수·UE 수입은 미검증이다.

재생성은 `node Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailTerrain-v1/build.cjs`로 실행한다. 로컬 뷰어가 실제 배치 데이터를 렌더링하며, `review.cjs --export`는 포트 9447의 별도 Chromium 디버깅 세션에서 컬러 타일과 확대 화면을 저장한다. 이어 `python Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailTerrain-v1/verify.py`를 실행하면 수치·픽셀 검사와 실제 타일 조립본을 갱신한다. Python에는 Pillow가 필요하다.

## 대륙 공통 원본

[전체 원본 뷰어](Atlas/IgnisTerrainMaster-v1/index.html)는 기존 세계지도 투영으로 고도와 배치 데이터를 겹쳐 보여 준다. [전체 미리보기](Atlas/IgnisTerrainMaster-v1/previews/overview.png)는 같은 수치 고도에서 만든 검토 화면이다. 채택한 지도 화풍을 다시 생성한 아트가 아니며 기존 해안·도시 좌표를 바꾸지 않는다.

| 자료 | 용도 |
|---|---|
| [원본 규격](Atlas/IgnisTerrainMaster-v1/manifest.json) | 좌표계, 고도 복호화, 격자 범위, 분할 목록과 원본 해시를 읽는다 |
| [16비트 고도 원본](Atlas/IgnisTerrainMaster-v1/terrain/height16.png) | 대륙 전체의 높이를 수치로 읽는다. 일반 컬러 이미지로 변환하지 않는다 |
| [해안 데이터](Atlas/IgnisTerrainMaster-v1/data/coastline.json), [육지 마스크](Atlas/IgnisTerrainMaster-v1/terrain/land-mask.png) | 기존 해안 벡터의 6,156개 꼭짓점과 격자상 육지 범위를 구분한다 |
| [지형 제어점](Atlas/IgnisTerrainMaster-v1/data/terrain-controls.json) | 산맥 5개, 고개 2개와 생성 후보 값을 수정한다 |
| [하천](Atlas/IgnisTerrainMaster-v1/data/rivers.json) | 14개 중심선, 흐름 방향, 기존 하상 고도와 폭·수면 후보를 읽는다 |
| [육로](Atlas/IgnisTerrainMaster-v1/data/roads.json), [수상 이동](Atlas/IgnisTerrainMaster-v1/data/navigation.json) | 육로 회랑 20개와 수상 이동로 4개를 구분한다 |
| [정착지](Atlas/IgnisTerrainMaster-v1/data/settlements.json) | 36곳의 기준점, 기존 시설 후보 86곳, 항구와 세계수 주거 제한 범위를 읽는다 |
| [경로 그래프](Atlas/IgnisTerrainMaster-v1/data/route-graph.json), [교차 검토](Atlas/IgnisTerrainMaster-v1/data/interactions.json) | 기존 경유점 연결과 교량·나루·강변 배치가 필요한 접촉 구간을 찾는다 |
| [생물군계](Atlas/IgnisTerrainMaster-v1/terrain/biome.png), [식생 밀도](Atlas/IgnisTerrainMaster-v1/terrain/vegetation-density.png), [경사](Atlas/IgnisTerrainMaster-v1/terrain/slope.png) | 후속 재질·식생 배치에 사용할 광역 후보 마스크를 읽는다 |
| [수치·접합 검사](Atlas/IgnisTerrainMaster-v1/records/verification.json), [뷰어 검사](Atlas/IgnisTerrainMaster-v1/records/browser-verification.json) | 실제 검사한 범위와 미검증 범위를 구분한다 |

### 좌표와 고도 해석

고도 격자는 기존 가상 구면의 방위정거도법을 사용한다. 기준점은 경도 −1°, 위도 54°, 구면 반지름은 6,366.197723675814km다. X는 동쪽, Y는 북쪽, Z는 위쪽이며 단위는 미터다. 지구 WGS84 자료로 취급하지 않는다. 원본의 `xy_km`와 기존 세계지도 L10 픽셀 기준점도 보존한다.

격자는 **4,097×2,561개 샘플, 가로·세로 간격 2,000m**다. 북서쪽 샘플의 좌표는 `[-3412000, 2866000]m`이며 열은 동쪽, 행은 남쪽으로 증가한다. 전체 작업 사각형은 8,192×5,120km이고 바다와 정렬 여백을 포함한다. 이 사각형 면적을 대륙의 구면 면적으로 해석하지 않는다. 식은 [원본 규격](Atlas/IgnisTerrainMaster-v1/manifest.json)의 `grid`를 따른다.

`height16.png`는 컬러 명암을 고도로 추측한 결과가 아니라, 산맥·하천 제약과 절차적 패턴으로 계산한 수치 고도 설계안이다. 픽셀의 16비트 정수값을 `u`라고 하면 `고도(m) = u × 0.125 − 1024`다. 현재 육지 최고 고도 후보는 4,346.375m다. 0.125m는 저장 단위이고 공간 상세도는 2km다. 바다는 고도 0의 자리표시자이며 해저 지형은 없다. 육지 마스크를 함께 읽어야 한다.

40개 분할 파일은 8열×5행이며 각각 513×513개 샘플로 구성한다. 인접한 파일은 한 줄의 샘플을 공유한다. 재조립할 때 파일마다 512셀씩 진행하면 전체 원본과 정확히 일치한다. 이 분할은 이전 세계 픽셀 기반 L0·L10 타일을 대체하지 않는 **수치 고도용 작업 분할**이다. 뷰어는 공통 투영 변환으로 기존 세계지도 위치에 표시한다.

### 배치 데이터의 확정 범위

해안선과 모든 정착지·하천·이동로의 기존 좌표는 그대로 복사했다. 이름, ID, 기존 하상 고도와 시설 후보 위치도 유지한다. 산맥 고도, 계곡 폭, 도로 폭, 수면 높이와 식생 마스크는 새 제작 후보다. 기존 배치 자료 자체의 `design_candidate_not_canon` 상태도 유지하며 서사 정본을 수정하지 않는다.

육로의 중심선은 기존 **이동 회랑**이다. 주요 육로 폭 6m와 지선 4m, 테른 강 폭 180m와 지류 55m는 초기 후보이며 수치 고도 격자보다 작으므로 벡터로 보관한다. 강 위에 도로 메시를 바로 만들지 않는다. 수상 이동로는 도로에서 분리했고, 기존 경유점 그래프는 교차점에서 자동으로 갈라지지 않는다.

정착지의 `source_context_bounds_xy_km`는 기존 그림이 다룬 범위다. 건물 밀집지의 외곽이나 토목 평탄화 영역으로 취급하지 않는다. 시설 86곳은 기존 점 위치이며 건물의 폭·높이·방향·필지는 아직 없다. 세계수의 기존 주거 제한 반경 12km는 별도 배치 제약으로 보존한다.

하천은 기존 하상 고도 단면을 광역 지형에 맞추도록 보정했다. 4km의 중심선 보간 구역은 낮은 격자 밀도에서 높이를 맞추기 위한 계산 범위이고 실제 강 폭이 아니다. 호수 두 곳은 기존 중심점·상징 반경·수면만 보존했다. 기존 호수 수면과 해당 하상 값이 같고 새 수면 후보와도 차이가 있어, 실제 호안선과 수심은 후속 조정 대상이다.

생물군계는 고도·위치·하천 접근성으로 나눈 후보이며 기후나 생태 시뮬레이션이 아니다. 식생 밀도 0부터 255까지는 상대값이다. 세부 식생을 배치할 때 도로·수면·정착지 영역과 세계수 제한을 적용해야 한다. `slope.png`의 값에 `90/255`를 곱하면 광역 경사각이며 통행 가능 판정에는 쓰지 않는다.

### 검증과 남은 일

[독립 Python 검사](Atlas/IgnisTerrainMaster-v1/verify.py)는 원본 좌표·이름·시설과 PNG를 다시 읽는다. 40개 파일 전체가 원본의 해당 영역과 일치하고 **67개 인접 경계가 모두 동일**함을 확인했다. 하천 제어점의 원본 하상과 격자 보간값의 최대 차이는 약 0.676m다. 2km 이하 간격으로 지정 하천 중심선을 검사했을 때 저장 단위 0.125m를 넘는 역경사는 없었다. 전체 유역의 배수·침식·홍수 검증을 뜻하지 않는다.

수치 검사는 21개, 로컬 파일 뷰어의 선택·레이어·휠·드래그 검사는 8개를 통과했다. [재생성 검사](Atlas/IgnisTerrainMaster-v1/records/rebuild-verification.json)에서는 같은 Node.js 실행 환경과 입력으로 생성한 데이터 파일 56개의 바이트가 모두 일치했다. 다른 실행 환경의 부동소수점 연산 차이까지 검증한 결과는 아니다.

교차 검토에는 도로·하천 선분 접촉 79건과 그중 겹치는 회랑 7건을 기록했다. 같은 꼭짓점의 중복 접촉도 포함하므로 교량 79개가 필요하다는 뜻이 아니다. 도로를 어느 강변에 놓을지, 어디를 교량·나루로 연결할지는 아직 정하지 않았다. 원본 이동로 꼭짓점에 직접 붙지 않는 정착지 ID 9개도 그래프에 기록했다. 중앙 정착지의 지선 누락과 항구의 해상 경유점 차이를 구분한 뒤 연결해야 한다.

벨레스의 첫 상세 자식 지형은 위 자료에서 확인한다. 외곽 접속 좌표는 후속 실제 월드 제작의 참고로 보존하며 이웃 구역의 상세 이미지 확장은 결정 55에 따라 중단했다. 나머지 대륙의 건물·골목·개별 수목, 기존 상세 그림과의 정밀 정합, 실제 통행성, UE 수입·실행은 아직 검증하지 않았다. 대륙 전체 고도 원본의 제작 완료와 대륙 전체 상세 아트의 완료를 구분한다.

재생성은 저장소 루트에서 `node Docs/ArtDirection/EtherWorldMap/Atlas/IgnisTerrainMaster-v1/build.cjs`, 수치 검사와 미리보기 출력은 `python Docs/ArtDirection/EtherWorldMap/Atlas/IgnisTerrainMaster-v1/verify.py`로 실행한다. Python에는 Pillow가 필요하며 수치 생성기는 Node.js 기본 모듈만 사용한다. 생성기는 이 원본 폴더의 파생 파일만 갱신하고 참조 원본을 수정하지 않는다. 뷰어는 로컬 `index.html`로 열 수 있다.

## 검토 자료

| 자료 | 확인할 것 |
|---|---|
| [연결 구간 뷰어](Atlas/IgnisTerrainPrototype-v1/index.html) | `연결 구간`, `길 확대`, `접합 확인`과 휠·드래그로 같은 좌표를 확인한다 |
| [길 확대 화면](Atlas/IgnisTerrainPrototype-v1/previews/B04-close.png) | 수관, 풀과 흙길의 상세도·반복 특성을 확인한다 |
| [케르딘 접합 화면](Atlas/IgnisTerrainPrototype-v1/previews/T11-junction.png) | 기존 도안의 강·진입로가 새 배경으로 바뀌는 부분을 확인한다 |
| [전체 연결 구간](Atlas/IgnisTerrainPrototype-v1/previews/B04-overview.png) | 기존 B04의 양 끝 정착지와 구간 범위를 확인한다 |
| [공통 데이터와 접합 후보](Atlas/IgnisTerrainPrototype-v1/records/plan.json) | 좌표, 이미지 참조, 재질·폭 전환 조건을 읽는다 |
| [재질 생성 기록](Atlas/IgnisTerrainPrototype-v1/records/generation.json) | 내장 이미지 도구의 입력문과 재질 원본을 확인한다 |
| [좌표·분할 출력 검사](Atlas/IgnisTerrainPrototype-v1/records/verification.json), [브라우저 검사](Atlas/IgnisTerrainPrototype-v1/records/browser-verification.json) | 검사한 동작과 실제 지형 품질의 범위를 구분한다 |

기존 [정착지 36곳의 상세 도안](IgnisSettlementDetails.md)과 채택된 v2 원본은 유지한다. 시제품은 기존 상세 타일 150장을 참조하며 해당 파일을 다시 생성하거나 변경하지 않는다.

## 전체 개별 이미지 방식의 규모

[전체 범위 계산](Atlas/IgnisSettlementDetails-v1/records/full-coverage-audit.json)은 기존 해안선의 타일 중심 포함 여부와 해안 통과 셀을 함께 계산했다. [재현 스크립트](Atlas/IgnisSettlementDetails-v1/audit-full-coverage.cjs)와 [L10 행별 열 범위](Atlas/IgnisSettlementDetails-v1/records/full-coverage-row-ranges.csv)를 보관한다. 지도상의 픽셀 면적을 구면 면적으로 해석하지 않는다.

| 항목 | 계산 결과 |
|---|---|
| 기존 대륙 작업 사각형을 L10으로 전개 | 20,971,520×10,485,760px, 52,428,800칸 |
| 해안 접촉 셀까지 포함한 L10 육지 | 24,662,273칸 |
| 기존 정착지 상세 150장의 L10 환산 면적 | 102칸 |
| 현재 타일 평균 파일 크기로 외삽한 육지 출력 | 약 256.5TB, 생성 원본·중첩·보정본·복제 제외 |

용량은 정착지 타일의 평균으로 추정한 값이며 새 황야 타일의 실측 크기가 아니다. 작은 마을의 L11 밀도는 L10보다 면적당 네 배의 타일이 필요하다. 이 계산은 개별 PNG 전면 제작이 현재 환경에서 가능한 후속 몇 장의 작업이 아님을 보여 주며, 절차적 방식의 실제 저장 용량이나 제작 기간을 산정한 결과는 아니다.

## 공통 좌표와 표현

기존 해안·하천·이동로와 정착지의 세계 픽셀 좌표를 사용한다. 동일한 좌표와 재질 입력에서 같은 바탕을 재현하며 카메라나 타일 ID를 식생 배치의 난수 기준으로 사용하지 않는다. 바탕의 확대 영역만 그려 수천만 장을 미리 저장하는 구조를 피한다.

풀·수관·암반·흙 네 재질은 내장 `image_gen`으로 만든 1,254×1,254px 원본의 네 구역이다. 무작위 회전과 겹침을 공통 좌표에 고정하고, 셰이더의 명시적인 축소 단계 선택으로 재질 경계에서 발생했던 격자 자국을 줄였다. 이 방식은 재질을 재사용하므로 모든 지역에 고유한 세부가 생기는 것은 아니다.

이 재질 시제품의 식생과 암반 분포는 기존 대륙 바탕의 색과 연속적인 절차적 패턴으로 만든 후보다. 시제품 렌더러는 새 수치 고도 원본과 아직 연결하지 않았고 유역 계산도 하지 않는다. 지형 명암을 새로운 측량 고도로 취급하지 않는다. 현행 재질의 가로 비율은 기준 위도에 맞춘 시제품 값이며 대륙 외곽의 실거리 재질 밀도는 추가 검토가 필요하다.

육로와 강은 같은 좌표선을 사용해 흙·자갈과 물 재질로 표현한다. 해상 이동선은 도로로 그리지 않는다. 케르딘·길목 주변에는 기존 그림의 과장된 진입로 폭과 케르딘 강 폭을 잇는 국소 전환 후보를 둔다. 정착지 기준점과 기존 선의 꼭짓점은 옮기지 않으며, 폭 전환은 실제 통행 폭이나 하천 측량값이 아니다.

기존 정착지 일대의 투영된 외곽을 따라 바탕을 완만하게 전환한다. 단순한 직사각형 경계만 흐리게 했을 때 남았던 사선 테두리를 줄이는 처리다. 마을 내부와 구역 밖의 수관 크기·배치·색 차이는 별도 시각 검토 대상이다.

## 검증과 후속 범위

좌표·분할 출력 검사는 기존 정착지 36곳의 기준점, 해안·강·경로 꼭짓점, 상세 타일 150장의 경계와 참조를 대조한다. 시험 위치의 한 장 렌더링과 좌우 독립 렌더링의 RGB 픽셀, 다른 위치를 그린 뒤 돌아온 결과를 비교한다. 이 검사는 절차적 재현과 분할의 일치를 확인하며 지도 전체의 의미상 연결을 증명하지 않는다.

브라우저 검사는 재질 로딩, WebGL 실행, 양 끝 정착지와 구간 확대, 실제 드래그·휠 입력, 원본 비교와 오류 발생 여부를 확인한다. 기록된 제출 시간은 GPU 전체 프레임 시간이나 UE 성능 측정값이 아니다.

첫 구간의 확대 화면에서 재질 격자 자국이 줄고 흙길·수관을 구분할 수 있음을 확인했다. 정착지 접합부의 폭 변화도 완화했다. 다만 그림의 강과 길이 가진 세부 굴곡, 식생 밀도 차이와 재질 반복은 남아 있어 **전체 적용의 품질 검증은 완료되지 않았다**. 렌더러로 다른 대륙 위치를 볼 수 있다는 사실과 그 지역의 아트 검토 완료를 구분한다.

재질 시제품의 추가 검증·확장은 결정 55에 따라 중단했다. 새 수치 고도 원본과의 연결, 두 정착지 접합의 양방향 검토, 다른 연결로와 해안·산악·건조·설원 표본의 품질은 미검증으로 남긴다. 벨레스 정밀 정합, 교량·나루, 실제 시설 치수·통행성과 UE 구현도 미검증이다. 에테르 전체 제작은 다른 대륙 설정 이후로 보류한다.
