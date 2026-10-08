# 아이템과 인벤토리: 공유 정의와 개인 상태를 나누기

`ItemDefinition`은 아이템 종류의 공통 설정을, `InventoryComponent`는 실제 보유 항목의 GUID·수량·가방 칸을 소유한다. `Source/CCL/Items/`를 읽으며 정의에 상태를 넣지 않는 이유와 Fast Array의 변경 표시를 이해한다. 장착 판정은 [장비](Equipment.md), 표시와 드래그는 [기존 InventoryUI 예시](InventoryUI.md)로 이어진다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `3b369682333bf541e91ee5078b03be38a87e4d75` |
| 도입·변경 기준 | `6a9cecb09da31030d54f28c1da59c73a393f402d` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전은 UObject Fragment와 초기 보관 구현, 변경 기준은 구조체 Fragment·슬롯 체계 도입이다. 이후 탄약과 거래 알림 보류가 추가된 현재까지 설명한다. 근거 결정은 15·16이며 사양은 [ProgressionFoundation](../ProgressionFoundation.md)이 소유한다.

## 2. 같은 포션과 서로 다른 항목

플레이어 두 명이 같은 포션 정의를 사용해도 수량은 달라야 한다. 정의 에셋에 수량을 쓰면 같은 정의를 참조하는 다른 소유자에게도 상태가 섞인다.

장착한 검도 소유 목록에서 사라진 물건이 아니다. 현재는 같은 GUID를 유지하고 가방 칸만 `INDEX_NONE`으로 바꾼다. 소유, 가방 배치와 장착을 별개 관계로 표현하는 것이다.

## 3. DataAsset, Fragment와 Fast Array

`UPrimaryDataAsset`은 공유 설정을 담는 UObject 자산이다. Fragment는 선택 기능 단위이며 이 프로젝트에서는 `FInstancedStruct`로 값과 UScriptStruct 타입을 함께 보관한다. 이름·설명·중첩 수는 Definition의 공통 필드다.

`FInstancedStruct::GetPtr<T>`는 보관한 구조체를 지정 타입으로 조회한다. `TObjectPtr`는 UObject 참조이고 구조체의 값 저장과는 수명 모델이 다르다. 엔진의 실제 API 근거는 [엔진 근거](EngineEvidence.md)에 있다.

`FFastArraySerializer`는 항목 변경을 명시하는 복제 방식이다. 항목 수정·추가에는 `MarkItemDirty`, 삭제에는 `MarkArrayDirty`가 필요하다. Fast Array 자체는 UE5에서 처음 나온 기능이 아니다. 구조체 Fragment로의 전환은 프로젝트 결정이다.

## 4. 이전의 보관 추가

출처: `3b36968`, [CCLInventoryComponent.cpp](../../Source/CCL/Items/CCLInventoryComponent.cpp):17의 `UCCLInventoryComponent::Add`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
FGuid UCCLInventoryComponent::Add(UCCLItemDefinition* Definition, int32 Quantity)
{
    if (!GetOwner()->HasAuthority() || !CanAdd(Definition, Quantity))
    {
        return FGuid();
    }
    for (auto& Entry : List.Entries)
    {
        if (Entry.Definition == Definition && Quantity <= Definition->GetMaxStack() - Entry.Quantity)
        {
            Entry.Quantity += Quantity;
            List.MarkItemDirty(Entry);
            const FGuid Id = Entry.Id;
            OnChanged.Broadcast();
            return Id;
        }
    }
    auto& Entry = List.Entries.AddDefaulted_GetRef();
    Entry.Id = FGuid::NewGuid();
    Entry.Definition = Definition;
    Entry.Quantity = Quantity;
    List.MarkItemDirty(Entry);
    const FGuid Id = Entry.Id;
    OnChanged.Broadcast();
    return Id;
}
```

이전에도 서버 권한·중첩 검사와 GUID, 변경 알림이 있었다. 후속 가방 슬롯 기능이 추가됐다고 기본 보관 전체를 새로 만들었다고 설명할 필요는 없다.

## 5. 변경 계기와 데이터 변환

결정 15는 공통 정보를 Definition으로 옮기고 선택 기능을 구조체 Fragment로 구성하도록 정했다. 결정 16은 슬롯과 부착 위치를 태그로 확장했다.

`UCCLItemDefinition::PostLoad`는 이전 UObject Fragment를 읽어 새 구조체로 옮기고 이전 배열을 비운다. `FragmentSchemaVersion`은 반복 변환을 막는다. 기존 클래스 선언이 남아 있는 이유는 호환 로드이며 새 편집 방식이 여전히 UObject Fragment라는 뜻은 아니다.

## 6. 현재의 보관 추가

출처: `50e5838`, [CCLInventoryComponent.cpp](../../Source/CCL/Items/CCLInventoryComponent.cpp):18의 `UCCLInventoryComponent::Add`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
FGuid UCCLInventoryComponent::Add(UCCLItemDefinition* Definition, int32 Quantity)
{
    if (!GetOwner()->HasAuthority() || !CanAdd(Definition, Quantity))
    {
        return FGuid();
    }
    for (auto& Entry : List.Entries)
    {
        if (Entry.Slot >= 0 && Entry.Definition == Definition && Quantity <= Definition->GetMaxStack() - Entry.Quantity)
        {
            Entry.Quantity += Quantity;
            List.MarkItemDirty(Entry);
            const FGuid Id = Entry.Id;
            NotifyChanged();
            return Id;
        }
    }
    int32 FreeSlot = 0;
    while (FindSlot(FreeSlot))
    {
        ++FreeSlot;
    }

    auto& Entry = List.Entries.AddDefaulted_GetRef();
    Entry.Slot = FreeSlot;
    Entry.Id = FGuid::NewGuid();
    Entry.Definition = Definition;
    Entry.Quantity = Quantity;
    List.MarkItemDirty(Entry);
    const FGuid Id = Entry.Id;
    NotifyChanged();
    return Id;
}
```

장착 항목의 `Slot`은 음수이므로 가방 중첩 대상으로 사용하지 않는다. 들어갈 기존 스택이 없으면 비어 있는 칸을 찾아 새 항목을 만들고 변경을 표시한다.

`CanAdd`는 요청 수량이 한 스택 한도를 넘으면 거부한다. 큰 수량을 여러 칸으로 자동 분할하는 일반화된 알고리즘으로 읽으면 안 된다. 가방에 들어가는 항목 수와 장착된 소유 항목 수도 구분한다.

## 7. 이동·픽업·알림 연결

`MoveToSlot`은 서버에서 원본 GUID와 대상 칸을 검사한다. 대상이 차 있으면 두 항목의 칸 번호를 교환하고 각각 Dirty로 표시한다. 화면이 배열 인덱스를 바꿨다는 사실만으로 서버 배치가 변경되지는 않는다.

`ACCLWorldPickup::TryCollect`는 거리·차폐를 확인하고 `bCollected`를 먼저 설정한다. 보관 추가가 실패하면 이를 되돌리고, 성공하면 월드 Actor를 제거한다. 같은 픽업이 중복 지급되지 않게 하는 순서다.

`NotifyChanged`는 일반 상황에서 즉시 델리게이트를 호출한다. 상점 구매처럼 여러 상태를 맞추는 중에는 `BeginTransaction`과 `EndTransaction`으로 알림을 보류한다. 이는 자동 롤백 API가 아니므로 호출자가 실패 복원을 맡는다.

## 8. 권한과 참조 수명

변경은 소유 Actor의 서버 권한을 검사하고 목록은 `COND_OwnerOnly`로 복제한다. Inventory는 Character나 UI를 요구하지 않아 일반 Actor에도 붙일 수 있다.

GUID는 항목의 정체성이고 배열 요소 주소는 안정적인 장기 참조가 아니다. 변경 콜백이 배열을 바꿀 수 있어 Add는 알림 전에 GUID를 복사해 반환한다. 클라이언트 UI도 GUID를 전달하고 서버에서 다시 조회한다.

## 9. 삭제와 복원 실패

수량이 0이 되면 항목을 삭제하고 배열 변경을 표시한다. Loadout은 보관 변경을 구독해 사라진 장비의 효과·슬롯 참조를 정리한다.

`Restore`는 모든 후보의 중복 GUID, 칸, 수량과 탄창 범위를 검사한 후 기존 목록을 비운다. 후보 검증 도중 실패하면 그 함수는 기존 목록을 교체하지 않는다. 전체 세션 복원의 원자성까지 보장하는 것은 아니므로 [세션·저장](SessionSave.md)의 실패 경계를 함께 읽는다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 종류별 아이템 클래스 | 특수 동작 구현이 직접적 | 조합 수가 늘면 클래스 계층 확대 |
| UObject Fragment | 객체 기반 확장·인라인 편집 | 개별 객체와 참조 수명 관리 |
| FInstancedStruct Fragment | 값 기능을 하나의 배열에서 편집 | 타입 조회·변환·검증 필요 |
| 전체 배열 복제 | 구현 이해가 단순 | 항목 변화의 명시적 추적을 별도 고려 |

현재 방식은 보관과 기능 실행의 분리에 초점을 둔다. Fast Array를 사용했다는 사실만으로 대규모 인벤토리 성능이 측정된 것은 아니다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 동일 정의를 두 소유자에게 지급 | 각 GUID·수량 독립 |
| 중첩 한도 초과·가방 부족 | 거부 |
| 장착 항목 보관 | 소유 GUID 유지, 가방 칸 없음 |
| 슬롯 교환·삭제 | 소유자 복제와 변경 통지 |
| 잘못된 복원 목록 | 기존 목록 보존 |

ProgressionFoundation과 AgentFoundation의 기존 검사 기록을 대조했다. 이번에는 코드·발췌·링크를 검사하며 게임 빌드나 실행은 새로 하지 않았다.

## 12. 이해 확인

**왜 정의 에셋에 LoadedAmmo를 두지 않을까?**

같은 무기 종류의 개별 항목마다 탄창 수가 다르기 때문이다. 탄창은 GUID별 항목에 저장한다.

**거래 중 NotifyChanged를 보류하는 이유는 무엇일까?**

가방만 바뀌고 계정이 아직 확정되지 않은 중간 상태를 구독자가 처리하지 않게 하기 위해서다. 실패 데이터의 복원은 호출자가 별도로 해야 한다.
