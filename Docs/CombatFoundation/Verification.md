# 전투 검증

전투를 고친 뒤 무엇을 검사할지, 어디까지 확인됐는지 볼 때 읽는다.

## 검증 방법

`Tools/Validation/run_combat_smoke.ps1`은 Standalone, Dedicated, Listen과 호스트 조작을 고르고 `-Impaired`로 지연·손실을 설정한다. 서버 판정과 실제 클라이언트 Ability 입력을 연결해 다음을 검사한다. 실행 순서와 증거 기록은 `ccl8-verify` 스킬의 전투 기능 파일을 따른다.

- 기본 공격 적중과 비용의 단일 차감, 정면 가드와 입력 해제.
- 가드 붕괴 시 해당 공격 차단, 후속 피해, 후방 가드 실패.
- 패링 성공 구간과 공격자 경직, 회피 무적, 비용 부족 거부.
- 스태미나 없는 표적, 중복·종료된 공격 거부, 자원 상한.
- 재스폰 후 ASC 유지와 Avatar 교체, 자원 초기화, 능력 중복과 이전 생명 효과 제거.

에디터 자동 검사 `Source/CCL/Tests/CCLGameplayRuleTests.cpp`는 멀티프로세스 스모크로만 확인하던 규칙 8개를 맡는다. 전투 판정 결과, 피해 효과 거부 시 부작용 없음, 적 패턴 순환, 인벤토리 추가·이동·제거, 장착과 사용, 저장 기록 거부 경로, 재접속 신원 키, 세계 저장 구간 규칙이다. 테스트 월드는 `CreateWorld` 뒤 `InitializeActorsForPlay`, `BeginPlay`, `WorldSettings->NotifyBeginPlay`를 호출해야 Actor의 `BeginPlay`가 실행된다. GameMode가 없는 월드에서 마지막 호출을 빠뜨리면 Fighter의 공격 정의가 비어 있다.


화면 검사는 개발 구성의 선택적 `-CCLVisualSmoke` 실행으로 한다. 월드 서브시스템이 Standalone에서 적 AI를 멈추고 기본 자세, 공격, 가드, 패링, 회피, 사망 화면을 저장한다. 표현을 비교하기 위한 고정 장면이고 AI 추적과 네트워크 검사를 대신하지 않는다. Shipping과 Test 구성에서는 서브시스템을 생성하지 않는다.

에셋 생성 도구는 `Tools/Validation/create_combat_playground.py`와 `UCCLCombatAssetLibrary`다. 프로젝트 전용 공격·회피 시퀀스, AnimBP, Montage, AbilitySet, 아이템, StateTree, 맵을 만든다. 원본 시퀀스는 두고 복제한 시퀀스의 루트 이동을 잠가 AbilityTask 이동과 겹치지 않게 한다. 가드·패링 자세와 색상 표시는 임시 표현이다.

## 검증 상태

UE 5.8.3의 전투 Standalone·Dedicated, 자동 검사 154개와 기존 기능 회귀 결과는 [2026-10-11 재검증 기록](../QA/FullReviewReverification-2026-10-11.md)이 소유한다. 이전 기록에서 남은 사실은 다음과 같다.

- **예측 효과 제거:** 클라이언트의 `RemoveActiveGameplayEffect`는 권한이 없어 거부된다(`<GAS>/Private/AbilitySystemComponent.cpp:1275`). `UCCLCombatAbility`의 Busy와 방어 구간 효과는 서버가 제거를 확정하고, 클라이언트의 예측 효과는 GAS의 예측 키 정리와 서버 복제를 따른다. 엔진의 예측 제거 제한을 완화하는 콘솔 변수는 쓰지 않는다.
- **효과 생성자:** `CCLEffects.cpp`의 생성자는 `CreateDefaultSubobject`로 GameplayEffectComponent를 만들고 `GEComponents`에 등록한다. 생성자에서 `FindOrAddComponent`를 쓰면 이름 없는 `NewObject`로 초기화 오류가 난다.
- **지연과 손실:** 100ms 왕복 지연, 2% 손실에서 Dedicated 전투가 통과했다.
- **UE 5.9 호환:** 2026-10-07 UE 5.9.0에서 Standalone, Dedicated, Listen 원격과 호스트, 지연과 손실 5개 구성이 통과했다. 이 환경에서 만든 바이너리 에셋을 UE 5.8.1에서 여는 것은 검증하지 않았다.
- **테스트 대기:** 에디터 초기 로딩에 대비한 기본 시작 대기는 600초이고 `-StartupTimeoutSeconds`로 바꾼다.
- **내비게이션:** 전투 맵과 진행 맵의 볼륨 브러시가 비어 있던 결함은 [CampaignFoundation](../CampaignFoundation.md)이 소유한다. 고정 위치 판정 통과와 실제 AI 접근 확인은 구분한다.
- **표현:** 기본 자세, 공격, 가드, 패링, 회피, 사망 6개 캡처를 확인했다. 최종 조작감과 예고 가독성은 추가 검토가 필요하다.

## 미검증

정확한 방어 구간 경계, 불가 공격, 동시 피격, 서버 예측 거절, 벽 충돌, 재접속·늦은 관전자, 최종 표현 품질은 추가 실행 검증이 필요하다. 자동 시나리오만으로 통과했다고 판단하지 않는다. 임시 마네킹과 색상 표시는 최종 아트가 아니다.

## 엔진 소스 근거

GAS는 UE5에서 처음 도입된 기능이 아니다. 이 구현은 UE 5.8의 Attribute 복제·예측과 GameplayEffect Component 구성을 쓴다. 자체 능력·효과 스케줄러를 줄일 수 있지만 ActorInfo 재연결, 생명 효과 정리, 예측 보정을 검증해야 한다.

2026-10-06 집의 `<Engine>/Build/Build.version`에서 UE 5.8.1, Changelist 0을 확인했다. Graft 조회를 출발점으로 아래 실제 설치 소스를 확인했다. `<GAS>`는 `<Engine>/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities`다. 엔진 API 확인과 프로젝트의 빌드·실행 검증은 구분한다.

| 근거 | 확인 내용 |
|---|---|
| `<GAS>/Private/AbilitySystemComponent_Abilities.cpp:161` | InitAbilityActorInfo의 OwnerActor·AvatarActor 설정 |
| `<GAS>/Private/AbilitySystemComponent_Abilities.cpp:1604` | TryActivateAbility 실행 경로 |
| `<GAS>/Private/AbilitySystemComponent_Abilities.cpp:2846` | 로컬 입력 해제를 Ability와 복제 이벤트에 전달 |
| `<GAS>/Private/Abilities/Tasks/AbilityTask_WaitInputRelease.cpp:16` | 예측 클라이언트의 입력 해제를 서버 이벤트로 전달 |
| `<GAS>/Private/Abilities/GameplayAbility.cpp:592` | CommitAbility가 CommitCheck 이후 비용·쿨다운 적용 경로 실행 |
| `<GAS>/Private/AbilitySystemComponent.cpp:525`, `:975` | 효과 Spec 생성과 대상 ASC 적용 |
| `<GAS>/Public/GameplayEffect.h:1117` | GameplayTag로 SetByCaller magnitude 설정 |
| `<GAS>/Public/ActiveGameplayEffectHandle.h:39` | Instant 효과의 IsValid와 WasSuccessfullyApplied 의미 구분 |
| `<GAS>/Public/AttributeSet.h:206`, `:220`, `:402` | 효과 실행 후처리·값 변경 전처리·Attribute RepNotify |
| `<GAS>/Public/AbilitySystemComponent.h:539` | Attribute 값 변경 델리게이트 조회 |
| `<GAS>/Private/GameplayEffect.cpp:5195`, `:5226` | Mixed·Minimal의 활성 효과 복제 범위와 소유 연결 검사 |
| `<GAS>/Public/Abilities/GameplayAbilityTypes.h:46`, `:65` | InstancedPerActor 권장과 LocalPredicted 선언 |
| `<GAS>/Public/Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h:33` | Root Motion 기반 이동 Task 진입점 |
