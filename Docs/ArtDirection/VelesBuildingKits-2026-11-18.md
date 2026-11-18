# 벨레스 B안 모듈형 건축 키트 비교

[아트 방향](../ArtDirection.md)의 하위 조사 문서다. 기준은 [결정 66](../decisions/0066-veles-b-art-direction.md)과 [B안 구역 시안](VelesConcepts-2026-11-17/B-village.md)이다. 남은 후보의 최종 채택은 미확정이며 9종 제외는 결정 67로 확정했다.

[이미지 포함 비교표](VelesBuildingKits-2026-11-18/index.html) · [검토 데이터](VelesBuildingKits-2026-11-18/data.json) · [작업 지도](../Work/VelesBuildingKits.md)

## 판단

**K01로 공방과 주택을 먼저 시험하고 K02를 조립 대안으로 비교한다.** 보유 자산으로 시작할 수 있지만 완성 프리팹을 그대로 놓으면 B안의 낮고 넓은 건물 비례가 나오지 않는다. 붉은 지붕·청록 덧문·겨자 천막은 맞춤 보완이 필요하다. K01은 재료 구성이 가깝고 K02는 분리된 부품이 명료하다.

사용자 검토와 결정 67에 따라 K04·K05·K06·K08·K10·K12·K14·K15·K16의 9종을 아트풍·시대배경 불일치로 제외했다. 부분 부재 활용도 추천하지 않는다. 남은 7종은 검토 대상이며 채택 확정이 아니다. 근거는 [결정 67](../decisions/0067-veles-building-kit-exclusions.md)이다. 제외 전 제안은 검토 데이터의 previousAssessment에 보존했다.

## 확인 범위

2026-11-18 Chrome 확인. 보유 라이브러리 504개 중 건물 및 건축 10개·환경 143개의 목록을 읽고, 후보 16개의 상세 페이지와 대표 이미지 26장을 확인했다. 16개 모두 라이브러리 보유 표시를 확인했다. 나머지 상품의 상세·전체 갤러리와 Fab 시장 전체를 조사한 결과는 아니다.

상품 구성·지원 버전은 판매자 표기이고, 형태·질감 평가는 열어 본 이미지에 근거한다. 활용안과 수정량은 제안이다. 신규 구매 가격은 미확인이다. 다운로드·UE 5.8.3 임포트·스냅·피벗·UV·충돌·성능 검증은 실행하지 않았다.

밝은 회벽·따뜻한 굵은 목재·낮고 넓은 박공·제한된 석조 기단을 비교 기준으로 삼았다. 원래 지도에 없는 우물·풍차·수차·물길을 패키지 구성 때문에 추가하지 않는다.

## 남은 검토 대상 7종

| 후보·상세 링크 | 판정 / 예상 수정량 | 공방 적용 제안 | 주택 적용 제안 | 주요 차이 |
|---|---|---|---|---|
| K01 [Modular Medieval Town with Interior](https://www.fab.com/listings/9a10f51b-8844-45c9-89c5-7d4d43d75761) | 우선 시험 / 중간 | S02 (60,124)에 벽·보·바닥·문틀을 낮고 넓게 재조립한다. 중앙 운반 통로를 열고 남쪽 작업 마당과 연결한다. 굴뚝 부품은 검토하되 단조 화덕은 별도로 설계한다. | S03–S06의 낮은 기본 주택에 벽·바닥을 공유한다. 현관 방향, 창 위치, 옆 증축부를 변형한다. 다층 프리팹을 그대로 반복하지 않는다. | 노출 목조 골조, 밝은 회벽, 석조 하부가 B안의 재료 구성에 가깝다. 기본 건물은 높고 좁으며 지붕이 가파르고 일부 끝이 휘어 있다. 목재도 B안보다 어둡다. |
| K02 [Modular Fantasy House](https://www.fab.com/listings/32ba409c-4425-41c2-b8f1-0a582778969b) | 조립 대안 / 중간 | K01과 별도 시험안으로 보·기둥·바닥을 이용해 열린 작업장 틀을 만든다. 제공 벽난로를 단조 화덕으로 바로 간주하지 않는다. | S03–S06 기본 주택의 외벽·박공·바닥 대안. 내부가 필요한 주택도 같은 부품군으로 구성할 수 있다. | 부품 전개에서 박공·지붕 패널·보·벽·바닥이 분리되어 보인다. 골조는 단순하지만 회갈색이 강하고 석재 비중이 크다. |
| K03 [Medieval Village](https://www.fab.com/listings/f89fe420-1942-4c0e-8707-e7f41dbe3017) | 조건부 대안 / 많음 | 창고·작업장 부품 대안. 공방 정면의 넓은 개구부와 정돈된 골조는 재조립해야 한다. | S04–S06의 낮은 주택 변형 후보. 제공 마을 배치와 우물을 가져오지 않고 현행 건물 좌표에 배치한다. | 낮은 지붕과 생활 골목은 활용할 수 있다. 거친 석벽, 불규칙한 기와, 어두운 목재와 노후 표현은 B안보다 강하다. 부품 전개에서 벽·개구부·바닥의 분할을 확인했다. |
| K07 [Nordic Fishing Hut - Coastal Lakeside Cabin [Interior + Exterior]](https://www.fab.com/listings/e56e7e02-01b8-4a15-a108-aa5d76ea0e10) | 수변 보조 / 많음 | 나루 주변 외부 시설·부속 작업 공간의 목재 부품 후보로 제한한다. | 기존 S01 건물 위치에 작은 수변 보조 건물을 만들 때 검토. 일반 주택 기본형으로 반복하지 않는다. | 실사 목재 오두막이며 풀 지붕과 노후 판벽이 B안과 다르다. 대표 이미지에 자전거가 있고 설명에 가전·전기 설비가 포함된다. |
| K09 [Medieval Docks](https://www.fab.com/listings/e953b0a4-e384-431d-88fa-470c32874e9a) | 수변 보조 / 중간 이상 | 나루와 연결되는 소형 목조 부속 시설·데크 후보. 공방 본체는 주 키트로 통일한다. | 수변의 소수 부속 주택 대안. 지붕을 교체하고 기단 높이를 현행 지형에 맞춘다. | 목조 항구 주택과 잔교가 있지만 초가지붕과 두꺼운 석조 강안이 눈에 띈다. 해항 프리셋 규모가 벨레스 나루보다 크다. |
| K11 [Medieval Game Environment](https://www.fab.com/listings/2e11a225-a6ea-4781-a3e1-fe975b461894) | 보류 / 많음 | 낡은 작업 구조 참고에 한정한다. 주 건축 모듈로 확인된 자료가 부족하다. | 숲 인접 건물의 재료 참고 후보. B안의 정돈된 외관으로 바꾸는 작업이 크다. | 목조 건물은 있지만 시커먼 목재, 깨진 지붕판과 강한 노후 표현이 보인다. |
| K13 [Medieval Modular Wall Pack - Kitbash Collection](https://www.fab.com/listings/b1ad8ff7-2bf2-49fc-a685-fa2af0a1d73f) | 부재 한정 / 전체 집으로는 많음 | 석재 기단·화덕 주변 벽의 국소 후보. 화덕 완제품으로 간주하지 않는다. | 석조 하부가 필요한 곳의 일부 벽 후보. 일반 주택 몸체로는 부적합하다. | 재질 이미지에서 석벽과 아치 개구부 중심임을 확인했다. 목조 골조 키트가 아니다. |

## 사용자 제외 9종

| ID | 상품 | 제외 사유 |
|---|---|---|
| K04 | [Advanced Village Pack](https://www.fab.com/listings/250a5622-d5fd-49fc-b1c4-0550a444e117) | 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다. |
| K05 | [Stylized Village Fatpack](https://www.fab.com/listings/9c5894eb-9f40-4e8b-98be-8471ae620c8b) | 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다. |
| K06 | [Stylized Windmill Valley Environment](https://www.fab.com/listings/40dfca50-70f1-42f6-91dd-1abe39672f80) | 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다. |
| K08 | [Stylized Harvest Farm Environment](https://www.fab.com/listings/d8b51a6c-a5b2-41cf-8f49-14e936a27de3) | 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다. |
| K10 | [Modular Building Set](https://www.fab.com/listings/474a0598-ed86-40b6-baa1-c801d96ef4ab) | 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다. |
| K12 | [Stylized - Clay Forest Village Pack](https://www.fab.com/listings/d2b0d6d6-c154-4bad-a579-1d9a1befd28b) | 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다. |
| K14 | [Modular House](https://www.fab.com/listings/308a9d58-2f19-4c75-b433-86545a1d7bf8) | 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다. |
| K15 | [Storage House Set](https://www.fab.com/listings/c20d8785-cee6-4c07-9ee7-6ed8d990fb84) | 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다. |
| K16 | [Watermills / Nature Environment](https://www.fab.com/listings/a2ab9a3c-4862-421a-8f58-47463695c6ba) | 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다. |

## 장소별 조립안

| 장소 | 지도 기준 | 재사용할 부재 | 보완할 것 |
|---|---|---|---|
| 공방 S02 | (60,124), 남쪽 작업 마당 (60,110) | K01 벽·보·바닥·문틀을 낮고 넓게 조립. K02는 별도 대안으로 비교한다. | 완만한 붉은 지붕, 깊은 처마, 큰 운반 개구부, 겨자 천막, 외부 화덕·배연부를 보완한다. |
| 주택 S03–S06 | 기존 건물 중심·방향·골목 폭 유지 | K01로 기본 주택 1채를 만든 뒤 창·출입구·현관·옆 증축부를 변형한다. | 청록 덧문, 밝은 회벽, 따뜻한 골조, 낮고 넓은 붉은 지붕을 공방과 공유한다. |
| 나루 부속 건물 S01 | 나루 (0,0)의 기존 배치 유지 | K07·K09의 잔교·목재 부품만 보조 후보. 주택 몸체는 주 키트와 통일한다. | 풀·초가 지붕과 현대 생활물 제거. 대형 해항·선박·성벽 프리셋은 가져오지 않는다. |
| 석조 하부·화덕 주변 | 필요한 기존 건물에 한정 | K13 석벽 부재를 국소 비교한다. | 완성 주택이나 단조 화덕 세트로 보지 않는다. 돌 크기·노후도·색을 맞춘다. |

## 상세 근거

### K01 Modular Medieval Town with Interior

- 현재 판정: 우선 시험.
- 출처: [Fab 상세](https://www.fab.com/listings/9a10f51b-8844-45c9-89c5-7d4d43d75761), Wily Pumpkin. 보유 표시 확인.
- 판매자 표기: 구조 부품 전체 모듈형, 내부 지원. 주택 프리팹 3개와 선술집 1개. 전체 메시 151개, LOD 4단계, 자동 충돌, Blueprint 14개. 메시 수는 건축 전용 수량이 아니다.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 4.16–4.27, 5.0–5.8.
- 이미지 관찰: 노출 목조 골조, 밝은 회벽, 석조 하부가 B안의 재료 구성에 가깝다. 기본 건물은 높고 좁으며 지붕이 가파르고 일부 끝이 휘어 있다. 목재도 B안보다 어둡다.
- 수정 제안: 따뜻한 목재와 밝은 회벽으로 통일. 완만한 붉은 지붕, 청록 덧문, 깊은 처마와 겨자색 천막을 보완한다.
- 미확인: 스냅 간격, 넓은 개구부 조합, 재질 슬롯과 피벗, 5.8.3 실행 미확인.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/667af605-e5eb-452f-b9db-b17bd9d0225c/0b89926a-5575-412c-8574-320b1c451a41.jpg), [갤러리 12](https://media.fab.com/image_previews/gallery_images/e3cb2009-7834-4213-8569-ad8b999a3bcd/9fa2f6fb-3567-45a7-accf-7ce8e87d41e7.jpg), [갤러리 3](https://media.fab.com/image_previews/gallery_images/4a9098ef-2dc5-49cd-b0a7-9303991f2977/56c1f8ad-5f93-4c05-910d-3d8a538605e0.jpg).

### K02 Modular Fantasy House

- 현재 판정: 조립 대안.
- 출처: [Fab 상세](https://www.fab.com/listings/32ba409c-4425-41c2-b8f1-0a582778969b), Next Level 3D. 보유 표시 확인.
- 판매자 표기: 설명상 건축 부품 약 90개와 소품 약 40개. 기술 사양 전체 메시 136개. 창·벽·계단·지붕·바닥으로 내외부 조립. LOD와 자동 충돌 표기.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 4.16–4.27, 5.0–5.5.
- 이미지 관찰: 부품 전개에서 박공·지붕 패널·보·벽·바닥이 분리되어 보인다. 골조는 단순하지만 회갈색이 강하고 석재 비중이 크다.
- 수정 제안: 석벽 비중을 줄이고 회벽·목재 중심으로 통일. 지붕 경사·폭, 청록 덧문과 천막을 보완한다. K01과 섞기 전에 치수와 텍셀 밀도를 맞춘다.
- 미확인: 지원 표기는 5.5까지다. 5.8.3 전환, 모듈 치수, UV·재질 분리와 문 동작 미확인.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/e709edb7-0ea5-4495-9500-7a0b31ebfe24/9cfef645-b842-44c8-adcd-cf2f74831e18.jpg), [갤러리 6](https://media.fab.com/image_previews/gallery_images/e4743524-b517-4723-9dec-589f2f67ba86/7e10eec9-bfc0-4037-ad03-75d8cbdf33e4.jpg).

### K03 Medieval Village

- 현재 판정: 조건부 대안.
- 출처: [Fab 상세](https://www.fab.com/listings/f89fe420-1942-4c0e-8707-e7f41dbe3017), Marco Maria Rossi. 보유 표시 확인.
- 판매자 표기: 전체 Static Mesh 496개, 건물 Blueprint 17개. 진입 가능한 내부, 동적 문·덧문·간판, Nanite와 Lumen 지원 표기.
- 제공 형식: Unreal Engine, Complete project. 지원 UE 버전: 4.11–4.27, 5.0–5.8.
- 이미지 관찰: 낮은 지붕과 생활 골목은 활용할 수 있다. 거친 석벽, 불규칙한 기와, 어두운 목재와 노후 표현은 B안보다 강하다. 부품 전개에서 벽·개구부·바닥의 분할을 확인했다.
- 수정 제안: 돌벽을 회벽·목재 중심으로 바꾸고 파손·이끼·기와 변형을 줄인다. 붉은 지붕과 청록 덧문 재질을 맞춘다.
- 미확인: 스냅 규격과 실제 수정량 미확인. Complete project의 맵·설정을 CCL에 그대로 적용하지 않는다.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/e310569f-639a-4f9e-9cc3-08682bd694dc/7f0236fd-bd77-4659-a77c-62266451513b.jpg), [갤러리 11](https://media.fab.com/image_previews/gallery_images/3689558e-fb49-4531-a372-17a56b08e76d/38b42f8b-c839-4fc5-b1ab-ec16f7937aa2.jpg), [갤러리 12](https://media.fab.com/image_previews/gallery_images/7de85a39-6d49-48c5-bdaa-f03ea094932f/ad5e5e91-08f1-4598-951a-f137a6492267.jpg).

### K04 Advanced Village Pack

- 현재 판정: 사용자 제외 (결정 67). 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다..
- 출처: [Fab 상세](https://www.fab.com/listings/250a5622-d5fd-49fc-b1c4-0550a444e117), Advanced Asset Packs. 보유 표시 확인.
- 판매자 표기: 전체 메시 126개. 주택·나무 커스터마이즈 Blueprint와 낮밤 예제 제공. 벽·지붕 단위의 자유 조립 범위는 확인하지 못했다.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 4.8–4.27, 5.0–5.4.
- 이미지 관찰: 큰 지붕과 작은 창, 과장된 집 비례가 동화풍이다. B안의 실물 비례와 재질 밀도 차이가 크다.
- 활용 제외: 수정 후 활용을 제안하지 않는다. 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다.
- 미확인: 내부, 세부 모듈 분할, 5.8.3 호환 미확인.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/a676354b-c24e-4a72-9be8-cda370ec9988/c7336010-74dd-418e-a164-dd070a8cf65b.jpg).

### K05 Stylized Village Fatpack

- 현재 판정: 사용자 제외 (결정 67). 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다..
- 출처: [Fab 상세](https://www.fab.com/listings/9c5894eb-9f40-4e8b-98be-8471ae620c8b), Meshingun Studio. 보유 표시 확인.
- 판매자 표기: 전체 고유 에셋 450개. 모듈형 벽·문·아치·판재와 소품·식생 포함. 전개에서 지붕 패널·목재 골조·벽·계단 확인. 내부 완성 지원은 미확인.
- 제공 형식: Unreal Engine, Unity, Asset package. 지원 UE 버전: 4.23–4.27, 5.0–5.8.
- 이미지 관찰: 정돈된 큰 면과 색 변화가 B안에 가깝다. 기본 건물은 뾰족하고 휜 지붕, 높은 탑과 장식이 강하다. 기와는 청록색의 둥근 골 형태다.
- 활용 제외: 수정 후 활용을 제안하지 않는다. 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다.
- 미확인: 내부, 치수·피벗, 지붕 수정 비용 미확인. 본문과 사양의 LOD 수 표기가 달라 실제 파일 확인이 필요하다.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/fe20899a-a20e-494c-a3c9-19e41b1bcfd9/edea0397-0037-44ca-b3ab-7e547b69aa23.jpg), [갤러리 24](https://media.fab.com/image_previews/gallery_images/d2a4eb8a-85f2-40be-b9e6-2fe2e0cbfc66/2a793677-ede4-4372-b0c7-bc2ada7c4c6b.jpg), [갤러리 22](https://media.fab.com/image_previews/gallery_images/6677678f-3a8a-4683-bce3-98cb4a06b9c8/3e79b125-e6ea-492f-80b4-a8330eafdf28.jpg).

### K06 Stylized Windmill Valley Environment

- 현재 판정: 사용자 제외 (결정 67). 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다..
- 출처: [Fab 상세](https://www.fab.com/listings/40dfca50-70f1-42f6-91dd-1abe39672f80), StylArts. 보유 표시 확인.
- 판매자 표기: 전체 고유 메시 93개, LOD와 자동 충돌 표기. 전개에서 건물과 분리된 벽·문·지붕 요소를 확인했다.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 5.0–5.8.
- 이미지 관찰: 밝고 따뜻한 색감은 맞지만 휜 처마, 둥근 벽과 지붕이 B안의 직선 골조와 다르다.
- 활용 제외: 수정 후 활용을 제안하지 않는다. 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다.
- 미확인: 내부와 스냅 규격, 건축 전용 부품 수 미확인.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/d17c8aad-13bb-43be-b40a-d9b71541eeb0/2ce6e53c-a90d-4f38-8805-b8545e5e7754.jpg), [갤러리 14](https://media.fab.com/image_previews/gallery_images/f72ba79a-4af4-4771-86df-40b59e8952fa/960daa9c-3abe-4105-ae07-52ff5f87eac8.jpg).

### K07 Nordic Fishing Hut - Coastal Lakeside Cabin [Interior + Exterior]

- 현재 판정: 수변 보조.
- 출처: [Fab 상세](https://www.fab.com/listings/e56e7e02-01b8-4a15-a108-aa5d76ea0e10), Studio Fjuna. 보유 표시 확인.
- 판매자 표기: 집·잔교 모듈과 Level Instance, 열리는 문·창. 전체 메시 569개. LOD 대신 Nanite 표기. 일부 POM·RVT 재질. 소개용 Fluidflux·Ultra Dynamic Sky는 미포함이라고 명시.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 5.2–5.8.
- 이미지 관찰: 실사 목재 오두막이며 풀 지붕과 노후 판벽이 B안과 다르다. 대표 이미지에 자전거가 있고 설명에 가전·전기 설비가 포함된다.
- 수정 제안: 풀 지붕을 교체하고 노후도를 낮춘다. 가전·전기 설비·자전거·국기 등 현대 생활물을 제외한다.
- 미확인: 집 모듈 치수·혼합성, RVT 의존성과 실제 성능 미확인.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/299b50ef-934a-4aac-bd2e-79927223de39/e7b5ba3f-9ef1-4e4a-bc5d-0ca4bdfad368.jpg).

### K08 Stylized Harvest Farm Environment

- 현재 판정: 사용자 제외 (결정 67). 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다..
- 출처: [Fab 상세](https://www.fab.com/listings/d8b51a6c-a5b2-41cf-8f49-14e936a27de3), Emberlight Creations. 보유 표시 확인.
- 판매자 표기: 전체 메시 61개, 자동 충돌, LOD 없음. 벽·지붕 단위 모듈 건축으로 확인되지 않았다.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 4.27, 5.0, 5.7–5.8.
- 이미지 관찰: 붉은 헛간, 과장된 소품, 평면적인 색과 둥근 형태가 농장 게임풍이다.
- 활용 제외: 수정 후 활용을 제안하지 않는다. 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다.
- 미확인: 건축 부품 분할과 내부 미확인.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/ae3003cd-e0ce-4b36-b813-9391ea7f2847/bef5ddc2-5a7f-411e-9b10-5bd844184ea0.jpg).

### K09 Medieval Docks

- 현재 판정: 수변 보조.
- 출처: [Fab 상세](https://www.fab.com/listings/e953b0a4-e384-431d-88fa-470c32874e9a), PolyPixel. 보유 표시 확인.
- 판매자 표기: 다층 모듈 건물과 내부, 벽·창·계단·지붕·바닥. 건물 배치 10종과 각각 4변형 표기. 세부 부품 수는 본문과 사양이 달라 확정하지 않았다.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 4.9–4.27, 5.2.
- 이미지 관찰: 목조 항구 주택과 잔교가 있지만 초가지붕과 두꺼운 석조 강안이 눈에 띈다. 해항 프리셋 규모가 벨레스 나루보다 크다.
- 수정 제안: 성벽·대형 선박·다층 잔교를 그대로 가져오지 않는다. 목재 색과 지붕을 B안에 맞춘다.
- 미확인: 5.8.3 전환, 스냅 규격과 기존 3m 잔교 폭에 맞는 조합 미확인.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/61c58083-33cb-401f-b2a7-c3c4d52f9636/26a53552-7d6b-42c9-88ec-90970bf9cf37.jpg).

### K10 Modular Building Set

- 현재 판정: 사용자 제외 (결정 67). 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다..
- 출처: [Fab 상세](https://www.fab.com/listings/474a0598-ed86-40b6-baa1-c801d96ef4ab), PurePolygons. 보유 표시 확인.
- 판매자 표기: 교환 가능한 모듈·재질. 내부는 1층 지원이며 실내 소품은 미포함이라고 설명한다.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 4.5–4.27, 5.0–5.4.
- 이미지 관찰: 도시 상가용 창, 평지붕, 석재 입면과 현대 간판 중심이다.
- 활용 제외: 수정 후 활용을 제안하지 않는다. 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다.
- 미확인: 5.8.3 호환 미확인. Modular라는 이름만으로 적합 판정하지 않았다.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/215b65c5-d245-4e85-a22f-fd6d2d9d9740/f94f0327-147a-479b-9565-619cf1449a6f.jpg).

### K11 Medieval Game Environment

- 현재 판정: 보류.
- 출처: [Fab 상세](https://www.fab.com/listings/2e11a225-a6ea-4781-a3e1-fe975b461894), Quixel Megascans. 라이브러리 제목: Medieval Village Megascans Sample. 보유 표시 확인.
- 판매자 표기: 라이브러리와 상세 제목이 다르지만 동일 링크다. 건축·식생·소품을 포함한 샘플 프로젝트. UE5.3의 Nanite·Lumen·World Partition 전환 설명. 본문에 UE 기반 제품 전용 문구가 있다.
- 제공 형식: Unreal Engine, Complete project. 지원 UE 버전: 4.26–4.27, 5.3–5.9.
- 이미지 관찰: 목조 건물은 있지만 시커먼 목재, 깨진 지붕판과 강한 노후 표현이 보인다.
- 수정 제안: 노후도와 불규칙한 형태를 줄이고 지붕·골조를 정돈해야 한다.
- 미확인: 건축 부품 수, 스냅과 내부 미확인. 5.9는 사이트 표기이며 실행 검증 결과가 아니다.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/42edc74f-c63b-4e84-b2df-74da89d9c679/fd5c4e89-cafc-4268-97d4-7003ed0beacc.jpg), [갤러리 5](https://media.fab.com/image_previews/gallery_images/e91c6781-1668-44f8-b00f-607d8de8a903/79309ad1-300a-41a8-bc6f-52ccd53630c5.jpg).

### K12 Stylized - Clay Forest Village Pack

- 현재 판정: 사용자 제외 (결정 67). 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다..
- 출처: [Fab 상세](https://www.fab.com/listings/d2b0d6d6-c154-4bad-a579-1d9a1befd28b), Studio Kobo. 보유 표시 확인.
- 판매자 표기: 집·소품·식생 42개, 점토 재질과 Toon Outline·Kuwahara 후처리 제공.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 4.26–4.27, 5.0–5.6.
- 이미지 관찰: 둥근 점토 형태와 장난감 같은 집 비례가 B안과 다르다.
- 활용 제외: 수정 후 활용을 제안하지 않는다. 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다.
- 미확인: 내부와 건축 모듈 조립 범위 미확인.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/61a456e7-fdd8-47b4-b3aa-77a578afb17f/b3992ac8-a5b0-4b4d-bc20-4dcf8ed051b9.jpg).

### K13 Medieval Modular Wall Pack - Kitbash Collection

- 현재 판정: 부재 한정.
- 출처: [Fab 상세](https://www.fab.com/listings/b1ad8ff7-2bf2-49fc-a685-fa2af0a1d73f), Vampawn. 보유 표시 확인.
- 판매자 표기: 벽 48개, 5cm·10cm 배수 조립과 실물 축척 표기. 4K PBR. 지붕·바닥을 포함한 완성 건축 세트는 아니다.
- 제공 형식: Unreal Engine, Blender, glb, OBJ, FBX, Asset package. 지원 UE 버전: 5.5.
- 이미지 관찰: 재질 이미지에서 석벽과 아치 개구부 중심임을 확인했다. 목조 골조 키트가 아니다.
- 수정 제안: 석재 크기·색을 맞추고 회벽·목재·지붕은 별도로 마련한다.
- 미확인: 정확한 모듈 크기·두께, 양면 마감·충돌과 5.8.3 전환 미확인.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/68eca06f-a57e-40b9-8e35-0adf4e9de1c6/7e662a22-02fe-4d2e-b934-6924d2c27103.jpg), [갤러리 2](https://media.fab.com/image_previews/gallery_images/71fd4cb7-d3a4-4022-a3a2-36f00dff2f9a/3721b7cb-8236-40ef-9236-9161cfc48479.jpg).

### K14 Modular House

- 현재 판정: 사용자 제외 (결정 67). 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다..
- 출처: [Fab 상세](https://www.fab.com/listings/308a9d58-2f19-4c75-b433-86545a1d7bf8), QuadArt. 보유 표시 확인.
- 판매자 표기: 내외부 모듈, 200개 이상 모듈과 주택 19종 표기. 문·창·차고 개폐.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 4.14–4.27, 5.0–5.8.
- 이미지 관찰: 현대 북미 교외 주택, 사이딩 외벽과 현대 창호가 기준이다.
- 활용 제외: 수정 후 활용을 제안하지 않는다. 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다.
- 미확인: 5.8.3 실행 미확인. 기능 수와 시각 적합성은 별개다.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/1c4d8ee2-7d82-4c60-abc0-3d8da7b79bb6/d88f5f45-0866-44f1-83b0-5e6c02dd898e.jpg).

### K15 Storage House Set

- 현재 판정: 사용자 제외 (결정 67). 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다..
- 출처: [Fab 상세](https://www.fab.com/listings/c20d8785-cee6-4c07-9ee7-6ed8d990fb84), Oleksandr Sychov. 보유 표시 확인.
- 판매자 표기: 건축·소품 메시 400개 이상, 모듈 구성과 재질 색 변경 제공.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 4.15–4.27, 5.0–5.8.
- 이미지 관찰: 금속 선반, 형광등, 공업용 바닥의 현대 물류 창고다.
- 활용 제외: 수정 후 활용을 제안하지 않는다. 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다.
- 미확인: 건축 외 용도의 소품 전수 검토는 하지 않았다.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/f1b0abcd-e3fd-4149-ae0d-03a8436caf7a/5e1010b5-e6aa-47aa-8797-11aad2a07f83.jpg).

### K16 Watermills / Nature Environment

- 현재 판정: 사용자 제외 (결정 67). 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다..
- 출처: [Fab 상세](https://www.fab.com/listings/a2ab9a3c-4862-421a-8f58-47463695c6ba), Leartes Studios. 라이브러리 제목: Watermills / Nature Environment ( Watermills Nature Environment Nature Ruin ). 보유 표시 확인.
- 판매자 표기: 수차·자연 환경 장면과 에셋, ULAT 호환을 설명한다. 도구는 별도 지원 요청으로 받을 수 있다고 안내하며 수령은 실행하지 않았다.
- 제공 형식: Unreal Engine, Asset package. 지원 UE 버전: 4.26–4.27, 5.0–5.8.
- 이미지 관찰: 짙은 이끼, 폐허 같은 석벽과 깊은 그늘 중심이다. B안 생활 마을보다 폐허 환경에 가깝다.
- 활용 제외: 수정 후 활용을 제안하지 않는다. 사용자 검토: B안의 아트풍·시대배경과 맞지 않아 활용 후보에서 제외한다.
- 미확인: 건축 부품 수, 스냅·내부와 5.8.3 실행 미확인.
- 열어 본 이미지: [갤러리 1](https://media.fab.com/image_previews/gallery_images/f70021ab-76cb-4910-8f66-4ce0fd083e4c/86df219e-e574-4c43-862c-75b355aff013.jpg).

## 다음 검증 제안

1. K01로 공방 1채와 기본 주택 1채를 같은 조명·카메라·캐릭터 기준으로 시험한다. K02는 별도 비교안으로 둔다.
2. 모듈 크기·피벗·문 높이·골조 접합·내부 이동·충돌을 검사한다. 건물 전체를 비균일 확대해 지도 면적에 끼워 맞추지 않고 부재 수로 조절한다.
3. 지붕 경사와 덧문·천막을 맞춘 뒤 재질의 색·거칠기·텍셀 밀도를 통일한다. Megascans의 공구·작업대·돌·식생은 이 외관 기준에 맞춰 추가한다.
4. 시안과 플레이 화면이 맞는지 확인한 뒤 주택 변형을 늘린다. 원본 86개 건물 중심·방향을 유지하며, 86종의 고유 건물을 만들 필요는 없다.

수정량은 상대적 추정이며 인일 견적이 아니다. 이번 작업은 비교·선별에서 종료했고, 모델 수정과 프로젝트 임포트는 진행하지 않았다.
