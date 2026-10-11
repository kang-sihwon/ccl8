# 공통 시간: 생활과 환경을 같은 완료 시각으로 맞추기

시간 입력의 마지막 분할과 60배율의 반올림 문제는 [시간 분할 정밀도](EnvironmentTimeAccuracy.md)에서 실제 수정 전후로 이어 읽는다.

공통 실행기는 처리할 시간을 먼저 정하고 참여한 계산이 성공했을 때 완료 시각을 옮긴다. 이 장에서는 Agent의 독립 Tick이 왜 바뀌었는지, 후보에서 실행한 `Commit`이 왜 원본 세계를 바로 바꾸지 않는지 이해한다.

## 비교 기준과 문제

범위는 자연환경 1단계이다. 비교 기준은 `50e5838` → `fe5c143` → `64a7549` → `a817082`와 작업 트리이며 전체 해시·브랜치 관계·작업 트리 식별값은 [비교 근거](EnvironmentEvidence.md)에 있다. `WT-2026-10-10`은 문서 작성 중 읽은 미커밋 소스 사본을 뜻한다. 이후 수정된 HEAD와 동일하다고 가정하지 않는다.

주민은 물건을 거래하고 같은 세계에는 비가 내린다고 생각해 보자. 생활은 15세계 초를 처리했는데 물 계산이 실패하면 저장 파일 속 두 시스템이 서로 다른 시각을 가리킨다. 단순히 시계 숫자만 되돌리면 이미 완료한 거래를 다음 시도에서 또 실행할 수 있다. [결정 19](../DesignLog.md)는 공통 시간과 영구 상태를 함께 요구했고, [시간 트랜잭션 설계](../EnvironmentPlan.md#시간-트랜잭션-구현과-검증)는 이 부분 실패를 구현 문제로 기록했다.

`GameSeconds`는 게임 진행 시간, `WorldSeconds`는 배율을 적용한 세계 경과 시간이다. `Ticket`은 준비한 한 번의 진행을 식별하는 값이다. `MoveTemp`는 C++ 이동을 위한 UE 유틸리티이며 트랜잭션을 자동 제공하는 기능은 아니다. 원본 보존은 별도의 후보 객체를 만들고 실패 가능한 계산을 교체 전에 끝내는 코드 순서에서 나온다.

## 이전 구현의 실행 순서

출처: `50e5838`, [CCLAgentWorldSubsystem.cpp](../../Source/CCL/Agents/CCLAgentWorldSubsystem.cpp), `void UCCLAgentWorldSubsystem::Tick`, 66-78줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
void UCCLAgentWorldSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bRunning)
    {
        PendingSeconds += DeltaTime;
        if (PendingSeconds >= 0.25)
        {
            Simulation.AdvanceTo(Simulation.GetTime() + PendingSeconds * 60.);
            PendingSeconds = 0;
        }
    }
}
```

매 Tick의 `DeltaTime`을 모은 뒤 0.25초를 넘으면 60배를 생활 시뮬레이션에 넘겼다. 마지막 줄은 처리 요청 후 대기 시간을 0으로 만든다. 이 구조는 생활 하나를 구동할 때 읽기 쉽지만, 다른 소비자의 성공 여부를 확인하는 자리가 없다. 60은 당시 구현 상수였고 최종 게임 배율로 확정된 값은 아니다.

## 후보에서 완료를 연습하고 원본을 교체한다

첫 변경은 시계와 생활의 실패 경계를 맞췄다. 다음은 `fe5c143`의 연결 함수에서 입력 시각 일치 검사 뒤 실행하는 부분이다.

출처: `fe5c143`, [CCLWorldAdvance.cpp](../../Source/CCL/Environment/CCLWorldAdvance.cpp), `FCCLWorldClock CandidateClock`, 16-29줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
FCCLWorldClock CandidateClock = Clock;
FCCLWorldStep Step;
if (!CandidateClock.Prepare(MaxGameSeconds, MaxWorldSeconds, Step, Error) ||
    !CandidateClock.Commit(Step.Ticket, Error))
{
    return false;
}

if (!Life.TryAdvanceTo(Step.WorldToSeconds, Error, MaxLifeSlices))
{
    return false;
}

Clock = MoveTemp(CandidateClock);
```

1. `CandidateClock`은 원본의 복사본이다. Prepare가 처리할 구간과 티켓을 만든다.
2. 후보의 Commit이 티켓을 확인한다. 아직 원본 Clock은 그대로다.
3. `TryAdvanceTo`가 생활 후보를 계산·검증한다. 이 함수도 실패하면 기존 생활을 유지한다.
4. 생활이 성공하면 시계 후보를 원본에 옮긴다. 이 뒤에 실패할 다른 계산을 두지 않아야 한다.

이후 `a817082`에서는 같은 구간을 소비하는 표면 후보를 생활 앞에 추가했다.

출처: `a817082`, [CCLWorldAdvance.cpp](../../Source/CCL/Environment/CCLWorldAdvance.cpp), `FCCLSurfaceSimulation CandidateSurface`, 25-44줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
FCCLSurfaceSimulation CandidateSurface;
if (Surface)
{
    CandidateSurface = *Surface;
    if (!CandidateSurface.Advance(Step, Error))
    {
        return false;
    }
}

if (!Life.TryAdvanceTo(Step.WorldToSeconds, Error, MaxLifeSlices))
{
    return false;
}

if (Surface)
{
    *Surface = MoveTemp(CandidateSurface);
}
Clock = MoveTemp(CandidateClock);
```

현재 작업 트리에서는 표면 진행에 천체·관찰자·차폐·개구부 문맥도 전달한다. 날씨·불·실내 공기는 표면 후보 안에 있으므로 생활 실패 시 함께 폐기된다. 작업 트리에 추가된 Ecology와 Prepare 콜백은 후속 생태·저장 작업이다. 위 발췌는 그 코드가 없던 확정 커밋으로 7단계까지의 공통 원리를 설명한다. 전체 현재 함수는 [CCLWorldAdvance.cpp](../../Source/CCL/Environment/CCLWorldAdvance.cpp)에 있다.

## 배율과 대기 시간의 계산

설명용으로 게임 2초를 10배, 다음 3초를 20배로 받았다고 하자. 앞 구간의 세계 시간은 `2 × 10 = 20초`, 뒤 구간은 `3 × 20 = 60초`다. 합계는 80세계 초다. 마지막 배율 20을 전체 5초에 곱하면 100초가 되어 과거 구간의 의미가 바뀐다.

`FCCLWorldClock`은 대기 구간에 당시 배율을 남긴다. 처리 예산을 넘는 구간은 다음 시도까지 보존한다. 실패는 완료 시각의 진행을 막으며 오류와 대기량은 별도로 표시한다. 세계 배율 0에서도 게임 시간은 진행할 수 있다. 그래서 불·환기는 계속되고 계절·강수 진행은 멈출 수 있다. 새 게임 시간 입력 자체를 중단하는 일시 정지와 구분한다.

## 서버·저장·종료의 연결

`64a7549`는 독립 Agent Tick을 공통 World 실행기로 옮겼다. `UCCLWorldSimulationSubsystem`이 시계를 소유하고 `UCCLAgentWorldSubsystem`은 생활 정본을 유지한다. WorldSubsystem은 월드에 붙는 서비스라는 뜻이며, 두 Subsystem이 각각 같은 생활을 복사해 별도로 진행하지 않는다. 클라이언트는 복제된 완료 상태를 읽는다.

월드를 떠날 때는 완료된 생활·시계·환경을 같은 저장 묶음으로 캡처한다. 복원 시 세계 ID와 완료 시각도 검사한다. 시각이 같은 두 파일이 반드시 같은 세계에 속하는 것은 아니다. 저장·맵 전환과 부분 실패의 연결은 [저장과 실험장](EnvironmentPersistence.md)에서 이어진다.

후보 복사는 실패 처리를 설명하기 쉽지만 지역·개체가 많아질수록 복사량이 커진다. 변경 기록만 보관하는 대안은 복사를 줄이는 대신 모든 실패 경로의 되돌리기를 구현해야 한다. 현재 규모의 비용은 코드 구조만으로 단정할 수 없다. 이것은 프로젝트의 소유권 변경이며 UE4에서 UE5로 바뀌며 자동 도입된 계약이 아니다.

## 확인과 이해 점검

이번 작성에서는 Git 소스와 기존 기록을 대조했다. 실행 기록은 [환경 계획](../EnvironmentPlan.md)의 시간 트랜잭션·서버 시간 검증 절, 검사 코드는 [CCLWorldAdvanceTests.cpp](../../Source/CCL/Environment/CCLWorldAdvanceTests.cpp)와 [CCLWorldClockTests.cpp](../../Source/CCL/Environment/CCLWorldClockTests.cpp)다. 새 빌드나 게임 실행은 하지 않았다.

| 상황 | 확인할 결과 |
|---|---|
| 생활 진행 예산을 부족하게 지정 | 표면과 완료 시각 보존, 대기 구간 유지 |
| 같은 대기 구간을 정상 예산으로 재시도 | 거래·강수 한 번 적용 |
| 배율 변경 전에 대기 시간이 남음 | 입력 당시 배율 유지 |
| 저장 복원으로 시각이 과거로 이동 | 새로운 실행 세대로 과거 비동기 결과 구분 |

**후보 시계가 먼저 Commit했으니 생활 실패 때 시간이 이미 흐른 것 아닐까?** 원본과 후보는 다른 값 객체다. 마지막 대입 전까지 원본 Clock은 바뀌지 않는다. 다만 소비자가 외부 객체를 미리 수정한다면 복사만으로 복구할 수 없으므로 외부 효과도 확정 경계에 맞춰야 한다.

**세계 배율 60을 캐릭터 이동에도 곱해야 할까?** 현재 계약은 이동 시간과 세계 시간을 나눈다. 비가 쌓이는 속도와 플레이어가 한 걸음 옮기는 속도의 조정은 서로 다른 문제다.
