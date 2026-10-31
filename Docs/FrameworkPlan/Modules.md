# 공용 모듈의 경계와 계층

새 모듈을 만들거나 모듈 사이의 의존을 바꿀 때 읽는다. 실제 의존 목록의 정본은 각 모듈의 `Build.cs`다.

## 독립 선택의 기준

독립 시스템은 UE5와 작은 공통 계약에는 의존할 수 있지만, 사용하지 않는 다른 게임 시스템을 필수로 요구하지 않아야 한다. 예를 들어 UI만 선택한 프로젝트에는 인벤토리·Agent·지형 구현이 필요하지 않아야 한다.

1. 게임이 공용 시스템을 참조하고, 공용 시스템은 특정 게임의 클래스·맵·에셋을 참조하지 않는다.
2. 게임마다 달라질 규칙과 데이터는 공개 계약으로 전달한다. 변경되지 않는 공통 실행은 시스템이 구현한다.
3. 상태의 소유자, 생성·해제 시점, 서버 권위와 복제 범위를 시스템별로 명시한다.
4. 시스템 사이의 연결은 선택 가능한 어댑터로 둔다. 참조 방향을 바꾸기 위해 모든 데이터를 공통 모듈에 몰아넣지 않는다.
5. 시간과 저장은 여러 시스템이 기대는 바닥 계약이다. 저장 참여, 구간 등록, 시간 읽기 같은 공통 계약은 `KSICore`에 두고 구체 시계와 저장 실행은 별도 모듈로 둔다. 외부 시각·저장 참여 계약만으로 충분한 시스템은 그 입력만 받는다.

현재 CCL의 구현이 이 조건을 모두 만족한다는 판정은 아니다. 시스템을 추출할 때 실제 코드·에셋·설정 의존을 다시 확인한다.

## 공용 모듈 목록

| 모듈 이름 | 공용 책임 | 독립 사용 확인 사례 | 관련 사양 |
|---|---|---|---|
| `KSICore` | 저장 참여·구간 등록·시간 읽기·표면 질의처럼 여러 시스템이 공유하는 계약 | 다른 `KSI` 모듈 없이 엔진에만 의존해 컴파일 | [세계 저장 참여자 학습](../Learning/EnvironmentRestoreParticipants.md) |
| `KSIUI` | 화면 등록·수명·레이어·입력 문맥·표시 제어 | 전투나 인벤토리 없이 메뉴·도구 화면 사용 | [UIFoundation](../UIFoundation.md) |
| `KSIInventory` | 아이템 정의·인스턴스·보관·이동·소유권 | 캐릭터나 장비 없이 상자·창고 사용 | [ProgressionFoundation](../ProgressionFoundation.md) |
| `KSISave` | 불변 파일 묶음·완료 게시·복구 | 게임 codec 없이 호스트가 검증한 바이트를 저장 | [SessionFoundation](../SessionFoundation.md) |
| `KSICombat` | 판정 문맥·대상 검증·중복 적중 방지·결과 전달 | 무기·인벤토리 없이 함정·파괴 가능한 물체에 적용 | [CombatFoundation](../CombatFoundation.md), [Coding](../Coding.md) |
| `KSIMap` | 마커 등록·공개 조건·좌표 투영·지도 자료 | Agent·퀘스트 없이 위치 자료 제공. UI 연결은 선택 | [지도 학습](../Learning/Map.md) |
| `KSIWorldTime` | 공통 시계·배율·실행 구간·후보 확정 | Agent·날씨 없이 시간과 실행 순서 사용 | [EnvironmentPlan](../EnvironmentPlan.md) |
| `KSICelestial` | 천체 위치·현지 시각·일조 계산 | 외부 시각을 받아 자체 시계·날씨·지형 없이 계산 | [EnvironmentPlan](../EnvironmentPlan.md) |
| `KSITerrain` | 지형 데이터·편집·메시·충돌·표면 질의 | 물·눈·생태 없이 지형 편집 사용 | [EnvironmentTerrain](../EnvironmentPlan/Terrain.md) |
| `KSIAgent` | ID·Feature·Store·Handle·Lease와 상태 교체 | 경제·인벤토리·전투 없이 개체 상태와 단일 작성자 검증 | [AgentFoundation](../AgentFoundation.md) |

표의 모듈은 단계 1-4에서 추출했다. KSIEnvironment의 범위와 모듈별 실제 검증은 [단계 4](Stage4.md#단계-4-구현과-검증)를 따른다. 게임 동작의 정본은 관련 사양서가 소유한다.

단계 4에서 KSICelestial은 외부 시각을 받는 별도 계산 모듈로 유지했다. KSIAgent는 상태·작성권 수명을 소유하고, 성격·가치관·경제·삶 모델과 Actor/Mass 연결은 CCL에 남겼다.

## 선택 확장과 후속 후보

| 기능 | 첫 분리에서의 위치 |
|---|---|
| 장비·소비·탄약 | 아이템 상태와 실행 시스템을 연결하는 선택 확장 |
| 무기·투사체·방어 행동 | 전투 판정과 게임 규칙을 연결하는 선택 확장 |
| Agent 경제·생활·생태 | Agent와 자원·시간·환경을 연결하는 선택 확장 |
| 눈 위 이동·접촉 | 환경 상태와 CharacterMovement를 연결하는 선택 확장 |
| HUD·지도·인벤토리 화면 | 해당 시스템과 공용 UI를 연결하는 게임별 구성 |

물·눈·날씨·열·공기·식생·생태 계산은 단계 4에서 KSIEnvironment로 추출했다. 상호작용과 보존 계약을 공유하므로 개별 모듈로 더 나누지 않았다. 표현·내비게이션·게임 규칙 연결은 CCL이 소유한다.

## 목표 계층

공용 모듈은 아래 계층을 따른다. 아래 계층은 위 계층을 참조하지 않는다. 같은 계층의 독립 시스템끼리도 서로 참조하지 않으며, 필요한 연결은 조합 계층이나 게임 모듈이 맡는다. 이 표는 의존 방향의 기준이다. 단계 1-4에서 추출한 모듈의 실제 의존 목록은 각 모듈의 `Build.cs`가 정본이다.

| 계층 | 모듈 | 참조할 수 있는 대상 |
|---|---|---|
| 0. 공통 계약 | `KSICore` | 엔진 |
| 1. 공통 실행 | `KSIWorldTime`, `KSISave` | 엔진, 계층 0 |
| 2. 독립 시스템 | `KSIUI`, `KSIMap`, `KSIInventory`, `KSICombat`, `KSICelestial`, `KSITerrain`, `KSIAgent` | 엔진, 계층 0-1 |
| 3. 조합 시스템 | `KSIEnvironment`, 선택 확장 | 엔진, 계층 0-2 |
| 게임 | `CCL`, `CCLEditor` | 엔진, 모든 공용 모듈 |

## 모듈 분리와 플러그인 제외

공용 시스템은 CCL 프로젝트의 `Source/` 아래에 별도 C++ 모듈로 분리한다. 모듈 사이의 의존 방향은 각 `Build.cs`가 선언하고 빌드 도구가 강제한다. UBT는 모듈 순환을 오류로 처리하며 `CircularlyReferencedDependentModules`에 명시한 예외만 허용한다 (`<Engine>/Source/Programs/UnrealBuildTool/Configuration/UEBuildModule.cs:1380`). 그래서 남은 폴더 순환은 해당 시스템을 모듈로 옮기기 전에 풀어야 한다. [Epic 모듈 문서](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules)

플러그인은 현재 범위에서 제외한다. 플러그인을 쓰지 않으면 `.uplugin` 관리와 추가 플러그인 경로의 패키지 재배치를 다루지 않아도 된다. 대신 다음 제약이 생긴다.

- 모듈에는 Content 폴더가 없다. 공용 모듈은 에셋을 직접 참조하지 않고 설정이나 데이터로 받는다. 지형·눈·환경처럼 머티리얼과 FX가 시스템과 묶인 경우, 그 에셋은 사용하는 프로젝트마다 둔다.
- 플러그인의 활성화 설정이 없다. 각 프로젝트는 `.uproject`의 `Modules`, 타깃의 `ExtraModuleNames`와 `Build.cs` 의존으로 사용할 모듈을 고른다.
- 다른 프로젝트와 장기적으로 공유·동기화하는 방식은 정하지 않았다. 단계 1의 독립 검사에는 모듈 소스 복사를 사용했다.

다른 프로젝트와의 공유가 실제로 필요해지면 플러그인을 다시 검토한다. 모듈을 플러그인으로 감싸도 모듈 이름이 같으면 타입 경로 `/Script/<Module>`은 바뀌지 않는다. 따라서 모듈 분리 때 처리한 리다이렉트를 다시 하지 않는다. Game Features는 플러그인 기반이므로 이 계획의 범위 밖이다.

특정 시스템만 사용할 때 나머지 시스템을 필수로 불러오지 않는 구성을 우선한다. 런타임 기능과 에디터 도구는 모듈 종류로 나눈다. `Source/CCLEditor/`는 Combat·Items·Agents를 참조하는 게임 전용 제작 도구와 Details 등록을 소유한다. `CCL`에는 눈 Animation Blueprint 제작 코드와 PIE 검사 때문에 조건부 `UnrealEd`·`AnimGraph` 등의 의존이 남는다. 이 의존은 Game·Server 빌드에서는 추가하지 않는다. 에디터 의존 목록의 정본은 각 모듈의 `Build.cs`다.

### 다른 프로젝트에서의 사용

단계 1은 모듈 소스를 최소 프로젝트에 복사해 사용했다. 실제 개인 프로젝트의 장기 배포·동기화 방식은 정하지 않았으며 후보는 두 가지다.

1. 공용 모듈을 별도 저장소에 두고, 각 프로젝트의 `Source/` 아래에 git submodule이나 복사로 넣는다.
2. 각 `.uproject`의 `AdditionalRootDirectories`로 공용 루트를 지정한다. UBT는 그 루트의 `Source/`에서도 모듈 규칙을 찾는다 (`<Engine>/Source/Programs/UnrealBuildTool/Configuration/Rules/RulesCompiler.cs:324-328`, `:365-368`). 이 방법의 바이너리 출력 위치, 에디터 로드와 패키징은 확인하지 않았다.

공용 저장소 생성과 다른 개인 프로젝트 수정은 아직 정하지 않았다. 최소 프로젝트의 복사 검증 결과는 아래에 기록했고, 공용 루트 방식과의 비교는 후속 작업으로 남긴다.

## 시스템별 엔진 의존과 네트워크 전제

선택 사용을 판정하려면 모듈마다 필수 엔진 기능과 네트워크 전제를 정해야 한다. `CCL`은 GAS, StateTree, Mass, Niagara와 게임 UI의 MVVM을 의존한다. 모듈별 실제 의존은 아래 단계별 구현 범위와 각 Build.cs를 따른다.

| 대상 | 정할 항목 |
|---|---|
| `KSIUI` | CommonUI·UMG 필수, MVVM은 CCL에 유지. 정확한 목록은 `Source/KSIUI/KSIUI.Build.cs` |
| `KSICombat` | GAS 필수. 입력·능력·효과 수명은 GAS가 소유하며 게임 정책과 분리. 정확한 목록은 `Source/KSICombat/KSICombat.Build.cs` |
| `KSIAgent` | GameplayTags를 사용하고 Mass·StateTree는 요구하지 않는다. Actor/Mass와 게임 실행은 CCL에 둔다 |
| 공통 | 서버 권위와 복제를 기본 전제로 둘지. 싱글 플레이 프로젝트도 그 비용을 지는지 |
| 공통 | 지원 엔진 버전 범위. 모든 사용 프로젝트가 같은 소스 엔진을 쓰는지 |
