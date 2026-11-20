---
number: 68
date: 2026-11-19
title: "아이템 Fragment 계층 재구성"
topic: "아이템·장비"
status: "유효"
progress: "구현 전"
summary: "구조체 Fragment를 유지하고 Equipment→HandEquipment→Weapon→Melee·Ranged, Visual→SkeletalMesh, Ammo·Usable·Perishable로 재구성한다. 손 점유는 HandEquipment가 소유하고 각 Fragment가 자기 검증과 인스턴스 상태 타입을 선언한다"
owners: "[장비](../ProgressionFoundation/Equipment.md)"
refines: [15]
supersedes_part: [16]
---

# 2026-11-19 — 결정 68: 아이템 Fragment 계층 재구성

### 배경

사용자는 `CCLItemDefinition.h`의 Fragment 구성을 새로 만든다는 가정으로 다시 판단해 달라고 요청했다. Visual·SkeletalMesh, Weapon·MeleeWeapon, Equipment·HandEquipment, HarvestTool, Ammo의 계층을 제시하고 Combat, Display, Stack, Consumable은 필요 없다고 봤다. 검토안은 [아이템 축 재설계 검토](../ProgressionFoundation/ItemRedesign-2026-11-19.md)다.

검토에서 다음을 확인했다. 이전 UObject Fragment와 구조체 Fragment가 함께 남아 있다. `ValidateDefinition`이 모든 Fragment 종류를 직접 안다. 탄창과 음식 상태가 구조체 하나에 섞여 있다. 사용자는 Fragment를 구조체로 유지하고(Q1), 방어구를 포함한 모든 장비는 Equipment로, 무기 관련 속성은 HandEquipment 아래로 두도록 정했다(Q2).

### 결정

[결정 15](0015-struct-fragments-and-two-handed-gear.md)의 구조체 Fragment(`FInstancedStruct`)를 유지하고 계층을 다음으로 구체화한다. 이름은 `FCCLVisualFragment`처럼 접미사 형식으로 쓴다.

| 계층 | 쓰는 아이템 |
|---|---|
| `Equipment` → `HandEquipment` → `Weapon` → `MeleeWeapon`·`RangedWeapon` | Equipment만: 방어구. HandEquipment까지: 방패. Weapon 아래: 무기 |
| `Visual` → `SkeletalMesh` | 바닥 표시와 장착 메시 |
| `Ammo`, `Usable`, `Perishable` | 탄약, 사용 아이템, 부패 아이템 |

- [결정 16](0016-tagged-slots-and-attachments.md)이 Weapon에 둔 한손·양손 사용 방식(`HandUsage`)을 HandEquipment로 옮긴다. 방패도 손을 점유하기 때문이다. 값은 OneHanded와 TwoHanded만 두고, 한손 무기를 양손으로 잡는 전환은 나중에 넣는다.
- Display, Stack, Combat, Consumable Fragment는 만들지 않는다. 이름·아이콘·최대 중첩 수는 정의 필드다. HarvestTool은 채집 루프를 설계할 때까지 만들지 않는다.
- `MaxStackCount`는 한 칸의 최대 개수인 정의 값이다. 실제 개수는 인스턴스의 `Quantity`다. 장비는 기본값 1을 쓰고 소모품과 재료는 높여 설정한다. Usable은 한 번 쓸 때 `Quantity`를 1 줄이며 별도 사용 횟수를 두지 않는다.
- 각 Fragment는 자기 검증(`Validate`)과 인스턴스 상태 타입을 선언한다. 정의 클래스는 Fragment 종류를 직접 알지 않는다.

Fragment별 필드, 인스턴스 상태 저장, 검증 목록은 [장비](../ProgressionFoundation/Equipment.md)가 소유한다.

### 트레이드오프

새 Fragment를 더할 때 정의 클래스와 검증 함수를 고치지 않아도 되고, 상태가 Fragment별로 나뉘어 탄창과 음식이 서로의 필드를 갖지 않는다. 대신 Fragment 계층이 깊어져 `FindFragment<FCCLWeaponFragment>`가 파생 구조체도 찾는다는 규칙을 검사로 지켜야 한다. Consumable을 없애므로 회복의 중심(충전식·소모품·음식)이 정해지면 Usable의 능력으로 표현해야 하고, 그 기획은 아직 정하지 않았다.
