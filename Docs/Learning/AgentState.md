# Agent 지속 상태: ID, 세대와 실행권으로 쓰기를 제한하기

Agent의 삶은 Actor가 아닌 Record에 남는다. Actor·간략 실행기·Mass가 같은 Record를 다룰 때 Store가 세대와 실행권을 검증한다. `Source/CCL/Agents/CCLAgentTypes.cpp`와 `CCLAgentSnapshot.cpp`에서 교체·저장·실패 때 원본을 지키는 방식을 이해한다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 도입 기준 | `68672713e47b19b0d4adb51bdc21c7c9bbe78eb6` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

`6867271`이 선택적 Feature·Store·스냅샷의 최초 구현이다. 이전에는 이 지속 Agent 기반이 없었다. 이후 생활 실행과 Mass 연결이 추가됐으며 공통 쓰기 계약은 유지됐다. 정본은 결정 18과 [AgentFoundation](../AgentFoundation.md)이다.

## 2. Actor가 사라진 뒤의 개인

주민 Actor가 언로드돼도 부채·기억·목표가 사라져서는 안 된다. 동시에 화면에 있는 Actor와 먼 지역 간략 실행기가 같은 시간을 두 번 진행하면 안 된다.

따라서 개인의 정체성과 현재 실행 객체를 나누고, 현재 상태를 바꿀 권한을 하나만 발급해야 한다. 이 실행권은 디스크 잠금이나 멀티스레드 mutex와는 다르다.

## 3. Definition, Record와 Feature Registry

Definition은 불변 설정, Record는 개인의 지속 값, Processor는 실행 코드다. Feature는 `FInstancedStruct`와 버전으로 저장하며 Registry가 태그에 맞는 구조체 타입·의존성·검증·변환 함수를 등록한다.

`FCCLAgentHandle`의 ID는 개인을, Generation은 현재 Store 항목 세대를 식별한다. `FCCLAgentLease`는 Handle, Writer와 Epoch를 묶는다. 한 번 반납한 Writer가 오래된 Lease를 다시 사용하지 못하도록 Epoch를 바꾼다.

FInstancedStruct는 [엔진 근거](EngineEvidence.md)의 UE5 타입을 사용한다. 세대·Lease 정책은 엔진 자동 기능이 아닌 프로젝트의 값 저장 계약이다.

## 4. 최초 실행권 발급

출처: `6867271`, [CCLAgentTypes.cpp](../../Source/CCL/Agents/CCLAgentTypes.cpp):86의 `FCCLAgentStore::Acquire`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
FCCLAgentLease FCCLAgentStore::Acquire(FCCLAgentHandle Handle, FGuid Writer)
{
    check(IsInGameThread());
    auto* Entry = Records.Find(Handle.Id);
    if (!Entry || Entry->Generation != Handle.Generation || Entry->Writer.IsValid() || !Writer.IsValid())
    {
        return {};
    }

    Entry->Writer = Writer;
    Entry->Epoch = NextEpoch++;
    return {Handle, Writer, Entry->Epoch};
}
```

이미 Writer가 있으면 발급하지 않는다. 게임 스레드 검사를 둔 직렬 쓰기 모델이다. 이를 여러 스레드가 동시에 접근해도 안전한 저장소라고 설명하면 안 된다.

## 5. 확장 요구와 유지한 원칙

결정 18은 Actor가 없어도 상태와 시간이 남고 쓰기 주체가 하나여야 한다고 정했다. 생활 거래가 상대방 경험까지 바꾸면서 여러 Record를 함께 검증할 필요가 생겼다.

현재 `CommitBatch`는 관련 후보를 모두 확인한 뒤 게시한다. 이는 경제·개인 상태가 엇갈리는 부분 적용을 막기 위한 구현이며 자세한 연결은 [생활과 경제](LifeEconomy.md)에 있다.

## 6. 현재 Commit과 일괄 검사

출처: `50e5838`, [CCLAgentTypes.cpp](../../Source/CCL/Agents/CCLAgentTypes.cpp):100의 `FCCLAgentStore::Commit`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool FCCLAgentStore::Commit(const FCCLAgentLease& Lease, FCCLAgentRecord Record,
    const FCCLFeatureRegistry& Registry, FString& Error)
{
    check(IsInGameThread());
    auto* Entry = Records.Find(Lease.Handle.Id);
    if (!Entry || !Lease.Writer.IsValid() || Entry->Generation != Lease.Handle.Generation ||
        Entry->Writer != Lease.Writer || Entry->Epoch != Lease.Epoch || Record.Id != Lease.Handle.Id ||
        Record.LastSimulatedTime < Entry->Record.LastSimulatedTime || !Registry.UpgradeAndValidate(Record, Error))
    {
        return false;
    }

    Entry->Record = MoveTemp(Record);
    return true;
}
```

Writer와 세대가 맞아도 Epoch가 다르면 실패한다. 시간 역행과 Record ID 바꿔치기도 거부한다. Registry가 후보 Feature를 변환·검증한 후에만 원본을 교체한다.

출처: `50e5838`, [CCLAgentTypes.cpp](../../Source/CCL/Agents/CCLAgentTypes.cpp):116의 `FCCLAgentStore::CommitBatch`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool FCCLAgentStore::CommitBatch(TArray<TPair<FCCLAgentLease, FCCLAgentRecord>> Updates,
    const FCCLFeatureRegistry& Registry, FString& Error)
{
    check(IsInGameThread());
    TSet<FGuid> Seen;
    for (auto& Update : Updates)
    {
        const auto& Lease = Update.Key;
        auto& Record = Update.Value;
        const auto* Entry = Records.Find(Lease.Handle.Id);
        if (!Entry || Seen.Contains(Record.Id) || !Lease.Writer.IsValid() || Entry->Generation != Lease.Handle.Generation ||
            Entry->Writer != Lease.Writer || Entry->Epoch != Lease.Epoch || Record.Id != Lease.Handle.Id ||
            Record.LastSimulatedTime < Entry->Record.LastSimulatedTime || !Registry.UpgradeAndValidate(Record, Error))
        {
            return false;
        }

        Seen.Add(Record.Id);
    }

    for (auto& Update : Updates)
    {
        Records[Update.Key.Handle.Id].Record = MoveTemp(Update.Value);
    }

    return true;
}
```

검증 루프와 게시 루프를 나눴다. 첫 후보가 맞는다고 즉시 원본에 써 버리면 뒤 후보가 실패했을 때 부분 변경이 남기 때문이다.

## 7. 저장과 복원의 내부 경로

`Store.Snapshot`은 활성 Writer가 하나라도 있으면 실패하고, 결과를 ID 순서로 정렬한다. 호출자가 실행을 정지·수집해 쓰기 권한을 반납해야 한다.

`UCCLAgentSnapshot::Encode`는 Store 값을 SaveGame 메모리로 직렬화하고 CRC를 붙인다. Decode는 크기·CRC·버전·개수 검사 뒤 `Store.Replace`로 전달한다. Replace는 기존 Writer가 없는지와 모든 후보를 검사한 뒤 새 세대를 발급한다.

생활 전체 저장은 Agents만 저장하는 이 기초 스냅샷과 구분한다. `FCCLLifeSimulation`은 경제·기회·사건 등 교차 참조를 포함해 별도로 검증한다.

## 8. 소유와 실행 범위

Store는 값 상태를 소유하고 Actor·Mass는 이를 참조하는 실행 표현이다. 조회 포인터를 장기 보관하는 대신 ID와 현재 Handle을 다시 얻어 세대를 확인한다.

서버 생활 Subsystem이 실제 월드 시뮬레이션을 진행한다. 클라이언트에 전체 기억·계정·기회를 복제하지 않는다. 이 구조 자체가 임의 클라이언트 입력을 안전하게 검증해 주는 것은 아니므로 진입 계층의 권한 검사가 필요하다.

## 9. 실패와 교체

미래 Feature 버전, 알려지지 않은 태그, 다른 구조체 타입과 누락된 의존성은 조용히 기본값으로 바꾸지 않고 실패한다. 변환도 후보 복사본에서 수행한다.

오래된 Handle·반납된 Lease는 Commit할 수 없다. 스냅샷 검증 실패 시 기존 Store를 유지한다. 로컬 SaveGame 역직렬화는 신뢰할 수 없는 외부 파일을 받는 보안 경계로 사용하지 않는다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| Actor 멤버만 저장 | 한 객체에서 이해하기 쉬움 | 언로드·간략 실행과 수명 충돌 |
| ID만 사용 | 참조 형식이 단순 | 교체 전 콜백을 구분하기 어려움 |
| ID + Generation + Lease | 오래된 참조·중복 쓰기 거부 | 실행권 반환·실패 복구 계약 필요 |

값 복사와 사전 검증은 비용이 든다. 현재의 게임 스레드 모델을 대규모 병렬 시뮬레이션으로 확장하려면 별도 동기화 설계와 측정이 필요하다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 같은 Record에 두 Writer | 두 번째 Acquire 거부 |
| Release 뒤 같은 Lease로 Commit | 거부 |
| Replace 뒤 옛 Handle 조회 | 무효 |
| Feature 미래 버전·잘못된 타입 | 원본 보존하며 실패 |
| 활성 Writer가 있는 Snapshot | 실패 |
| Batch 뒤쪽 후보 오류 | 앞 후보도 게시하지 않음 |

기존 AgentFoundationTests·LifeSimulationTests·MassAgentTests의 결과는 AgentFoundation에 기록돼 있다. 이번에는 코드·기록을 대조했고 자동화 실행은 새로 하지 않았다.

## 12. 이해 확인

**ID가 같으면 Handle도 계속 유효해야 할까?**

같은 개인이 새 Store 상태로 교체됐을 수 있다. Generation을 확인해 교체 이전 실행 결과가 새 상태를 덮지 않게 한다.

**저장 직전에 Writer를 무조건 지우면 될까?**

실행기에 아직 반영하지 않은 결과가 있을 수 있다. 먼저 실행을 정지하고 결과를 Commit한 뒤 Lease를 반환해야 한다.
