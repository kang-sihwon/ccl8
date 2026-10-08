# 장비: 슬롯 점유와 메시 부착을 분리하기

장비 교체는 새 장비가 차지할 부위와 이전 장비가 돌아갈 가방 공간을 먼저 계산한다. 외형은 부착 지점 태그를 캐릭터의 실제 소켓으로 해석한다. `Source/CCL/Items/CCLLoadoutComponent.cpp`, `CCLItemTags.cpp`, `CCLAttachmentProfile.cpp`에서 이 두 판단의 경계를 배운다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `3b369682333bf541e91ee5078b03be38a87e4d75` |
| 도입·변경 기준 | `6a9cecb09da31030d54f28c1da59c73a393f402d` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전은 단일 장비 GUID·효과, 변경 기준은 구조체 Fragment와 태그 슬롯·양손 점유 구현이다. 현재 계약은 [ProgressionFoundation](../ProgressionFoundation.md), 요구 이유는 [DesignLog](../DesignLog.md)의 결정 15·16을 따른다.

## 2. 양손 장비 교체의 문제

오른손 검과 왼손 방패를 양손 무기로 바꾸면 기존 두 장비를 가방으로 돌려보내야 한다. 빈 칸이 부족할 때 절반만 교체하면 슬롯·보관·효과가 서로 다른 상태가 된다.

또한 양손 점유는 손이 두 개라는 규칙이고 메시를 두 번 생성한다는 뜻이 아니다. 현재는 하나의 GUID가 두 손을 점유하고 메시 하나를 오른손에 표시한다.

## 3. 태그, GUID와 효과 핸들

`FGameplayTag`는 등록된 계층형 이름이다. `Equipment.Slot.Hand.Right` 같은 구체적인 슬롯과 상위 분류 태그를 구분해야 한다. Equip은 허용·기본 슬롯을, Weapon은 한손·양손 사용 방식을 소유한다.

GUID는 실제 소유 아이템 하나를 식별한다. `FActiveGameplayEffectHandle`은 그 아이템 때문에 ASC에 적용된 효과 하나를 추적한다. 같은 GUID가 두 손에 있어도 효과 핸들은 한 번만 적용한다.

`AttachmentProfile`은 프로젝트 데이터 자산이다. GameplayTag와 소켓 기능은 UE4에도 있었으며 프로필을 도입한 것은 프로젝트 설계 변경이다.

## 4. 이전의 단일 장비

출처: `3b36968`, [CCLLoadoutComponent.cpp](../../Source/CCL/Items/CCLLoadoutComponent.cpp):90의 `UCCLLoadoutComponent::Equip`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

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

출처: `50e5838`, [CCLLoadoutComponent.cpp](../../Source/CCL/Items/CCLLoadoutComponent.cpp):100의 `UCCLLoadoutComponent::Equip`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

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

    TArray<FCCLInventoryEntry> Placement = Inventory->GetEntries();
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
            return Report(false, TEXT("Equipment state is unavailable."));
        }

        int32 Free = 0;
        while (Placement.ContainsByPredicate([Free](const auto& Item) { return Item.Slot == Free; }))
        {
            ++Free;
        }

        if (Free >= Inventory->Capacity)
        {
            return Report(false, TEXT("Not enough inventory space to return equipment."));
        }

        Value->Slot = Free;
    }
```

실제 가방 복사본에서 새 장비의 칸을 먼저 비우고 반환할 항목마다 빈 칸을 찾는다. 공간이 부족하면 원본 슬롯과 효과를 교체하기 전에 실패한다. 새 장비가 비운 칸도 반환 공간으로 이용할 수 있다.

이후 새 효과를 만들고 보관 배치를 적용한다. 보관 복원이 실패하면 새 효과를 제거한다. 성공하면 반환된 장비의 효과를 제거하고 EquipmentSlots를 교체한 뒤 Avatar를 동기화한다.

## 7. 부착 지점에서 실제 소켓까지

`CCLEquipment::GetOccupiedSlots`는 장착과 저장 검증에서 같은 점유 규칙을 제공한다. `SyncAvatar`는 현재 Fighter에 손별 ItemDefinition을 연결하고 `RefreshEquipmentVisuals`를 호출한다.

실제 소켓 해석은 다음과 같다.

출처: `50e5838`, [CCLAttachmentProfile.cpp](../../Source/CCL/Items/CCLAttachmentProfile.cpp):8의 `UCCLAttachmentProfile::Resolve`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

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
