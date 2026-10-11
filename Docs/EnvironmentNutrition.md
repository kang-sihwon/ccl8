# 집계 생물의 에너지·먹이·기아

집계 생물도 세계 시간이 흐르면 에너지를 소비하고, 같은 서식지의 먹이를 먹거나 기아로 죽는다. `Source/CCL/Environment/CCLEcologyNutrition.cpp`가 계산을 맡는다. [생태계·생애](EnvironmentEcology.md)의 하위 사양이며 범위는 [결정 19](DesignLog.md)의 단계 8을 따른다. 이 문서의 에너지와 먹이 계수는 게임 실험용 값이다.

## 개체 수만 저장하던 집계의 한계

집계는 같은 출생 시각의 생물을 연령군 하나로 묶어 개체 수를 기록한다. 예를 들어 20마리 가운데 3마리를 Actor로 표현하면 집계에 17마리, 예약에 3마리가 남는다. 이전에는 예약 개체의 행동만 에너지와 먹이를 계산했다. 그래서 같은 세계에 있어도 집계 17마리는 굶지 않았다.

`main`의 비교 기준은 `004a8c3`이다. 그 기준의 `CCLEcologyBehavior.cpp::AdvanceBehavior`는 먹이를 재생한 뒤 `Behaviors`를 순회했다. 다음 코드는 순서만 남긴 발췌다.

```cpp
// 004a8c3: 먹이 재생 다음에 명시적인 개체 행동만 계산한다.
TArray<FGuid> Ids, FoodIds;
Behaviors.GetKeys(Ids);
ForageSources.GetKeys(FoodIds);
```

현재 구현은 같은 위치에서 집계 계산을 먼저 실행한다. 후보가 실패하면 호출자 `AdvanceTo`가 원본을 유지한다.

```cpp
if (!AdvanceAggregate(At, Error))
{
    return false;
}

TArray<FGuid> Ids, FoodIds;
Behaviors.GetKeys(Ids);
ForageSources.GetKeys(FoodIds);
```

종의 `AggregateDietMask`가 0이면 기존 동작을 유지한다. 활성 종이 있으면 `CCLEcologyAdvance.cpp::AdvanceCandidate`가 정수 세계초마다 경계를 만든다. Actor가 없고 등록된 먹이가 없어도 대사와 기아가 진행한다. 이는 프레임마다 모든 생물을 생성하는 방식보다 상태가 적지만, 각 생물의 식사 이력과 경쟁 순서를 표현하지 못한다.

## 에너지 계산과 음식 원장

`FCCLEcologyNutrition`은 개체당 평균 에너지 `Energy`, 에너지가 없었던 시간 `StarvedSeconds`, 마지막 계산 시각 `LastStepSeconds`를 가진다. 에너지는 0에서 1 사이다. `Charge`는 마지막 계산 이후 소비량을 먼저 빼고, 실제로 에너지가 0이었던 시간만 기아에 더한다.

시험 입력의 에너지 1과 초당 대사 0.25를 대입하면 2초 후 에너지는 `1 - 0.25 × 2 = 0.5`다. 4초가 되면 0이 된다. 기아 제한이 2초라면 먹이가 없는 6초 경계에서 죽는다. 이 순서는 `ReservationStarvationAndRestart`가 검사한다.

먹이 한 단위가 에너지 0.2를 주고, 평균 에너지가 0.35인 17마리를 모두 채우려면 다음과 같이 계산한다.

1. 한 마리의 부족분은 `1 - 0.35 = 0.65`다.
2. 전체 부족분은 `0.65 × 17 = 11.05`다.
3. 필요한 먹이는 `11.05 / 0.2 = 55.25`단위다.

`AdvanceAggregate`는 같은 서식지이면서 먹이 마스크가 맞는 공급원에서 가용량과 필요량 중 작은 값을 꺼낸다. 공급원의 `AvailableUnits`를 줄이고 `ConsumedUnits`를 늘린다. 개체군의 `AggregateConsumedUnits`에도 같은 소비를 기록한다. 이 누계는 개체가 죽거나 집계로 돌아가도 남는다. `ValidateBehavior`는 집계 소비와 명시적인 개체 행동 소비의 합을 전체 공급원의 소비와 대조한다.

소비 순서는 정렬한 개체군·연령군·먹이 GUID로 고정한다. 집계와 행동 없는 예약이 먼저 먹고 명시적인 행동이 뒤에서 먹는다. 저장 재개의 순서를 고정하기 쉽지만 앞 순서가 부족한 먹이를 선점할 수 있다. 공정한 경쟁이나 포식 관계가 필요한 경우에는 별도의 배분 정책이 필요하다. 섭식 후 에너지가 양수가 되면 기아 시간을 해제한다. 소량의 반복 섭식에 따른 장기 영양실조·체중 손실은 이 정책에 없다.

## 예약과 반환 시 상태 이전

`Materialize`는 집계에서 한 마리를 차감하면서 그 연령군의 에너지와 기아를 개체에 복사한다. `AddBehavior`도 이 상태를 이어받으므로 행동을 새로 붙여 에너지를 1로 초기화하지 않는다. 행동이 없는 예약은 Actor·Mass·숨김 여부와 관계없이 집계 정책으로 진행한다.

일반 예약 개체를 반환할 때는 수량만 더하지 않는다. 반환 시각까지 양쪽 대사를 정산하고, 개체 수로 가중한 평균 에너지를 저장한다. 비교 기준의 `Release`와 현재 구현에서 의미가 달라진 부분은 다음과 같다.

```cpp
// 004a8c3
++Candidate.Populations[I.PopulationId].Cohorts[I.CohortId].Count;

// 현재: Policy.AggregateDietMask가 활성일 때 수량을 늘리기 전에 실행한다.
Cohort.Nutrition.Merge(Cohort.Count, I.Nutrition, 1,
    WorldSeconds, Policy.AggregateMetabolism);
++Cohort.Count;
```

집계 9마리의 평균이 0.2이고 반환할 한 마리의 에너지가 0.8이라면 새 평균은 `(9 × 0.2 + 1 × 0.8) / 10 = 0.26`이다. 총 에너지는 같지만 0.8이었던 한 마리의 차이는 사라진다. 기아 시간은 둘 중 긴 값을 보존하며 이후 에너지가 양수인 계산에서 해제한다. 중요한 개체는 기존 정책대로 예약 ID를 유지하므로 일반 집계로 합치지 않는다.

집계 정책이 활성인 종의 개별 행동도 `Charge`를 사용해 실제로 에너지가 없었던 시간만 기아에 더한다. 기존 비활성 종은 이전의 단계 전체를 세는 규칙을 유지한다. 집계 이주와 출생·유입도 같은 `Merge`를 사용한다. 출생·유입의 초기 에너지는 1이다. 기아 사망은 세계·개체군·연령군·개체·사건 시각에서 결정한 ID로 기존 사망 원장을 사용한다. 행동 없는 예약이 죽으면 실행권도 회수한다. Agent의 생애 상태로 소유권을 이전하는 기능은 아직 연결하지 않았다.

## 저장 형식과 이전 파일

생태 본문은 형식 6이다. 종의 집계 식사 정책, 연령군과 예약의 영양 상태, 개체군의 집계 소비 누계를 저장한다. 바깥 세계 봉투는 기존 형식 7을 유지한다. 형식 번호가 가리키는 소유자가 서로 다르므로 함께 증가시킬 필요는 없다.

생태 형식 1-5에는 이 필드가 없다. 읽을 때 `AggregateDietMask = 0`으로 두어 과거 저장에 없던 대사나 기아를 임의로 활성화하지 않는다. 새 실험장 초기 상태는 정책을 활성화한다. 이전 저장에 새 정책을 적용해 실험하려면 전체 초기화로 새 기준 상태를 만든다.

구형 회귀 검사는 최신 파일의 버전 숫자만 바꾸면 안 된다. `CCLEcologyLegacyTest.h`는 형식 6에서 추가된 필드를 실제로 제거해 형식 5의 바이트 배치를 만든다. 기존 형식 3·4 검사도 이 결과에서 각 형식의 필드를 제거한다. 별도의 수작업 형식 5 입력으로 정책 비활성 변환을 검사한다.

## 구역 06에서 확인하는 값

두 환경 맵의 **F7 → 06 생태계**에서 먹이 추가, Actor·Mass·숨김 전환을 실행한다. 각 서식지 바닥의 표시는 개체군 수·먹이·개체 에너지·집계 에너지를 보여 준다. 메뉴는 개체와 집계의 에너지·섭취 누계를 나란히 표시한다. 서버 공개 행에는 집계 에너지와 소비 누계도 포함되며 클라이언트가 이를 복제받는다. 공간 이동과 섭식은 기존 지형·수중·공중 질의를 사용한다.

초기 먹이는 서식지마다 100단위다. 초기 에너지 0.35인 20마리를 1까지 채우려면 `20 × 0.65 / 0.2 = 65`단위가 필요하다. 이전 30단위는 집계 17마리의 필요량에도 부족해 예약 3마리의 이동·섭식 검사에서 섭취량이 0이 됐다. 정상 섭식을 관찰하는 실험장과 먹이 부족을 확인하는 자동 검사에 다른 입력을 사용한다. 수치 정의는 `CCLEcologyExperiment.cpp::MakeInitial`에 있다.

## 실행 근거와 재현

검증 환경은 2026-10-11 UE 5.8.3이다. 아래 경로는 프로젝트 기준이며 기기별 절대 경로는 생략했다. 원본 결과는 로컬 `Saved/`에 보관한다.

| 검사 | 확인한 결과 | 근거 |
|---|---|---|
| 프로젝트 파일 생성·Editor 빌드 | 성공 | `Saved/EnvironmentStages/Nutrition-FinalGPF.log`, `Nutrition-FinalBuild.log` |
| 전체 CCL 자동화 | 158/158, 경고·실패·미실행 0 | `Saved/Tests/Automation/20261011-170903-503/report/index.json` |
| 영양 상태 자동화 | 기아 사망·예약 반환·실행권 회수·분할 재개·실패 원본 보존·구형 변환 | `CCL.Environment.Ecology.Nutrition.ReservationStarvationAndRestart` |
| 이주·개별 행동으로 이전 | 소수 시각에서 에너지 이전, 중복 이주의 한 번 반영, 집계와 개체의 같은 기아 종료 시각 | `CCL.Environment.Ecology.Nutrition.MigrationAndBehaviorTransfer` |
| 집계 규모·반복 저장 | 100개 개체군의 합계 100만 마리, 세계 3600초, 60회 저장·복원 | `CCL.Environment.Ecology.Nutrition.ScaleAndRepeatedRestore` |
| 구역 06 화면·조작·복원 | Scenario Standalone 통과 | `Saved/Tests/EcologyZone/Standalone-20261011-171115-438` |
| 구역 06의 집계 관측·표현·복원 복제 | Listen Scenario, Editor Dedicated Playground 통과. 80ms·2%, 첫·늦은 접속 | `Saved/Tests/EcologyZone/Listen-20261011-170929-939`, `Dedicated-20261011-171021-691` |
| 실제 공간에서 새 프로세스 이어 진행 | Scenario Standalone 통과 | `Saved/Tests/EcologySpace/EnvironmentScenario-Standalone-20261011-161611-586` |
| 실제 공간에서 지연·손실 | Listen Scenario, Dedicated Playground 통과. 80ms·2%, 첫·늦은 접속 | `Saved/Tests/EcologySpace/EnvironmentScenario-Listen-20261011-161653-847`, `EnvironmentPlayground-Dedicated-20261011-161030-941` |

집계 규모 검사는 100개 연령군으로 100만 마리를 표현한다. 위 자동화 실행에서 진행·직렬화·반복 복원은 0.688277초, 최대 스냅샷은 42,272바이트였다. 이는 값 모델을 한 번 실행해 측정한 결과이며 100만 Actor·Mass Entity나 전체 월드의 성능 수치가 아니다. `editor.log`의 `CCL_ECOLOGY_SCALE` 행이 원본이다. 실제 시간 1시간의 장기 실행도 이 검사에 포함하지 않는다.

공간 이동의 위 세 실행은 개별 행동의 기아 시간 정밀화 전 결과다. 전체 자동화와 구역 06의 화면·두 네트워크 실행은 정밀화와 메뉴 표시를 포함한 최종 Editor 빌드에서 수행했다. 패키지 실행은 아래에 별도로 기록한다.

### Windows Game·Server 패키지의 생태 실행

같은 최종 영양 상태 구현을 Windows Development Game·Server로 각각 빌드·쿠킹했다. 생성 로그는 `Saved/EnvironmentStages/Nutrition-FinalGamePackage.log`, `Nutrition-ServerPackage.log`이며 두 UAT 모두 종료 코드 0이다. 아래 Dedicated 검사의 서버는 실제 `CCLServer.exe`, 클라이언트는 실제 `CCL.exe`다.

| 검사 | 결과 | 근거 |
|---|---|---|
| Playground 구역 06 화면·조작·저장 복원 | 통과, 캡처에서 집계 에너지·섭취 표시 확인 | `Saved/Tests/EcologyZone/Standalone-20261011-171512-853` |
| Scenario 공간 이동·새 프로세스 재개 | 통과 | `Saved/Tests/EcologySpace/EnvironmentScenario-Standalone-20261011-171543-526` |
| Scenario 구역 06 복제·표현·복원 | Dedicated 통과, 80ms·2%, 첫·늦은 접속 | `Saved/Tests/EcologyZone/Dedicated-20261011-172032-226` |
| Playground 공간 이동·섭식·복원 | Dedicated 통과, 80ms·2%, 첫·늦은 접속 | `Saved/Tests/EcologySpace/EnvironmentPlayground-Dedicated-20261011-172057-462` |

이 네 결과는 구역 09의 관찰 시간 수정 전 패키지다. 후속 통합 패키지의 검사와 전체 월드 계측은 [환경 계획](EnvironmentPlan.md), 생성·실행 파일 경로는 [패키징](Packaging.md)이 소유한다.

재현 명령은 다음과 같다. 네트워크 검사는 지연·손실 인자가 엔진 로그에 적용됐는지도 확인한다.

```powershell
Tools/Validation/run_automation.ps1 -Filter 'CCL.' -TimeoutSeconds 300
Tools/Validation/run_ecology_zone.ps1 -Rendered -Map EnvironmentScenario
Tools/Validation/run_ecology_space.ps1 -Map EnvironmentScenario -Mode Standalone
Tools/Validation/run_ecology_space.ps1 -Map EnvironmentScenario -Mode Listen -Port 20415 -PacketLagMs 80 -PacketLossPercent 2
Tools/Validation/run_ecology_space.ps1 -Map EnvironmentPlayground -Mode Dedicated -Port 20416 -PacketLagMs 80 -PacketLossPercent 2
```

식생·피식자와 실제 먹이 공급원의 연결, Agent 생애 소유권 이전, 개체 간 회피와 거리 기반 자동 표현 전환은 남아 있다. 이번 공개 행은 실험장 관측이며 원거리 대규모 개체군의 관심 영역별 복제 정책은 제공하지 않는다. 단계 9의 패키징·전체 월드 계측과 남은 범위는 [환경 계획](EnvironmentPlan.md)이 소유한다.
