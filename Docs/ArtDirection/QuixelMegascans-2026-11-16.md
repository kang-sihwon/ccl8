# Quixel Megascans 3D 리소스 적합성 조사

2026-11-16 기준, Quixel Megascans의 3D 상품 **8,825개**를 수집해 ccl8의 세계관·아트 방향과 대조했다. **벨레스의 자연물·목재 구조재·생활 소품에 쓸 후보가 많다.** 동굴·해안·유적·설지 재료도 확보 후보가 있으나, 에테르 설비와 검은 결정의 상태 변화, 거목, 인물·적·보스는 별도 제작·조사가 필요하다. 아래 적합도는 도입 우선순위 제안이며 채택 결정이 아니다.

- [전체 8,825개 검색·필터 표](QuixelMegascans-2026-11-16/catalog.html): 이름·지역·용도 검색, 분류·적합도·상세 확인 여부 필터, 항목별 Fab 링크를 제공한다. 파일 하나로 오프라인에서 열 수 있다.
- [전체 분류 데이터](QuixelMegascans-2026-11-16/catalog.json), [수집 범위와 출처](QuixelMegascans-2026-11-16/coverage.json).
- 원본: [Quixel Megascans 3D 판매자 목록](https://www.fab.com/sellers/Quixel%20Megascans?listing_types=3d-model).

## 조사 범위와 증거

**전체 조사는 상품 목록의 전수 수집·1차 선별을 뜻한다.** 8,825개 전부의 상세 페이지나 이미지를 개별 검토한 것은 아니다. 상품명·Fab 분류에 규칙을 적용해 전 행에 지역·용도·주의점을 붙였고, 아래 대표 41개는 상세 설명과 제공 포맷을 직접 확인했다. 일부 대표 이미지를 대조했지만 전 상품의 외형 검증으로 확대하지 않았다. 제공자의 설명은 실행 검증과 구분한다.

Chrome에서 19개 3D 분류를 각각 끝까지 탐색하고 상품 URL로 중복을 제거했다. 판매자 상단의 8.8K 표시는 반올림된 값이며 수집한 고유 URL은 8,825개다. 같은 이름의 서로 다른 상품은 별도 행으로 보존했다. 수집 중 다른 필터에서 발견한 브러시 264개는 제외했다. 표면·데칼·브러시·다른 판매자 상품은 이번 3D 필터 범위 밖이다. 팩에 포함된 개별 메시 수는 8,825개에 중복 가산하지 않았다.

목록 카드에서 무료로 표시된 상품은 **8,756개**, 라이브러리 저장 표시만 있고 가격이 없는 상품은 **7개**, 가격을 읽을 수 없었던 상품은 **62개**다. 이는 해당 세션의 표시 기록이며 모든 상품의 향후 가격·소유권·사용 권한을 보증하지 않는다. 일부 상세 설명에 UE 전용 문구가 있으므로 목록의 무료 표시만으로 라이선스를 통일해서 판단하지 않는다. 상세 확인한 라이선스 표시와 제공자의 UE 전용 문구는 전체표에 따로 기록했다.

다운로드·구매·라이브러리 추가·UE 임포트는 수행하지 않았다. UE 5.8.3 호환성, 폴리곤 수, LOD, Nanite, 식생 바람, 충돌, NavMesh, 메모리·프레임 비용은 미검증이다. 열린 메시·닫힌 메시 표시는 제공자의 표면 구조 설명이며 충돌 품질을 보장하지 않는다. 물리 크기는 사이트에 표시된 축 순서를 유지했다.

## 프로젝트 대조 기준

| 근거 | 이번 조사에 적용한 기준 |
|---|---|
| [세계관 정본](../Narrative/World.md) | 에테르는 생활 설비에 쓰는 유한 자원이다. 검은 결정은 손상·과도한 채취와 관련된 물리적 현상이며 임의의 저주·악의 상징으로 바꾸지 않는다. |
| [시나리오 정본](../Narrative/Scenario.md) | 벨레스 공방·나루, 숲과 수원 동굴, 로덴 장인조합, 해외 항만·고원·범람원·극지의 맥락으로 후보를 연결한다. 정상 설비를 무차별 파괴하는 공간으로 구성하지 않는다. |
| [아트 방향 초안](../ArtDirection.md) | PBR 자연광, 큰 색면, 읽히는 실루엣, 생활감 있는 목재·직물·석재를 우선한다. 세부 룩은 제안이다. |
| [첫 장 시안](EtherFirstChapter.md) | 벨레스 목조·판자길·붉은 지붕·청록 덧문·겨자 천막, 황금 침엽수·붉은 이끼·청록 개울과 맞춘다. 이 팔레트는 시각 제안이며 정본 사실과 구분한다. |
| [벨레스 아트 블록아웃](../Work/VelesBlockout.md) | 공방·물터·숲문 구간의 지면·수면·조명·원경 개선에 먼저 쓸 재료를 찾는다. 목록 조사만으로 실제 아트 품질이나 게임 검증을 완료 처리하지 않는다. |

## 전수 1차 분류

| 판정 | 상품 수 | 의미 |
|---|---:|---|
| A 우선 검토 | 2,387 | 벨레스·이그니스의 범용 자연물·생활 소품 우선 후보 |
| B 조건부 검토 | 4,608 | 후속 지역 또는 문화·기술·재질 수정 조건이 있는 후보 |
| C 우선 제외 | 1,479 | 현대·산업·교통·총포 요소로 현재 방향에서 우선 제외 |
| D 추가 확인 | 351 | 이름만으로 판단하기 어려워 외형·맥락 추가 확인 |

위 수는 **이름·분류로 선별한 검토 대상 수**다. A도 즉시 사용 승인이 아니며, B에는 다른 대륙에 적합할 가능성이 높은 자연물과 수정이 필요한 문화·시대 요소가 함께 들어 있다. C는 현재 방향에서 우선 제외할 제안이지 사용 불가능하다는 기술 판정은 아니다. D는 외형이나 맥락을 더 확인해야 하는 항목이다. 이 보수적 규칙은 오탐·누락이 가능하므로 실제 도입은 아래 상세 후보부터 진행하는 편이 낫다.

### Fab 분류별 수집 수

| 분류 | 고유 상품 수 |
|---|---:|
| 자연/암석 | 2,715 |
| 건축 | 2,180 |
| 도구/소품 | 1,764 |
| 자연/나무 | 579 |
| 가구 | 527 |
| 자연/식물 | 260 |
| 음식 | 239 |
| 환경 | 172 |
| 전자/기술 | 115 |
| 자연/파편 | 57 |
| 캐릭터 | 49 |
| 자연/버섯 | 46 |
| 자연/눈 | 43 |
| 무기 | 41 |
| 자연/해저 | 15 |
| 자연/숲 | 8 |
| 자연/지형 | 8 |
| 운송 | 5 |
| 의류 | 2 |
| **합계** | **8,825** |

Fab의 분류는 그대로 보존했다. 예를 들어 전자·기술에도 수동 망치·끌·쐐기가 있고, 환경에도 도로 경계석이 있다. 분류명만으로 포함·제외하지 않았다.

## 지역별 활용 판단

| 지역·장면 | 맞는 리소스 계열 | 적합성 판단·제작 조건 |
|---|---|---|
| 벨레스 공방·나루·시장 | 목재 보·상자·통·밧줄·마차·노·모루·책·그릇 | 가장 먼저 검토할 계열. 새것과 폐품의 비율을 맞춰 현재 주민이 사는 공간으로 만든다. 붉은 지붕·청록 덧문·겨자 천막은 재질·모듈 작업이 필요하다. |
| 이그니스 숲·수원 동굴 | 이끼 바위·지면 조각·뿌리·줄기·고사리·관목·동굴 돌기둥 | 숲길과 물터의 기본 재료에 적합하다. 황금 침엽수의 온전한 수관과 거목은 별도 확보한다. 얼음은 사고 구간의 국소 냉기로 제한한다. |
| 아르베나 해안·항만 | 해안 암벽·해변 돌·유목·항구 목재·밧줄·배 부품 | 해안 지형 재료가 풍부하다. 선단·대형 선박·해안 포식자와 파도·항해 기능을 제공하는 것으로 해석하지 않는다. |
| 오르다스 외항·내해·고원·수로 | 사암·건조 암석·포장석·수동 도구·시장 소품 | 건조 고원 활용은 제안이다. 대륙 전체가 사막이거나 Bazaar의 실존 문화권을 따른다고 확정하지 않는다. 수로·물 배분 장치는 별도 설계한다. |
| 세라디온 숲·범람원·유적 | 활엽수·습지 식생·석조 부재·폐허 조합 | 숲과 유적을 겹치는 재료로 유용하다. 로마식 기둥·여신상·종교 문양은 수정하거나 제외한다. 현용 수원·시험 설비와 폐허를 구분한다. |
| 니베르 극지·탐사대 | 눈 둔덕·얼음 절벽·차가운 암석·운반 소품 | 설지 형상에 적합하다. 탐사 장비와 잔존 장치는 현대 산업 소품을 그대로 쓰기보다 세계의 기술 체계에 맞춰 제작한다. |

## 상세 확인한 대표 후보 41개

아래 모든 행은 상품 상세 설명을 확인했다. 포맷·물리 크기·열린 메시 표시는 제공자 기재 사실이며, 용도·수정점·적합도는 프로젝트에 대한 제안이다. 가격 열은 전체표와 일관되게 목록 카드 기준을 유지한다. 나무 팩의 LOD·Simple/PivotPainter·계절·상태·바람 제어는 설명에 있는 기능이며 ccl8에서 검증한 결과가 아니다.

| 번호 | 상품·Fab 링크 | 적합도·용도 | 판단과 수정 필요점 | 확인 포맷·물리 정보·가격 |
|---|---|---|---|---|
| 1 | [Medieval Village Megascans Sample](https://www.fab.com/listings/2e11a225-a6ea-4781-a3e1-fe975b461894) | B 조건부 검토<br>벨레스·이그니스 환경 참고 | 상세 제목은 Medieval Game Environment다. 설명은 UE4.26 기반이고 파일 정보에는 4.26–4.27 및 5.3–5.8 지원을 표시한다. UE 전용 문구가 있으며 ccl8의 5.8.3 실행은 미검증이다. 마을·숲 구성부터 참고한다. | UE<br>파일 정보: UE 4.26–4.27 및 5.3–5.8; Windows; Complete project<br>라이브러리 저장 표시·가격 미표시 |
| 2 | [Dark Ruins Megascans Sample](https://www.fab.com/listings/836ed2f8-e2d6-49be-98d3-59d104bd351e) | B 조건부 검토<br>세라디온 유적·동굴의 석재 구성 | 폐허의 석재·구조 조합 후보. 어두운 장면을 벨레스의 기본 분위기로 가져오지 않는다. 제공자는 높은 품질과 초기 로딩 부담을 안내한다. | UE<br>무료 표시 |
| 3 | [Goddess Temple Megascans Sample](https://www.fab.com/listings/471eaad7-3c05-45bd-ac37-02dddb17780f) | B 조건부 검토<br>세라디온 유적의 구조 부품 | 여신상·종교 상징은 세계관 확정 없이 채택하지 않는다. 소개 이미지의 지형·지면·일부 산포물이 미포함이라고 명시한다. 레이 트레이싱·RTX 안내도 별도 검토한다. | UE<br>라이브러리 저장 표시·가격 미표시 |
| 4 | [Old Mine](https://www.fab.com/listings/dc4dc139-f3a3-4375-875f-d1831506953e) | B 조건부 검토<br>벨레스 수원 동굴·옛 채취 시설 | 61개 에셋. 목재 지보·암석·굴 부품을 선별한다. 광산 레일·산업 장치의 시대감을 확인하고 정상 수원 설비와 폐시설을 구분한다. 장면 파일 안내는 UE 5.4–5.6이다. | UE, FBX<br>무료 표시 |
| 5 | [Bazaar](https://www.fab.com/listings/ccde8d34-48fc-4876-b1cd-1658af34ffbb) | B 조건부 검토<br>벨레스 시장·해외 항구의 소품 | 94개 에셋. 천막·그릇·상자·식재료를 선별하고 겨자색 천막 등 마을 색채에 맞춘다. 아랍·페르시아풍 건축과 문양은 해당 대륙 문화로 자동 채택하지 않는다. 장면 안내는 UE 5.4–5.6이다. | UE, FBX<br>무료 표시 |
| 6 | [Medieval Banquet](https://www.fab.com/listings/d422ac6c-ce50-4273-8d4a-2ac594a3a2b2) | B 조건부 검토<br>벨레스 식탁·로덴 실내의 소품 | 77개 에셋. 음식·그릇·촛대·가구를 선별한다. 호화 연회장 전체보다 주민의 공동 식탁과 생활 소품에 우선 활용한다. 장면 안내는 UE 5.4–5.6이다. | UE, FBX<br>무료 표시 |
| 7 | [Nordic Forest Ledge Moss Small 02](https://www.fab.com/listings/aa3c1bba-0534-46dc-99a0-1087c027f70f) | A 우선 검토<br>벨레스 숲길·개울 둔덕 | 이끼 낀 낮은 지형 조각. 붉은 이끼·청록 개울 시안에 맞춰 색을 조절하고 반복 배치의 이음새를 가린다. 열린 메시이므로 측면 노출을 확인한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>0.83m x 1.88m x 0.29m; Open mesh<br>가격 미표시 |
| 8 | [Mossy Forest Boulder](https://www.fab.com/listings/c790fa59-8a05-4d40-b645-3bc28eaf5982) | A 우선 검토<br>벨레스 숲길·물터의 바위 | 덩어리감 있는 이끼 바위. 이끼 색·거칠기를 통일하고 전투 길목에는 실루엣을 방해하지 않게 배치한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>1.17m x 1.08m x 0.53m; Closed mesh<br>무료 표시 |
| 9 | [Common Fern](https://www.fab.com/listings/082e031b-c83b-47d6-b099-fcacc7792878) | A 우선 검토<br>벨레스 숲의 낮은 식생 | 고사리 군락 후보. 투명도·투과광 맵을 쓰는 재질과 바람·거리별 밀도를 실제 프로젝트에서 확인한다. | FBX, glTF<br>무료 표시 |
| 10 | [Lady Fern](https://www.fab.com/listings/3eead001-af7f-4b65-afcc-89c368f089bc) | A 우선 검토<br>벨레스 숲·습한 동굴 입구 | 고사리 변주 후보. Common Fern과 잎 크기·초록 채도·밀도를 맞추고 공격 예고를 가리지 않는다. | FBX, glTF<br>무료 표시 |
| 11 | [Lingonberry](https://www.fab.com/listings/a9f7d56d-6d4c-4ae9-8b9b-f88f7e0a80fa) | A 우선 검토<br>이그니스 숲의 낮은 관목 | 링곤베리 관목 후보. 채집 가능한 열매 기능은 제공 여부가 확인되지 않았다. 시각 소품과 상호작용 대상을 구분한다. | FBX, glTF<br>무료 표시 |
| 12 | [Dwarf Birch](https://www.fab.com/listings/7f125b9e-32c6-4fd2-a0eb-eecab72d7701) | A 우선 검토<br>이그니스 숲 가장자리·북부 관목 | 낮은 자작나무 식생 후보. 거목이나 주 수관을 대신하는 용도로 분류하지 않는다. | FBX, glTF<br>무료 표시 |
| 13 | [Cotton Grass](https://www.fab.com/listings/6b9ba65d-5c47-4f95-a0ca-c08dbefac066) | A 우선 검토<br>벨레스 물터·습지 가장자리 | 솜풀 후보. 물가의 밝은 포인트로 제한 배치하고 바람·수면 경계의 표현을 검토한다. | FBX, glTF<br>무료 표시 |
| 14 | [Historic Medieval Farm Wagon Wood 02](https://www.fab.com/listings/4908a992-5818-4632-ad97-3ab3ba550796) | A 우선 검토<br>벨레스 운반·농가·나루 | 목제 마차로 생활·운송 맥락에 맞는다. 바퀴 회전·견인·물리 기능은 확인되지 않았으므로 정적 소품부터 검토한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>3.1m x 1.53m x 1.46m; Closed mesh<br>무료 표시 |
| 15 | [Historic Medieval Harbour Crate Wood Old 01](https://www.fab.com/listings/bc92a9d4-0400-4e7c-b937-c32a72f0af20) | A 우선 검토<br>벨레스 나루·공방의 적재물 | 작은 목제 항구 상자. 크기와 적재 간격을 맞추고 에테르 운송 상자라면 전용 표식·내부 구조를 별도 제작한다. | FBX, glTF, 변환 GLB/glTF/USDZ<br>0.42m x 0.52m x 0.42m; Closed mesh<br>무료 표시 |
| 16 | [Medieval Modular Wall](https://www.fab.com/listings/3d22b5f4-fd0d-4ae7-8c20-76df76a7627c) | B 조건부 검토<br>벨레스 건물 일부·로덴 도시 | 목재 골조와 벽돌 벽의 모듈이다. 벨레스 목조 비율·붉은 지붕·청록 덧문 기준과 대조한다. 열린 메시의 뒷면과 모듈 연결을 확인한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>0.18m x 3.51m x 2.01m; Open mesh<br>무료 표시 |
| 17 | [Old Wooden Beam](https://www.fab.com/listings/60f16cdb-5683-45c7-a757-7d1e80ea1886) | A 우선 검토<br>벨레스 기둥·판자길·공방 구조 | 긴 목재 보 후보. 실제 치수와 모듈 간격에 맞춰 조합하고 부재 끝면·균열·텍셀 밀도를 확인한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>0.42m x 4.02m x 0.27m; Closed mesh<br>무료 표시 |
| 18 | [Wooden Barrel](https://www.fab.com/listings/895fd4fd-2f0b-4ff4-84f2-2ba8df57b13c) | A 우선 검토<br>벨레스 창고·나루·식료품 보관 | 목제 통 후보. 금속 테두리의 거칠기와 목재 색을 다른 소품에 맞춘다. 내용물과 상호작용은 별도다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>0.69m x 0.69m x 0.95m; Closed mesh<br>무료 표시 |
| 19 | [Wooden Crate](https://www.fab.com/listings/c7461bce-dcd7-4b1c-8260-47cd45411236) | A 우선 검토<br>벨레스 공방·시장·운송 | 목제 상자 후보. 같은 이름의 변형 상품은 URL로 구분한다. 반복 배치 크기·회전·사용 흔적을 변주한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>0.39m x 0.56m x 0.45m; Closed mesh<br>무료 표시 |
| 20 | [Blacksmith Anvil](https://www.fab.com/listings/ffcfdb35-b850-48c8-8863-1c6c9c41fd41) | A 우선 검토<br>벨레스 장인 주인공의 공방 | 모루 후보. 열린 메시로 표시되므로 바닥·뒷면 노출과 충돌을 확인한다. 제작 애니메이션·도구 접촉 기능은 별도다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>0.42m x 0.42m x 0.7m; Open mesh<br>무료 표시 |
| 21 | [Historic Miscellaneous Anvil Mixed Old](https://www.fab.com/listings/94f12e9a-687f-4e58-af55-6ba7740d9b88) | A 우선 검토<br>벨레스 공방의 모루 변주 | 다른 비례의 모루 후보. 열린 메시이며 실제 치수를 작업대와 비교한다. 녹과 마모를 조절해 현재 사용 중인 공방으로 읽히게 한다. | FBX, glTF, 변환 GLB/glTF/USDZ<br>0.58m x 0.81m x 0.4m; Open mesh<br>무료 표시 |
| 22 | [Historic Miscellaneous Book Mixed Old 10](https://www.fab.com/listings/f611a3c1-cd05-4d6b-9c1c-0ae47b1aef1a) | A 우선 검토<br>공방 기록·로덴 장인조합 문서 | 정적 책 소품. 기록 전달 맥락에 맞지만 읽기 UI·페이지 애니메이션·설정 텍스트는 별도 제작한다. | FBX, glTF, 변환 GLB/glTF/USDZ<br>0.13m x 0.18m x 0.03m; Closed mesh<br>무료 표시 |
| 23 | [Historic Miscellaneous Oar Wood Old 01](https://www.fab.com/listings/b8793ea7-b5c2-431f-be9c-09427ebd9b24) | A 우선 검토<br>벨레스 나루·작은 배 | 목제 노 후보. 배·사람과 길이를 비교하고 손잡이와 노 젓기 동작의 기준점을 별도 맞춘다. | FBX, glTF, 변환 GLB/glTF/USDZ<br>0.11m x 2.39m x 0.08m; Closed mesh<br>무료 표시 |
| 24 | [Historic Miscellaneous Candelabra Metal Rusty 02](https://www.fab.com/listings/7dcfb96c-ddcb-4d0b-aa4e-0aa6fc7c1244) | A 우선 검토<br>벨레스 실내·공방의 조명 소품 | 촛대 형상 후보. 에테르 등불로 쓰려면 광원·결정 장착부·소모 상태를 새로 설계한다. 촛대 자체를 에테르 장치로 동일시하지 않는다. | FBX, glTF, 변환 GLB/glTF/USDZ<br>0.31m x 0.46m x 0.2m; Closed mesh<br>무료 표시 |
| 25 | [Cave African Column Vertical Stone Medium 06](https://www.fab.com/listings/2eaf242f-7a91-4cd5-bc53-f5bff7aededb) | A 우선 검토<br>수원 동굴의 돌기둥·통로 | 침식된 돌기둥 후보. 뿌리·얼음·천창빛과 조합한다. 높이·가로폭을 임의로 해석하지 말고 가져온 뒤 축과 충돌을 확인한다. | FBX, glTF, 변환 GLB/glTF/USDZ<br>1.88m x 2.38m x 1.11m; Closed mesh<br>무료 표시 |
| 26 | [Ice Cliff](https://www.fab.com/listings/6f7741ec-d346-47dc-81f2-546527c3b049) | B 조건부 검토<br>니베르 극지·이그니스 국소 결빙 | 얼음 절벽 후보. 열린 메시이며 국소 서리 표현에는 큰 절벽 대신 일부 형상만 쓰는 편이 낫다. 벨레스 숲 전체를 설원으로 바꾸지 않는다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>1.96m x 3.63m x 5.03m; Open mesh<br>무료 표시 |
| 27 | [Snow Embankment](https://www.fab.com/listings/5c0c8fc3-6284-4604-8913-884edf1053d4) | B 조건부 검토<br>니베르 설지·동굴 주변 잔설 | 눈 둔덕 후보. 열린 메시이므로 바닥에 묻어 경계를 처리한다. 첫 장은 이상 현상이 발생한 좁은 범위에만 사용한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>2.77m x 4.62m x 0.53m; Open mesh<br>무료 표시 |
| 28 | [Historic Gemstone Mineral Quartz Stone Hard 03](https://www.fab.com/listings/bfa75cad-8482-4912-bca0-3923e72f9638) | B 조건부 검토<br>에테르·검은 결정의 형상 출발점 | 작은 석영 덩어리다. 금빛·투명도·냉기·방출·파괴 표현을 새로 만들어야 한다. 바르툴의 외부 결박 구조와 기능을 제공하는 에셋은 아니다. | FBX, glTF, 변환 GLB/glTF/USDZ<br>0.13m x 0.23m x 0.11m; Closed mesh<br>무료 표시 |
| 29 | [Nordic Coastal Cliff](https://www.fab.com/listings/f53f0b71-fd1e-4c8b-9e65-bda485469f86) | B 조건부 검토<br>아르베나 해안·항구 절벽 | 해안 암벽 후보. 파도·젖음·해조류와 맞춰 재질을 조절한다. 해안이라는 이유만으로 벨레스 내륙에 주력 배치하지 않는다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>2.03m x 2.92m x 2.44m; Closed mesh<br>무료 표시 |
| 30 | [Sandstone Boulder](https://www.fab.com/listings/87b23d7e-5250-441f-aea7-d94dda164d19) | B 조건부 검토<br>오르다스 고원의 건조 암석 구간 제안 | 사암 바위 후보. 오르다스 전체가 사막이라는 확정 설정은 없으므로 건조 지형 활용은 지역 아트 제안으로 한정한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>2.12m x 2.84m x 1.49m; Closed mesh<br>무료 표시 |
| 31 | [Roman Column](https://www.fab.com/listings/bee67c68-4b29-4c3c-b59d-629598f8304f) | B 조건부 검토<br>세라디온 유적의 석조 부재 | 로마풍 기둥이다. 실존 문화의 장식·비례를 그대로 세계의 정식 건축양식으로 채택하지 않는다. 파손 부재나 재조합을 우선 검토한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>0.49m x 0.5m x 1.81m; Closed mesh<br>무료 표시 |
| 32 | [Modular Mine Tunnel Start End](https://www.fab.com/listings/c6750ced-c165-49d3-a1b1-63a9846ff231) | B 조건부 검토<br>수원 동굴의 옛 통로·지보 | 광산 터널 시작·끝 모듈. 열린 메시이고 통로 치수·카메라·이음새·충돌을 확인해야 한다. 광산 전체 맥락보다 목재 지지 구조를 선별한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>3.21m x 3.05mx 2.36m; Open mesh<br>무료 표시 |
| 33 | [Rope Coil](https://www.fab.com/listings/8ec7aab4-8be8-442e-95ab-840a8fa32932) | A 우선 검토<br>벨레스 나루·공방·운반 장치 | 감긴 밧줄 정적 소품. 열린 메시이므로 가까운 시점에서 틈을 확인한다. 연결·당기기·절단 동작은 별도 구현한다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>0.18m x 0.18m x 0.24m; Open mesh<br>무료 표시 |
| 34 | [Historic Medieval Weapon Quiver Leather 01](https://www.fab.com/listings/a7d06d31-13b3-47e9-b899-f97b3bf15822) | B 조건부 검토<br>고블린 운반물·사냥꾼 장비 | 가죽 화살통 정적 소품 후보. 리그·착용 소켓·꺼내기 동작·충돌·무기 성능은 확인되지 않았다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>0.11m x 0.09m x 0.42m; Closed mesh<br>무료 표시 |
| 35 | [Norway Maple](https://www.fab.com/listings/1f65d477-f44e-4ccb-95c1-788c99e33b56) | B 조건부 검토<br>이그니스 마을 주변·혼합림 | Norway Maple 나무 팩. 설명상 고유 나무 15종과 계절·상태·바람 제어가 있다. 활엽수이므로 황금 침엽수 주 수관을 대신하지 않는다. UE 전용 문구가 있다. | UE<br>라이브러리 저장 표시·가격 미표시 |
| 36 | [Common Hazel](https://www.fab.com/listings/81bc7ba6-4686-4f94-9d2b-83eb1fdc4079) | B 조건부 검토<br>이그니스 숲 가장자리·마을 주변 | Common Hazel 나무 팩. 설명상 고유 나무 15종과 계절·상태·바람 제어가 있다. 활엽수 보조 식생으로 검토한다. UE 전용 문구가 있다. | UE<br>라이브러리 저장 표시·가격 미표시 |
| 37 | [European Beech](https://www.fab.com/listings/d11cc01d-9422-41b7-950f-416c9ce79caf) | B 조건부 검토<br>이그니스 혼합림·세라디온 숲 | European Beech 나무 팩. 설명상 고유 나무 17종과 계절·상태·바람 제어가 있다. 주 수종·잎 색·밀도를 지역별로 통일한다. UE 전용 문구가 있다. | UE<br>라이브러리 저장 표시·가격 미표시 |
| 38 | [European Hornbeam](https://www.fab.com/listings/c6f917b6-ffcb-4b86-9d9f-5274ba7f6a8e) | B 조건부 검토<br>이그니스 혼합림·세라디온 숲 | European Hornbeam 나무 팩. 설명상 고유 나무 27종과 계절·상태·바람 제어가 있다. 활엽수 수관의 비율을 제한한다. UE 전용 문구가 있다. | UE<br>라이브러리 저장 표시·가격 미표시 |
| 39 | [European Black Alder](https://www.fab.com/listings/9de7ce19-5813-42d2-a5f0-5e6447006f72) | B 조건부 검토<br>벨레스 물가·범람원 주변 | European Black Alder 나무 팩. 설명상 고유 나무 22종과 계절·상태·바람 제어가 있다. 수변 활엽수 보조 후보이며 황금 침엽수 대체재로 보지 않는다. | UE<br>라이브러리 저장 표시·가격 미표시 |
| 40 | [Nordic Forest Tree Trunk Spruce Large](https://www.fab.com/listings/397bcca9-1302-4eda-914d-dac6d20fa9b1) | A 우선 검토<br>벨레스 숲의 줄기·고사목 조각 | Spruce라는 이름이 있지만 약 2m 규모의 줄기 조각이며 열린 메시다. 잎·수관을 갖춘 완전한 침엽수 에셋으로 집계하지 않는다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>1.32m x 1.47m x 2.05m; Open mesh<br>무료 표시 |
| 41 | [Forest Terrain](https://www.fab.com/listings/8a8981c4-bb94-4184-84b7-6c70d7c75ef1) | A 우선 검토<br>벨레스 숲길의 지면 덧대기 | 작은 낙엽 지면 메시 조각이다. Landscape나 월드 지형 전체가 아니다. 열린 메시의 경계를 묻고 지면·식생과 색을 맞춘다. | FBX, glTF, ZBrush, 변환 GLB/glTF/USDZ<br>2.53m x 1.66m x 0.18m; Open mesh<br>가격 미표시 |

## 우선 도입 검토 순서와 부족한 자산

첫 검토 묶음은 **Old Wooden Beam, Wooden Crate, Wooden Barrel, Blacksmith Anvil, Rope Coil, Mossy Forest Boulder, Nordic Forest Ledge Moss Small 02, Common Fern, Lingonberry, Cotton Grass, Cave African Column Vertical Stone Medium 06, Forest Terrain**의 12개다. 공방·물터·숲문에서 각각 작은 구간에 배치하면 목재·암석·식생·지면의 재질 차이를 비교하기 쉽다. 같은 이름의 변형이 많으므로 위 상세표의 URL을 사용한다. 도입 자체는 이번 조사에 포함하지 않았다.

| 부족한 자산·기능 | 이번 목록에서 확인한 한계 | 후속 작업 제안 |
|---|---|---|
| 황금 침엽수 숲·거목 | 상세 확인한 완전한 나무 팩 5종은 활엽수다. Spruce 줄기 조각은 수관이 아니다. | 침엽수 전체 메시·계절 재질을 별도로 확보하고 거목의 실루엣·뿌리·공간은 전용 제작한다. |
| 주인공·주민·고블린·바르툴 | 캐릭터 분류 49개는 뼈·두개골 등 소품 위주다. | 리그·스켈레톤·전투 애니메이션을 갖춘 별도 캐릭터 후보를 조사한다. |
| 에테르 등불·보온·수원·무휴기관 | 광물·촛대·수동 공구는 형상 재료일 뿐 상태 변화와 작동 장치를 제공하지 않는다. | 정상·소모·고장 상태와 금빛·투명화·냉기를 가진 전용 부품·재질·효과를 만든다. |
| 검은 결정 외부 결박·보스전 파괴 | 석영 덩어리에는 결박 구조·파괴 단위·약점 표시가 없다. | 바르툴 생존과 외부 결박 해제라는 전투 목적에 맞게 전용 구조를 설계한다. |
| 벨레스 건축의 통일성 | 중세·현대·실존 지역 부품이 섞여 있다. | 기둥 위 집·넓은 판자길·지붕·덧문의 모듈 규격과 재질 팔레트를 먼저 정한다. |
| 수면·지형·항해·상호작용 | 이번 필터는 3D 상품이다. 물 셰이더·Landscape 재질·시스템·애니메이션의 전수 조사가 아니다. | 필요한 표면·데칼·수면 시스템과 기능을 별도 범위로 조사한다. |

## 사용 전 확인할 조건

1. 상세 페이지의 최신 제공 파일·가격·라이선스와 설명의 UE 전용 문구를 함께 확인한다. 표는 2026-11-16의 표시 기록이다.
2. UE 5.8.3의 별도 검토 공간에서 치수·축·피벗·재질·열린 면·충돌·NavMesh를 확인한다. UE4.26 또는 UE 5.4–5.6 안내를 현재 엔진에서 검증된 것으로 바꾸지 않는다.
3. 대표 광원 아래 목재·암석·직물의 색·거칠기·텍셀 밀도를 맞춘다. 식생의 잎 가독성·바람·투과광과 근거리·원거리 비용을 측정한다.
4. 전투 시점에서 바닥 경계·적 실루엣·공격 예고를 확인한 후 밀도를 늘린다. 과도한 스캔 디테일과 녹·폐허 표현이 생활감과 큰 색면을 해치지 않게 한다.

## 결과표 검증 범위

8,825개 고유 URL, 19개 분류 합계, 상세 41개 행, 가격·적합도 합계, 제외한 브러시의 부재, HTML 내장 데이터와 JSON의 일치를 검사했다. 검색·적합도·분류·상세 확인 필터, 페이지 이동, 결과 없음, 초기화의 JavaScript 로직을 정적 실행으로 검사했다. 문서 검사기는 591개 문서에서 오류 0개를 보고했다. 로컬 HTML을 브라우저에서 여는 동작은 도구의 파일 URL 정책으로 차단되어 화면 렌더링은 확인하지 못했다.

이 문서는 조사 결과와 후보 제안을 소유한다. 세계관·건축 문화·최종 룩·에셋 채택·구매 결정과 실행 검증 상태는 변경하지 않는다.
