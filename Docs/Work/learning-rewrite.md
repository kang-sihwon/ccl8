# 학습 문서 재작성

`Docs/Learning/`의 장 가운데 발췌한 "현재" 코드가 지금 코드와 달라진 곳을 현재 코드 기준으로 다시 쓰는 작업이다. 근거는 [결정 28](../decisions/0028-agent-workflow-restructure.md)이고 형식은 [시스템 학습 문서 작성 규칙](../SystemExplanationGuide.md)이 소유한다.

## 목적지

- 장마다 비교 기준에 현재 커밋을 더하고, 옛 `CCL*` 이름과 새 `KSI*` 이름의 대응을 적는다.
- 발췌한 함수는 현재 코드와 대조한다. 같으면 출처를 현재 커밋으로 바꾸고, 달라졌으면 달라진 이유를 이력으로 확인해 쓴다.
- 줄 번호는 함수 이름으로 바꾼다.
- `node Tools/Agent/docs.cjs check`가 통과한다.
- 새로 빌드하거나 실행하지 않았으면 그렇게 적는다.

## 발췌 비교 결과

2026-10-31에 `출처:` 줄이 있는 발췌를 현재 소스와 접두사 `CCL`→`KSI`를 무시하고 비교했다. 아래 "달라짐"은 비교한 함수가 `50e5838` 이후에 바뀌었다는 뜻이다. 이전 커밋(`6867271` 등)의 발췌가 지금과 다른 것은 정상이라 세지 않았다.

## 진행

2026-10-31에 모든 장을 `d4888b0` 기준으로 다시 대조했다. 장마다 커밋이 하나씩 있다.

- [x] UI 기반, UI 연출, 지도, 콘텐츠 제작, 인벤토리 UI, 환경 후속 근거, 환경 맵 설정. 발췌가 접두사만 달랐다.
- [x] 전투 장(GASCombat, Weapons, Equipment, EnemyAI). `ResolveHit`의 재진입 가드, 팀 정보의 `SourceFacts` 이동, 장비 교체의 인벤토리 트랜잭션을 설명에 반영했다.
- [x] 아이템·성장·세션·캠페인·마을(ItemInventory, Progression, SessionSave, Campaign, VillageServices). `Use`의 소모 순서, 구매의 데이터 구동 상품, 세션 형식 8을 반영했다.
- [x] Agent·HUD·연출·이동(AgentState, AgentDecision, AgentExecution, LifeEconomy, HUDDialogue, Cinematics, MovementRespawn). `Enter`의 생존 검사를 반영했다.
- [x] 환경 장 전체. 미커밋 사본(`WT-2026-10-10`)을 인용하던 발췌는 `c8816e7`에 커밋된 것을 확인하고 `d4888b0`로 다시 대조해 출처를 바꿨다. 근거 문서(EnvironmentEvidence, EnvironmentFollowupEvidence, EnvironmentPlanningHistory)는 당시 기록으로 두고 안내만 고쳤다.

## 아직 모르는 것

- 바뀐 코드가 어느 사고를 겪은 뒤의 수정인지는 대부분 커밋 본문에 없다. 장에는 "이유 기록 없음"으로 적었다. 이유를 아는 사람이 있으면 채운다.
- 빌드와 게임 실행은 하지 않았다. 코드와 문서의 대조만 했다.
