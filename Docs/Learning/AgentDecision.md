# Agent 판단과 기억: 아는 기회 중 실행할 의도를 선택하기

판단 커널은 값 스냅샷으로 후보를 평가하고 점수의 기여를 남긴다. 실제 자원 변경은 실행 단계가 맡는다. `Source/CCL/Agents/CCLDecision.cpp`, `CCLAgentFeatures.cpp`와 `CCLLifeSimulation.cpp`에서 성향·욕구·관계·목표가 어떻게 다음 선택으로 이어지는지 이해한다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `68672713e47b19b0d4adb51bdc21c7c9bbe78eb6` |
| 도입·변경 기준 | `aea2debe066b2d95fd41cb9f83100e551a0bbadb` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전 기준에는 Feature와 관측·기억의 기반이 있었지만 `CCLDecision.cpp`는 없었다. `aea2deb`에서 생활 판단 커널이 도입됐다. 최초 이전 코드는 기억 입력, 현재 코드는 그 정보의 소비 경로로 구분한다. 요구와 상태는 결정 18, [AgentFoundation](../AgentFoundation.md)을 따른다.

## 2. 다른 삶을 만드는 입력

같은 상인 정의라도 알고 있는 거래, 자기 잔액, 배고픔, 상대에 대한 신뢰가 다를 수 있다. 성격 수치만 바꿔도 실제 자원·실행 결과가 다음 판단에 돌아오지 않으면 생활의 차이를 설명하기 어렵다.

현재는 후보 생성, 점수 계산, 실행 결과와 기억 반영을 분리한다. ‘선택됨’은 아직 ‘성공함’이 아니다.

## 3. 사건, 관측과 의도

WorldEvent는 세계에서 일어난 사실이고 Observation은 개인이 지각한 정보다. PerceivedSubject는 관측한 상대이며 모르는 상대의 실제 ID를 채우면 전지적 지식이 생긴다.

EvidenceId는 증거 하나를 식별한다. 같은 사건의 재전달을 새 독립 증거로 세지 않도록 중복 검사에 쓴다. Intent는 선택한 활동·목표·기회와 시작·만료 시각이며 장기 LifeGoal과 구분한다.

이 값 커널은 프로젝트 코드다. UE5의 StateTree가 개인의 장기 목표를 자동 생성하는 구조가 아니다. StateTree는 선택된 의도를 실행한다.

## 4. 이전의 관측 입력

다음은 최초 Feature 기반의 관측 검증 부분이다.

출처: `6867271`, [CCLAgentFeatures.cpp](../../Source/CCL/Agents/CCLAgentFeatures.cpp):164의 `CCLAgentFeatures::Observe`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool CCLAgentFeatures::Observe(FCCLAgentExperience& Experience, const FCCLObservation& Observation)
{
    if (!Observation.EventId.IsValid() || !Observation.EvidenceId.IsValid() || !Observation.EventType.IsValid() ||
        !Unit(Observation.Confidence) || !FMath::IsFinite(Observation.Time) || Observation.Time < 0 ||
        (Observation.PerceivedSubject.Kind == CCLAgentTags::Unknown && Observation.PerceivedSubject.Id.IsValid()))
    {
        return false;
    }

    for (const auto& Belief : Experience.Beliefs)
    {
        if (Belief.EvidenceIds.Contains(Observation.EvidenceId))
        {
            return false;
        }
    }
```

관측의 유효성과 같은 EvidenceId의 중복을 확인한다. 이 단계만으로 어디로 가서 무엇을 할지는 선택되지 않는다. 기억을 판단 입력에 연결하는 후속 구현이 필요했다.

## 5. 판단과 실행을 나눈 이유

결정 18은 행동 결과가 자원·기억·관계를 바꾸고 그 변화가 다음 선택의 원인이 되도록 요구했다. 커널을 Actor·Mass에서 분리하면 같은 값 입력의 판단을 재현할 수 있다.

이는 물리 이동까지 비트 단위로 재현한다는 보장은 아니다. 이동 실패나 예약 충돌은 실행 결과로 돌아와야 하며, 점수 함수가 먼저 성공 보상을 적용하면 이 경계가 사라진다.

## 6. 현재 점수와 선택 유지

`Evaluate`는 후보를 OpportunityId 순으로 정렬하고 Base·LifeGoal·Relationship, 성향과 감정·욕구 기여를 Trace에 누적한다. 실행 불가능하거나 점수가 유한하지 않은 후보는 선택하지 않는다.

다음은 승자를 고른 뒤 기존 의도를 유지하는 검사다.

출처: `50e5838`, [CCLDecision.cpp](../../Source/CCL/Agents/CCLDecision.cpp):86의 `CCLDecision::Evaluate`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
    if (Current != INDEX_NONE && !Result.Traces[Winner].bUrgent &&
        (Input.Time - Input.CurrentIntent.StartedTime < Input.MinimumCommitment ||
         Result.Traces[Winner].Score - Result.Traces[Current].Score < Input.SwitchMargin))
    {
        Winner = Current;
    }

    const auto& Selected = Candidates[Winner];
    Result.Intent.Activity = Selected.Activity;
    Result.Intent.Target = Selected.Target;
    Result.Intent.OpportunityId = Selected.OpportunityId;
    Result.Intent.SupportingLifeGoalId = Selected.SupportingLifeGoalId;
    Result.Intent.StartedTime = Winner == Current ? Input.CurrentIntent.StartedTime : Input.Time;
    Result.Intent.ExpireTime = Input.Time + 3600;
    Result.bSelected = 1;
    return Result;
}
```

긴급하지 않은 작은 점수 차이만으로 매번 목적지를 바꾸지 않게 MinimumCommitment와 SwitchMargin을 사용한다. 긴급 욕구가 있는 후보는 이 유지 조건보다 우선할 수 있다. 위 발췌의 상수와 분기는 현재 구현이며 최종 밸런스 사양을 새로 확정하지 않는다.

## 7. 후보 생성과 장기 목표

`FCCLLifeSimulation::Decide`는 KnownOpportunities만 순회한다. 자기 잔액·물품·공간을 확인하고 거래 상대의 비공개 재고는 실행 때 재검사한다. 전 세계 기회를 모두 커널에 넣고 점수만 낮추는 방식이 아니다.

관계의 신뢰·호감·원한·공포와 실패 믿음이 후보의 RelationshipUtility를 만든다. LifeGoal은 지원하는 활동과 수혜자를 확인해 목표 기여를 준다. `CCLGoalPolicy::Evaluate`는 잔액, 실제 상환, 도움 영수증, 숙련·시설 상태에서 진행을 계산한다.

기억은 `Observe`에서 믿음과 관계를 갱신한다. 자신이 알고 있는 상대에 대해서만 관계가 바뀌며, 기억·믿음·증거 수에는 한도가 있다. 무한한 개인 연대기를 보존하는 구현은 아니다.

## 8. 입력 스냅샷과 실행권

커널은 값을 읽어 Result를 반환하고 Store에 직접 쓰지 않는다. Actor나 Mass가 동일한 입력을 넘기면 같은 판단 경로를 사용한다. 실제 Intent 게시와 자원 변경은 실행권을 가진 Simulation 경로에서 처리한다.

클라이언트 화면에 보여 줄 주민 이름·현재 활동과 서버의 개인 기억 전체는 다른 데이터다. 지도에도 관찰자별 공개 정책이 따로 있다.

## 9. 선택 실패와 경험

후보가 없으면 `bSelected`가 설정되지 않는다. 실행 직전 기회 버전·예약·자원 조건이 달라지면 실제 수행이 실패할 수 있다. 실패가 성공 경험·허기 해소로 반영되지 않아야 한다.

`AdvanceNeeds`는 경과 시간으로 욕구를 올리고 감정을 기준값 쪽으로 감쇠한다. 급한 식사가 선택됐다고 기존 부채 상환 목표 자체를 지우지는 않는다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 직업별 고정 일정 | 예측·연출이 쉬움 | 실제 부족·관계 변화 반영이 제한됨 |
| 점수만 선택 | 다양한 입력 반영 | 반복 전환과 원인 추적 관리 필요 |
| 설명 가능한 점수 + 의도 유지 | 선택 근거·재현 검사 가능 | 후보·기여·실패 기록 비용 |

현재는 작성된 축과 활동을 사용한다. 실제 인간 심리를 모델링했다거나 모든 성격 조합의 행동 품질을 검증했다는 의미는 아니다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 같은 입력 반복 | 같은 선택과 점수 기여 |
| 성격만·관계만·자원만 변경 | 원인별 선택 차이를 설명 |
| 같은 EvidenceId 반복 | 독립 증거·관계 변화 중복 방지 |
| 모르는 기회·모르는 공격자 | 숨은 정보를 판단에 주지 않음 |
| 실패한 수행 | 성공 보상·욕구 충족 없음 |
| 긴급 욕구 | 단기 의도 전환, 장기 목표 보존 |

기존 대조·관측·30일 검사는 AgentFoundation에 기록돼 있다. 이번에는 코드와 그 기록을 대조했으며 시뮬레이션은 새로 실행하지 않았다.

## 12. 이해 확인

**최고 점수 후보를 골랐는데 실제 거래가 실패할 수 있을까?**

가능하다. 상대의 비공개 재고나 실행 시점 조건은 판단 이후 검사한다. 선택과 성공은 다른 사건이다.

**의도를 유지하면 항상 더 좋은 선택일까?**

아니다. 작은 점수 변화로 행동이 흔들리는 비용을 줄이는 대신 즉시 최적 후보로 바꾸지 않을 수 있다. 긴급 욕구와 유지 기준을 함께 검증해야 한다.
