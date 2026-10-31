# 코드 구조 개선 계획

2026-10-11 전체 소스 설계 검토는 기반 계약은 유지할 가치가 있고, 결함 2개(D1-D2)와 설계 위험 6개(R1-R6)를 고쳐야 한다고 결론지었다. 고칠 대상은 세계 수명·저장의 소유권, 두 갈래의 저장 구조, 클라이언트 진입 클래스에 몰린 기능 규칙이다. 이 문서는 그 검토 결과와 개선 계획을 소유한다. 구현은 시작하지 않았고 착수 여부와 순서는 사용자가 정한다.

모듈 분리, `KSI` 명명, 공용 후보와 분리 단계는 [FrameworkPlan](FrameworkPlan.md)이 소유한다. 같은 날 진행 중인 장비·카탈로그·참여자 인터페이스 수정은 [코드 설계 후속 수정](ProgressionFoundation/Verification.md#2026-10-11-설계-후속-수정)이 소유한다. 이 문서는 `CCL` 모듈 안의 소유권·저장·진입점 구조를 다룬다. 겹치는 부분은 "다른 문서와의 관계"에 적는다.

## 하위 문서

| 문서 | 읽는 때 |
|---|---|
| [R1 설계 초안](CodeStructurePlan/R1Design.md) | 세계 소유권 구조 R1의 설계안을 검토하거나 구현할 때 |

## 목표와 현재 상태

| 항목 | 상태 |
|---|---|
| 문서 종류 | 계획. 지적은 2026-10-11 검토 시점의 기록 |
| 검토 기준 코드 | 커밋 `55afa51`. 검토 시점의 미커밋 변경은 문서 3개뿐이고 코드 변경은 없음. 이 문서의 줄 번호는 이 기준이다 |
| 엔진 근거 | UE 5.8.3 (`<Engine>/Build/Build.version`) |
| 검증 방식 | 정적 검토와 엔진 소스 확인. 빌드·실행·결함 재현은 하지 않음 |
| 구현 | 미착수 |
| 착수 순서 | 아래 "진행 순서"는 제안이며 사용자 승인 전 |
| 2026-10-26 확인 | D1의 대체 저장은 여전히 `UCCLAgentWorldSubsystem::Deinitialize`에 있고, D2의 검사 없는 역참조는 `Source/CCL/Environment/CCLWorldSimulationSubsystem.cpp:270`, `:292`에 남아 있다. 기준 커밋 이후 FrameworkPlan 단계 2-4가 끝났으므로 다른 지적의 코드 위치는 착수할 때 다시 대조한다 |

## 지적의 분류

이 문서는 [Coding](Coding.md#근거설계리뷰)의 리뷰 기준에 따라 지적을 세 종류로 나눈다.

| 분류 | 뜻 |
|---|---|
| 결함 (D) | 현재 코드에서 잘못된 결과, 데이터 손실 또는 크래시가 날 수 있는 경로 |
| 설계 위험 (R) | 지금 동작은 맞지만 변경·확장·분리 비용을 키우는 구조 |
| 선택 개선 (O) | 적용하면 이득이 있지만 미뤄도 결과가 틀리지 않는 항목 |

## 검토 결과 요약

유지할 기반 계약은 다음과 같다. 후보 상태를 계산한 뒤 모두 성공하면 확정하는 진행·복원 트랜잭션, 참여자의 Prepare·Commit·Cancel·Publish 계약, 판정과 부수 효과를 나눈 `UCCLHitRule`의 `Resolve`·`Commit`, 결과 통지용 `ICCLHitReceiver`, UI 핸들과 레지스트리, 클라이언트 요청의 서버 검증이다. 예를 들어 `UCCLLoadoutComponent::Learn`은 제공 목록에 없는 스킬을 거부한다 (`Source/CCL/Items/CCLLoadoutComponent.cpp:312`).

| ID | 분류 | 요약 | 권장 순서 |
|---|---|---|---|
| D1 | 결함 | 언로드 대체 저장이 서브시스템 해제 순서에 따라 참여자 구간을 빠뜨릴 수 있음 | 1 |
| D2 | 결함 (낮음) | 열 노출 서브시스템 포인터를 검사 없이 역참조 | 1 |
| R1 | 설계 위험 | 세계 시작·저장·이동을 Agent 서브시스템이 소유하고 Life가 참여자 계약 밖의 코어로 고정됨 | 2 |
| R2 | 설계 위험 | 세션 레코드와 세계 스냅샷의 두 저장 구조가 공존함 | 4 |
| R3 | 설계 위험 | PlayerController·HUD·GameInstance에 기능 규칙이 몰림 | 3 |
| R4 | 설계 위험 | 게임 상태와 결과가 한국어 표시 문구로 표현되고 복제됨 | 6 |
| R5 | 설계 위험 | 실험장·테스트 코드가 런타임 모듈과 Environment 폴더에 섞임 | 6 |
| R6 | 설계 위험 | 진행 단계마다 표면·생태 상태 전체를 복사함. 문서에 이미 기록된 위험 | 5 |
| O1-O5 | 선택 개선 | GAS 피해 경로, 저장 식별자, 팀 구분, 로그 카테고리, 헤더 배치 | 해당 작업 때 |

## 검토 범위와 측정 방법

자세히 읽은 범위는 세계 시뮬레이션 코어(`UCCLWorldSimulationSubsystem`, `CCLWorldAdvance`, `ICCLWorldParticipant`, 세계 스냅샷 검증, 열 노출의 Prepare), `UCCLAgentWorldSubsystem`, `ACCLPlayerController`, `ACCLHUD`, `UCCLLoadoutComponent`, `UCCLHitRule`과 효과 정의, `UCCLGameInstance`의 세션 저장, `FCCLSessionRecord`다. UI Core 헤더, `FCCLLifeSimulation`의 공개 API와 `UCCLFighterComponent`의 초기화는 훑어보았다.

지형·물·눈·불의 내부 계산, 표면·지형 복제, Mass·StateTree 연결, Editor 도구, Campaign 감독의 세부는 보지 않았다. 이 영역의 결함 여부는 미확인이다.

수치는 2026-10-11 작업 트리에서 다음 방법으로 측정했다.

- 폴더 의존은 `Source/CCL`의 최상위 폴더(`UI/Core`는 따로)를 단위로, 다른 폴더의 헤더를 `#include`한 횟수를 셌다.
- 한국어 문자열은 `Tests` 폴더와 `*Tests.cpp`를 제외한 코드에서 한글 음절이 들어간 `TEXT("...")` 리터럴을 셌다.
- 파일 수와 줄 수는 `.h`·`.cpp`만 셌다.

## 결함

### D1. 언로드 대체 저장의 참여자 구간 누락

`UCCLAgentWorldSubsystem::Deinitialize`는 맵 이동 저장이 실패한 경우 세계를 다시 저장해 세션 저장소에 넣는다 (`Source/CCL/Agents/CCLAgentWorldSubsystem.cpp:95-119`). 이때 인벤토리와 플레이어 저장 참여자가 이미 등록을 해제했으면, 그 구간이 빠진 세계가 정상 저장으로 남는다. 코드와 엔진 근거는 확인했고 실행 재현은 하지 않았다.

발생 경로는 다음과 같다.

1. 월드 해체가 시작되면 `OnWorldBeginTearDown`이 맵 이동 체크포인트를 저장한다 (`CCLAgentWorldSubsystem.cpp:223-241`). 지형 지역이 준비되지 않았으면 이 저장은 실패한다 (`CCLAgentWorldSubsystem.cpp:164-171`).
2. 참여자인 `UCCLInventoryPersistenceSubsystem`과 `UCCLPlayerPersistenceSubsystem`은 자기 `Deinitialize`에서 등록을 해제한다 (`Source/CCL/Items/CCLInventoryPersistenceSubsystem.cpp:92-99`, `Source/CCL/Session/CCLPlayerPersistenceSubsystem.cpp:64-71`).
3. 참여자가 Agent 서브시스템보다 먼저 해제되면 Agent의 대체 저장은 남은 참여자만 수집한다. `UCCLWorldSimulationSubsystem::Save`와 `CaptureParticipants`는 등록된 참여자를 순회할 뿐 누락을 검사하지 않는다 (`Source/CCL/Environment/CCLWorldSimulationSubsystem.cpp:323-357`, `:638-656`).
4. 스냅샷 검증은 존재하는 구간만 검사한다 (`Source/CCL/Environment/CCLWorldSnapshot.cpp:453-459`). 복원은 없는 구간을 빈 구간으로 처리한다 (`CCLWorldSimulationSubsystem.cpp:668`).

해제 순서는 엔진이 보장하지 않는다. UE 5.8.3은 서브시스템을 `Initialize` 호출 전에 맵에 넣는다 (`<Engine>/Source/Runtime/Engine/Private/Subsystems/SubsystemCollection.cpp:524-526`). 그래서 `InitializeDependency`로 끌려온 서브시스템은 끌어온 쪽보다 뒤에 들어간다. 해제는 이 맵의 순서대로 진행하며 의존 역순을 따르지 않는다 (`SubsystemCollection.cpp:349-358`). 인벤토리 저장 서브시스템이 먼저 생성되면 해제도 Agent보다 먼저 일어난다. 반면 모든 월드 서브시스템의 `PreDeinitialize`는 해제 전에 호출된다 (`<Engine>/Source/Runtime/Engine/Private/World.cpp:6474-6486`).

영향은 진행 손실이다. 보관함과 플레이어 프로필 구간이 빠진 세계가 세션 저장소에 들어가고, 다음 로드에서 그 상태가 비어 있는 채로 복원될 수 있다.

수정 방향은 두 가지를 함께 적용하는 것이다.

- 대체 저장을 `PreDeinitialize`로 옮기거나 없앤다. R1을 진행하면 이 책임은 `UCCLWorldSimulationSubsystem::PreDeinitialize`로 이동한다.
- 캡처할 때 등록된 구간은 비어 있어도 항상 기록한다. 현재 형식의 스냅샷에서는 등록된 구간의 누락을 거부하고, 누락 허용은 이전 형식의 변환에만 둔다.

검증은 자동 검사로 한다. 참여자 등록을 먼저 해제한 뒤 Agent의 대체 저장 경로를 실행하고, 결과 스냅샷에 인벤토리·플레이어 구간이 있는지 또는 저장이 거부되는지 확인한다. 수정 전 코드에서 이 검사가 실패하는지도 먼저 확인한다.

### D2. 열 노출 포인터의 역참조

`UCCLWorldSimulationSubsystem::AdvancePending`은 `UCCLThermalExposureSubsystem` 포인터를 얻은 뒤 (`CCLWorldSimulationSubsystem.cpp:245`) 준비 람다 안에서만 null을 검사한다 (`:250`). 실패 경로의 `Cancel` (`:268`)과 성공 경로의 `Commit` (`:290`)은 검사 없이 역참조한다. 두 서브시스템의 생성 조건이 같고 `Initialize`에서 의존 초기화를 하므로 (`:35`) 현재는 null이 되지 않는다. 생성 조건이 달라지면 크래시가 난다. R1에서 열 노출을 참여자로 옮기면 이 코드는 사라진다.

## 설계 위험

### R1. 세계 수명·저장의 소유권 역전

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

개선 방향은 세계 세션의 소유자를 세계 시뮬레이션으로 옮기고 Life·열 노출을 참여자로 내리는 것이다. 설계는 아래 "R1 설계 초안"에 있다.

### R2. 두 갈래의 저장 구조

세션 저장과 세계 저장이 서로 다른 구조로 같은 상태를 다룬다.

- `UCCLGameInstance::SaveSession`은 첫 로컬 플레이어의 인벤토리·장비·스킬·동전·퀘스트, 캠페인 감독의 처치 기록과 보급품 배치를 직접 모은다 (`Source/CCL/Session/CCLGameInstance.cpp:294-375`). 결과는 게임 고유 필드가 고정된 `FCCLSessionRecord`에 들어간다 (`Source/CCL/Session/CCLSessionRecord.h:46-89`).
- 세계 스냅샷은 이름 있는 구간과 참여자 방식이다. `SessionContainer`는 개인 세션 레코드가 저장하는 보관함을 세계 캡처에서 빼는 예외다 (`CCLWorldParticipant.h:59-60`, `CCLWorldSimulationSubsystem.h:33`).
- 형식 1-8의 버전별 분기가 레코드 검증 함수에 쌓여 있다 (`Source/CCL/Session/CCLSessionRecord.cpp:36-42`).

[Coding](Coding/Design.md#적용-예시)은 Save/Load의 공통 기반을 저장 레코드의 식별·버전·직렬화와 복원 연결로 정한다. 현재 세션 레코드는 저장 대상을 공통 구조에 고정하므로 이 원칙과 어긋난다. 새 저장 대상이 생기면 GameInstance, 레코드, 코덱 세 곳을 함께 고쳐야 한다.

개선 방향은 캠페인 진행을 Campaign 참여자 구간으로, 호스트 플레이어 상태를 PlayerPersistence 프로필로 옮기는 것이다. 그러면 세션 레코드는 세계 스냅샷과 메타데이터를 담는 봉투로 줄어들고 `SessionContainer` 예외가 필요 없어진다. R1의 참여자 계약이 정리된 뒤 진행한다.

비용은 세션 저장 형식의 변경이다. 형식 8 이하의 세션 파일을 새 구간으로 나누는 변환이 필요하다. 현재 구조를 유지하는 대안은 변환 비용이 없지만, 저장 대상이 늘 때마다 세 곳을 고치는 비용이 계속 남는다.

### R3. 클라이언트 진입 클래스의 기능 집중

| 대상 | 현재 | 개선 방향 |
|---|---|---|
| `ACCLPlayerController` | 선언된 Server RPC 11개(`Source/CCL/CCLPlayerController.h`) 중 다수가 PlayerState의 컴포넌트로 요청을 넘기기만 한다 (`Source/CCL/CCLPlayerController.cpp:479-501`, `:657-697`). `UCCLLoadoutComponent`는 자체 Server RPC도 가져 기준이 섞여 있다 (`Source/CCL/Items/CCLLoadoutComponent.h:54-63`) | 요청 RPC를 Inventory·Loadout·Expedition 컴포넌트로 옮긴다 |
| 게임 입력 | 입력 매핑과 `UInputAction`을 런타임에 `NewObject`로 만들고, 개발용 `K`를 포함한 키 25개를 코드에 고정한다 (`CCLPlayerController.cpp:55-145`). UI 입력은 데이터 자산 `UCCLUIInputData`를 쓴다 | 입력 설정을 데이터 자산으로 옮기고 태그로 바인딩한다. 키 재설정과 게임패드 경로가 생긴다. 근거는 결정 10의 UE5 현행 시스템 우선이다 |
| `ACCLHUD` | 0.1초 간격 Tick에서 (`Source/CCL/CCLHUD.cpp:30`) 적·NPC·줍기 Actor를 순회하며 문자열을 조립한다 (`:148`, `:165`, `:179`). 상점 아이템을 동기 로드하고 (`:169`) 실험용 PlayerController 타입을 검사한다 (`:120`) | 표시 모델이 상태 변경 이벤트로 갱신되게 하고 HUD는 화면 열기만 맡는다 |
| 상호작용 탐색 | 근처의 줍기·NPC를 찾는 코드가 HUD(`CCLHUD.cpp:165`, `:179`)와 PlayerController(`CCLPlayerController.cpp:459`, `:635`)에 중복된다 | 상호작용 인터페이스와 탐지 컴포넌트 하나로 합친다 |
| `UCCLGameInstance` | 메뉴 표시, 세션 시작·접속, 저장 조립, 화질·창 모드, 재접속 비밀 키를 한 클래스가 맡는다 (`Source/CCL/Session/CCLGameInstance.h`) | 기능별 `UGameInstanceSubsystem`으로 나누고 화질·창 모드는 `UGameUserSettings`로 옮긴다 |

이 구조에서는 새 기능을 붙일 때마다 진입 클래스를 고쳐야 하고, 같은 탐색 규칙이 두 곳에서 따로 바뀔 수 있다. 진입 클래스가 Campaign·Combat·Environment·Items를 모두 include하므로 FrameworkPlan 단계 1에서 UI 계층을 분리할 때도 걸림돌이 된다.

비용은 다음과 같다. RPC를 옮기면 네트워크 함수의 소유 Actor가 바뀌므로 Listen·Dedicated Server의 기존 조작 검사를 다시 실행해야 한다. 입력을 데이터 자산으로 옮기려면 입력 에셋을 만들고 기존 키 배치를 검증해야 한다. 표시 모델을 이벤트 방식으로 바꾸면 갱신 누락을 찾는 검사가 더 필요하다.

### R4. 표시 문구로 표현되는 게임 상태

서버가 한국어 표시 문구를 `FString`으로 복제한다. `UCCLLoadoutComponent::LastResult` (`CCLLoadoutComponent.h:87`), `UCCLExpeditionComponent::Notice` (`Source/CCL/Campaign/CCLExpeditionComponent.h:28`), `ACCLEnemyCharacter::PatternLabel` (`Source/CCL/Combat/CCLEnemyCharacter.h:59`)이 그 예다. `UCCLLoadoutComponent::Report`는 성공 여부를 반환하면서 표시 문구를 복제 값에 기록한다 (`Source/CCL/Items/CCLLoadoutComponent.cpp:512-520`). 테스트를 제외한 코드에서 한글 문자열 리터럴은 506개이고, `LOCTEXT` 계열은 `ACCLVillageSteward::DisplayName` 한 곳이다.

현지화는 [Direction](Direction.md)의 제작 범위에 없으므로 번역 자체는 이 지적의 대상이 아니다. 문제는 세 가지다.

- 결과를 문구로만 알 수 있어 다른 UI나 검사가 문자열을 비교하게 된다.
- 같은 문구가 연달아 나오면 복제 값이 바뀌지 않아 클라이언트가 새 결과를 구분하지 못한다.
- FrameworkPlan의 공용 후보인 인벤토리·장비 계열이 게임 고유 문구를 품게 된다.

[Coding](Coding/Design.md#장르에-독립적인-베이스-시스템-설계)의 "기능 실행과 표현을 분리한다"는 원칙과도 어긋난다. 개선 방향은 결과를 GameplayTag와 인자로 전달하고 문구는 클라이언트 UI 계층에서 정하는 것이다. 공용 후보 시스템부터 적용한다. 비용은 결과 태그 목록과 태그별 문구 표를 새로 관리하는 것이다. 문구를 그대로 복제하는 현재 방식은 관리할 표가 없지만 위 세 문제가 남는다.

### R5. 실험장·테스트 코드의 위치

Environment 폴더의 파일 138개 중 자동 검사 파일이 26개, 실험장 Actor·화면·GameMode 파일이 25개다. `CCLExperimentDirector.cpp`는 1,127줄, `CCLExperimentScreen.cpp`는 840줄이다. `Tests` 폴더는 89개 파일, 19,587줄이다. 런타임 클래스 안에도 검사용 시나리오가 있다. `FCCLLifeSimulation::MerchantScenario`가 그 예다 (`Source/CCL/Agents/CCLLifeSimulation.h:228`).

smoke 서브시스템은 Shipping·Test 구성에서 생성만 막고 클래스는 런타임 모듈에 남는다 (`Source/CCL/Tests/CCLWorldSmokeSubsystem.cpp:9-16`). `Source/CCL/Tests/CCLGameplayRuleTests.cpp`는 자동 검사 빌드 가드가 없다.

이 배치는 폴더 의존 측정을 흐린다. Environment가 Items를 include한 6곳과 Session을 include한 2곳은 모두 실험장·자동 검사 파일이다. FrameworkPlan 단계 1-2에서 공용 코드와 실험 코드를 다시 가려야 하는 비용도 생긴다.

개선 방향은 실험장 코드를 `Experiments/` 콘텐츠 폴더로, 자동 검사와 smoke 서브시스템을 별도 개발 모듈로 옮기는 것이다. 패키지 smoke 검사가 Development 패키지에서 계속 로드되어야 하므로 모듈 종류는 실제 패키지로 확인한 뒤 정한다. 비용은 실험장 맵과 Blueprint가 참조하는 타입 경로의 변경이다. 폴더만 옮기면 타입 경로가 그대로지만, 모듈을 옮기면 FrameworkPlan의 이동 절차처럼 CoreRedirects와 에셋 로드 확인이 필요하다.

### R6. 진행 단계의 전체 상태 복사

`CCLWorldAdvance::Advance`는 단계마다 표면과 생태 상태 전체를 후보로 복사한다 (`Source/CCL/Environment/CCLWorldAdvance.cpp:56`, `:67`). 세계 시뮬레이션의 Tick은 한 프레임에 최대 8단계를 진행한다 (`CCLWorldSimulationSubsystem.cpp:58-64`).

다음은 측정이 아닌 계산값이다. `FCCLSurfaceCell`은 `double` 10개와 `uint8` 하나라 정렬 후 약 88B다. 현재 한도 65,536셀([깊은 눈과 캐릭터 이동](EnvironmentSnow.md))에서는 복사 한 번이 약 5.5MiB이고, 한 프레임에 최대 약 44MiB를 할당·복사한다. 생태와 Life 후보 복사는 이 계산에 넣지 않았다.

이 위험은 [EnvironmentPlan](EnvironmentPlan.md)에 이미 기록되어 있고 규모 측정은 단계 9로 잡혀 있다. 측정용 CSV 범위(`CCLWorldAdvance`, `CCLSurfaceAdvance`)가 이미 있으므로 한도 근처 장면에서 먼저 한 번 측정할 것을 제안한다. 비용이 크면 표면 이중 버퍼 교체나 변경 지역만 복사하는 방식을 비교한다.

## 선택 개선

| ID | 대상 | 현재 | 개선 방향 |
|---|---|---|---|
| O1 | GAS 피해 경로 | 초기 능력치를 런타임 `NewObject<UGameplayEffect>`로 적용한다 (`Source/CCL/Combat/CCLFighterComponent.cpp:184`). 공통 상태 효과는 `Source/CCL/AbilitySystem/CCLEffects.cpp`의 C++ 생성자에서 정의해 수치를 데이터로 조정할 수 없다. 피해는 `UCCLHitRule`의 효과 사양 외에 열 노출(`Source/CCL/Environment/CCLThermalExposure.cpp:243`)과 Character(`Source/CCL/CCLCharacter.cpp:147`, `:178`)가 각자 Health를 직접 줄인다 | Damage 메타 Attribute와 Execution으로 피해 경로를 통일한다. 저항·무적·기록을 한 곳에서 처리할 수 있다. 열 피해가 `ICCLHitReceiver`를 거치지 않는 것이 의도인지는 미확인이다 |
| O2 | 저장 식별자 | 아이템·스킬을 에셋의 `FName`으로 저장한다 (`CCLGameInstance.cpp:316`, `:335`). 에셋 이름을 바꾸면 기존 저장을 읽지 못한다. 진행 중인 후속 수정은 중복 이름을 거부하지만 이름 변경은 다루지 않는다 | 정의에 고정 ID를 두거나 `FPrimaryAssetId`를 쓴다 |
| O3 | 팀 구분 | `UCCLFighterComponent::Team`이 `int32`다 (`Source/CCL/Combat/CCLFighterComponent.h:93`) | AIModule의 `FGenericTeamId`와 `IGenericTeamAgentInterface`를 쓰면 AI 인지와 같은 기준을 공유한다. AIModule은 이미 의존 중이다 |
| O4 | 로그 | 테스트를 제외한 코드에서 `LogTemp`를 65번 쓰고 프로젝트 로그 카테고리는 없다 | 시스템별 로그 카테고리를 선언한다. smoke 스크립트가 로그 줄을 파싱하는지 먼저 확인한다 |
| O5 | 헤더 배치 | 헤더 180개 중 16개가 `GENERATED_BODY()` 다음 빈 줄 규칙을 지키지 않는다. `Source/CCL/Session/CCLGameInstance.h`는 영역 구분 전체가 없다 | 해당 파일을 고칠 때 [Coding](Coding/CppLayout.md#헤더-배치)의 배치로 정리한다 |

## 진행 순서

| 단계 | 대상 | 진행 조건 | 완료 기준 |
|---|---|---|---|
| 1 | D1, D2 | 사용자 착수 지시 | D1 자동 검사가 수정 전에는 실패하고 수정 후에는 통과 |
| 2 | R1 | 단계 1 완료. FrameworkPlan 단계 2 이전 | 위 "검증 방법" 1-5 통과. `CCLWorldSimulationSubsystem`, `CCLWorldAdvance`, `CCLWorldSnapshot`, `CCLWorldParticipant`, `CCLWorldEnvironmentConfig`의 Agents include 0개 |
| 3 | R3 | 단계 2와 독립. CCL 콘텐츠 작업과 함께 진행 가능 | 입력 데이터 자산 적용, 진입 클래스의 RPC 이전, 기존 조작 검사 통과 |
| 4 | R2 | 단계 2 완료 | 세션 레코드에서 게임 고유 필드 제거, 이전 세션 저장의 변환 로드 |
| 5 | R6 측정 | 언제든 가능 | 한도 근처 장면의 CSV 측정값 기록 |
| 6 | R4, R5 | FrameworkPlan 단계 1의 폴더 이동과 함께 | 실험·검사 코드의 폴더·모듈 이동, 공용 후보의 표시 문구 제거 |

선택 개선 O1-O5는 해당 파일을 고치는 단계에서 함께 처리한다. 코드 단계마다 [Coding](Coding.md#코드-작업-완료-검증)의 완료 검증(프로젝트 파일 생성, `CCLEditor Win64 Development` 빌드, 기능 실행 검사)을 수행한다.

## 다른 문서와의 관계

### 코드 설계 후속 수정

[코드 설계 후속 수정](ProgressionFoundation/Verification.md#2026-10-11-설계-후속-수정)은 같은 날 다른 검토 결과로 시작한 수정이며, 이 문서를 작성할 때 진행 중이었다. 범위는 장비 변경의 재진입 거절, 카탈로그 중복 이름 검증, 인벤토리 기능 서비스 분리, 저장 참여자에서 시간 진행 인터페이스 분리, 표면 발행본 공유다.

겹치는 부분은 두 가지다. 시간 진행 인터페이스 분리는 R1의 전제이며, R1 초안은 그 결과를 기준으로 한다. 카탈로그 중복 이름 검증은 O2의 일부만 다룬다. 그 문서의 범위표에는 D1, R1의 소유권 이전, R2-R6이 없다. 그 수정의 검증 결과가 기록되면 이 문서의 줄 번호와 R1 초안을 다시 맞춘다.

### FrameworkPlan

FrameworkPlan 단계 2는 저장 참여 계약을 Environment 폴더에서 분리해 `KSICore`로 옮기는 것이다. 현재 참여자 계약에는 Life와 아이템 개념이 들어 있어 그대로 옮기면 `KSICore`가 게임 시스템에 의존하게 된다. 그래서 이 문서는 R1을 FrameworkPlan 단계 2의 선행 작업으로 제안한다.

R5의 실험·검사 코드 이동은 FrameworkPlan 단계 1의 `CCLEditor` 분리와 같은 성격의 작업이다. 두 작업을 함께 하면 폴더 의존을 한 번에 다시 측정할 수 있다.

FrameworkPlan의 단계 표에 이 선행 관계를 반영할지는 사용자가 정한다. 반영하기 전까지 두 문서의 단계 번호는 서로 독립적이다.

이 문서의 기준 커밋 `55afa51` 뒤에 FrameworkPlan 단계 2-4가 끝났다([단계 4](FrameworkPlan/Stage4.md#단계-4-구현과-검증)). 그래서 R1을 단계 2의 선행 작업으로 둔 제안과 아래 "진행 순서" 2단계의 "FrameworkPlan 단계 2 이전" 조건은 그 진행에 반영되지 않았다. 착수할 때 R1·R5의 범위를 KSI 모듈로 옮긴 결과와 다시 맞춘다.

## 미정 사항

- 착수 여부와 단계 순서
- R1의 전체 수정과 절반 수정 중 선택
- 세계 스냅샷 형식 변경 때 이전 저장의 변환 범위와 지원 기간
- R5의 개발 모듈 종류. Development 패키지의 smoke 검사 로드 조건을 확인한 뒤 정한다
- R4의 결과 태그 범위와 현지화 도입 여부
- `FCCLWorldClaims`로 보관함을 식별할 고정 ID의 존재 여부
