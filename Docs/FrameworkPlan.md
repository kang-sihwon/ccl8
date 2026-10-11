# 공용 Framework 분리 계획

여러 개인 UE5 프로젝트가 선택해 쓰는 공용 게임 시스템을 `KSI Framework`라는 이름으로 계획한다. 분리 단위는 프로젝트 안의 C++ 모듈이며, 플러그인은 현재 범위에서 제외한다. 실제 분리와 이름 변경은 아직 보류하고 사용자가 착수 시점을 정한다. 근거는 [결정 20](DesignLog.md#2026-10-09--결정-20-공용-framework-계획과-구현-보류)과 [결정 22](DesignLog.md#2026-10-11--결정-22-공용-시스템의-모듈-분리-방향과-ksi-이름)이다.

## 목표와 현재 상태

CCL은 공용 시스템을 사용하는 첫 프로젝트로 둔다. 공용 시스템은 상태 소유권, 수명, 실행, 저장·복제 계약을 제공한다. 각 게임은 필요한 시스템을 선택하고 게임 규칙과 콘텐츠를 연결한다.

| 항목 | 상태 |
|---|---|
| 공용화 범위 | 시스템 단위의 재사용 기반 |
| 분리 단위 | 프로젝트 안의 C++ 모듈. 플러그인은 현재 범위에서 제외 |
| 이름 규칙 | 공용 모듈 `KSI{System}`, 공용 타입 접두사 `KSI` |
| 묶음 이름 | `KSI Framework`. 문서에서 공용 시스템 전체를 부를 때만 사용 |
| 독립 후보 | 아래 목록으로 검토 범위 기록. 추출·독립 실행 검증 전 |
| 실제 분리 | 보류 |
| 착수 시점 | 사용자 지정. 착수하면 단계 1부터 진행 |
| 환경 후속 작업 | [EnvironmentPlan](EnvironmentPlan.md)의 별도 범위. 이 계획으로 착수하지 않음 |

장르 독립적인 계약과 콘텐츠 분리는 [Coding](Coding.md#장르에-독립적인-베이스-시스템-설계)을 따른다. UE의 기능 선택 기준도 유지한다. 이 계획은 엔진 소스를 수정하거나 UE의 UObject·GAS·Subsystem 등을 자체 구현으로 교체하는 계획이 아니다.

보스, 맵, 퀘스트 내용, 아이템 정의, 화면 구성과 연출 자료는 각 게임이 소유한다. 이 콘텐츠들을 공용 모듈이나 플러그인으로 분리하는 작업은 공용화 범위에 포함하지 않는다.

## 독립 선택의 기준

독립 시스템은 UE5와 작은 공통 계약에는 의존할 수 있지만, 사용하지 않는 다른 게임 시스템을 필수로 요구하지 않아야 한다. 예를 들어 UI만 선택한 프로젝트에는 인벤토리·Agent·지형 구현이 필요하지 않아야 한다.

1. 게임이 공용 시스템을 참조하고, 공용 시스템은 특정 게임의 클래스·맵·에셋을 참조하지 않는다.
2. 게임마다 달라질 규칙과 데이터는 공개 계약으로 전달한다. 변경되지 않는 공통 실행은 시스템이 구현한다.
3. 상태의 소유자, 생성·해제 시점, 서버 권위와 복제 범위를 시스템별로 명시한다.
4. 시스템 사이의 연결은 선택 가능한 어댑터로 둔다. 참조 방향을 바꾸기 위해 모든 데이터를 공통 모듈에 몰아넣지 않는다.
5. 시간과 저장은 여러 시스템이 기대는 바닥 계약이다. 저장 참여, 구간 등록, 시간 읽기 같은 공통 계약은 `KSICore`에 두고 구체 시계와 저장 실행은 별도 모듈로 둔다. 외부 시각·저장 참여 계약만으로 충분한 시스템은 그 입력만 받는다.

현재 CCL의 구현이 이 조건을 모두 만족한다는 판정은 아니다. 시스템을 추출할 때 실제 코드·에셋·설정 의존을 다시 확인한다.

## 이름 규칙

공용 모듈과 타입은 공통 접두사 `KSI`를 쓴다. `KSI`는 사용자 이름의 약자이며 결정 22에서 정했다. 게임 고유 모듈과 타입은 지금처럼 `CCL`을 유지한다.

| 대상 | 형식 | 예 |
|---|---|---|
| 공용 모듈 | `KSI{System}` | `KSICore`, `KSIUI`, `KSIMap` |
| 공용 타입 | UE 타입 접두사 + `KSI` + 이름 | `UKSIScreen`, `FKSIUIViewHandle`, `IKSIWorldParticipant` |
| export 매크로 | 모듈 이름 대문자 + `_API` | `KSICORE_API`, `KSIUI_API` |
| 게임 모듈 | 기존 이름 유지 | `CCL`, `CCLEditor` |

`Framework`는 모듈 이름에 붙이지 않는다. 결정 20의 `{Prefix}{System}Framework` 형식은 이름이 길다. 또한 UE 5.8.3에는 서버에서 UMG를 제어하는 실험 플러그인 `UIFramework`가 있어 `KSIUIFramework` 같은 이름이 그 플러그인과 혼동된다 (`<Engine>/Plugins/Experimental/UIFramework/UIFramework.uplugin`). `KSI Framework`는 문서에서 공용 시스템 전체를 부르는 이름으로만 쓴다.

접두사 자리에는 일반 단어를 쓰지 않는다. 접두사는 엔진·외부 플러그인과 이름이 겹치지 않게 하고 공용 코드의 출처를 드러내야 한다. `GameFramework`는 엔진 헤더 폴더(`#include "GameFramework/Character.h"`)와 ModularGameplay의 `UGameFrameworkComponent`와 섞인다. `GameModule`은 프로젝트의 주 게임 모듈을 뜻하는 엔진 용어이고, `Public`은 모듈 폴더와 `Build.cs`의 용어다. `Game`과 `Common`으로 시작하는 엔진 모듈도 많다. 예를 들어 `Common`을 접두사로 쓰면 `UCCLUIActionRouter`가 CommonUI의 `UCommonUIActionRouterBase`와 거의 같은 `UCommonUIActionRouter`가 된다. 접두사 뒤의 시스템 부분에는 일반 단어를 써도 된다.

2026-10-11 UE 5.8.3 소스에서 `KSI`의 이름 충돌을 확인했다. 엔진의 모듈 규칙 파일 이름 2,549개와 플러그인 이름 895개 중 `KSI`로 시작하는 이름은 없었다. 엔진의 C++·C# 소스에도 `KSI`로 시작하는 식별자가 없었고, CCL 저장소에도 아직 쓰는 곳이 없다.

모듈 이름은 첫 분리 뒤 바꾸지 않는다. 네이티브 타입의 경로 `/Script/<Module>`을 모듈 이름이 정하기 때문이다 (`<Engine>/Source/Runtime/CoreUObject/Private/Misc/PackageName.cpp:2324`). 모듈 이름을 바꾸면 그 모듈의 모든 타입에 리다이렉트가 다시 필요하다.

기존 `CCL` 모듈, 클래스와 파일은 분리를 착수하기 전까지 지금 이름을 유지한다. 타입을 공용 모듈로 옮길 때는 같은 작업에서 `KSI` 이름으로 바꾼다. 경로와 이름이 함께 바뀌어도 타입마다 CoreRedirects 한 줄로 처리할 수 있다. `WorldParticipant` 같은 개별 타입 이름을 다듬을지는 추출할 때 정한다.

## 독립 시스템 후보

| 모듈 이름 | 공용 책임 | 독립 사용 확인 사례 | 관련 사양 |
|---|---|---|---|
| `KSICore` | 저장 참여·구간 등록·시간 읽기처럼 여러 시스템이 공유하는 계약 | 다른 `KSI` 모듈 없이 엔진에만 의존해 컴파일 | [세계 저장 참여자 학습](Learning/EnvironmentRestoreParticipants.md) |
| `KSIUI` | 화면 등록·수명·레이어·입력 문맥·표시 제어 | 전투나 인벤토리 없이 메뉴·도구 화면 사용 | [UIFoundation](UIFoundation.md) |
| `KSIInventory` | 아이템 정의·인스턴스·보관·이동·소유권 | 캐릭터나 장비 없이 상자·창고 사용 | [ProgressionFoundation](ProgressionFoundation.md) |
| `KSISave` | 저장 실행·버전·직렬화·검증·복원 | 특정 게임 시스템 없이 등록한 상태만 저장 | [SessionFoundation](SessionFoundation.md) |
| `KSICombat` | 판정 문맥·대상 검증·중복 적중 방지·결과 전달 | 무기·인벤토리 없이 함정·파괴 가능한 물체에 적용 | [CombatFoundation](CombatFoundation.md), [Coding](Coding.md) |
| `KSIMap` | 마커 등록·공개 조건·좌표 투영·지도 자료 | Agent·퀘스트 없이 위치 자료 제공. UI 연결은 선택 | [지도 학습](Learning/Map.md) |
| `KSIWorldTime` | 공통 시계·배율·실행 구간·후보 확정 | Agent·날씨 없이 시간과 실행 순서 사용 | [EnvironmentPlan](EnvironmentPlan.md) |
| `KSICelestial` | 천체 위치·현지 시각·일조 계산 | 외부 시각을 받아 자체 시계·날씨·지형 없이 계산 | [EnvironmentPlan](EnvironmentPlan.md) |
| `KSITerrain` | 지형 데이터·편집·메시·충돌·표면 질의 | 물·눈·생태 없이 지형 편집 사용 | [EnvironmentTerrain](EnvironmentTerrain.md) |
| `KSIAgent` | 개체 상태·관측·기억·목표·행동 선택·실행 수명 | 경제·인벤토리·전투 없이 게임이 제공한 행동 실행 | [AgentFoundation](AgentFoundation.md) |

표는 분리 목표와 향후 검증 사례다. 개별 시스템의 현재 구현·검증 상태는 관련 사양서가 소유한다. 후보마다 모듈 하나를 바로 만들거나 후보 전부를 한 번에 추출한다는 뜻은 아니다.

두 후보는 착수할 때 경계를 다시 본다. `KSICelestial`은 외부 시각을 받는 계산 모듈 규모라 `KSIWorldTime`이나 환경 모듈 안에 둘 수 있다. `KSIAgent`에는 ID·Feature·Lease·Actor/Mass 전환 같은 일반 실행 수명과, 성격·가치관·경제 같은 CCL 고유의 삶 모델이 섞여 있다. 일반 실행 수명만 공용 후보로 보고, 삶 모델을 어디에 둘지는 착수할 때 정한다.

## 선택 확장과 후속 후보

| 기능 | 첫 분리에서의 위치 |
|---|---|
| 장비·소비·탄약 | 아이템 상태와 실행 시스템을 연결하는 선택 확장 |
| 무기·투사체·방어 행동 | 전투 판정과 게임 규칙을 연결하는 선택 확장 |
| Agent 경제·생활·생태 | Agent와 자원·시간·환경을 연결하는 선택 확장 |
| 눈 위 이동·접촉 | 환경 상태와 CharacterMovement를 연결하는 선택 확장 |
| HUD·지도·인벤토리 화면 | 해당 시스템과 공용 UI를 연결하는 게임별 구성 |

물·눈·날씨·열은 `KSIEnvironment`의 후속 후보로 남긴다. 상호작용과 물질 보존 계약이 강하게 연결되므로 현재는 각각의 독립 모듈로 확정하지 않는다. 단계별 구현과 저장·복원 검증을 진행한 뒤 내부 경계를 판단한다.

## 목표 계층

공용 모듈은 아래 계층을 따른다. 아래 계층은 위 계층을 참조하지 않는다. 같은 계층의 독립 시스템끼리도 서로 참조하지 않으며, 필요한 연결은 조합 계층이나 게임 모듈이 맡는다. 이 표는 의존 방향의 계획 기준이며 각 모듈의 정확한 내용은 추출할 때 확정한다.

| 계층 | 모듈 | 참조할 수 있는 대상 |
|---|---|---|
| 0. 공통 계약 | `KSICore` | 엔진 |
| 1. 공통 실행 | `KSIWorldTime`, `KSISave` | 엔진, 계층 0 |
| 2. 독립 시스템 | `KSIUI`, `KSIMap`, `KSIInventory`, `KSICombat`, `KSICelestial`, `KSITerrain`, `KSIAgent` | 엔진, 계층 0-1 |
| 3. 조합 시스템 | `KSIEnvironment` 후보, 선택 확장 | 엔진, 계층 0-2 |
| 게임 | `CCL`, `CCLEditor` | 엔진, 모든 공용 모듈 |

## 모듈 분리와 플러그인 제외

공용 시스템은 CCL 프로젝트의 `Source/` 아래에 별도 C++ 모듈로 분리한다. 모듈 사이의 의존 방향은 각 `Build.cs`가 선언하고 빌드 도구가 강제한다. UBT는 모듈 순환을 오류로 처리하며 `CircularlyReferencedDependentModules`에 명시한 예외만 허용한다 (`<Engine>/Source/Programs/UnrealBuildTool/Configuration/UEBuildModule.cs:1380`). 그래서 남은 폴더 순환은 해당 시스템을 모듈로 옮기기 전에 풀어야 한다. [Epic 모듈 문서](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules)

플러그인은 현재 범위에서 제외한다. 플러그인을 쓰지 않으면 `.uplugin` 관리와 추가 플러그인 경로의 패키지 재배치를 다루지 않아도 된다. 대신 다음 제약이 생긴다.

- 모듈에는 Content 폴더가 없다. 공용 모듈은 에셋을 직접 참조하지 않고 설정이나 데이터로 받는다. 지형·눈·환경처럼 머티리얼과 FX가 시스템과 묶인 경우, 그 에셋은 사용하는 프로젝트마다 둔다.
- 플러그인의 활성화 설정이 없다. 각 프로젝트는 `.uproject`의 `Modules`, 타깃의 `ExtraModuleNames`와 `Build.cs` 의존으로 사용할 모듈을 고른다.
- 다른 프로젝트와 공유하는 단위가 정해져 있지 않다. 사용 방법은 아래 "다른 프로젝트에서의 사용"에 미정으로 남긴다.

다른 프로젝트와의 공유가 실제로 필요해지면 플러그인을 다시 검토한다. 모듈을 플러그인으로 감싸도 모듈 이름이 같으면 타입 경로 `/Script/<Module>`은 바뀌지 않는다. 따라서 모듈 분리 때 처리한 리다이렉트를 다시 하지 않는다. Game Features는 플러그인 기반이므로 이 계획의 범위 밖이다.

특정 시스템만 사용할 때 나머지 시스템을 필수로 불러오지 않는 구성을 우선한다. 런타임 기능과 에디터 도구는 모듈 종류로 나눈다. 현재 에디터 도구는 런타임 `CCL` 모듈 안에서 `Target.bBuildEditor` 조건으로 컴파일된다 (`Source/CCL/CCL.Build.cs:19-32`). 이 도구는 Combat·Items·Agents를 참조하는 게임 전용 도구이므로 공용 모듈이 아니라 Editor 종류의 `CCLEditor` 모듈로 옮긴다.

### 다른 프로젝트에서의 사용

다른 프로젝트에서 공용 모듈을 사용하는 방법은 정하지 않았다. 현재 후보는 두 가지다.

1. 공용 모듈을 별도 저장소에 두고, 각 프로젝트의 `Source/` 아래에 git submodule이나 복사로 넣는다.
2. 각 `.uproject`의 `AdditionalRootDirectories`로 공용 루트를 지정한다. UBT는 그 루트의 `Source/`에서도 모듈 규칙을 찾는다 (`<Engine>/Source/Programs/UnrealBuildTool/Configuration/Rules/RulesCompiler.cs:324-328`, `:365-368`). 이 방법의 바이너리 출력 위치, 에디터 로드와 패키징은 확인하지 않았다.

공용 저장소 생성과 다른 개인 프로젝트 수정은 아직 정하지 않았다. 사용 방법은 추출한 모듈을 별도 최소 프로젝트에서 검증할 때 비교한다.

## 시스템별 엔진 의존과 네트워크 전제

선택 사용을 판정하려면 모듈마다 필수 엔진 기능과 네트워크 전제를 정해야 한다. 현재는 `CCL` 모듈 하나가 GAS, StateTree, Mass, Niagara, CommonUI와 MVVM을 함께 의존한다 (`Source/CCL/CCL.Build.cs:12`, `:17`, `:28`). 다음 항목은 미정이며 해당 모듈을 추출할 때 정한다.

| 대상 | 정할 항목 |
|---|---|
| `KSIUI` | CommonUI·MVVM을 필수 의존으로 둘지 |
| `KSICombat` | GAS를 필수 의존으로 둘지 |
| `KSIAgent` | Mass·StateTree를 필수 의존으로 둘지 |
| 공통 | 서버 권위와 복제를 기본 전제로 둘지. 싱글 플레이 프로젝트도 그 비용을 지는지 |
| 공통 | 지원 엔진 버전 범위. 모든 사용 프로젝트가 같은 소스 엔진을 쓰는지 |

## 재검토 시점

작업량 증가 자체보다 실제 변경 비용과 재사용 수요를 확인한다. 다음 상황은 분리, 특히 아래 단계 3-4를 다시 검토할 계기이며 자동 착수 조건은 아니다.

- 한 시스템을 바꿀 때 관련 없는 시스템의 코드·설정까지 반복해서 수정해야 한다.
- 공용 책임과 게임 규칙이 섞여 같은 기능을 다른 맥락에 적용하기 어렵다.
- 두 번째 개인 프로젝트에서 같은 시스템이 필요해 복사·수정이 시작될 상황이다.
- 한 시스템의 공개 계약과 검증 사례가 안정되어 별도로 유지할 근거가 생긴다.
- 시스템별 빌드·테스트·배포를 분리해야 할 구체적인 비용이 확인된다.

파일 수, 코드 줄 수, 환경 단계 번호를 착수 기준으로 고정하지 않는다. 특히 환경 4단계 전에 물리적 모듈 분리를 반드시 완료해야 한다는 일정은 두지 않는다. 다만 여러 시스템의 후보 계산·확정·저장 일관성은 기존 모듈 안에서도 유지해야 한다.

사용자가 착수를 지시하면 AI는 당시 변경 상태와 의존을 확인하고 아래 단계 1부터 진행한다. 한 단계의 결과를 사용자가 검토한 뒤 다음 단계로 넘어간다.

## 착수 후 진행 순서

다음은 실행 전 계획이며 수행 완료 기록이 아니다. 착수하면 아래 단계 순서로 진행한다.

| 단계 | 대상 | 진행 조건 |
|---|---|---|
| 1 | `CCLEditor`(`Editor` 폴더), `KSIUI`(`UI/Core`), `KSIMap`(`Map`) | 착수 지시. `UI/Core`와 `Map`은 다른 게임 폴더를 include하지 않는다. `CCLEditor`는 `CCL`에 의존하는 게임 모듈이다 |
| 2 | `KSICore`(저장 참여 계약·구간 등록·시간 읽기), 이어서 `KSIInventory` | 저장 참여 계약을 Environment 폴더에서 분리한다. 공개 정의·상태와 실행 코드의 게임 의존을 정리하고 [인벤토리 추출 조건](#인벤토리-추출-조건)을 충족한 뒤 추출한다 |
| 3 | `KSICombat`, AbilitySystem·Actions 정리 | 재검토 계기가 생길 때 판단한다. Items·Agents와의 연결 정리가 필요하다 |
| 4 | `KSIWorldTime`, `KSISave`, `KSICelestial`, `KSITerrain`, `KSIAgent`, 환경 계열 | 재검토 계기가 생길 때 판단한다. 남은 폴더 순환을 먼저 풀어야 한다 |

단계 1-2는 공유 수요가 없어도 CCL 안의 의존 방향을 바로잡는 이득이 있어 먼저 진행할 대상으로 둔다. 단계 3-4는 정리 비용이 크고 CCL 완성에 직접 도움이 적어 재검토 시점의 계기가 생길 때 판단한다. 단계 순서는 의존 정리 비용으로 정했으며, "독립 시스템 후보" 표의 순서는 구현 순서가 아니다.

각 시스템을 옮길 때는 다음 절차를 따른다.

1. AI는 선택한 시스템의 코드·에셋·설정 의존을 조사한다. 기존 결함과 검증 한계를 분리 작업의 기준으로 기록한다.
2. AI는 시스템의 공개 계약, 상태 소유자, 생성·해제, 권위와 저장·복제 경계를 정리한다. 게임별 규칙을 연결할 지점을 정한다.
3. AI는 기존 `CCL` 안에서 구체 게임 타입을 향한 의존을 정리한다. 기존 동작을 먼저 검증한다.
4. AI는 필요한 소스만 새 모듈로 옮기고 타입 이름을 `KSI` 접두사로 바꾼다. `Build.cs`, export 매크로, CoreRedirects, 설정 섹션 이름과 `.uproject`·타깃의 모듈 목록을 갱신한다.
5. AI는 CCL에서 기존 동작과 에셋 로드를 확인하고, 별도 최소 프로젝트에서 필요한 모듈만 사용해 독립성을 검증한다.
6. AI는 검증된 공용 코드의 버전과 지원 엔진 범위를 기록한다. 각 게임의 갱신·회귀 검증 방식을 정한 뒤 다른 프로젝트에서의 사용 방법을 선택한다.

구체 클래스·함수 시그니처와 파일 이동 목록은 각 단계를 시작할 때 확정한다.

### 인벤토리 추출 조건

`KSIInventory`의 추출 단위는 기본 아이템 정의 계약과 보관 실행이다. 현재 `Items` 폴더에는 장착·소비·탄약·음식의 정의와 실행이 함께 있으므로, 파일 위치만으로 공용 모듈에 포함할 대상을 정하지 않는다. 기본 정의·개별 상태·보관·이동·소유권 계약을 먼저 정하고 게임별 기능을 연결할 경계를 확인한다.

| 구분 | 분리 목표 | 추출 전에 확인할 현재 결합 |
|---|---|---|
| 기본 계약 | 정의 식별·확장 데이터, 인스턴스 ID·수량·보관 위치, 변경 알림과 저장·복제 | `UCCLItemDefinition`과 `FCCLInventoryEntry`가 기능별 타입을 필수로 요구하는지 |
| 장착·소비·무기 | 기능별 Fragment와 실행을 선택 확장 또는 CCL에 배치 | `CCLItemDefinition.h`·`CCLItemFragments.h`의 `UCCLCombatDefinition`, `UCCLAbilitySet`, `UCCLActionSet`, `UGameplayEffect` 참조와 `CCLLoadoutComponent`·`CCLInventoryWeapon`의 실행 의존 |
| 음식·환경·픽업 | 환경 관측과 월드 Actor 연결을 선택 확장 또는 CCL에 배치 | `CCLInventoryFood`, `CCLFoodEnvironment`, `CCLWorldPickup`과 저장 참여자의 환경 참조 |
| 기능별 개별 상태 | 기본 인벤토리가 기능을 몰라도 보관·저장할 계약 정의 | `FCCLInventoryEntry::Food`·`LoadedAmmo`, 해당 상태의 검증·직렬화와 기존 저장 호환 |

위 파일은 `Source/CCL/Items/` 아래에 있다. 실행 서비스를 분리해도 공개 Fragment와 개별 상태가 구체적인 전투·음식 타입을 요구하면 기본 인벤토리의 의존이 남는다. 기능별 타입을 옮길 위치와 저장 변환 방식은 착수 때 확정한다. 전투·능력 연결을 CCL에 남기면 단계 3의 `KSICombat` 추출을 기다리지 않고 기본 인벤토리를 검증할 수 있다. 기능별 데이터의 검증·직렬화 계약과 호환 처리 비용은 추가된다.

단계 2는 다음 조건을 모두 확인한 뒤 완료로 판정한다.

1. 기본 인벤토리의 공개 헤더·리플렉션 필드·구현·기본 에셋이 CCL의 전투·능력·행동·환경 타입을 요구하지 않는다. 엔진 의존 목록도 별도로 기록한다.
2. 별도 최소 프로젝트에서 `KSICore`, `KSIInventory`와 선언한 엔진 의존만으로 상자 두 개의 보관·이동·저장·복원을 실행한다. 장착·무기·음식 확장을 등록하지 않은 구성이 동작해야 한다.
3. CCL에 선택 확장을 연결한 구성에서는 장비·소비·탄약·음식과 기존 저장 복원의 회귀 검사를 통과한다. 타입 이동에 필요한 에셋 로드와 CoreRedirects도 확인한다.

## 분리 완료 기준

| 검증 | 기대 결과 |
|---|---|
| 최소 프로젝트 | 원본 CCL의 헤더·맵·설정 없이 선택한 `KSI` 모듈과 선언한 의존만으로 사용 |
| 게임 규칙 교체 | 공개 데이터·정책·어댑터로 다른 사용 사례를 연결 |
| 선택 사용 | 사용하지 않는 모듈을 모듈 목록에서 제외해도 선택한 모듈이 동작 |
| CCL 회귀 | 기존 게임 흐름과 해당 시스템의 자동·수동 검증 유지 |
| 수명·권위 | 맵 이동, 소유자 종료, 클라이언트·서버 분리 시 상태·자원 정리 |
| 저장 호환 | 대상 시스템의 기존 저장 복원 또는 명시적인 버전 변환 |
| 에셋 호환 | 기존 맵·Blueprint의 타입 참조를 실제 로드로 확인 |
| 빌드·패키지 | 필요한 Editor·게임·서버 대상 확인. 불필요한 에디터 의존 제외 |

성능 개선은 분리의 자동 효과로 주장하지 않는다. 빌드 시간, CPU 시간, 메모리와 전송량을 개선 대상으로 삼으면 동일한 조건에서 전후를 측정한다.

## 2026-10-11 의존 정리 현황

2026-10-11 전체 소스 검토에서 지적한 역방향 의존 다섯 개를 `CCL` 모듈 안에서 정리했다. 이는 "착수 후 진행 순서"에 있는 이동 절차 3번의 일부를 미리 수행한 것이며, 모듈·플러그인 분리 착수가 아니다.

| 이전 의존 | 정리 방식 |
|---|---|
| 전투 판정이 Agent 컴포넌트를 직접 호출 | Combat이 `ICCLHitReceiver`를 정의하고 Agent가 구현한다 |
| Agents가 Campaign Actor를 생성 | Agent 세계는 인구 변경만 알리고, Campaign의 `UCCLVillagePopulationSubsystem`이 Actor를 만든다. Agent 쪽은 `ICCLAgentEmbodiment`와 `ICCLAgentWorldHost`로 표현·세계 설정을 받는다 |
| 환경 세계 스냅샷 헤더가 Session을 include | 세계 스냅샷은 이름 있는 구간만 소유하고, 플레이어·보관함은 `ICCLWorldParticipant`로 자기 구간을 수집·검증·복원한다 |
| Items가 Combat의 Fighter를 호출 | Items가 `ICCLEquipmentPresenter`를 정의하고 Fighter가 구현한다 |
| AbilitySystem과 Combat의 상호 의존 | `UCCLCombatAbility`를 Combat으로 옮겨 AbilitySystem이 다른 게임 폴더를 include하지 않는다 |

폴더 단위의 순환은 아직 남아 있다. 적 캐릭터는 Combat 폴더에 있지만 Agent 컴포넌트를 조합하므로 Combat과 Agents가 서로 include한다. 환경 세계 시뮬레이션은 생활 시뮬레이션과 음식 인벤토리를 함께 확정하므로 Environment와 Agents, Environment와 Items도 서로 include한다. 분리를 착수하면 적 캐릭터를 콘텐츠 계층으로 옮기고 환경·생활·인벤토리의 공통 시간 계약을 먼저 나눠야 한다. 측정 기준은 하위 폴더 사이의 `#include` 방향이며 2026-10-11 작업 트리에서 확인했다.

### 단계 1-2의 전제와 분리 비용

결정 22의 단계 1-2 계획은 아래 의존 조사와 분리 비용 측정을 근거로 한다. 의존 설명은 2026-10-11 검토한 작업 트리 기준이며, 수치의 재현 기준은 표 아래에 구분한다.

- `UI/Core`와 `Map`은 같은 폴더의 헤더와 엔진 헤더만 include한다. 다른 게임 폴더는 include하지 않는다.
- `UI` 폴더 최상위에는 게임 화면이 함께 있다. `SCCLInventoryWidget.cpp`는 Items, `CCLMapScreen.cpp`는 Map, `CCLSessionMenuScreen.cpp`는 Session을 include한다. 이 화면들은 `KSIUI`로 옮기지 않고 `CCL`에 남긴다.
- Items는 저장 참여 계약이 Environment 폴더에 있어 Environment를 include한다 (`CCLInventorySnapshot.cpp`, `CCLInventoryPersistenceSubsystem.h`). 음식과 줍기 아이템도 환경 세계를 include한다 (`CCLInventoryFood.cpp`, `CCLFoodEnvironment.cpp`, `CCLWorldPickup.cpp`).
- Session은 Campaign을 include한다 (`CCLGameInstance.cpp`, `CCLPlayerPersistenceSubsystem.cpp`).

| 항목 | 측정값 | 분리할 때 필요한 작업 |
|---|---|---|
| 리플렉션 선언(`UCLASS`·`USTRUCT`·`UENUM`·`UINTERFACE`) | 307개 | 옮기는 타입의 경로가 `/Script/CCL.X`에서 바뀐다 |
| `CCL_API` 사용 | 323곳 | 옮기는 타입의 export 매크로를 새 모듈의 것으로 바꾼다 |
| `/Script/CCL`을 참조하는 에셋 | 454개 중 100개 | 옮긴 타입마다 CoreRedirects를 넣고 실제 로드로 확인한다 |
| `Config/`의 `/Script/CCL.` 참조 | 5곳 | 설정 섹션 이름과 클래스 경로를 바꾼다 |

소스와 설정 수치는 커밋 `ef44376d21811df02b5cf28b2c50105047921e2c`에 고정한다. 아래 명령은 저장소 루트의 PowerShell에서 실행하며 순서대로 307, 323, 5를 반환한다. 리플렉션 수치는 선언 매크로의 일치 건수이며 export 수치는 `CCL_API` 토큰의 사용 건수다. 옮길 타입 수나 리다이렉트 수를 그대로 뜻하지 않는다.

```powershell
$FrameworkRevision = 'ef44376d21811df02b5cf28b2c50105047921e2c'
(git grep -o -E '^[[:space:]]*U(CLASS|STRUCT|ENUM|INTERFACE)\(' $FrameworkRevision -- Source/CCL | Measure-Object -Line).Lines
(git grep -o -w 'CCL_API' $FrameworkRevision -- Source/CCL | Measure-Object -Line).Lines
(git grep -n -F '/Script/CCL.' $FrameworkRevision -- Config | Measure-Object -Line).Lines
```

에셋 수치는 2026-10-11 로컬 `Content/`의 `.uasset`·`.umap`을 대상으로 다시 확인했다. 다음 명령은 전체 파일 454개와 `/Script/CCL` 바이트열을 포함한 파일 100개를 반환한다. Git LFS의 실제 에셋을 받은 작업 트리에서 실행해야 하며, 포인터 파일을 검색한 수치와 구분한다. 이 바이트열 검사는 에디터에서 타입 참조와 로드를 검증하는 절차를 대신하지 않는다.

```powershell
(rg --files Content -g '*.uasset' -g '*.umap' | Measure-Object -Line).Lines
(rg -a -l -F '/Script/CCL' Content -g '*.uasset' -g '*.umap' | Measure-Object -Line).Lines
```

같은 날 후속 코드 수정을 포함한 작업 트리에서는 리플렉션 선언 308개와 `CCL_API` 330곳을 확인했다. 이 값은 위 커밋의 기준값에 합치지 않는다. 실제 분리 착수 시 대상 커밋과 측정 명령을 기록하고, 옮길 파일과 타입만 다시 집계한다.

## 미정 사항과 비용

최초 착수 날짜, 다른 프로젝트에서의 사용 방법, 시스템별 엔진 의존과 네트워크 전제, 개별 타입 이름의 정리는 미정이다. 분리를 미루는 동안 폴더 사이의 include 방향을 검증 도구로 검사할지도 정하지 않았다. 이 문서는 새 인터페이스나 다른 프로젝트의 지원을 완료했다고 선언하지 않는다.

공용화는 다음 게임에서 구현·검증을 다시 하는 비용을 줄일 수 있다. 대신 공개 API, 저장·에셋 호환, 버전 관리와 별도 프로젝트 검증 비용이 추가된다. 현재는 기능을 실제로 사용하며 경계를 확인하고, 분리 비용을 감당할 근거가 생기면 필요한 시스템부터 진행한다.

분리를 미루는 비용은 시간이 지나며 늘어난다. `/Script/CCL`을 참조하는 에셋이 늘면 옮길 때 넣을 리다이렉트와 로드로 확인할 에셋도 함께 늘어난다.
