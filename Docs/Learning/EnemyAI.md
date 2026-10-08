# 적 AI: 전투 의사와 StateTree 실행을 나누기

적은 StateTree에서 탐색·접근·공격·복귀를 실행하고, 현재 구현에서는 Agent의 성향·상태로 교전 지속 여부도 판단한다. `Source/CCL/Combat/CCLEnemyAIController.cpp`와 `CCLEnemyCharacter.cpp`를 따라 행동 선택이 GAS 공격으로 연결되는 과정을 읽는다. 생활 NPC의 작업 예약은 [Agent 실행](AgentExecution.md)에서 다룬다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `4238990815cc932a6f38c55f947be78065389651` |
| 도입·변경 기준 | `0931fbfb5c96c4da24acb9d2ed44243fc0e6171f` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

`4238990`의 최초 전투 AI와 현재의 Agent·공통 Action 연결을 비교한다. 콘텐츠별 공격 패턴은 `2d39c5f`에서 추가됐으며 당시 계약은 [ContentFoundation](../ContentFoundation.md)에 있다. 현재 코드의 추가 분기를 모두 최초 StateTree의 동작으로 소급하지 않는다.

## 2. 접근 성공과 공격 성공

적이 목표 근처에 도달했다고 곧바로 공격이 성공하는 것은 아니다. 대상을 잃거나 경직됐을 수 있고, 공격을 시작했어도 아직 회복 중일 수 있다.

그래서 StateTree Task는 `Running`, `Succeeded`, `Failed`를 구분한다. 공격 시작 여부와 행동 종료 여부를 별도 상태로 보관해야 한 프레임마다 같은 공격을 다시 시작하지 않는다.

## 3. StateTree와 Task 인스턴스

StateTree는 상태와 전이로 실행 흐름을 구성하는 UE5 계열 시스템이다. Behavior Tree의 Selector·Task와 목적이 겹치지만 같은 자산 형식은 아니다. 이 프로젝트는 `UStateTreeAIComponent`를 Controller에 만들고 `OnPossess`에서 정의된 트리를 시작한다.

Task의 정의와 실행별 데이터도 나뉜다. `FCCLEnemyTask`는 실행 모드에 따라 Controller 함수를 호출하고, `Context.GetInstanceData(*this)`는 현재 실행의 `bStarted`를 보관한다. 공유 Task 정의에 모든 적의 실행 상태를 기록하지 않는다.

`MoveToActor`와 `MoveToLocation`은 AIController의 내비게이션 이동 요청이다. StateTree가 실행 중이라는 사실만으로 경로 생성이나 이동이 성공했다고 판단할 수는 없다.

## 4. 이전의 공격 시작

최초 AI는 대상을 확인하고 ASC 입력을 직접 호출했다.

출처: `4238990`, [CCLEnemyAIController.cpp](../../Source/CCL/Combat/CCLEnemyAIController.cpp):133의 `ACCLEnemyAIController::StartAttack`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool ACCLEnemyAIController::StartAttack()
{
    auto* Enemy = Cast<ACCLEnemyCharacter>(GetPawn());
    auto* ASC = Enemy ? Cast<UCCLAbilitySystemComponent>(Enemy->GetAbilitySystemComponent()) : nullptr;

    if (!ASC || !HasTarget() || IsActionRunning())
    {
        return false;
    }

    StopMovement();
    ASC->AbilityInputTagPressed(CCLTags::Input_Attack);
    ASC->AbilityInputTagReleased(CCLTags::Input_Attack);
    return ASC->HasMatchingGameplayTag(CCLTags::State_Busy);
}
```

이 구조는 하나의 공격 정의를 사용하는 적에는 단순하다. 다만 입력 연결 위치가 플레이어와 적에 따로 생기고, 성향에 따른 교전 판단을 연결할 곳도 필요해졌다.

## 5. 패턴과 개인 상태의 추가

ContentFoundation은 일반 적 유형과 보스 체력별 공격 정의 선택을 기록한다. 결정 18은 경험·상태가 다음 선택에 영향을 주도록 요구한다. 현재 적은 생활 계정 없이 선택적 Agent Feature를 사용한다.

이는 모든 적에게 상점·부채·생활 일정을 붙였다는 뜻이 아니다. 교전 판단에 필요한 상태만 연결하며, 구체적인 목표·점수는 [Agent 판단](AgentDecision.md)을 참고한다.

## 6. 현재의 공격 시작

출처: `50e5838`, [CCLEnemyAIController.cpp](../../Source/CCL/Combat/CCLEnemyAIController.cpp):142의 `ACCLEnemyAIController::StartAttack`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool ACCLEnemyAIController::StartAttack()
{
    auto* Enemy = Cast<ACCLEnemyCharacter>(GetPawn());
    auto* ASC = Enemy ? Cast<UCCLAbilitySystemComponent>(Enemy->GetAbilitySystemComponent()) : nullptr;

    if (!ASC || !HasTarget() || IsActionRunning())
    {
        return false;
    }

    if (const auto* Agent = Enemy->FindComponentByClass<UCCLAgentComponent>(); Agent && !Agent->ShouldEngage())
    {
        ReconsiderAfter = GetWorld()->GetTimeSeconds() + 4;
        Target.Reset();
        return false;
    }
    StopMovement();
    Enemy->SelectAttackPattern();
    auto* Actions = Enemy->FindComponentByClass<UCCLActionComponent>();
    if (!Actions)
    {
        return false;
    }
    Actions->RequestInput(CCLTags::Input_Attack, true);
    Actions->RequestInput(CCLTags::Input_Attack, false);
    return ASC->HasMatchingGameplayTag(CCLTags::State_Busy);
}
```

먼저 기존 유효성 검사를 유지한다. Agent가 교전을 거부하면 목표를 비우고 재고려 시각을 늦춘다. 허용됐을 때만 이동을 멈추고 공격 패턴을 확정한 뒤 ActionComponent에 입력을 전달한다.

마지막 Busy 태그 검사는 요청을 보냈다는 사실과 실제 행동 시작을 구분한다. ActionComponent가 없으면 실패하며 공격했다고 기록하지 않는다.

## 7. 실행 중 상태와 콘텐츠 정의

`FCCLEnemyTask::Tick`의 Attack 분기는 아직 시작하지 않았을 때만 `StartAttack`을 호출한다. 시작한 뒤 Busy·Stagger 상태가 끝났을 때 성공한다. 대상이 사라지면 실패한다.

`ACCLEnemyCharacter::SelectAttackPattern`은 일반 적·보스에 맞는 공격 정의를 선택한다. 보스의 체력 구간에서 정의를 교체하되 이미 공유 중인 DataAsset의 피해·시간 값을 고치지 않는다. 실제 실행 시간과 적중은 GASCombat의 경로를 따른다.

탐색은 적의 원위치를 기준으로 감지 범위를 계산하고 `HasTarget`은 추적 한계를 확인한다. 매번 자기 현재 위치에서만 탐지 반경을 넓히면 원위치에서 너무 멀리 쫓아갈 수 있다는 점을 비교해서 읽으면 좋다. 이는 코드의 거리 기준에 대한 분석이다.

## 8. 권한과 수명

AIController의 의사 결정과 공격은 서버 경로다. 클라이언트는 복제된 적 상태와 애니메이션을 표시한다. 공격 피해의 최종 검증은 StateTree가 아닌 공통 적중 처리에 있다.

Controller가 Pawn을 잃으면 `OnUnPossess`에서 트리를 멈추고 약한 목표 참조를 비운다. 새 적이 이전 목표를 무조건 이어받지 않는다. 재생성 정책은 적의 설정과 캠페인 Director가 결정한다.

## 9. 실패와 복귀

목표 사망·접속 종료·추적 범위 이탈은 `HasTarget` 실패로 연결된다. 접근 중 행동이 시작되면 이동을 멈춘다. 복귀는 초점을 지우고 원위치로 이동하며 새 목표를 다시 찾는다.

CampaignFoundation에는 StateTree가 실행돼도 내비게이션 볼륨의 크기가 0이라 이동하지 않았던 실제 결함 기록이 있다. 그 기록을 ‘StateTree가 잘못 선택했다’로 설명하면 원인을 놓친다. 실행 상태, 내비게이션 투영, 실제 이동을 따로 확인해야 한다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| Controller Tick의 직접 분기 | 작은 적의 흐름이 한 파일에 보임 | 전이·대기·취소 조건이 얽히기 쉬움 |
| StateTree + Controller 실행 함수 | 상태와 실제 이동/GAS 호출을 구분 | 트리 자산·Task 데이터·코드를 함께 확인 |
| 판단과 실행을 한 점수 함수에 통합 | 경로가 짧음 | 실행 실패·취소와 재평가를 따로 설명하기 어려움 |

현재 구조도 경로 실패와 공격 시작 실패가 어떤 전이로 이어지는지 자산까지 확인해야 한다. 클래스 분리만으로 자연스러운 전투가 보장되지는 않는다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 적 감지 범위에 진입 | 목표 획득, 실제 내비게이션 이동 |
| 준비·회복 중 | 중복 공격 요청 방지 |
| 보스 체력 구간 변경 | 다음 공격에 패턴 선택 반영 |
| 목표 사망·추적 한계 이탈 | 목표 상실과 복귀 |
| 피격 후 교전 판단 변경 | Agent 판단에 따른 교전 거부 가능 |

기존 근거는 CombatFoundation, ContentFoundation과 [AgentFoundation](../AgentFoundation.md)의 전투·실제 마을 검사에 있다. 이번 작성에서는 소스와 기록을 읽었으며 AI 플레이를 새로 실행하지 않았다.

## 12. 이해 확인

**공격 Task가 시작 직후 성공을 반환하면 어떤 문제가 생길까?**

트리가 회복이 끝나기 전에 다음 상태로 넘어갈 수 있다. 현재는 시작 사실을 기억하고 전투 상태가 끝날 때까지 기다린다.

**StateTree가 Running이면 이동도 정상일까?**

트리 실행과 내비게이션 이동은 별도 계약이다. 경로 투영과 실제 위치 변화를 확인해야 한다.
