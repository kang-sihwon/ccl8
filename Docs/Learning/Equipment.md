# 장비: 슬롯 점유와 메시 부착을 분리하기

장비 교체는 새 장비가 차지할 부위와 이전 장비가 돌아갈 가방 공간을 먼저 계산한다. 외형은 부착 지점 태그를 캐릭터의 실제 소켓으로 해석한다. `Source/CCL/Items/CCLLoadoutComponent.cpp`, `CCLItemTags.cpp`, `CCLAttachmentProfile.cpp`에서 이 두 판단의 경계를 배운다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `3b369682333bf541e91ee5078b03be38a87e4d75` |
| 도입·변경 기준 | `6a9cecb09da31030d54f28c1da59c73a393f402d` |
| 인벤토리 모듈 분리 | `dbdd174dcdae6019918ae2dfb1c4c64c9d732eab` |
| 현재 기준 | `d4888b08154afab8712af974bce0ead2b90978ac` |

현재 기준 `d4888b0`은 2026-10-31의 `main`이며 로컬 원격 추적과 같았다. 원격 서버를 새로 조회한 결과는 아니다. `dbdd174`가 인벤토리 항목 타입을 `KSIInventory` 모듈로 옮겨서 `50e5838`의 `FCCLInventoryEntry`는 현재 `FKSIInventoryEntry`다. 발췌의 출처에는 커밋과 함수 이름을 적었고 상대 링크는 현재 체크아웃을 연다.

이전은 단일 장비 GUID·효과, 변경 기준은 구조체 Fragment와 태그 슬롯·양손 점유 구현이다. 현재 계약은 [ProgressionFoundation](../ProgressionFoundation.md), 요구 이유는 [Decisions](../Decisions.md)의 결정 15·16을 따른다.

## 2. 양손 장비 교체의 문제

오른손 검과 왼손 방패를 양손 무기로 바꾸면 기존 두 장비를 가방으로 돌려보내야 한다. 빈 칸이 부족할 때 절반만 교체하면 슬롯·보관·효과가 서로 다른 상태가 된다.

또한 양손 점유는 손이 두 개라는 규칙이고 메시를 두 번 생성한다는 뜻이 아니다. 현재는 하나의 GUID가 두 손을 점유하고 메시 하나를 오른손에 표시한다.

## 3. 태그, GUID와 효과 핸들

`FGameplayTag`는 등록된 계층형 이름이다. `Equipment.Slot.Hand.Right` 같은 구체적인 슬롯과 상위 분류 태그를 구분해야 한다. Equip은 허용·기본 슬롯을, Weapon은 한손·양손 사용 방식을 소유한다.

GUID는 실제 소유 아이템 하나를 식별한다. `FActiveGameplayEffectHandle`은 그 아이템 때문에 ASC에 적용된 효과 하나를 추적한다. 같은 GUID가 두 손에 있어도 효과 핸들은 한 번만 적용한다.

`AttachmentProfile`은 프로젝트 데이터 자산이다. GameplayTag와 소켓 기능은 UE4에도 있었으며 프로필을 도입한 것은 프로젝트 설계 변경이다.

## 4. 이전의 단일 장비

출처: `3b36968`, `Source/CCL/Items/CCLLoadoutComponent.cpp`의 `UCCLLoadoutComponent::Equip`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
    if (EquipmentEffect.IsValid())
    {
        ASC->RemoveActiveGameplayEffect(EquipmentEffect);
    }
    EquipmentEffect = NewEffect;
    EquippedId = Id;
    SyncAvatar();
    return Report(true, Id.IsValid() ? TEXT("Equipment applied.") : TEXT("Equipment removed."));
}
```

이전에는 새 효과를 만든 뒤 이전 효과를 제거하고 `EquippedId`를 교체했다. 위 발췌 앞에는 보유 여부·효과 타입·적용 성공 검사가 있다. 장비 하나를 바꾸기에는 충분하지만 여러 부위의 반환 공간을 표현하지 않는다.

## 5. 변경 이유

결정 15는 소유 목록과 장착 부위를 나누고 양손 무기 교체를 요구했다. 결정 16은 소켓 이름을 아이템마다 입력하는 대신 캐릭터별 프로필에서 해석하도록 했다.

아이템을 다른 캐릭터에 재사용하기 쉬워지는 대신 프로필·스켈레톤·실제 소켓의 일치 검사가 필요해진다. 소켓이 없으면 임의 본이나 루트에 붙이는 복구는 현재 계약에 없다.

## 6. 현재의 교체 준비

다음은 `Equip`이 새 점유와 반환 대상을 준비하는 부분이다. 앞의 서버 행동 가능 여부, 소유 GUID, 정의·슬롯 검사는 생략했다.

출처: `d4888b0`, [CCLLoadoutComponent.cpp](../../Source/CCL/Items/CCLLoadoutComponent.cpp)의 `UCCLLoadoutComponent::Equip`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
    TArray<FCCLEquippedSlot> NewEquipment = EquipmentSlots;
    TSet<FGuid> Returning;
    for (const auto& Existing : EquipmentSlots)
    {
        if (Existing.Id != Id && Occupied.Contains(Existing.Slot))
        {
            Returning.Add(Existing.Id);
        }
    }

    NewEquipment.RemoveAll([&](const FCCLEquippedSlot& Existing) { return Existing.Id == Id || Returning.Contains(Existing.Id); });
    for (FGameplayTag OccupiedSlot : Occupied)
    {
        FCCLEquippedSlot NewSlot;
        NewSlot.Slot = OccupiedSlot;
        NewSlot.Id = Id;
        NewEquipment.Add(NewSlot);
    }

    TArray<FKSIInventoryEntry> Placement = Inventory->GetEntries();
    for (auto& Value : Placement)
    {
        if (Value.Id == Id)
        {
            Value.Slot = INDEX_NONE;
        }
    }

    for (FGuid ReturnId : Returning)
    {
        auto* Value = Placement.FindByPredicate([ReturnId](const auto& Item) { return Item.Id == ReturnId; });
        if (!Value)
        {
            return Report(false, TEXT("장비 상태를 확인할 수 없다."));
        }

        int32 Free = 0;
        while (Placement.ContainsByPredicate([Free](const auto& Item) { return Item.Slot == Free; }))
        {
            ++Free;
        }

        if (Free >= Inventory->Capacity)
        {
            return Report(false, TEXT("장비를 돌려놓을 소지품 공간이 부족하다."));
        }

        Value->Slot = Free;
    }
```

실제 가방 복사본에서 새 장비의 칸을 먼저 비우고 반환할 항목마다 빈 칸을 찾는다. 공간이 부족하면 원본 슬롯과 효과를 교체하기 전에 실패한다. 새 장비가 비운 칸도 반환 공간으로 이용할 수 있다.

이후 흐름은 현재 코드 기준으로 다음과 같다. 먼저 `Inventory->CanRestore(Placement)`로 새 배치가 유효한지 확인하고 `BeginTransaction`으로 가방의 변경 알림을 보류한다. `bMutating`과 가방의 `bMutationLocked`를 가드로 켜서 도중에 같은 변경이 다시 들어오지 못하게 하고, `FScopedAggregatorOnDirtyBatch`로 속성 갱신을 한 번에 모은다. 범위를 빠져나갈 때 `FinishTransaction(bPublish)`가 호출되며 `bPublish`가 켜진 경우에만 변경 알림을 한 번 보낸다. 새 장비에 효과가 있고 아직 장착 전이면 효과를 검증해 적용하고, 실패하면 아직 배치를 바꾸기 전이므로 그대로 반환한다. 이어서 `RestoreValidated`로 배치를 확정하고 반환된 장비의 효과를 제거한 뒤 `EquipmentSlots`를 교체한다. 마지막에 `bPublish`를 켜고 `SyncAvatar`를 호출한다.

`50e5838`의 같은 함수는 효과를 먼저 적용하고 `Inventory->Restore(Placement)`가 실패하면 그 효과를 제거했다. 지금은 `Restore`가 실패할 수 있는 검증(`CanRestore`)을 앞으로 옮겨서 효과를 적용한 뒤 배치가 거부되는 경로가 없다. 그 대신 `Inventory->Restore`가 아닌 `RestoreValidated`를 부르므로 검증과 확정 사이에 가방이 바뀌지 않는다는 전제를 `bMutationLocked`가 지킨다. 이 변경은 `dbdd174`에 들어갔고 커밋 본문에 이유는 없다.

## 7. 부착 지점에서 실제 소켓까지

`CCLEquipment::GetOccupiedSlots`는 장착과 저장 검증에서 같은 점유 규칙을 제공한다. `SyncAvatar`는 현재 Fighter에 손별 ItemDefinition을 연결하고 `RefreshEquipmentVisuals`를 호출한다.

실제 소켓 해석은 다음과 같다.

출처: `d4888b0`, [CCLAttachmentProfile.cpp](../../Source/CCL/Items/CCLAttachmentProfile.cpp)의 `UCCLAttachmentProfile::Resolve`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLAttachmentProfile::Resolve(const USkeletalMesh* Mesh, FGameplayTag Point, FName& OutSocket, FString& Error) const
{
    OutSocket = NAME_None;
    Error.Reset();
    if (!Mesh || !ReferenceMesh || Mesh->GetSkeleton() != ReferenceMesh->GetSkeleton())
    {
        Error = TEXT("Attachment profile requires a mesh with the reference skeleton.");
        return false;
    }

    const FCCLAttachmentSocket* Binding = nullptr;
    for (const auto& Value : Bindings)
    {
        if (Value.Point == Point)
        {
            if (Binding)
            {
                Error = FString::Printf(TEXT("Duplicate attachment point: %s"), *Point.ToString());
                return false;
            }

            Binding = &Value;
        }
    }

    const auto* Socket = Binding ? Mesh->FindSocket(Binding->Socket) : nullptr;
    if (!Point.IsValid() || !Socket || Mesh->GetRefSkeleton().FindBoneIndex(Socket->BoneName) == INDEX_NONE)
    {
        Error = FString::Printf(TEXT("Missing attachment mapping, explicit socket or socket bone: %s"), *Point.ToString());
        return false;
    }

    OutSocket = Binding->Socket;
    return true;
}
```

참조 스켈레톤을 먼저 확인하고 지점 태그의 중복 매핑을 거부한다. 마지막으로 실제 소켓과 소켓의 본을 검사한다. 화면 미리보기와 월드 캐릭터가 같은 장비 정의를 사용해도 소유하는 렌더링 자원은 별개다.

## 8. 권한과 표현

장착 목록은 서버가 변경하고 소유자에게 복제한다. 다른 플레이어가 보는 손별 외형은 Fighter의 복제 상태로 연결된다. 가방 내용 전체를 관찰자에게 공개해야 장비가 보이는 구조는 아니다.

PlayerState의 Loadout은 재스폰 후에도 남는다. 새 Pawn에는 `SyncAvatar`로 표현을 다시 붙인다. 무기의 Source ID 행동 등록도 실제 장착 여부에 맞춰 유지한다.

## 9. 해제와 오류

`Unequip`은 유효한 빈 가방 칸을 확인한 뒤 보관 배치를 복원하고 슬롯·효과를 제거한다. 가방이 차 있으면 해제도 실패할 수 있다.

Inventory 변경으로 장비 GUID가 없어지면 `OnInventoryChanged`가 해당 효과·슬롯·행동 소스를 정리한다. 소켓 해석 실패는 경고와 외형 생략으로 드러나며 잘못된 위치에 억지로 표시하지 않는다.

## 10. 대안과 비용

| 선택 | 이점 | 비용 |
|---|---|---|
| 고정 슬롯 enum | 컴파일 시 분기가 명확 | 부위 확장과 저장 변환 필요 |
| 태그 슬롯 | 데이터로 허용 조합을 표현 | 정확한 태그 범주·점유 검증 필요 |
| 아이템의 직접 소켓 이름 | 연결 단계가 적음 | 캐릭터별 소켓 차이가 정의에 섞임 |
| 캐릭터 AttachmentProfile | 매핑을 캐릭터별로 집중 | 프로필 누락·중복·스켈레톤 검증 필요 |

태그는 잘못된 조합을 자동으로 막지 않는다. 데이터 검증과 런타임 검사를 함께 사용하는 비용을 감수한다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 검·방패에서 양손 무기로 교체 | 두 항목 반환, 효과 중복 없음 |
| 반환 공간 부족 | 기존 장착 유지 |
| 같은 GUID의 양손 저장 | 완전한 두 손 점유만 허용 |
| 잘못된 소켓·프로필 | 오류와 외형 생략 |
| 재스폰 | 같은 장비가 새 Avatar에 연결 |

기존 태그·저장·장비 검사 기록은 ProgressionFoundation에 있다. 프로필 Details의 수동 조작은 그 기록에서도 미확인으로 남아 있다. 이번에는 소스와 기록만 대조했다.

## 12. 이해 확인

**슬롯 수만큼 효과를 적용하면 왜 안 될까?**

양손 장비는 같은 아이템 하나다. 두 슬롯을 순회하며 효과를 적용하면 동일 아이템 보너스를 두 번 받을 수 있다.

**소켓이 없을 때 본 이름을 대신 사용하면 편하지 않을까?**

설정 오류가 숨겨지고 캐릭터마다 의도하지 않은 외형이 생긴다. 현재 계약은 명시적 소켓만 허용한다.
