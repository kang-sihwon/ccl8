# 장비

장착 규칙, 슬롯과 부착 태그, 변경 순서, 장비 에셋 도구를 바꿀 때 읽는다. 근거는 [결정 15](../decisions/0015-struct-fragments-and-two-handed-gear.md)와 [결정 16](../decisions/0016-tagged-slots-and-attachments.md)이다.

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

`Equip`, `Unequip`, `Use`, `Learn`은 변경 중 재진입을 거절한다. 장비 배치를 미리 검증한 다음 `BeginTransaction`으로 알림을 묶고 인벤토리 쓰기를 잠근다. 슬롯과 효과 핸들을 확정하고 GAS 속성 배치를 끝낸 뒤 외형을 동기화한다. 마지막으로 `Inventory.OnChanged`를 한 번 보낸다. 이 알림이 끝날 때까지 잠금을 유지한다. 저장 복원도 같은 쓰기 보호를 적용한다. 효과 적용이 거절되면 이전 장착과 인벤토리를 유지한다.

GAS의 효과 추가 콜백 자체를 숨기지는 않는다. 해당 콜백에서 들어온 Loadout 명령과 인벤토리의 공개 변경 API를 거절한다. 콜백이 ASC를 직접 변경하거나 Actor를 파괴하는 임의 동작까지 되돌리는 범용 트랜잭션은 아니다. 알림 지연과 명령 가드는 동기 콜백의 중간 상태 접근을 제한하는 대가가 있다.

엔진 근거는 UE 5.8.3의 `<Engine>/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent.cpp:1037`의 동기 적용 질의와 `GameplayEffect.cpp:4740`의 효과 추가 알림이다. `FScopedAggregatorOnDirtyBatch`는 속성 계산 알림의 배치이며 모든 GAS 콜백을 지연하는 도구가 아니다. GAS는 UE4에도 존재하던 시스템이며, 이 계약은 UE 5.8.3의 동기 효과 콜백과 속성 배치 동작에 맞춘 것이다. Build.cs 의존은 늘리지 않았다.

인벤토리 쓰기 가능 여부는 `UKSIInventoryComponent::CanMutate`가 소유자의 권위와 Loadout 쓰기 잠금으로 판단한다.

음식 명령은 `FCCLInventoryFood`, 탄약 명령은 `FCCLInventoryWeapon` 기능 서비스가 맡고, 호출자가 인벤토리를 명시적으로 전달한다. 저장 항목과 검증은 호환성 때문에 기존 `Food`, `LoadedAmmo` 필드를 유지한다. 기본 추가·소비의 음식 처리와 기능 상태 검증 연결도 남아 있으므로 완전한 장르 독립 상태 스키마로 분리한 것은 아니다.

## 에셋 도구

- `Tools/Validation/migrate_equipment_tags.py`는 기존 정의 12개와 기본 마네킹을 백업한 뒤 변환을 요청한다. 기본 마네킹에 `CCL_Grip_L/R`, `CCL_Shield_L/R`, `CCL_Stow_Back` 소켓을 추가하고 `Content/Progression/DA_HumanoidAttachments.uasset`을 만든다. 기존 소켓과 이미 있는 프로필 설정은 덮어쓰지 않는다. 재시작한 새 에디터 모듈에서 실행해야 하며, 구버전 모듈에서는 에셋 저장 전에 실패한다.
- `Tools/Validation/create_equipment.py`는 기존 정의를 덮어쓰지 않고 시험용 장비 8종을 만든다.
- `migrate_item_fragments.py`는 기존 정의 네 개의 별도 변환 도구다. `migrate_equipment_tags.py` 실행이 그 네 개를 포함해 백업·로드 변환·저장을 마쳤으므로 따로 실행할 필요가 없다.

구현 위치는 `Source/CCL/Items/CCLItemFragments.h`, `CCLLoadoutComponent.cpp`, `Source/CCL/UI/SCCLInventoryWidget.cpp`다.
