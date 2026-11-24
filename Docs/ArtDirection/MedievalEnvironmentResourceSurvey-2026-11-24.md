# MedievalEnvironment 중세 마을 리소스

UE 5.8.3의 `MedievalEnvironment` 프로젝트에서 직접 배치하거나 편집하는 에셋 2,728개를 `/Game/ExternalPackage` 아래에 모았다. 마을의 집을 조립할 때는 `BuildingParts`에서 지붕·벽·기둥을 고른다. 원작자가 이 부재를 어떻게 조합했는지는 `Demo/MedievalEnvironment/Maps/MedievalVillage_P_WP`에서 확인할 수 있다.

| 찾는 대상 | `ExternalPackage` 아래 경로 | 수량과 예시 |
|---|---|---|
| 지붕·벽·기둥 | `BuildingParts/Roofs`, `BuildingParts/Walls`, `BuildingParts/BeamsAndPillars` | 지붕 87개, 벽 30개, 보·기둥 24개. `SM_House04`는 원본의 지붕 부재다. |
| 문·창 | `BuildingParts/DoorsAndGates`, `BuildingParts/WindowsAndShutters` | 문 7개, 창·덧문 5개. `SM_MedievalModularDoor1x2M`처럼 크기가 이름에 들어간 부재가 있다. |
| 나무·야생동물 | `Nature/Trees`, `Nature/Wildlife/Crow` | 가문비나무 메시 15개. 까마귀의 메시·리그·애니메이션·AI는 `Crow`에서 찾는다. |
| 대장간·생활 소품 | `Props/WorkshopTools`, `Props/Containers`, `Props/Lighting` | 공방 도구 21개, 용기 10개. 모루는 `SM_BlacksmithAnvilA`, 망치는 `SM_BlacksmithHammer`다. |
| 우물·울타리·도로 | `Structures/Wells`, `Structures/FencesAndRailings`, `Structures/Roads` | 우물 8개, 울타리·난간 14개, 길 5개. 우물 몸체와 손잡이를 따로 고를 수 있다. |
| 재질·텍스처·소리 | `Materials/MedievalEnvironment`, `Textures/MedievalEnvironment`, `Audio/MedievalEnvironment` | 재질 계열 508개, 텍스처 계열 919개, 오디오 계열 479개. |

에셋 이름이 같은 항목은 원본 폴더 이름으로 하위 경로를 구분했다. 예를 들어 우물 부재 `SM_WellBase`는 원래 경로에 따라 구별할 수 있다. [전체 패키지의 원래 경로와 새 경로](MedievalEnvironmentResourceSurvey-2026-11-24/ExternalPackage-paths.tsv)

## 월드 파티션과 참조 확인

월드 파티션이 소유하는 외부 액터 패키지 7,103개와 외부 오브젝트 패키지 5개는 엔진의 `__ExternalActors__`와 `__ExternalObjects__` 아래에서 소유 맵의 새 경로를 따른다. 마을 맵은 액터 6,965개, `Test` 맵은 138개다. 두 맵 모두 원본과 액터 GUID, 스트리밍 범위, 공간 로딩 설정, 런타임 그리드, 데이터 레이어가 일치했다.

검증 사본에서 일반 에셋 2,721개를 모두 로드했다. 정적 메시의 재질 슬롯·바운드·LOD 수, 재질 인스턴스의 부모와 텍스처, 캐릭터 에셋의 스켈레톤을 원본과 대조했다. 구버전 패키지를 UE 5.8.3에서 다시 저장한 뒤 달라진 레지스트리 참조와 메시 통계 태그는 같은 버전에서 원래 경로로 다시 저장한 대조본과 일치했다.

실제 프로젝트에 적용한 뒤 9,836개 패키지의 에셋 레지스트리가 검증 사본과 일치했다. 프로젝트 내부에서 찾을 수 없는 하드 참조 대상은 0개였고, 소프트 참조 대상 39개는 원본에도 없었다. 단독 리디렉터 패키지는 0개였다. 맵 7개를 모두 다시 열어 로드된 액터의 이름·클래스·트랜스폼·메시를 대조했고, 일반 에셋 52종도 클래스별로 하나씩 다시 로드했다.

프로젝트 시작 맵·게임 모드·패키징 대상 맵의 경로는 `DefaultEngine.ini`와 `DefaultGame.ini`에 반영했다. 원본 콘텐츠와 두 설정 파일은 `MedievalEnvironment/Saved/BeforeExternalPackage-20261124`에 보관했다.
