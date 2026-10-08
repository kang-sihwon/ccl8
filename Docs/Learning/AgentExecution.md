# Agent 실행: 같은 삶을 Actor·StateTree·Mass에 연결하기

Agent의 영구 기록과 화면 속 Actor는 수명이 다르다. 이 장은 생활 시뮬레이션을 월드에 연결하고, 선택한 활동을 StateTree로 수행하며, Mass 표현을 떠날 때 결과를 돌려주는 과정을 설명한다. [Agent 상태](AgentState.md), [판단](AgentDecision.md), [생활·경제](LifeEconomy.md)를 먼저 읽는다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `aea2debe066b2d95fd41cb9f83100e551a0bbadb` |
| 도입·변경 기준 | `0931fbfb5c96c4da24acb9d2ed44243fc0e6171f` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

`aea2deb`의 생활 Actor 실행과 `0931fbf`에서 추가한 Mass 연결을 비교한다. Mass 이전에 별도의 가상 Mass 코드를 두지 않는다. 계약과 기록은 [AgentFoundation](../AgentFoundation.md)과 [직접 확인 안내](../AgentReview-2026-10-08.md)에 있다.

## 2. 해결할 문제

멀리 있는 상인도 돈과 기억을 유지해야 하지만 모든 개인을 항상 Actor로 움직이면 비용이 커진다. 가까워져 Actor로 나타난 상인을 축약 시뮬레이션도 동시에 실행하면 거래·시간이 중복 적용된다.

따라서 개인의 ID와 기록을 유지하고, 지금 어느 실행기가 그 개인을 처리하는지 구분한다. 현재 구현은 이 경계의 기반이며 모든 주민을 자동으로 대규모 Actor/Mass LOD 전환하는 완성 시스템은 아니다.

## 3. 세 가지 서로 다른 장치

`UWorldSubsystem`은 World 수명에 연결된 서비스다. 맵이 바뀌어 World가 교체되면 GameInstance에 보관한 세션 자료와 다시 연결해야 한다.

StateTree는 상태·전이·Task로 실행 흐름을 구성한다. 프로젝트의 판단 커널이 무엇을 할지 고르면 StateTree Task가 이동과 수행을 진행한다. 전투용 StateTree와 별도로 생활 활동의 실행을 구성한다.

Mass Entity는 Actor가 아닌 데이터 중심 표현이다. Fragment는 Entity에 붙는 값 구조체이며 `FMassEntityManager`가 생성·접근·삭제를 맡는다. 이 프로젝트의 Lease는 엔진 Mass 기능이 아니라 Agent Store의 독점 쓰기 계약이다. StateTree·Mass를 사용하는 UE5 구조를 UE4의 모든 AI가 자동 대체된 것으로 설명하지 않는다.

## 4. 이전의 Actor 실행

생활 AIController는 StateTree Component의 자동 실행을 끄고 Possess에서 시작한다. Intent를 선택할 때 기회를 예약하고, 이동 후 도착 거리·예약·활동 조건을 확인한다.

다음은 Actor 수명이 끝나는 정리 경로다.

출처: `aea2deb`, [CCLAgentAIController.cpp](../../Source/CCL/Agents/CCLAgentAIController.cpp):29의 `ACCLAgentAIController::OnUnPossess`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void ACCLAgentAIController::OnUnPossess()
{
    StateTree->StopLogic(TEXT("Agent unloaded"));
    StopMovement();
    OpportunityId.Invalidate();
    Super::OnUnPossess();
}
```

이동만 멈추면 예약이 남을 수 있다. `AbortIntent`로 실행 의도를 정리하고 StateTree 로직과 이동을 멈춘 뒤 Controller의 소유를 해제한다.

## 5. 표현과 삶을 분리한 이유

결정 18은 개인의 삶과 Actor/Mass 표현의 분리를 요구했다. 코드에서 분석하면, 값 기록을 정본으로 두면 화면에 없던 동안의 자원·기억을 새 Actor에 연결할 수 있다.

반대로 포인터를 개인의 ID로 쓰면 Actor 파괴 후 같은 사람임을 판단하기 어렵다. 현재 구현은 영구 ID와 기록 Handle·Lease, 표현 객체를 구분한다.

## 6. 현재 Mass 진입과 반환

Mass 진입은 Store의 Agent를 찾고 쓰기 실행권을 얻은 뒤 Entity를 만든다.

출처: `50e5838`, [CCLMassAgentBridge.cpp](../../Source/CCL/Agents/CCLMassAgentBridge.cpp):5의 `CCLMassAgentBridge::Enter`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
FMassEntityHandle CCLMassAgentBridge::Enter(FMassEntityManager& Manager, FCCLAgentStore& Store, FGuid Id)
{
    const auto Handle = Store.GetHandle(Id);
    const auto* Record = Store.Find(Handle);
    if (!Record)
    {
        return {};
    }
    const auto Lease = Store.Acquire(Handle, FGuid::NewGuid());
    if (!Lease.Writer.IsValid())
    {
        return {};
    }
    const TArray<const UScriptStruct*> Types = {FCCLMassAgentFragment::StaticStruct()};
    const auto Entity = Manager.CreateEntity(Manager.CreateArchetype(Types));
    auto& Fragment = Manager.GetFragmentDataChecked<FCCLMassAgentFragment>(Entity);
    Fragment.Intent = Record->Intent;
    Fragment.Lease = Lease;
    return Entity;
}
```

Fragment는 Intent와 Lease를 보관한다. 판단은 같은 `CCLDecision::Evaluate`를 호출한다. 현재 Fragment가 모든 Feature·물리·GAS 상태를 복제하는 것은 아니다.

반환할 때는 최신 기록에 Fragment의 Intent를 넣어 Store에 Commit한다.

출처: `50e5838`, [CCLMassAgentBridge.cpp](../../Source/CCL/Agents/CCLMassAgentBridge.cpp):40의 `CCLMassAgentBridge::Leave`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool CCLMassAgentBridge::Leave(FMassEntityManager& Manager, FMassEntityHandle Entity, FCCLAgentStore& Store,
    const FCCLFeatureRegistry& Registry, FString& Error)
{
    if (!Manager.IsEntityValid(Entity))
    {
        return false;
    }
    const auto& Fragment = Manager.GetFragmentDataChecked<FCCLMassAgentFragment>(Entity);
    const auto* Current = Store.Find(Fragment.Lease.Handle);
    if (!Current)
    {
        return false;
    }
    auto Updated = *Current;
    Updated.Intent = Fragment.Intent;
    if (!Store.Commit(Fragment.Lease, MoveTemp(Updated), Registry, Error))
    {
        // Retain the Entity and its pending result for explicit recovery.
        return false;
    }
    Store.Release(Fragment.Lease);
    Manager.DestroyEntity(Entity);
    return true;
}
```

Commit에 실패하면 Entity를 먼저 없애지 않는다. 보류 결과를 유지해 명시적 복구가 가능하게 한다. 성공한 뒤 Lease를 놓고 Entity를 삭제한다.

## 7. 월드에서 실제 수행까지

`UCCLAgentWorldSubsystem::OnWorldBeginPlay`는 게임 월드의 캠페인 권한 측에서 세션 ID와 저장 자료를 확인한다. 시나리오 초기화나 Load가 성공한 뒤 주민·작업장을 배치한다.

현재 Tick은 누적 게임 시간이 0.25초 이상이면 60배의 시간을 Simulation에 넘긴다. 60배는 현재 코드의 상수이며 최종 게임 배율의 설계 근거가 확인된 수치는 아니다.

Actor 수행은 SelectIntent, Approach, Perform 순으로 이어진다. 이동은 30초 제한과 거리 검사, 수행은 도착 거리·예약·기회 Revision을 확인한다. 수행 중 주기적으로 재판단하고 급한 허기 때문에 다른 기회로 전환할 수 있다. 실제 거래는 Simulation의 Execute가 맡는다.

## 8. 권한과 중복 실행 방지

서버 Simulation이 삶의 정본이다. 주민의 표시 이름·현재 활동·위치처럼 필요한 표현만 Actor 경로와 연결한다. 개인의 모든 기억을 클라이언트에 공개하지 않는다.

ActiveActors에 등록된 개인은 시간 진행 중 축약 활동 실행에서 제외한다. Mass의 Lease와 Actor의 활동 예약은 역할이 다르다. Lease는 기록 쓰기 권한, 예약은 기회를 현재 누가 수행하는지 관리한다.

현재 연결부가 대규모 병렬 쓰기를 허용한다는 뜻은 아니다. Store의 게임 스레드 계약을 따라야 한다.

## 9. 취소·월드 종료·복원

AIController의 AbortIntent는 이동을 멈추고 기회 예약을 해제한다. 이동 실패, 시간 초과, 소유 해제에도 같은 경계가 필요하다.

WorldSubsystem은 저장 시 주민 위치를 Agent 기록에 반영한다. 월드 종료 시 같은 세션의 GameInstance 저장소에 Snapshot을 보관한다. Restore는 자료 검증 이후 생활 주민을 재배치하고 체력 상태를 복원한다.

Entity를 파괴했다고 Store의 Lease가 자동으로 해제된다고 가정하지 않는다. 반대로 Commit 실패 후 강제 Release·삭제하면 아직 게시하지 못한 결과를 잃을 수 있다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 모든 개인을 Actor로 유지 | 이동·상호작용 경로가 하나 | 대규모 비가시 개체 비용 |
| 멀리 있는 개인을 삭제하고 재생성 | 구현과 표시 관리가 단순함 | 개인의 삶·기억 연속성 상실 |
| 값 정본 + 표현 연결 | 삶을 유지하며 표현 변경 가능 | 전환·실행권·복구 계약 필요 |

현재 Mass 검사는 연결 계약의 근거다. 수만 개체의 프레임 예산이나 Actor와 동일한 전투 품질의 증거로 사용할 수 없다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| Actor가 활동 중 시간 진행 | 축약 실행의 중복 거래 없음 |
| 이동 실패·UnPossess | 예약 해제와 이동·로직 종료 |
| Mass 진입 중 다른 Writer 존재 | 추가 실행권 획득 실패 |
| Mass 반환 Commit 실패 | Entity와 보류 결과 유지 |
| 맵 전환·복원 | 같은 Agent ID와 삶의 상태 연결 |

관련 근거는 [CCLMassAgentTests.cpp](../../Source/CCL/Tests/CCLMassAgentTests.cpp), [Agent 월드 검사](../../Source/CCL/Tests/CCLAgentWorldSmokeSubsystem.cpp)다. 이번에는 소스와 이전 검사 기록을 대조했고 게임 실행·성능 측정은 하지 않았다.

## 12. 이해 확인

**활동 예약과 Store Lease를 하나의 bool로 합쳐도 될까?**

권한 범위가 다르다. 기회 점유와 기록 쓰기를 구분해야 이동 취소, 다른 기회 선택, Mass 반환 실패를 각각 처리할 수 있다.

**Mass가 Actor보다 항상 빠르다고 이 문서에서 결론 낼 수 있을까?**

없다. 현재 확인한 것은 Entity와 기록의 연결이다. 실제 개체 수, Processor 구성, 이동·표현 비용으로 성능을 측정해야 한다.
