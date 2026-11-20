# 장비

장착 규칙, 슬롯과 부착 태그, 변경 순서, 장비 에셋 도구를 바꿀 때 읽는다. 근거는 [결정 15](../decisions/0015-struct-fragments-and-two-handed-gear.md)와 [결정 16](../decisions/0016-tagged-slots-and-attachments.md)이다. "장착 규칙"부터 "에셋 도구"까지는 현재 구현이다. [결정 68](../decisions/0068-item-fragment-hierarchy.md)-[70](../decisions/0070-rebuild-item-data-without-conversion.md)으로 바뀔 목표는 [재설계 계약](#재설계-계약)이 소유하며, 구현하면 현재 구현 절을 그 내용으로 바꾼다.

## 장착 규칙

장착 부위는 왼손·오른손·갑옷·신발·망토·목걸이·반지 두 칸이다. 한손 장비와 방패는 어느 손에나 장착하며 기본 손은 오른손이다. 양손 장비는 같은 GUID로 두 손을 점유하고 효과는 한 번 적용한다. 장착 항목은 소유 목록에 남고 가방 슬롯만 `INDEX_NONE`으로 바뀐다. 교체할 때는 새 장비가 비우는 칸을 포함해 이전 장비의 반환 공간을 확인하고, 부족하면 장착 상태와 효과를 바꾸지 않는다. 장비를 가방으로 드래그해 해제할 때는 빈 칸이 필요하다.

좌클릭·우클릭은 왼손·오른손 장비에 정의된 액션을 실행한다. Loadout과 ASC는 같은 PlayerState에 속한다. 손 선택 RPC를 먼저 보내고 GAS 예측 입력을 실행해 서버가 현재 보유 장비로 전투 정의를 고른다. 장비가 전투 Fragment를 갖지 않으면 맨손 공격 정의를 쓴다.

## 태그 설정

장비와 무기 설정은 Gameplay Tag로 지정하고, 부착 지점 태그를 캐릭터별 실제 소켓에 연결한다.

| 설정 | 편집 위치와 계약 |
|---|---|
| 장착 부위 | Equip의 `AllowedSlots`, `DefaultSlotTag`: `Equipment.Slot.*`의 구체적인 슬롯 |
| 한손·양손 | Weapon의 `HandUsage`: `Weapon.HandUsage.OneHanded` 또는 `TwoHanded` 중 하나 |
| 부착 위치 | Visual의 `Attachments`: 슬롯 태그, `Attachment.*` 지점 태그, 아이템별 오프셋 |
| 메시 소켓 | `UCCLAttachmentProfile`의 `ReferenceMesh`, `Bindings`: 지점 태그를 실제 소켓 이름에 연결 |
| 에디터 미리보기 | 프로필의 `PreviewItem`, `PreviewSlot`: 기준 포즈의 캐릭터와 선택 장비 표시 |

`CCLEquipment::GetOccupiedSlots`를 장착과 저장 검증이 함께 쓴다. 복제된 장비 목록은 태그와 GUID로 구성한다. 실제 메시 부착은 Fighter가 프로필을 해석하며, 매핑이나 소켓이 없으면 경고를 남기고 표시를 생략한다. 본 이름을 소켓으로 간주하거나 루트에 대신 붙이지 않고, 프로필에 등록한 실제 소켓만 쓴다. 프로필 데이터 검증과 미리보기는 중복 매핑·부적합한 스켈레톤·잘못된 아이템 설정도 확인한다. 이전 Equip의 양손 플래그와 Visual의 소켓 이름은 로드 변환에만 남기고 편집 화면에서는 숨겼다.

기본 마네킹에는 별도의 명명된 장착 소켓을 추가하고 기존 본과 소켓을 보존한다. 양손 슬롯 점유와 보조 손 IK는 다른 것이며, 양손 무기는 두 손 슬롯을 점유하되 오른손에 메시 하나를 표시한다. 기존 구조체 에셋은 로드할 때 변환하고 저장 형식 1-3은 태그 형식 4로 읽는다.

`Loadout`은 장비 표현과 손 행동을 `ICCLEquipmentPresenter`로 요청하고 `UCCLFighterComponent`가 이 계약을 구현한다. 그래서 Items 폴더는 Combat의 구체 컴포넌트를 include하지 않는다. 검증 근거는 [2026-10-11 설계 후속 수정](Verification.md#2026-10-11-설계-후속-수정)이 소유한다.

## 장비 변경의 확정 순서

서버 PlayerState의 Loadout이 장착과 훈련 상태를 소유한다.

`Equip`, `Unequip`, `Use`, `Learn`은 변경 중 재진입을 거절한다. 인벤토리 변경은 편집 객체 `FKSIInventoryEdit`로 한다([재설계 계약](#구현-방향)의 첫 항목, 2026-11-20 구현). 편집을 열면 그 인벤토리의 다른 쓰기가 막히고, 후보 배치를 만든 뒤 `Validate`로 미리 검증한다. 효과 적용이 성공하면 `Commit`으로 후보를 반영하고 슬롯과 효과 핸들을 확정한 뒤 GAS 속성 배치를 끝내고 외형을 동기화한다. 편집이 닫힐 때 `Inventory.OnChanged`를 한 번 보내며, 이 알림이 끝날 때까지 잠금을 유지한다. 효과 적용이 거절되면 `Commit`하지 않으므로 이전 장착과 인벤토리가 그대로다. `Use`는 소모를 검증한 뒤 효과를 적용하고 성공했을 때만 확정한다. `Learn`은 확정하지 않는 편집을 열어 효과 콜백 동안 쓰기를 막는다. 저장 복원도 같은 편집 객체를 쓴다.

GAS의 효과 추가 콜백 자체를 숨기지는 않는다. 해당 콜백에서 들어온 Loadout 명령과 인벤토리의 공개 변경 API를 거절한다. 콜백이 ASC를 직접 변경하거나 Actor를 파괴하는 임의 동작까지 되돌리는 범용 트랜잭션은 아니다. 알림 지연과 명령 가드는 동기 콜백의 중간 상태 접근을 제한하는 대가가 있다.

엔진 근거는 UE 5.8.3의 `<Engine>/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent.cpp:1037`의 동기 적용 질의와 `GameplayEffect.cpp:4740`의 효과 추가 알림이다. `FScopedAggregatorOnDirtyBatch`는 속성 계산 알림의 배치이며 모든 GAS 콜백을 지연하는 도구가 아니다. GAS는 UE4에도 존재하던 시스템이며, 이 계약은 UE 5.8.3의 동기 효과 콜백과 속성 배치 동작에 맞춘 것이다. Build.cs 의존은 늘리지 않았다.

인벤토리 쓰기 가능 여부는 `UKSIInventoryComponent::CanMutate`가 소유자의 권위와 열린 편집 객체로 판단한다. `Commit`은 바뀐 항목만 `MarkItemDirty`하고 기존 항목의 복제 ID를 유지한다. `FFastArraySerializerItem`의 복사가 복제 ID를 초기화하기 때문이다(UE 5.8.3 `<Engine>/Source/Runtime/Net/Core/Classes/Net/Serialization/FastArraySerializer.h:308`).

음식 명령은 `FCCLInventoryFood`, 탄약 명령은 `FCCLInventoryWeapon` 기능 서비스가 맡고, 호출자가 인벤토리를 명시적으로 전달한다. 두 서비스와 Loadout은 friend 없이 편집 객체로만 항목을 바꾼다. 음식이 있는 항목의 추가·제거 규칙은 `UCCLInventoryComponent`의 `StageAdd`·`StageRemove`가 맡는다. 저장 식별자와 음식 환경 바인딩은 두 영속성 서브시스템만 정하므로 그 둘의 friend는 남겼다. 저장 항목과 검증은 호환성 때문에 기존 `Food`, `LoadedAmmo` 필드를 유지한다. Fragment별 상태 배열로 나누는 것은 재설계 단계 4다.

## 에셋 도구

- `Tools/Validation/migrate_equipment_tags.py`는 기존 정의 12개와 기본 마네킹을 백업한 뒤 변환을 요청한다. 기본 마네킹에 `CCL_Grip_L/R`, `CCL_Shield_L/R`, `CCL_Stow_Back` 소켓을 추가하고 `Content/Progression/DA_HumanoidAttachments.uasset`을 만든다. 기존 소켓과 이미 있는 프로필 설정은 덮어쓰지 않는다. 재시작한 새 에디터 모듈에서 실행해야 하며, 구버전 모듈에서는 에셋 저장 전에 실패한다.
- `Tools/Validation/create_equipment.py`는 기존 정의를 덮어쓰지 않고 시험용 장비 8종을 만든다.
- `migrate_item_fragments.py`는 기존 정의 네 개의 별도 변환 도구다. `migrate_equipment_tags.py` 실행이 그 네 개를 포함해 백업·로드 변환·저장을 마쳤으므로 따로 실행할 필요가 없다.

구현 위치는 `Source/CCL/Items/CCLItemFragments.h`, `CCLLoadoutComponent.cpp`, `Source/CCL/UI/SCCLInventoryWidget.cpp`다.

## 재설계 계약

2026-11-19에 정한 목표 구조다. 구현 전이며 진행은 [아이템 축 재설계 작업 지도](../Work/ItemRedesign.md)가 소유한다. 검토 과정과 대안은 [아이템 축 재설계 검토](ItemRedesign-2026-11-19.md)에 있다.

### Fragment

근거는 [결정 68](../decisions/0068-item-fragment-hierarchy.md)이다. 모든 Fragment는 공용 루트 `FKSIItemFragment`를 상속하고 `Validate`, `GetInstanceStateType`, `ValidateState`를 필요한 만큼 재정의한다. 정의 클래스는 Fragment를 돌며 이 함수를 부를 뿐 종류를 알지 않는다.

| Fragment | 부모 | 필드 | 인스턴스 상태 |
|---|---|---|---|
| `FCCLVisualFragment` | 루트 | `WorldMesh`(월드 표시와 정적 장착 메시) | 없음 |
| `FCCLSkeletalMeshFragment` | Visual | `EquippedMesh`, `AnimClass` | 없음 |
| `FCCLEquipmentFragment` | 루트 | `AllowedSlots`(부모 태그 허용), `DefaultSlot`, `Attachments`(슬롯→부착 지점·오프셋), `Effects`(`FKSIEffectGrant` 배열) | 없음 |
| `FCCLHandEquipmentFragment` | Equipment | `HandUsage`(OneHanded·TwoHanded), `Moveset`(`UCCLMoveset`) | 없음 |
| `FCCLWeaponFragment` | HandEquipment | `WeaponType` 태그 | 없음 |
| `FCCLMeleeWeaponFragment` | Weapon | 근접 공격 수치·전투 정의 | 없음 |
| `FCCLRangedWeaponFragment` | Weapon | `Profile`(투사체·속도·재장전 시간의 유일한 소유자), `AcceptedAmmo` 태그, `MagazineSize` | `FCCLMagazineState` |
| `FCCLAmmoFragment` | 루트 | `AmmoType` 태그 | 없음 |
| `FCCLUsableFragment` | 루트 | `UseAbility` | 없음 |
| `FCCLPerishableFragment` | 루트 | `Policy` | `FCCLFoodState` |

- 방어구는 Equipment만, 방패는 HandEquipment까지, 무기는 Weapon 아래를 쓴다. Equipment 계열과 Visual 계열은 정의마다 하나만 둔다. `FindFragment<T>`가 파생형도 찾기 때문이다.
- 이름·설명·아이콘·`MaxStackCount`는 정의 필드다. 장비는 `MaxStackCount` 기본값 1을 쓴다. 실제 개수는 인스턴스의 `Quantity`이고, Usable은 한 번 쓸 때 1을 줄인다.
- 인스턴스 상태는 항목의 `TArray<FInstancedStruct> States`에 Fragment별로 둔다. 조회는 없으면 null을 돌려주는 `FindState`와 만드는 `FindOrAddState`로 나눈다.
- 무기 종류와 자세는 Equipment의 장착 Gameplay Effect가 `Weapon.Type.*`, `Stance.*` 태그로 부여한다. 다른 시스템은 아이템 대신 이 태그를 본다.
- 슬롯 태그는 계층으로 둔다(`Equipment.Slot.Hand.Left`, `Equipment.Slot.Ring.1`). 허용 범위는 부모 태그로 표현한다.
- Display, Stack, Combat, Consumable, HarvestTool은 만들지 않는다. HarvestTool은 채집 루프를 설계할 때 정한다.

### 손 입력과 능력 출처

근거는 [결정 69](../decisions/0069-hand-input-and-ability-sources.md)다.

- 좌·우클릭(`IA_LeftHand`·`IA_RightHand`)은 `Input.Hand.Left`·`Input.Hand.Right` 태그로 ASC에 바로 보낸다.
- 능력은 `UKSIActionComponent`의 출처 등록부로만 부여한다. 출처는 몸, 장비, 훈련, 상태다. 스펙의 `SourceObject`는 데이터 오브젝트이고, 아이템은 `FindSource(handle)`로 찾는다. AbilitySet과 ActionSet 경로는 이 등록부로 합친다.
- `UCCLMoveset` 항목은 상대 입력, 능력 클래스, 데이터 오브젝트다. 상대 입력은 쥔 손(Held), 반대 손(Other), 고정 입력(재장전, 패링)이다. 등록할 때 쥔 손에 맞는 입력 태그로 바꾼다. 몽타주와 타이밍은 무브셋이, 개별 수치는 Weapon 계열 Fragment가 가진다.
- 빈 손에는 맨손 무브셋을 등록한다. 맨손 무브셋은 가드와 패링을 가진다.
- 공유 고정 입력(재장전)은 두 손이 다 가지면 둘 다 실행한다. 배타 입력(패링, Q)은 두 손이 다 가지면 왼손만 등록한다. 회피(Shift)는 몸 출처다.
- 적은 패턴마다 스펙을 등록하고(`SourceObject`는 전투 정의), AI가 고른 스펙을 서버에서 활성화한다. `AttackOverride`와 활성 아이템은 없앤다.
- 무기 스킬과 그 출처는 정하지 않았다.

### 구현 방향

결정이 아닌 구현 선택이다. 구현 중에 더 나은 방법이 보이면 바꾸고 이 절을 고친다.

- 인벤토리 변경은 RAII 편집 객체 `FKSIInventoryEdit`로만 한다. 복사본에서 바꾸고, `Commit`이 검증한 뒤 한 번에 반영하고 알림을 한 번 보낸다. `Commit`하지 않으면 소멸자가 버린다. friend와 외부 잠금 플래그 조작을 없앤다. 2026-11-20에 구현했다(단계 1). 공용 루트의 `Validate`·`GetInstanceStateType` 가상 함수도 이때 더했고, 이름 `FKSIItemFragment`로의 변경은 리다이렉트 시험 표의 형식 문제 때문에 단계 2로 미뤘다.
- `UCCLLoadoutComponent`를 장비(`UCCLEquipmentComponent`: 슬롯, 효과, 장비 출처 등록)와 훈련(`UCCLTrainingComponent`: 포인트, 습득 스킬, 훈련 출처 등록)으로 나눈다. 둘 다 PlayerState에 붙고 자기 저장 항목만 복원한다. 손 입력 RPC는 GAS로 옮겨 없앤다.
- 결과는 복제 문자열 대신 `ECCLEquipResult` 같은 결과 코드로 돌려준다. 원격 요청은 Client RPC로 코드만 보내고, 문구는 UI가 지역화 텍스트로 고른다.
- 정의 에셋은 생성 라이브러리로 다시 만들고, 세션 저장 형식 번호를 올려 이전 저장을 거절한다([결정 70](../decisions/0070-rebuild-item-data-without-conversion.md)).
- 형식만 바꾸는 정리(들여쓰기, 영역 구분)는 동작 변경과 다른 커밋으로 한다.
