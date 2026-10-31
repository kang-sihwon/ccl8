# 공통 시간의 정밀도: 마지막 분할을 끝내는 조건

시계는 입력받은 게임·세계 종료 시각을 보존하고, 마지막 분할에서 두 시각에 함께 도달한다. [공통 시간](EnvironmentTime.md)의 후보 확정을 이해했다면, 이 장에서는 계산이 성공했는데도 아주 작은 잔여 시간이 남을 수 있는 이유를 배운다.

## 비교 기준과 문제

이전 코드는 `a817082`, 수정이 포함된 코드는 `c8816e7`이다. 현재 기준은 2026-10-31의 `main`인 `d4888b0`이고, 시계는 2026-10-12에 `Source/KSIWorldTime/`으로 옮겨졌다([Framework 파일 이동표](../FrameworkPlan/Stage4.md#코드-위치의-전후-비교)). 이전 `c8816e7`의 종료 조건이 `d4888b0`의 `KSIWorldClock.cpp`에 글자 그대로 남아 있음을 확인했다. 브랜치와 전체 커밋, 실제로 읽은 기존 검사 결과는 [후속 비교 근거](EnvironmentFollowupEvidence.md)에 있다. 

[시간 분할 정밀도 기록](../EnvironmentPlan/Integration/Clock.md#시간-분할-정밀도-구현검증)은 60배율에서 남은 게임·세계 구간을 함께 검사하도록 바꾼 이유를 설명한다. 다음 숫자는 그 조건을 이해하기 위한 예다. 모든 배율에 대한 수학적 정확성을 증명하는 예는 아니다.

게임 시간 0.25초를 60배로 받으면 세계 시간 입력은 `0.25 × 60 = 15초`다. 한 번에 처리할 세계 시간 예산이 1초라면 15번 나눈다. 각 분할의 게임 시간은 `1 ÷ 60초`다. 실수의 수학에서는 15번의 합이 0.25지만, `double`은 유한한 이진수로 근사한다. 나누기와 반복 덧셈의 반올림 순서가 달라지면 저장한 끝점과 계산한 합계가 다를 수 있다.

`Pending`은 아직 소비하지 않은 입력 구간이다. `GameToSeconds`와 `WorldToSeconds`는 입력받을 때 보존한 두 끝점이다. `MaxGameSeconds`와 `MaxWorldSeconds`는 이번 시도에서 허용하는 처리량이다. 이 값들은 프로젝트의 시계 계약이며 UE4에서 UE5로 바뀌며 자동 제공된 기능이 아니다.

## 이전 조건과 수정한 조건

이전 코드도 끝점 자체는 저장했다. 다만 끝점을 선택할 조건을 이번에 계산한 `Delta`와 남은 게임 시간의 정확한 동등 비교로 판정했다.

출처: `a817082`, `Source/CCL/Environment/CCLWorldClock.cpp`의 `FCCLWorldClock::Prepare`. 연속 발췌이며 들여쓰기만 정리했다.

```cpp
if (Delta == Pending.GameSeconds)
{
    // Preserve the accepted boundary exactly when finishing an input segment.
    Candidate.GameToSeconds = Pending.GameToSeconds;
    Candidate.WorldToSeconds = Pending.WorldToSeconds;
}
```

세계 시간 예산으로 나누어 구한 Delta와 반복 처리 뒤 남은 게임 시간이 조금 다르면 이 분기에 들어오지 못한다. 문제를 단순히 ‘double에 ==를 쓰면 안 된다’로 외우면 수정의 의미를 놓친다. 여기서 필요한 것은 두 시계의 입력 구간을 함께 끝낼 수 있는지 판정하는 일이다.

출처: `d4888b0`, [KSIWorldClock.cpp](../../Source/KSIWorldTime/Private/KSIWorldClock.cpp)의 `FKSIWorldClock::Prepare`. 연속 발췌이며 들여쓰기만 정리했다. `c8816e7`에서 들어온 수정이 그대로다.

```cpp
if (Pending.GameToSeconds - State.GameSeconds <= MaxGameSeconds &&
    Pending.WorldToSeconds - State.WorldSeconds <= MaxWorldSeconds)
{
    // Test both accepted endpoints, not a repeatedly rounded scale conversion.
    Candidate.GameToSeconds = Pending.GameToSeconds;
    Candidate.WorldToSeconds = Pending.WorldToSeconds;
}
```

첫 조건은 남은 게임 구간이 게임 예산 안에 있는지 검사한다. 둘째 조건은 남은 세계 구간이 세계 예산 안에 있는지 검사한다. 둘 다 참이면 저장한 끝점을 그대로 후보에 넣는다. 마지막 구간을 다시 배율로 변환하지 않으므로 두 시계가 다른 반올림 경로를 따라 끝나는 문제를 줄인다.

이 분기 앞에서는 양수·유한 예산과 미처리 입력을 검사하고, 중간 분할용 Delta를 계산한다. 분기 뒤에서는 후보의 유한성·진행량을 검사하고 티켓을 발급한다. 실제 원본 시각은 `Commit`이 유효한 티켓을 확인한 뒤 바꾼다. 끝점을 후보에 넣었다고 저장 완료나 생활 진행 완료가 되는 것은 아니다.

## 취소·저장·재시도의 연결

시계를 준비한 뒤 생활 계산이 실패하면 같은 입력 구간을 다시 처리해야 한다. `Abort`는 준비를 취소하고, 다음 Prepare는 같은 끝점을 대상으로 삼는다. 시계를 저장·복원할 때도 완료 시각뿐 아니라 대기 구간의 끝점과 배율을 보존해야 한다.

[CCLWorldClockTests.cpp](../../Source/CCL/Environment/CCLWorldClockTests.cpp)의 `FCCLWorldClockSplitBoundaryTest::RunTest`(자동화 이름 `CCL.Environment.Clock.SplitAcceptedBoundaries`)는 시작 세계 시각 0, 86400, 315360000초에서 각각 200개 입력을 처리한다. 입력 하나를 15번 나누므로 `3 × 200 × 15 = 9000`개 분할이다. 각 분할에서 준비·취소·재준비 후 두 끝점을 비교하고, 확정·저장·새 시계 복원을 반복한다.

입력마다 일시 정지된 100게임 초도 별도로 넣는다. 이 입력은 결과에 포함되지 않아야 하므로 최종 게임 시각은 `(Frame + 1) × 0.25`다. 최종 세계 시각은 `Start + (Frame + 1) × 15`다. 테스트가 검사하는 것은 잔여 대기 시간의 부재와 두 끝점의 일치다.

## 대안과 비용

허용 오차 이내의 잔여 시간을 삭제하면 구현은 짧아진다. 그러나 그 시간이 날씨·거래 입력인지 판단하지 않고 버릴 수 있다. 모든 시간을 정수 단위로 바꾸는 대안은 표현할 배율과 시간 해상도, 저장 형식을 다시 정해야 한다. 현재 수정은 기존 형식을 유지하면서 두 단위의 남은 양을 매번 검사하는 비용을 선택했다. 이는 기록된 설계 이유에 대한 코드 해설이다.

실패 후 다음 프레임을 기다리는 실험 실행기는 별도 문제다. 재시도할 때 같은 0.25초를 다시 큐에 넣으면 입력이 중복된다. 현재 실행 경로의 자동 입력 보류와 재시도 상태는 [CCLExperimentDirector.cpp](../../Source/CCL/Environment/CCLExperimentDirector.cpp)의 `RunClock`과 `CompleteCase`, [CCLWorldSimulationSubsystem.cpp](../../Source/CCL/Environment/CCLWorldSimulationSubsystem.cpp)의 `AcquireManualAdvance`에서 확인할 수 있다. 끝점 수정만으로 비동기 재시도 전체가 해결됐다고 해석하지 않는다.

## 확인과 이해 점검

이번에는 코드를 실행하지 않고 Git과 기존 보고서를 읽었다. 정밀도 수정 당시 144개, 후속 전체 검토 뒤 154개 자동 검사 보고서의 실패·경고·미실행이 0임을 확인했다. 실행 범위와 이후 패키지 미검증은 [후속 비교 근거](EnvironmentFollowupEvidence.md)를 따른다.

**세계 시각 하나만 끝점으로 맞추면 충분할까?** 게임 시간 소비량이 남으면 대기 구간이 끝나지 않거나 다음 환기·불 계산의 게임 시간 입력이 어긋날 수 있다. 두 단위의 예산과 끝점을 함께 확인해야 한다.

**9000개 분할이 통과했으니 60배 세계의 처리 성능도 충분할까?** 이 검사는 시계 값과 저장 재개의 정확성을 확인한다. 물·불·생태를 포함한 프레임 비용과 장시간 대기열 증가는 별도 측정 대상이다.
