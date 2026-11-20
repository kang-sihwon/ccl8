---
number: 69
date: 2026-11-19
title: "손 입력과 능력 출처 등록부"
topic: "아이템·장비"
status: "유효"
progress: "구현 전"
summary: "손 입력은 `Input.Hand.Left/Right` 태그로 GAS에 바로 보낸다. 몸·장비·훈련·상태의 능력을 출처 등록부 하나로 부여하고, HandEquipment는 무브셋을 참조한다. 적은 패턴별 스펙을 서버에서 활성화하고 AttackOverride와 활성 아이템을 없앤다"
owners: "[장비](../ProgressionFoundation/Equipment.md), [CombatFoundation](../CombatFoundation.md)"
amends: [15]
refines: [8]
---

# 2026-11-19 — 결정 69: 손 입력과 능력 출처 등록부

### 배경

현재 손 행동은 enum 경로, `HandActions` 경로, 재장전 경로의 셋으로 실행되고, 능력 부여도 AbilitySet과 ActionSet의 두 경로다. 입력 태그에 손 구분이 없어 클라이언트가 활성 아이템을 고르는 선행 RPC를 보내야 하고, 총기 발사는 서버의 `TryActivateAbility`로만 실행돼 예측이 없다. 적은 `AttackOverride`로 공격 정의를 바꿔 끼운다.

사용자는 "입력은 Enhanced Input에서 GAS로 넘기면 GAS가 서버·클라이언트 검증을 하는데 왜 그 앞에서 또 하느냐"고 물었고, 손을 태그로 정해 보내면 된다고 제안했다. 무기와 스킬이 늘어날 때 가장 좋은 방식을 물은 뒤 출처 등록부 안(A)을 골랐다. 무기 스킬이 캐릭터·무기·방어구 중 무엇의 능력인지는 아직 모른다고 판단했다.

### 결정

- Enhanced Input의 `IA_LeftHand`·`IA_RightHand`를 `Input.Hand.Left`·`Input.Hand.Right` 태그로 GAS에 바로 보낸다. 서버 RPC로 활성 아이템을 먼저 고르지 않는다.
- 능력은 출처 등록부 하나로 부여한다. 출처는 몸, 장비, 훈련, 상태다. `UKSIActionComponent`의 `RegisterSource`·`RemoveSource`를 확장하고 AbilitySet과 ActionSet 경로를 하나로 합친다.
- HandEquipment는 무브셋 에셋(`UCCLMoveset`)을 참조한다. 몽타주와 타이밍은 무브셋이 공유하고, 개별 수치는 Weapon·MeleeWeapon·RangedWeapon Fragment가 가진다. 무브셋 항목은 상대 입력(Held, Other, 재장전·패링 같은 고정 입력), 능력 클래스, 데이터 오브젝트를 둔다.
- 스펙의 `SourceObject`에는 데이터 오브젝트를 넣는다. 아이템은 등록부의 `FindSource(handle)`로 식별한다.
- 빈 손에는 맨손 무브셋을 등록한다. 맨손 무브셋이 가드와 패링을 가진다.
- 두 손이 같은 공유 고정 입력(재장전)을 가지면 둘 다 실행한다.
- 패링은 아이템이 가져오는 배타 입력(Q)이다. 두 손이 다 가지면 왼손이 우선한다. 회피는 몸 출처에 둔다. [결정 8](0008-dodge-guard-parry.md)의 회피·가드·패링을 입력 출처로 구체화한 것이다.
- 적은 패턴마다 스펙을 등록하고(`SourceObject`는 전투 정의), AI가 고른 스펙을 서버에서 활성화한다. `AttackOverride`와 활성 아이템 개념을 없앤다.
- 무기 종류와 자세는 장착 Gameplay Effect가 부여하는 태그(`Weapon.Type.*`, `Stance.*`)로 다른 시스템과 잇는다.
- 무기 스킬과 그 출처는 정하지 않는다. 등록부가 모든 출처를 받을 수 있게만 만든다.

[결정 15](0015-struct-fragments-and-two-handed-gear.md)가 Loadout에 맡긴 손별 액션의 실행 경로를 이 등록부로 보완한다. 무브셋 형식과 입력 규칙의 정본은 [장비](../ProgressionFoundation/Equipment.md)와 [CombatFoundation](../CombatFoundation.md)이다.

### 트레이드오프

손 행동과 능력 부여가 한 경로가 되고, 원격 클라이언트도 GAS의 예측을 그대로 쓴다. 새 무기는 무브셋과 Fragment 수치만으로 늘릴 수 있다. 대신 장비를 바꿀 때마다 능력 스펙을 부여·회수하므로 복제 비용과 교체 직후 입력의 예측 정확도를 측정해야 한다. 아직 측정하지 않았다. 손마다 같은 입력 태그를 동적 태그로 구분하므로, 두 손에 같은 능력 클래스가 있어도 스펙을 손별로 찾는 검사가 필요하다.
