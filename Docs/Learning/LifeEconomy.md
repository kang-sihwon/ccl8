# 생활·경제 시뮬레이션: 선택을 실제 자원과 다음 경험으로 연결하기

주민이 식사를 선택해도 돈이나 음식이 없으면 식사는 실패해야 한다. 이 장은 `FCCLLifeSimulation`과 `CCLEconomy`가 기회, 거래, 경험, 저장을 연결하는 방법을 설명한다. [Agent 판단](AgentDecision.md)을 먼저 읽으면 선택과 실행의 경계를 이해하기 쉽다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `68672713e47b19b0d4adb51bdc21c7c9bbe78eb6` |
| 도입·변경 기준 | `aea2debe066b2d95fd41cb9f83100e551a0bbadb` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전 기준에는 Agent 기록·Feature·스냅샷이 있었지만 `CCLLifeSimulation.cpp`와 `CCLEconomy.cpp`는 없었다. 따라서 없는 이전 거래 코드를 만들어 비교하지 않는다. 요구와 검증 기록은 [AgentFoundation](../AgentFoundation.md), 결정 18을 따른다.

## 2. 해결할 문제

상인이 가진 음식 한 개를 두 주민이 동시에 얻거나, 저장 후 같은 구매 요청을 다시 처리해 두 번 돈을 빼면 삶의 기록을 신뢰할 수 없다. 화면의 성공 문구와 실제 계좌·물품·욕구가 함께 변해야 한다.

장기 목표도 실제 거래에 연결돼야 한다. ‘빚을 갚고 싶다’는 성향과 실제 상환액은 다른 값이다. 현재 구조는 판단, 거래 검증, 상태 확정, 결과 경험을 나눠 처리한다.

## 3. 기회, 거래와 멱등성

Opportunity는 활동 장소·종류·조건·Revision을 가진 실행 기회다. Revision은 판단 당시의 기회와 실행 당시의 기회가 같은 조건인지 확인한다.

RequestId는 요청 한 번을 식별한다. 같은 ID와 같은 조건의 재시도는 기존 결과를 반환한다. 같은 ID에 다른 조건을 담으면 거부한다. 이를 멱등 처리라고 하며, 네트워크 전송 자체의 신뢰성과는 별개다.

경제의 Account는 돈, Inventory는 자원 수량·용량, Obligation은 부채, Ownership은 시설 소유 상태를 나타낸다. 아이템 화면의 GUID 기반 인벤토리와 경제 자원 원장은 같은 컨테이너가 아니다. [상점](VillageServices.md)의 연결부가 두 상태의 변경을 조정한다.

`USaveGame`과 `UGameplayStatics::SaveGameToMemory`는 UObject 저장 자료를 바이트로 만드는 UE 기능이다. 저장 전 일관된 상태를 모으는 일은 프로젝트 책임이며 UE4에도 필요했던 문제다.

## 4. 이전의 기록 저장

생활 시뮬레이션 이전에는 Agent 자체의 기록을 저장할 기반이 있었다. 그 구조는 [Agent 상태](AgentState.md)의 실제 전후 코드에서 확인한다. 이후 경제·세계 기회·사건이 생기면 Agent 배열만으로 같은 세계를 복원할 수 없다.

아래는 생활 시뮬레이션 도입 시 저장할 상태를 모으는 함수다.

출처: `aea2deb`, [CCLLifeSimulation.cpp](../../Source/CCL/Agents/CCLLifeSimulation.cpp):307의 `FCCLLifeSimulation::Capture`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool FCCLLifeSimulation::Capture(FCCLSimulationSnapshot& Snapshot) const
{
    FCCLSimulationSnapshot Candidate;
    if (!Agents.Snapshot(Candidate.Agents))
    {
        return false;
    }

    Candidate.Seed = Seed;
    Candidate.Time = Time;
    Candidate.Sequence = Sequence;
    Candidate.Economy = Economy;
    Candidate.Opportunities = Opportunities;
    Candidate.Events = Events;
    Candidate.Results = Results;
    Snapshot = MoveTemp(Candidate);
    return true;
}
```

먼저 Agent Snapshot이 성공해야 한다. 성공한 후보에 난수 시드, 시각, ID 생성 순번, 경제, 기회, 사건, 실행 결과를 함께 넣는다. 호출자가 넘긴 Snapshot은 후보 준비가 끝난 뒤 교체한다.

## 5. 경제를 별도 값 상태로 둔 이유

결정 18의 요구는 자원·기억·관계가 다음 행동의 원인이 되는 삶이다. 코드 구조에서 분석하면, 경제 후보를 먼저 계산하면 Actor 이동 실패와 자원 확정을 분리해 검사할 수 있다.

모든 활동을 단일 경제 거래로 설명할 수 있는 것은 아니다. 활동 Processor의 결과와 Agent Feature 검증도 필요하다. 활동을 추가할 때 성공 표시만 넣고 실제 자원·경험 변화를 빠뜨리면 이 계약을 만족하지 못한다.

## 6. 현재 실행의 확정 경계

`Execute`는 기존 RequestId 결과를 확인하고 Agent·기회·Revision·예약, 활동 Processor와 적용 결과를 검사한다. 아래는 경제 후보에 거래를 적용하는 부분이다.

출처: `50e5838`, [CCLLifeSimulation.cpp](../../Source/CCL/Agents/CCLLifeSimulation.cpp):767의 `FCCLLifeSimulation::Execute`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
    FCCLEconomyState Candidate;
    Candidate.Accounts = Economy.Accounts;
    Candidate.Inventories = Economy.Inventories;
    Candidate.Obligations = Economy.Obligations;
    Candidate.Ownerships = Economy.Ownerships;
    const auto Transaction = CCLEconomy::Execute(Candidate, Request);
    if (!Transaction.bSucceeded)
    {
        FCCLAgentRecord Failed = *Agent;
        if (auto* Experience = MutableFeature<FCCLAgentExperience>(Failed, CCLAgentTags::Feature_Experience))
        {
            FCCLObservation Observation;
            Observation.EventId = RequestId;
            Observation.EvidenceId = RequestId;
            Observation.Time = Time;
            Observation.EventType = CCLAgentTags::Failure;
            Observation.PerceivedSubject.Kind = O->ProviderId == AgentId ? CCLAgentTags::Unknown : CCLAgentTags::Agent;
            Observation.PerceivedSubject.Id = O->ProviderId == AgentId ? FGuid() : O->ProviderId;
            CCLAgentFeatures::Observe(*Experience, Observation);
        }

        const auto FailureLease = Agents.Acquire(Agents.GetHandle(AgentId), RequestId);
        Agents.Commit(FailureLease, Failed, Registry, Error);
        Agents.Release(FailureLease);
        return Finish(Transaction.Failure);
    }
```

실패하면 성공 보상을 게시하지 않고 실패 경험을 남기는 경로로 간다. 성공하면 Agent 실행권을 얻어 자신과 필요한 상대 기록의 변경을 `CommitBatch`로 확인한다. Agent 확정이 실패하면 경제 후보를 본 상태로 옮기지 않는다.

Agent 확정까지 성공한 뒤 후보의 계좌·재고·부채·시설을 게시하고 거래 결과를 기록한다. 이 순서를 ‘게임 전체가 항상 원자적으로 실행된다’는 보장으로 넓히면 안 된다. 여기서 다루는 경계는 해당 실행의 후보와 게시 순서다.

## 7. 거래 안쪽과 목표 갱신

`CCLEconomy::Execute`는 기존 영수증과 요청 조건을 대조한 뒤 잔액·가격·자원 수량·용량·상환 조건을 검사한다. 변경할 계좌와 재고를 임시 값에 계산하고 검증을 통과하면 게시한다. 실패 영수증도 Journal에 남을 수 있으므로 ‘실패하면 모든 바이트가 동일하다’고 설명하지 않는다.

`CCLGoalPolicy`는 실제 잔액, 상환 기록, 도움으로 전달한 자원, 시설 수준과 숙련에서 목표 진행을 계산한다. 의도 선택만으로 목표 달성량이 늘지 않는다.

`AdvanceTo`는 목표 시각까지 시간 경계와 사건 시각을 나눠 진행한다. 욕구·목표를 갱신하고, Actor가 직접 실행 중인 Agent를 제외한 대상에 축약 실행을 적용한다. 상대에게 생긴 경험을 다음 순회가 덮지 않도록 최신 Agent 기록을 다시 읽는다.

## 8. 소유권과 시간

`UCCLAgentWorldSubsystem`이 서버 월드에서 Simulation을 소유한다. 게임 화면은 원장을 직접 수정하지 않고 검증된 활동·거래 경로를 사용한다.

현재 `AdvanceTo`는 `void`이며 중간 실패나 부분 진행 가능성이 있다. 호출했다는 이유만으로 요청 시각까지 완료됐다고 판단할 수 없다. [환경 계획](../EnvironmentPlan.md)은 이 경계에 완료 시각·오류 계약을 추가할 것을 요구한다.

시간 배율은 [Actor 실행](AgentExecution.md)에서 설명한다. 돈 계산과 물리 이동의 시간 단위를 하나로 생각하지 않는다.

## 9. 저장·실패·복구

현재 Load는 최소 길이와 64MiB 상한, CRC를 확인하고 SaveGame을 읽은 뒤 `Initialize`의 상태 검증으로 넘긴다. CRC는 손상 탐지이며 외부의 악의적인 변경을 인증하는 서명은 아니다.

출처: `50e5838`, [CCLLifeSimulation.cpp](../../Source/CCL/Agents/CCLLifeSimulation.cpp):382의 `FCCLLifeSimulation::Load`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool FCCLLifeSimulation::Load(const TArray<uint8>& Bytes, FString& Error)
{
    if (Bytes.Num() < 32 || Bytes.Num() > 64 * 1024 * 1024)
    {
        return false;
    }

    const int32 Size = Bytes.Num() - sizeof(uint32);
    uint32 CRC;
    FMemory::Memcpy(&CRC, Bytes.GetData() + Size, sizeof(CRC));
    if (CRC != FCrc::MemCrc32(Bytes.GetData(), Size))
    {
        return false;
    }

    TArray<uint8> Payload;
    Payload.Append(Bytes.GetData(), Size);
    const auto* Save = Cast<UCCLLifeSave>(UGameplayStatics::LoadGameFromMemory(Payload));
    return Save && Initialize(Save->State, Error);
}
```

열린 쓰기 실행권이 있으면 Capture의 Agent Snapshot이 실패할 수 있다. 저장 버튼이 눌렸다는 사실과 일관된 저장이 완료됐다는 사실을 구분해야 한다. 맵 전환에서 Simulation을 보존하는 경로는 [세션 저장](SessionSave.md)과 AgentExecution을 함께 읽는다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 활동마다 즉시 값 수정 | 작은 프로토타입에서 단순함 | 중간 실패·중복 처리의 복구가 흩어짐 |
| 후보 복사 후 검증·확정 | 실패 전에 불변 조건 확인 가능 | 복사·검증 비용, 확정 경계 관리 |
| 전체 이벤트 재생 기반 | 이력 추적·재구성에 유리 | 이벤트 버전·재생·압축 설계 필요 |

현재 Journal과 Results가 있다고 전체 시스템이 이벤트 소싱인 것은 아니다. 현재 값과 실행 기록을 함께 저장하는 구현으로 읽는다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 같은 RequestId·같은 거래 재전달 | 잔액·물품 중복 변경 없음 |
| 같은 ID·다른 거래 | 기존 결과를 새 조건에 재사용하지 않음 |
| 잔액·재고 부족 또는 용량 초과 | 자원 이전과 성공 보상 없음 |
| 실패 경험 이후 재판단 | 실패 정보가 후보 평가에 반영 |
| 저장 후 복원 | 시각·순번·원장·기회·결과가 일관됨 |
| 실행권을 쥔 채 저장 | 미완성 상태를 Snapshot으로 게시하지 않음 |

관련 코드는 [CCLLifeSimulationTests.cpp](../../Source/CCL/Tests/CCLLifeSimulationTests.cpp)다. 이번 작성에서는 코드와 기존 검증 기록을 확인했으며 테스트나 장기 시뮬레이션을 새로 실행하지 않았다.

## 12. 이해 확인

**최고 점수로 선택한 식사에 즉시 허기 회복을 적용하면 무엇이 잘못될까?**

이동·예약·재고·지불이 실패해도 보상을 얻게 된다. 성공 조건을 실행에서 확인하고 그 결과를 경험과 욕구에 반영해야 한다.

**CRC가 일치하면 저장을 그대로 신뢰해도 될까?**

아니다. 자료형·ID·수량·참조 관계 등 의미 검증이 별도로 필요하며 CRC 자체는 변경 주체를 인증하지 않는다.
