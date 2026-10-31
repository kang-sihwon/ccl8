# 최소 전투 설계와 구현

마네킹의 맨손 공격 하나와 일반 몹 한 종류로 공격, 회피, 가드, 패링, 사망, 개별 재스폰을 연결했고 Standalone, Dedicated, Listen에서 서버 판정을 검증했다. 이 문서는 전투의 책임 배치, ASC 수명, 행동과 프로토타입 수치, 판정 계약, 검증 기록을 소유한다. 범위와 초기 수치의 근거는 [결정 8](decisions/0008-dodge-guard-parry.md)과 [결정 11](decisions/0011-minimal-combat-and-asc-lifetime.md)이다. 장르에 독립적인 기반과 UE5 현행 시스템 우선 원칙은 [Coding](Coding.md)이 소유한다.

공용 판정·입력·능력 부여·행동 수명은 `Source/KSICombat/`에 있다. `Source/CCL/AbilitySystem`은 게임 ASC·AbilitySet 파생형과 AttributeSet·효과를, `Combat`은 능력·판정 정책·적·투사체를 소유한다. 게임별 무기·아이템·UI 연결도 CCL에 남긴다. 모듈 경계와 UE 5.8.3 검증은 [Framework 단계 3](FrameworkPlan/Stage3.md#단계-3-구현과-검증)이 소유한다.

## 하위 문서

| 문서 | 읽는 때 |
|---|---|
| [구조와 호출 계약](CombatFoundation/Contracts.md) | 책임 배치, ASC 수명, 판정 계약, 콘텐츠 경계를 바꿀 때 |
| [행동과 초기 수치](CombatFoundation/Actions.md) | 수치, 입력, 방향, 판정 순서를 바꿀 때 |
| [검증](CombatFoundation/Verification.md) | 검사 방법, 미검증 항목, 엔진 근거가 필요할 때 |

## 목표와 경계

서버가 행동 허용, 적중, 방어 결과, 능력치, 사망을 확정한다. Standalone, Dedicated Server, Listen Server에서 같은 규칙을 쓴다. 로컬 입력은 GAS의 예측 실행을 쓰고 상대 피해는 서버 결과를 따른다. 과거 시점으로 되돌리는 적중 판정은 넣지 않았다.

피해 대상은 플레이어와 적 사이로 제한한다. 스태미나 없는 파괴 가능한 표적도 같은 적중 경로로 처리한다. 최종 협동·대전·아군 피해와 저장 규칙은 [GameDesign](GameDesign.md)의 미정 상태를 따른다. 인벤토리와 성장은 [ProgressionFoundation](ProgressionFoundation.md), 저장은 [SessionFoundation](SessionFoundation.md)이 소유한다. 락온과 입력 버퍼는 없다.

이동 검증용 `MultiplayerPlayground`와 별도로 `CombatPlayground`를 둔다. 이 맵은 World Partition을 쓰고 스트리밍은 끈다. 전투장 환경용 StaticMesh에는 Nanite를 적용한다([결정 10](decisions/0010-prefer-current-ue5-systems.md)).
