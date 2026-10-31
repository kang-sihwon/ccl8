# 설계 위험과 선택 개선

지적의 분류와 기준 커밋은 [코드 구조 개선 계획](../CodeStructurePlan.md)을 따른다. 줄 번호는 기준 커밋 `55afa51`의 값이다.

## 검토 범위와 측정 방법

자세히 읽은 범위는 세계 시뮬레이션 코어(`UCCLWorldSimulationSubsystem`, `CCLWorldAdvance`, `ICCLWorldParticipant`, 세계 스냅샷 검증, 열 노출의 Prepare), `UCCLAgentWorldSubsystem`, `ACCLPlayerController`, `ACCLHUD`, `UCCLLoadoutComponent`, `UCCLHitRule`과 효과 정의, `UCCLGameInstance`의 세션 저장, `FCCLSessionRecord`다. UI Core 헤더, `FCCLLifeSimulation`의 공개 API와 `UCCLFighterComponent`의 초기화는 훑어보았다.

지형·물·눈·불의 내부 계산, 표면·지형 복제, Mass·StateTree 연결, Editor 도구, Campaign 감독의 세부는 보지 않았다. 이 영역의 결함 여부는 미확인이다.

수치는 2026-10-11 작업 트리에서 다음 방법으로 측정했다.

- 폴더 의존은 `Source/CCL`의 최상위 폴더(`UI/Core`는 따로)를 단위로, 다른 폴더의 헤더를 `#include`한 횟수를 셌다.
- 한국어 문자열은 `Tests` 폴더와 `*Tests.cpp`를 제외한 코드에서 한글 음절이 들어간 `TEXT("...")` 리터럴을 셌다.
- 파일 수와 줄 수는 `.h`·`.cpp`만 셌다.

## R1. 세계 수명·저장의 소유권 역전

세계 시계·저장은 FrameworkPlan의 계층 1이고 Agent는 계층 2다. 현재 코드는 반대로 Agent가 세계의 시작·저장·이동을 소유한다.

확인한 사실은 다음과 같다.

- `UCCLWorldSimulationSubsystem::Start`를 호출하는 곳은 `UCCLAgentWorldSubsystem::OnWorldBeginPlay` 하나다 (`CCLAgentWorldSubsystem.cpp:54-93`).
- 도메인별 세계 바이트, 맵 이동 묶음, 실험 초기 스냅샷은 Agent 폴더의 `UCCLAgentSessionStore`가 보관한다 (`Source/CCL/Agents/CCLAgentWorldSubsystem.h:14-32`). 이 저장소의 필드는 모두 `public`이라 어느 코드든 바꿀 수 있다.
- Agent가 없는 환경 실험 맵은 `ACCLWorldEnvironmentConfig`가 `ICCLAgentWorldHost`를 구현해 세계를 시작한다 (`Source/CCL/Environment/CCLWorldEnvironmentConfig.h:23`).
- Life는 참여자 계약 밖의 코어다. `Start`·`Save`·`Restore`가 `FCCLLifeSimulation&`을 받고 (`Source/CCL/Environment/CCLWorldSimulationSubsystem.h:27`, `:34-35`), `CCLWorldAdvance::Advance`의 매개변수에도 들어 있다 (`Source/CCL/Environment/CCLWorldAdvance.h:19-25`). 구간 검증 문맥에도 Life가 있다 (`Source/CCL/Environment/CCLWorldParticipant.h:15`).
- 진행할 때 세계 시뮬레이션이 Agent 서브시스템을 직접 가져온다 (`CCLWorldSimulationSubsystem.cpp:218-228`). 열 노출도 참여자가 아니라 코어 진행 코드에 직접 들어 있다 (`:245-290`).
- 공통 참여자 계약에 아이템 개념이 들어 있다. `ClaimedItems`, `KeptItems`, `SessionContainer`가 그 예다 (`CCLWorldParticipant.h:18`, `:60`, `:78`).
- Environment가 Agents 헤더를 include한 29곳 중 20곳은 실험장·자동 검사 파일이다. 나머지 9곳이 `CCLWorldSimulationSubsystem`, `CCLWorldAdvance`, `CCLWorldSnapshot`, `CCLThermalExposure`, `CCLWorldEnvironmentConfig`에 있다.

영향은 세 가지다. 이대로는 FrameworkPlan의 `KSIWorldTime`·`KSICore`를 Agent 없이 분리할 수 없다. Agent 호스트가 없는 게임은 세계 시계가 돌지 않는다. 새 시스템이 세계 진행에 참여할 때마다 코어 함수의 시그니처가 늘어난다. `CCLWorldAdvance::Advance`의 매개변수가 15개가 된 것도 같은 원인이다.

개선 방향은 세계 세션의 소유자를 세계 시뮬레이션으로 옮기고 Life·열 노출을 참여자로 내리는 것이다. 설계는 [R1 설계 초안](R1Design.md)에 있다.

## R2. 두 갈래의 저장 구조

세션 저장과 세계 저장이 서로 다른 구조로 같은 상태를 다룬다.

- `UCCLGameInstance::SaveSession`은 첫 로컬 플레이어의 인벤토리·장비·스킬·동전·퀘스트, 캠페인 감독의 처치 기록과 보급품 배치를 직접 모은다 (`Source/CCL/Session/CCLGameInstance.cpp:294-375`). 결과는 게임 고유 필드가 고정된 `FCCLSessionRecord`에 들어간다 (`Source/CCL/Session/CCLSessionRecord.h:46-89`).
- 세계 스냅샷은 이름 있는 구간과 참여자 방식이다. `SessionContainer`는 개인 세션 레코드가 저장하는 보관함을 세계 캡처에서 빼는 예외다 (`CCLWorldParticipant.h:59-60`, `CCLWorldSimulationSubsystem.h:33`).
- 형식 1-8의 버전별 분기가 레코드 검증 함수에 쌓여 있다 (`Source/CCL/Session/CCLSessionRecord.cpp:36-42`).

[Coding](../Coding/Design.md#적용-예시)은 Save/Load의 공통 기반을 저장 레코드의 식별·버전·직렬화와 복원 연결로 정한다. 현재 세션 레코드는 저장 대상을 공통 구조에 고정하므로 이 원칙과 어긋난다. 새 저장 대상이 생기면 GameInstance, 레코드, 코덱 세 곳을 함께 고쳐야 한다.

개선 방향은 캠페인 진행을 Campaign 참여자 구간으로, 호스트 플레이어 상태를 PlayerPersistence 프로필로 옮기는 것이다. 그러면 세션 레코드는 세계 스냅샷과 메타데이터를 담는 봉투로 줄어들고 `SessionContainer` 예외가 필요 없어진다. R1의 참여자 계약이 정리된 뒤 진행한다.

비용은 세션 저장 형식의 변경이다. 형식 8 이하의 세션 파일을 새 구간으로 나누는 변환이 필요하다. 현재 구조를 유지하는 대안은 변환 비용이 없지만, 저장 대상이 늘 때마다 세 곳을 고치는 비용이 계속 남는다.

## R3. 클라이언트 진입 클래스의 기능 집중

| 대상 | 현재 | 개선 방향 |
|---|---|---|
| `ACCLPlayerController` | 선언된 Server RPC 11개(`Source/CCL/CCLPlayerController.h`) 중 다수가 PlayerState의 컴포넌트로 요청을 넘기기만 한다 (`Source/CCL/CCLPlayerController.cpp:479-501`, `:657-697`). `UCCLLoadoutComponent`는 자체 Server RPC도 가져 기준이 섞여 있다 (`Source/CCL/Items/CCLLoadoutComponent.h:54-63`) | 요청 RPC를 Inventory·Loadout·Expedition 컴포넌트로 옮긴다 |
| 게임 입력 | 입력 매핑과 `UInputAction`을 런타임에 `NewObject`로 만들고, 개발용 `K`를 포함한 키 25개를 코드에 고정한다 (`CCLPlayerController.cpp:55-145`). UI 입력은 데이터 자산 `UCCLUIInputData`를 쓴다 | 입력 설정을 데이터 자산으로 옮기고 태그로 바인딩한다. 키 재설정과 게임패드 경로가 생긴다. 근거는 결정 10의 UE5 현행 시스템 우선이다 |
| `ACCLHUD` | 0.1초 간격 Tick에서 (`Source/CCL/CCLHUD.cpp:30`) 적·NPC·줍기 Actor를 순회하며 문자열을 조립한다 (`:148`, `:165`, `:179`). 상점 아이템을 동기 로드하고 (`:169`) 실험용 PlayerController 타입을 검사한다 (`:120`) | 표시 모델이 상태 변경 이벤트로 갱신되게 하고 HUD는 화면 열기만 맡는다 |
| 상호작용 탐색 | 근처의 줍기·NPC를 찾는 코드가 HUD(`CCLHUD.cpp:165`, `:179`)와 PlayerController(`CCLPlayerController.cpp:459`, `:635`)에 중복된다 | 상호작용 인터페이스와 탐지 컴포넌트 하나로 합친다 |
| `UCCLGameInstance` | 메뉴 표시, 세션 시작·접속, 저장 조립, 화질·창 모드, 재접속 비밀 키를 한 클래스가 맡는다 (`Source/CCL/Session/CCLGameInstance.h`) | 기능별 `UGameInstanceSubsystem`으로 나누고 화질·창 모드는 `UGameUserSettings`로 옮긴다 |

이 구조에서는 새 기능을 붙일 때마다 진입 클래스를 고쳐야 하고, 같은 탐색 규칙이 두 곳에서 따로 바뀔 수 있다. 진입 클래스가 Campaign·Combat·Environment·Items를 모두 include하므로 FrameworkPlan 단계 1에서 UI 계층을 분리할 때도 걸림돌이 된다.

비용은 다음과 같다. RPC를 옮기면 네트워크 함수의 소유 Actor가 바뀌므로 Listen·Dedicated Server의 기존 조작 검사를 다시 실행해야 한다. 입력을 데이터 자산으로 옮기려면 입력 에셋을 만들고 기존 키 배치를 검증해야 한다. 표시 모델을 이벤트 방식으로 바꾸면 갱신 누락을 찾는 검사가 더 필요하다.

## R4. 표시 문구로 표현되는 게임 상태

서버가 한국어 표시 문구를 `FString`으로 복제한다. `UCCLLoadoutComponent::LastResult` (`CCLLoadoutComponent.h:87`), `UCCLExpeditionComponent::Notice` (`Source/CCL/Campaign/CCLExpeditionComponent.h:28`), `ACCLEnemyCharacter::PatternLabel` (`Source/CCL/Combat/CCLEnemyCharacter.h:59`)이 그 예다. `UCCLLoadoutComponent::Report`는 성공 여부를 반환하면서 표시 문구를 복제 값에 기록한다 (`Source/CCL/Items/CCLLoadoutComponent.cpp:512-520`). 테스트를 제외한 코드에서 한글 문자열 리터럴은 506개이고, `LOCTEXT` 계열은 `ACCLVillageSteward::DisplayName` 한 곳이다.

현지화는 [Direction](../Direction.md)의 제작 범위에 없으므로 번역 자체는 이 지적의 대상이 아니다. 문제는 세 가지다.

- 결과를 문구로만 알 수 있어 다른 UI나 검사가 문자열을 비교하게 된다.
- 같은 문구가 연달아 나오면 복제 값이 바뀌지 않아 클라이언트가 새 결과를 구분하지 못한다.
- FrameworkPlan의 공용 후보인 인벤토리·장비 계열이 게임 고유 문구를 품게 된다.

[Coding](../Coding/Design.md#장르에-독립적인-베이스-시스템-설계)의 "기능 실행과 표현을 분리한다"는 원칙과도 어긋난다. 개선 방향은 결과를 GameplayTag와 인자로 전달하고 문구는 클라이언트 UI 계층에서 정하는 것이다. 공용 후보 시스템부터 적용한다. 비용은 결과 태그 목록과 태그별 문구 표를 새로 관리하는 것이다. 문구를 그대로 복제하는 현재 방식은 관리할 표가 없지만 위 세 문제가 남는다.

## R5. 실험장·테스트 코드의 위치

Environment 폴더의 파일 138개 중 자동 검사 파일이 26개, 실험장 Actor·화면·GameMode 파일이 25개다. `CCLExperimentDirector.cpp`는 1,127줄, `CCLExperimentScreen.cpp`는 840줄이다. `Tests` 폴더는 89개 파일, 19,587줄이다. 런타임 클래스 안에도 검사용 시나리오가 있다. `FCCLLifeSimulation::MerchantScenario`가 그 예다 (`Source/CCL/Agents/CCLLifeSimulation.h:228`).

smoke 서브시스템은 Shipping·Test 구성에서 생성만 막고 클래스는 런타임 모듈에 남는다 (`Source/CCL/Tests/CCLWorldSmokeSubsystem.cpp:9-16`). `Source/CCL/Tests/CCLGameplayRuleTests.cpp`는 자동 검사 빌드 가드가 없다.

이 배치는 폴더 의존 측정을 흐린다. Environment가 Items를 include한 6곳과 Session을 include한 2곳은 모두 실험장·자동 검사 파일이다. FrameworkPlan 단계 1-2에서 공용 코드와 실험 코드를 다시 가려야 하는 비용도 생긴다.

개선 방향은 실험장 코드를 `Experiments/` 콘텐츠 폴더로, 자동 검사와 smoke 서브시스템을 별도 개발 모듈로 옮기는 것이다. 패키지 smoke 검사가 Development 패키지에서 계속 로드되어야 하므로 모듈 종류는 실제 패키지로 확인한 뒤 정한다. 비용은 실험장 맵과 Blueprint가 참조하는 타입 경로의 변경이다. 폴더만 옮기면 타입 경로가 그대로지만, 모듈을 옮기면 FrameworkPlan의 이동 절차처럼 CoreRedirects와 에셋 로드 확인이 필요하다.

## R6. 진행 단계의 전체 상태 복사

`CCLWorldAdvance::Advance`는 단계마다 표면과 생태 상태 전체를 후보로 복사한다 (`Source/CCL/Environment/CCLWorldAdvance.cpp:56`, `:67`). 세계 시뮬레이션의 Tick은 한 프레임에 최대 8단계를 진행한다 (`CCLWorldSimulationSubsystem.cpp:58-64`).

다음은 측정이 아닌 계산값이다. `FCCLSurfaceCell`은 `double` 10개와 `uint8` 하나라 정렬 후 약 88B다. 현재 한도 65,536셀([깊은 눈과 캐릭터 이동](../EnvironmentPlan/Snow.md))에서는 복사 한 번이 약 5.5MiB이고, 한 프레임에 최대 약 44MiB를 할당·복사한다. 생태와 Life 후보 복사는 이 계산에 넣지 않았다.

이 위험은 [EnvironmentPlan](../EnvironmentPlan.md)에 이미 기록되어 있고 규모 측정은 단계 9로 잡혀 있다. 측정용 CSV 범위(`CCLWorldAdvance`, `CCLSurfaceAdvance`)가 이미 있으므로 한도 근처 장면에서 먼저 한 번 측정할 것을 제안한다. 비용이 크면 표면 이중 버퍼 교체나 변경 지역만 복사하는 방식을 비교한다.

## 선택 개선

| ID | 대상 | 현재 | 개선 방향 |
|---|---|---|---|
| O1 | GAS 피해 경로 | 초기 능력치를 런타임 `NewObject<UGameplayEffect>`로 적용한다 (`Source/CCL/Combat/CCLFighterComponent.cpp:184`). 공통 상태 효과는 `Source/CCL/AbilitySystem/CCLEffects.cpp`의 C++ 생성자에서 정의해 수치를 데이터로 조정할 수 없다. 피해는 `UCCLHitRule`의 효과 사양 외에 열 노출(`Source/CCL/Environment/CCLThermalExposure.cpp:243`)과 Character(`Source/CCL/CCLCharacter.cpp:147`, `:178`)가 각자 Health를 직접 줄인다 | Damage 메타 Attribute와 Execution으로 피해 경로를 통일한다. 저항·무적·기록을 한 곳에서 처리할 수 있다. 열 피해가 `ICCLHitReceiver`를 거치지 않는 것이 의도인지는 미확인이다 |
| O2 | 저장 식별자 | 아이템·스킬을 에셋의 `FName`으로 저장한다 (`CCLGameInstance.cpp:316`, `:335`). 에셋 이름을 바꾸면 기존 저장을 읽지 못한다. 진행 중인 후속 수정은 중복 이름을 거부하지만 이름 변경은 다루지 않는다 | 정의에 고정 ID를 두거나 `FPrimaryAssetId`를 쓴다 |
| O3 | 팀 구분 | `UCCLFighterComponent::Team`이 `int32`다 (`Source/CCL/Combat/CCLFighterComponent.h:93`) | AIModule의 `FGenericTeamId`와 `IGenericTeamAgentInterface`를 쓰면 AI 인지와 같은 기준을 공유한다. AIModule은 이미 의존 중이다 |
| O4 | 로그 | 테스트를 제외한 코드에서 `LogTemp`를 65번 쓰고 프로젝트 로그 카테고리는 없다 | 시스템별 로그 카테고리를 선언한다. smoke 스크립트가 로그 줄을 파싱하는지 먼저 확인한다 |
| O5 | 헤더 배치 | 헤더 180개 중 16개가 `GENERATED_BODY()` 다음 빈 줄 규칙을 지키지 않는다. `Source/CCL/Session/CCLGameInstance.h`는 영역 구분 전체가 없다 | 해당 파일을 고칠 때 [Coding](../Coding/CppLayout.md#헤더-배치)의 배치로 정리한다 |
