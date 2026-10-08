# 생활 Agent·무기·지도 확인 안내

개인의 목표·경험·자원이 다음 선택에 영향을 주는 생활 시뮬레이션과 실제 마을 실행을 연결했다. 총기 투사체, 미니맵·전체맵, 마을 진입·승리 연출도 같은 Campaign에서 확인할 수 있다. 구현 계약과 검사 근거는 [AgentFoundation](AgentFoundation.md)이 소유한다.

## 직접 확인

1. `CCL.uproject`의 `/Game/Maps/Campaign`을 실행한다. 마을 진입 연출이 끝나면 카메라와 HUD가 돌아온다. `Esc`로 연출을 건너뛸 수 있다.
2. 주민 가까이에서 `T`로 대화한다. 생활 NPC는 현재 활동·목표·진행과 자신의 잔액을 대사로 알린다. 퀘스트 담당자는 기존 퀘스트·상점 기능을 유지하며 생활 행동도 수행한다.
3. 오른쪽 위 미니맵을 확인하고 `M`으로 전체맵을 연다. 파란색은 나, 노란색은 NPC, 빨간색은 보이는 적이다. `Esc`로 닫는다.
4. 마을 시작 지점 주변의 Pistol·Rifle·탄약을 `E`로 수집한다. `I`에서 무기를 장착한다.
5. `R`로 장전한다. Rifle은 왼쪽 클릭으로 발사하고 오른쪽 클릭을 유지하면 조준한다. 한손 장비는 장착한 손의 입력을 사용한다. 탄환은 서버에서 이동·충돌한다.
6. 살아 있고 행동 중이 아닐 때 `Esc` 메뉴에서 저장한다. 재개 후 주민의 목표·재산·부채·기억·관계와 탄창 상태를 확인한다. 호스트 개인 체크포인트 정책은 [SessionFoundation](SessionFoundation.md#이번-저장-정책)을 따른다.

주민의 작업은 이동 후 일정 시간 수행하므로 시작 직후에는 실행 중인 모습이 먼저 보인다. 보관 시설의 개선은 실제 비용을 지불할 수 있고 개선 행동을 선택했을 때만 일어난다. 30일 시험은 별도의 간략 실행 검증이며 실제 마을을 즉시 30일 돌리는 단축키는 없다.

## 주요 클래스 관계

아래는 구현 책임을 축약한 의사코드다. 실제 선언과 프로퍼티는 `Source/CCL/Agents/`에 있다.

```cpp
FCCLAgentRecord {
    Guid Id;
    PrimaryAssetId DefinitionId;
    Map<Tag, FeatureState{Version, InstancedStruct}> Features;
    PersistentIntent Intent;
    LocationReference Location;
}

FCCLAgentStore {
    Map<AgentId, RecordWithGenerationAndWriter> Records;
    Acquire(); Commit(); CommitBatch(); Release();
}

FCCLLifeSimulation {
    AgentStore Agents;
    FeatureRegistry Registry;
    EconomyState Economy; // canonical accounts, inventories, obligations, ownerships
    Array<WorldOpportunity> Opportunities;
    Array<ScheduledWorldEvent> Events;
    Array<LifeExecutionReceipt> Results;
    Decide(); Execute(); AdvanceTo(); Save(); Load();
}

UCCLAgentWorldSubsystem { LifeSimulation Simulation; }
UCCLAgentComponent { Guid AgentId; } // Actor representation of a persistent record
ACCLAgentAIController { StateTreeAIComponent StateTree; ReservationToken Reservation; }
CCLDecision::Evaluate(DecisionInput) -> DecisionResult;

UCCLLifeGoalDefinition {
    GoalTag;
    GoalProgressEvaluator ProgressEvaluator;
    GoalCompletionPolicy CompletionPolicy;
    TagContainer SupportingActivities;
}

UCCLActionComponent { Map<SourceId, AbilityGrants> Sources; }
ACCLProjectile { ProjectileMovement Movement; CapturedEffect; CapturedTeam; }
UCCLMapSubsystem { RegionDefinition Region; Array<MarkerView> Markers; }
UCCLCinematicSubsystem { Guid Active; Camera; Audio; Light; UIPresentationHandle; }
```

Agent Record는 Actor를 참조하지 않는다. 생활·사회 기능이 필요 없는 적은 성격·욕구·경험만 조합할 수 있다. Mass 실험은 같은 판단 함수를 호출하고 의도·실행권을 Fragment로 옮긴다. 인벤토리·생활 기록을 Mass Fragment에 복제하지 않는다.

## 데이터 확인 위치

| 계층 | 실제 데이터 | 확인 위치 |
|---|---|---|
| 개인 특성 | 위험 감수·공격성·호기심·사회성·자기 통제·공감성, 물질적 이익·의무·타인 복지 | `FCCLAgentTraits::Axes` |
| 개인의 삶 | 직업·역할·집·일터·사회 연결·숙련·장기 목표 | `FCCLLifeState` |
| 실제 자원 | 계정·인벤토리·소유권·의무 ID, 정수 잔액과 부채 | `FCCLAgentResourceLinks`, `FCCLEconomyState` |
| 경험과 관계 | 관측한 사건·상대, 기억·믿음·증거, 신뢰·호감·원한 | `FCCLAgentExperience` |
| 현재 상태 | 욕구 긴급도·감정 강도와 감쇠, 부상, 단기 의도 | `FCCLAgentNeeds`, `FCCLPersistentIntent` |
| 결과 설명 | 요청 ID·기회·실행 시각·성공 여부·실패 이유·선택 점수 기여 | `FCCLLifeExecutionReceipt` |

같은 상인 Definition을 사용하는 초기 조건은 `/Game/Progression/DA_MerchantLifeScenario`에 있다. 기본 성격 편집 UI는 0-100을 쓰고 내부 계산은 0-1이다. 새로운 성격 축이나 활동을 추가하면 대응하는 행동 차이 검사를 함께 추가한다.

## 결과와 제한

검사 목록과 로그는 [AgentFoundation의 검증 표](AgentFoundation.md#검증)에 있다. 30일 결과 파일은 로컬 `Saved/Tests/LifeSimulation/merchants-30-days.txt`다. 날짜별 자원과 관계를 읽고 `execution=` 줄의 점수 기여를 보면 선택과 결과를 이어서 확인할 수 있다.

기반과 시험용 콘텐츠를 구현한 상태다. 최종 마을 아트, 총기 애니메이션·총성, 동물별 행동, 탐험 안개와 손님의 개인 저장은 포함하지 않는다. Mass는 실제 Entity를 사용하는 판단·상태 이전 시험이며 군중 이동·전투 구현이 아니다. 현재 구현 범위의 세부 제한은 [AgentFoundation](AgentFoundation.md#현재-제한)을 따른다.
