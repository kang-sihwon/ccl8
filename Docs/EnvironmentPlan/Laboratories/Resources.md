# 실험장 리소스

전용 레벨에 연결한 실험용 리소스, Fab 후보, 제출 범위를 소유한다. 상위 문서는 [기능별 환경 실험장 사용법](../Laboratories.md)이다.

## 리소스와 제출 범위

### 연결한 실험용 리소스

나무 외에 `SM_Grass_01`, `SM_Flower_01`, `SM_Bush_01`과 실제 의존성을 보유 Fab 폴더에서 공유 폴더로 복사했다. `copy_laboratory_foliage.py`가 복사를, `configure_laboratory_life.py`가 전용 레벨 연결을 수행한다.

동물은 [Quaternius의 양](https://opengameart.org/content/lowpoly-animated-farm-animal-pack), mujtaba-io의 [비둘기](https://opengameart.org/content/low-poly-3d-pigeon-model-rigged-animated-untextured)와 [개미](https://opengameart.org/content/ant-3d-model-rigging-animated-low-poly-ish)를 사용한다. 이 세 출처는 CC0다. 초기 [Quaternius 물고기](https://quaternius.com/packs/animatedfish.html)는 보유 Fab AnimatedPondFish의 `SKM_Fish`와 `SKM_Fish_Animation`으로 교체했다. CC0 원본의 사용 파일과 변환 내역은 `Tools/Validation/laboratory_animal_sources.json`에 남아 있다.

`copy_laboratory_fern_pond.py`는 선택한 양치·물고기 메시, 수영 애니메이션과 의존성을 포함한 에셋 20개를 `/Game/TestResources/PN_Fern`, `/Game/TestResources/AnimatedPondFish`로 복사한다. `configure_laboratory_fern_pond.py`는 기존 다른 종류의 설정을 유지하면서 물고기 프로필과 양치 메시를 교체한다. 물고기는 14본, 수영 루프 1.25초이며 +Y인 머리 방향을 -90° 회전해 이동 방향 +X에 맞췄다. 크기는 1.25배다. 원본 Fab 폴더는 보존하고 데모 맵·캐릭터·이동 Blueprint는 공유 폴더에 포함하지 않는다.

Blender 원본의 리그 이름을 보존한 FBX를 사용한다. 초기 양·물고기 임포트는 Unreal의 `Armature` 제거 처리 후 애니메이션 배율이 맞지 않아 화면에 정상 표시되지 않았다. `export_laboratory_animal.py`와 `import_laboratory_animals.py`는 명시적 리그 루트를 유지하며, 양의 걷기·대기는 별도 베이크 후 `import_laboratory_gaits.py`로 연결한다. 동물의 표시 프로필은 레벨의 `CreatureVisuals` 배열에 메시·이동·대기·좌표 보정·기준 속도를 저장한다.

### 보유 Fab 라이브러리에서 확인한 후보

2026-10-14 로그인된 Fab 라이브러리에서 다음 제품의 보유 상태를 확인했다. 로컬 프로젝트의 에셋 목록과 계정의 보유 목록은 다르다. 이후 PN_Fern과 AnimatedPondFish는 사용자가 다운로드했으며 위의 선택 에셋을 연결했다. Advanced Flock System은 도입하지 않았다.

| 용도 | 보유 제품과 제품 페이지에서 확인한 구성 | 적용 판단 |
|---|---|---|
| 지상 동물·새·곤충 | [Stylized Fantasy Creatures Bundle](https://www.fab.com/listings/8e208ab4-667d-4a86-870a-f0c3e9b0e080): 21종, Skeletal Mesh 26개, 리그·애니메이션 포함. 양 24개·독수리 15개·말벌 12개 애니메이션 | 사용자가 보유 버전 문제를 알려 적용 후보에서 제외함 |
| 물고기·수생 식물 | [Animated Pond Fish](https://www.fab.com/listings/91c3a100-3ca3-49c3-bd40-73152ee1f35e): 리그·애니메이션이 있는 물고기 5개, 수련 3개, 연못·수면 포함 | 선택한 물고기 메시와 수영 루프를 연결함. 수생 식물은 미적용이며 팩의 이동 Blueprint는 사용하지 않음 |
| 군집 이동 | [Advanced Flock System](https://www.fab.com/listings/70626312-656b-4037-8e69-4cba921d04d4): 영역 제한, 장애물 회피, 추종과 군집 매개변수를 제공하는 플러그인 | 물고기 군집 이동의 검토 후보. 세계 시간 배율·결정론·저장·Actor/Mass 전환과의 호환성은 미검증 |

위 세 제품 페이지는 UE 5.8 지원을 표시한다. 실제 도입 여부와 검증 범위는 위 표와 [검증 기록](Verification.md#검증-범위)을 따른다. 식생 라이브러리에서도 기존 후보인 optimized Grass Library, Meadow Flowers, Fern Collection과 Norway Maple의 보유 상태를 확인했다. 양치는 다운로드한 PN_Fern에서 선정했다.

### 생물과 식생의 공개 Fab 후보

2026-10-14 제품 페이지의 형식과 설명을 확인한 후보 목록이다. 이후 실제 연결한 리소스는 위의 연결 기록을 따른다. 나머지 후보의 UE 5.8.3 임포트와 애니메이션 재생은 미검증이다. 생물은 현재 생태 계산의 위치·활동 상태에 메시와 애니메이션을 연결해야 한다. 제품에 포함된 별도 AI를 그대로 실행하면 프로젝트의 이동·먹이 계산과 중복될 수 있다.

| 용도 | 후보와 확인한 구성 | 연결할 내용 |
|---|---|---|
| 새 | [Birds](https://www.fab.com/listings/c5b54173-3087-4060-9380-5f9c500c31ec), UE 형식, 까마귀·비둘기·갈매기, 87개 애니메이션 | 공중 이동 방향에 비행·회전 애니메이션 연결. 착지 행동은 현재 실험에 별도 구현 필요 |
| 물고기 | [Freshwater Fish Collection Animated](https://www.fab.com/listings/e9b2af6f-cc6a-4d29-baa1-3155b3f403bf), 15종, Maya·FBX와 애니메이션 | 수중 경로를 따라 헤엄 재생. FBX 임포트·스케일·스켈레톤 확인 필요 |
| 지상 동물 | [Animalia Red Deer](https://www.fab.com/listings/b50abc7d-c347-469f-b130-c6536f1e420a), 사슴과 애니메이션, gFur 필수 아님 | 이동·섭취·정지 상태와 동작 연결 |
| 곤충 | [Butterfly Animated](https://www.fab.com/listings/8761bf1c-339c-444e-b725-90600aa64c94), 리그와 비행 반복 2개 | 변환 형식과 UE 임포트 확인 후 비행 루프 연결 |
| 풀 | [optimized Grass Library](https://www.fab.com/listings/8b68642e-35f4-438e-82b4-799fc2228303), 무료 표시, UE 형식, 12종·110개 군집과 Pivot Painter 바람 | 생체량 크기, 고사·적설 재질, 환경 풍속 연결 |
| 꽃 | [Meadow Flowers](https://www.fab.com/listings/e648b1bd-11f6-4895-bd43-15e399c80297), 무료 표시, UE 형식, 20종·96개 군집 | 꽃 종류와 성장 크기 연결. 조회 페이지에 지역 제한 안내가 있어 계정에서 확인 필요 |
| 양치 | [Fern Collection](https://www.fab.com/listings/b778bd8f-524c-42b6-b60c-4caac59029c1), 무료 표시, UE 형식, 8종과 셰이더 바람 | 식생 상태와 바람 파라미터 연결 |
| 수생 식물 | [Aquarium Plants Collection](https://www.fab.com/listings/3bdf1ee1-0c9d-45b3-9420-925383ee2ff3), UE 형식, 성장·바람 애니메이션 | 실제 물 깊이에 맞춰 배치하고 생체량을 성장 파라미터에 연결 |
| 버섯 | [Mushroom Pack](https://www.fab.com/listings/b15ed5a5-3e34-4ad6-8fdc-f97f6f6d6101), 49개 메시, UE·FBX 형식 | 성장·고사 크기와 상태 재질 연결 |
| 관목 | [Realistic bush](https://www.fab.com/listings/fe9c415b-ccac-4e89-925f-f5bf7543aae5), UE 형식, 여러 수형과 4K 텍스처 | 패치 크기에 맞는 수형 선정 후 성장·휴면 상태 연결 |
| 이끼 | [Realistic Moss Foliage Plants Pack](https://www.fab.com/listings/fe0260a4-1a8f-459d-bb2f-a58ea6561345), UE 형식, 이끼 배치 Blueprint 포함 | 이끼 메시만 추출해 토양 수분·생체량과 연결 |
| 조류 | [Water with algae PBR Textures](https://www.fab.com/listings/8f8f716b-f3cc-4dfe-a2b1-4102fd3338ed), 물·진흙·조류 혼합 PBR 텍스처 8종 | 조류는 수면 패치 재질로 표현하는 후보. 독립 식물 메시가 아니며 생체량에 따라 피복률을 조절할 구현 필요 |
| 부패 음식 | [4K Scanned Bread](https://www.fab.com/listings/5d913fa4-a175-4365-869a-ea0061e20930?lang=en), 통빵·반쪽·슬라이스 3종, FBX·Blender 형식, 메시당 3–7천 삼각형 | 현재 실험용 빵의 외형 후보. 신선도 색 보간과 곰팡이 마스크를 별도 연결해야 하며 부패 기능이 포함된 상품은 아님 |

나무는 현재 복사된 Norway Maple을 활용할 수 있다. 계정 보유 여부는 위의 보유 목록에서 확인한 제품에 한정한다.

실제로 배치한 Fab 메시·Niagara와 그 머티리얼·텍스처 의존성을 `/Game/TestResources`로 복사하고 참조를 바꿨다. 원본 `/Game/Fab`는 보존하며 `.gitignore`로 제외했다. 기존 프로젝트의 마네킹·환경 머티리얼 등은 원래 공유 경로를 유지한다.

추가 Fab 리소스 제출 대상은 `Content/TestResources/`다.

`Tools/Validation/create_test_laboratories.py`는 누락된 전용 레벨과 리소스 사본을 만든다. `verify_test_resources.py`는 원본 Fab 참조와 복사 리소스의 누락된 의존성을 검사한다. 둘 다 에디터 Python에서 실행하며 결과는 `Saved/Laboratories/`에 남긴다. 에셋 파일은 저장소의 LFS 규칙을 따른다.

`update_laboratory_presentation.py`는 기존 전용 레벨에 강수 리소스를 연결하고 물고기 수면 재질을 만든다. Clock을 삭제하기 전에 패키지 참조가 없는지 검사한다. PIE를 종료한 에디터에서 실행한다.
