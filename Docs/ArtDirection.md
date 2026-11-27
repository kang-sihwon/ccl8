# 아트 방향 초안

Orbis의 그래픽 스타일을 ccl8의 마을, 전투 구간, 보스 공간에 적용한다. 이 문서는 그 방향을 구체화한 시각 기준 제안, 제작 방식 제안, 검토할 산출물을 소유한다. Orbis와 스타일을 공유하는 근거는 [결정 1](decisions/0001-production-scope.md)이다. 지도는 [결정 34](decisions/0034-concept-and-ingame-map-roles.md)에 따라 개념 지도와 인게임 지도 UI의 용도를 구분하고, [결정 43](decisions/0043-map-art-and-ui-standard.md)의 화풍을 구체화해 [결정 46](decisions/0046-unified-map-v2-adoption.md)으로 확정한 v2의 화풍·공통 UI·지명 표기를 제작 기준으로 사용한다. 그 밖의 세부 제안은 AI 해석이며 게임 장면의 최종 룩과 캐릭터 디자인은 사용자 검토가 필요하다.

## 하위 문서

| 문서 | 읽는 때 |
|---|---|
| [벨레스 자연환경 사진 자료집](ArtDirection/VelesNature-2026-11-26/index.html) · [장면별 배치 제안](ArtDirection/VelesNature-2026-11-26/README.md) | 나루에서 상류 개울까지 실제 자연, 게임, 벨레스 화면을 비교하고 나무와 관목을 어디에 모을지 정할 때 |
| [중세 주거 건축 도감](ArtDirection/MedievalDwellings-2026-11-26/README.md) · [사진 갤러리](ArtDirection/MedievalDwellings-2026-11-26/index.html) | 유럽과 지중해의 농가·점포 주택·저택·거주용 성 109곳에서 집의 평면, 지붕선과 규모를 비교할 때 |
| [벨레스 B안 건축 키트 비교](ArtDirection/VelesBuildingKits-2026-11-18.md) | 조사한 16종 중 제외 9종의 사유와 남은 7종의 공방·주택 활용안을 비교할 때 |
| [벨레스 출항 전 지역 콘셉트](ArtDirection/VelesDepartureRegions-2026-11-25/index.html) | 마을·삼림의 조감도와 동굴 단면을 세부 장소별 환경 시안 13장과 함께 볼 때 |
| [UE5_2 리소스 조사](ArtDirection/UE52ResourceSurvey-2026-11-20.md) | 원본 팩을 가로질러 집·나무·조립 부재를 찾고 전체 파일의 경로와 UE 클래스를 확인할 때 |
| [UE5_5 석조 건축 부재](ArtDirection/UE55ResourceSurvey-2026-11-24.md) | 문틀·창·벽 48개의 형태를 비교하고 정리 후 경로를 찾을 때 |
| [MedievalEnvironment 중세 마을 리소스](ArtDirection/MedievalEnvironmentResourceSurvey-2026-11-24.md) | 지붕·벽·수목·대장간 도구를 고르고 마을 맵과 외부 액터의 새 경로를 확인할 때 |
| [벨레스 아트 콘셉트 3안](ArtDirection/VelesConcepts-2026-11-17.md) | 리소스 선정 전에 현실감·반실사·형태 중심의 목표 화면을 비교할 때 |
| [에테르 첫 장 MJ 시안 목록](ArtDirection/EtherFirstChapter.md) | 도입부터 바르툴전 이후 출항까지 YouArt에서 만들 디자인과 장면 시안을 고를 때 |
| [에테르 세계지도 시안](ArtDirection/EtherWorldMap.md) | 육지 35%·이그니스 2,000만 km²의 계산 보정 지도와 대륙·벨레스 지역 확대를 볼 때 |
| [지도 제작 기준과 채택 시안](ArtDirection/EtherWorldMap/MapPresentation.md) | 벨레스·이그니스·에테르 최종 시안 v2와 이후 지도의 화풍·UI·지명 표기 기준을 적용할 때 |
| [Quixel Megascans 3D 리소스 조사](ArtDirection/QuixelMegascans-2026-11-16.md) | 8,825개 상품의 전체 검색표와 상세 확인한 41개 후보를 세계관·지역·제작 조건별로 비교할 때 |
| [벨레스 Megascans 상세·이미지 선별](ArtDirection/QuixelMegascansReview.md) | 우선 후보 2,387개의 상세 검토 진행과 실제 이미지로 추린 공방·나루·숲 리소스를 비교할 때 |

## 참고 근거

2026-10-05에 확인한 Orbis의 `Docs/ArtConcept.md`를 요약했다. 원문은 기본 렌더링 방향을 후보로, 주인공 성별과 세부 디자인을 미결로 표시하므로 원문의 후보를 ccl8의 확정안으로 취급하지 않는다. 이 문서는 참고 방향만 기록하고 Orbis 원문·이미지·얼굴 후보 풀을 가져오지 않는다.

## 시각 기준(제안)

벨레스 환경 콘셉트는 [결정 66](decisions/0066-veles-b-art-direction.md)에 따라 [B안의 서정적인 반실사 화풍](ArtDirection/VelesConcepts-2026-11-17/B-lyrical.png)을 기준으로 한다. 후속 [공방·나루·숲 시안](ArtDirection/VelesConcepts-2026-11-17/B-scenes.md)은 기존 지도·배치 위치를 반영한다. B 원본의 압축된 공간 구성과 아래 다른 미확정 항목까지 채택한 것은 아니다.

| 항목 | ccl8에 적용할 방향 |
|---|---|
| 전체 스타일 | PBR 재질을 바탕으로 자연스러운 빛, 생활감 있는 판타지 공간과 읽기 쉬운 큰 색면을 구성한다 |
| 마을·일반 지역 | 서정적인 자연, 사용 흔적이 있는 목재·직물·석재, 편안한 노출과 대기 원근을 쓴다 |
| 보스 공간 | 침식된 구조물, 차가운 색, 낮은 채도와 제한된 광원으로 불길함과 공간의 압도감을 만든다 |
| 지역 간 일관성 | 같은 재질·노출 기준을 유지하고 공간 형태, 명도, 색으로 분위기를 바꾼다 |
| 전투 가독성 | 어두운 곳에서도 공격 예고, 무기 궤적, 캐릭터 실루엣과 바닥 경계를 읽을 수 있게 한다 |

대항해 문화, 해안 지형, 천체 장식은 Orbis의 구체적 소재다. ccl8에서 얼마나 쓸지는 마을과 시나리오 초안에 맞춰 제안한다. 천체·계절·동적 날씨와 실험장 구현 범위는 [EnvironmentPlan](EnvironmentPlan.md)이 소유한다.

## 제작 방식(제안)

먼저 작은 전투 공간에 캐릭터, 적, 바닥, 배경을 함께 배치해 조명·재질 기준을 정한다. 정지 이미지뿐 아니라 실제 이동과 공격 중에도 실루엣이 구분되는지 확인한다.

쓸 수 있는 리그, 애니메이션, 환경 에셋의 일관성을 조사한 뒤 직접 제작 범위를 정한다. 에셋 재사용은 제작 기간을 줄이지만 비례·질감·동작의 차이를 맞추는 작업이 필요하다. 전면 직접 제작은 통일성을 확보하기 쉽지만 모델링과 애니메이션 제작량이 커진다.

컨셉 시안과 3D 모델링용 기준 이미지에 쓸 생성 도구는 [컨셉 이미지 도구 비교 테스트](ConceptImageToolTest.md)로 고른다.

대표 시간대 하나의 룩을 먼저 검증한다. 여러 시간대와 날씨의 시각 결과는 EnvironmentPlan의 실험장에서 검증하고 전투 가독성을 함께 확인한다.

## 검토할 산출물

- 마을과 보스 공간을 같은 화면 기준으로 비교할 수 있는 무드보드
- 플레이 가능한 대표 전투 공간의 조명·재질 샘플
- 주인공·일반 적·보스의 실루엣과 애니메이션 호환성 초안
- [벨레스 장인 정면 시안과 게임 레퍼런스](ArtDirection/VelesArtisanWomen-r8-2026-11-20/references.md)

## 미정

- 이미지 시안은 하위 문서에서 후보를 비교한다. 최종 에셋 선정과 구매는 정하지 않았다.
- 성별, 얼굴, 의상, 건축 문화, 몬스터 외형.
- 하드웨어, 프레임 목표, 렌더링 기능. ccl8 장면에서 측정하기 전까지 정하지 않으며 Orbis의 수치를 그대로 쓰지 않는다.
- 벨레스 마을의 창·골목·작업 마당·강변 형태는 [시각 자료집](ArtDirection/VelesVillageReferences-2026-11-25/README.md)에서 레퍼런스를 고른 뒤 콘셉트 시안으로 검토한다.
- [벨레스 마을 콘셉트 10안](ArtDirection/VelesVillageOptions-2026-11-25/index.html)에서 골목형 주거지, 공방 마당, 나루, 경작지 등 공간 구성에 따른 차이를 비교한다. 레벨에 적용할 안은 사용자 검토 뒤에 정한다.
