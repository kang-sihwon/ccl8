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

- [x] UIFramework, UIPresentation, Map, ContentValidation, InventoryUI, EnvironmentFollowupEvidence, EnvironmentMapConfiguration. 발췌가 접두사만 달랐고, 끝났다.
- [ ] AgentExecution. 달라짐: `CCLMassAgentBridge::Enter`가 죽은 Agent를 거부한다.
- [ ] Weapons. 달라짐: `ACCLProjectile::Impact`.
- [ ] GASCombat. 달라짐: `UCCLCombatComponent::ResolveHit`(일부 발췌).
- [ ] Equipment. 달라짐: `UCCLLoadoutComponent::Equip`(일부 발췌).
- [ ] ItemInventory. 달라짐: `UCCLInventoryComponent::Add`.
- [ ] Progression. 달라짐: `UCCLLoadoutComponent::Learn`, `Use`.
- [ ] Campaign. 달라짐: `ACCLCampaignDirector::NotifyEnemyDefeated`.
- [ ] SessionSave. 달라짐: `UCCLGameInstance::LoadSession`.
- [ ] VillageServices. 달라짐: `UCCLExpeditionComponent::Buy`.
- [ ] AgentDecision. 달라짐: `CCLDecision::Evaluate`(일부 발췌).
- [ ] HUDDialogue. 이전 기준의 `ACCLHUD::DrawHUD`를 현재 코드에서 찾지 못했다. 현재 HUD는 표시 모델로 바뀌었는지 확인한다.
- [ ] EnemyAI, LifeEconomy, Cinematics, MovementRespawn. 발췌는 같거나 일부만 같다. 옛 이름과 줄 번호만 고친다.
- [ ] 환경 장(EnvironmentAir, Celestial, Fire, Snow, Terrain, Time, TimeAccuracy, Water, Weather, RestoreParticipants, Persistence). 발췌 형식이 달라 위 비교를 하지 못했다. 일부는 "기록 당시 기준을 유지한다"는 안내만 있다.

## 아직 모르는 것

- 환경 장을 현재 코드로 다시 쓰는 범위. 비교 학습 기록으로 두는 쪽이 나은지 사용자 판단이 필요하다.
