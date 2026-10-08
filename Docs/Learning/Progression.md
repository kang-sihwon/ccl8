# 성장과 소비: 영구 효과와 생명 자원의 경계

훈련은 PlayerState의 학습 상태와 지속 효과를 만들고, 회복품은 현재 생명의 체력을 바꾼다. 둘 다 `Source/CCL/Items/CCLLoadoutComponent.cpp`에서 요청을 검증하지만 재스폰과 실패 때 다루는 상태가 다르다. 이 장에서는 비용과 효과의 적용 순서를 이해한다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 도입 기준 | `3b369682333bf541e91ee5078b03be38a87e4d75` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

`3b36968`에서 소비·훈련이 처음 구현됐다. 그 이전에 완성된 성장 시스템은 없었다. 현재까지 `Learn`의 핵심 검사·효과·비용 순서는 유지된다. 달라지지 않은 계약도 실제 전후 코드로 확인한다. 사양과 수치는 [ProgressionFoundation](../ProgressionFoundation.md)을 따른다.

## 2. 포인트와 체력의 다른 수명

훈련 포인트를 써서 최대 체력을 올린 뒤 죽었다고 훈련을 다시 배워야 해서는 안 된다. 반면 사망 직전 체력은 새 생명에 그대로 이어지지 않는다.

따라서 학습 목록과 효과 핸들은 지속 상태로, 현재 체력은 새 생명 초기화 대상으로 구분한다. UI의 ‘배웠음’ 문자열만 저장해서는 실제 Attribute 효과를 복원할 수 없다.

## 3. GameplayEffect의 기간과 성공

`Infinite` 효과는 명시적으로 제거할 때까지 유지된다. `FActiveGameplayEffectHandle`은 그런 활성 효과의 수명을 추적한다. `Instant` 효과는 즉시 수치를 바꾸므로 ‘유효한 활성 핸들이 있는가’와 ‘적용에 성공했는가’를 구분해야 한다.

현재 훈련은 Infinite 효과만 허용하고 핸들을 보관한다. 소비는 `WasSuccessfullyApplied`를 검사한다. GAS의 이런 수명 구분은 UE4에도 있었고, 이 장의 책임 배치는 프로젝트 설계다.

## 4. 최초 구현의 훈련

출처: `3b36968`, [CCLLoadoutComponent.cpp](../../Source/CCL/Items/CCLLoadoutComponent.cpp):126의 `UCCLLoadoutComponent::Learn`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLLoadoutComponent::Learn(UCCLSkillDefinition* Definition)
{
    if (!CanAct() || !Definition || !AvailableSkills.Contains(Definition) || IsLearned(Definition) ||
        Definition->PointCost <= 0 || Points < Definition->PointCost || !Definition->Effect || !FMath::IsFinite(Definition->Magnitude) ||
        Definition->Effect->GetDefaultObject<UGameplayEffect>()->DurationPolicy != EGameplayEffectDurationType::Infinite)
    {
        return Report(false, TEXT("Training unavailable or not enough points."));
    }
    const FActiveGameplayEffectHandle Handle = GetASC()->ApplyEffect(Definition->Effect, Definition->Magnitude);
    if (!Handle.IsValid())
    {
        return Report(false, TEXT("Training effect failed."));
    }
    SkillEffects.Add(Handle);
    Learned.Add(Definition);
    Points -= Definition->PointCost;
    return Report(true, TEXT("Training learned."));
}
```

서버에서 선택 가능한 스킬인지, 이미 배웠는지, 비용과 효과가 유효한지 먼저 확인한다. 효과 적용에 성공한 뒤에만 목록과 포인트를 바꾼다. 비용을 먼저 빼고 실패 보상을 따로 구현하는 구조가 아니다.

## 5. 유지한 계약과 추가 요구

이후 장비가 여러 부위로 확대되고 퀘스트 보상이 훈련 포인트로 이어졌어도 학습 성공 순서는 유지됐다. [ContentFoundation](../ContentFoundation.md)은 귀환 보상으로 포인트를 주는 콘텐츠를 기록한다.

당시 `Learn`의 세부 검사 순서를 선택한 독립적인 결정 이유는 미기록이다. 효과 적용 전제와 후속 상태 변경을 묶는 것이 부분 성공을 줄인다는 설명은 코드에 대한 분석이다.

## 6. 현재의 훈련과 회복

출처: `50e5838`, [CCLLoadoutComponent.cpp](../../Source/CCL/Items/CCLLoadoutComponent.cpp):279의 `UCCLLoadoutComponent::Learn`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLLoadoutComponent::Learn(UCCLSkillDefinition* Definition)
{
    if (!CanAct() || !Definition || !AvailableSkills.Contains(Definition) || IsLearned(Definition) ||
        Definition->PointCost <= 0 || Points < Definition->PointCost || !Definition->Effect || !FMath::IsFinite(Definition->Magnitude) ||
        Definition->Effect->GetDefaultObject<UGameplayEffect>()->DurationPolicy != EGameplayEffectDurationType::Infinite)
    {
        return Report(false, TEXT("Training unavailable or not enough points."));
    }
    const FActiveGameplayEffectHandle Handle = GetASC()->ApplyEffect(Definition->Effect, Definition->Magnitude);
    if (!Handle.IsValid())
    {
        return Report(false, TEXT("Training effect failed."));
    }
    SkillEffects.Add(Handle);
    Learned.Add(Definition);
    Points -= Definition->PointCost;
    return Report(true, TEXT("Training learned."));
}
```

첫 구현과 같은 검사·효과·비용 계약을 볼 수 있다. 새 기능이 추가됐다는 이유로 이 함수의 의미까지 바뀌었다고 가정하지 않는다.

회복은 별도 함수로 처리한다.

출처: `50e5838`, [CCLLoadoutComponent.cpp](../../Source/CCL/Items/CCLLoadoutComponent.cpp):253의 `UCCLLoadoutComponent::Use`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLLoadoutComponent::Use(FGuid Id)
{
    if (!CanAct())
    {
        return Report(false, TEXT("Cannot use an item now."));
    }
    auto* Inventory = GetInventory();
    const auto* Entry = Inventory ? Inventory->Find(Id) : nullptr;
    const auto* Fragment = Entry && Entry->Definition ? Entry->Definition->FindFragment<FCCLItemFragment_ConsumableData>() : nullptr;
    auto* ASC = GetASC();
    if (!Fragment || !Fragment->Effect || !FMath::IsFinite(Fragment->Magnitude) || Fragment->Magnitude <= 0.f)
    {
        return Report(false, TEXT("Select a recovery item."));
    }
    if (ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) >= ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()))
    {
        return Report(false, TEXT("Health is already full."));
    }
    if (!ASC->ApplyEffect(Fragment->Effect, Fragment->Magnitude).WasSuccessfullyApplied())
    {
        return Report(false, TEXT("Item effect failed."));
    }
    Inventory->Remove(Id, 1);
    return Report(true, TEXT("Recovery item used."));
}
```

보유 아이템의 ConsumableData를 조회하고 체력이 이미 최대이면 소비하지 않는다. 효과 적용 성공 후 아이템 하나를 제거한다. 선택한 UI 슬롯 번호 대신 소유 GUID로 다시 조회한다.

## 7. 재스폰과 저장 내부 연결

Character의 `InitializeAbilitySystem`은 `SyncAvatar(true)`를 호출한다. 장착·훈련으로 최대 체력이 변한 ASC에 새 Avatar를 붙인 뒤 현재 체력을 최대치로 맞춘다.

저장 복원에서는 `Loadout::Restore`가 이전 효과를 정리하고 저장된 스킬을 다시 적용한다. 학습 목록 자체와 효과 인스턴스를 별도로 취급하기 때문이다. 이 함수 전체가 임의 실패에 완전 롤백된다고 가정해서는 안 되며, 세션은 새 월드에서 적용하고 실패하면 메뉴로 돌아간다.

## 8. 권한과 수명

`CanAct`는 서버 권한, 유효한 Avatar와 Dead·Busy·Stagger 태그를 확인한다. AvailableSkills에 없는 자산이나 임의 비용을 클라이언트가 지정해 학습할 수는 없다.

Learned·Points는 소유자에게 복제한다. 화면은 복제 결과를 읽고 서버 판정을 대신하지 않는다. 학습 효과는 PlayerState의 ASC를 따라 생명 교체를 넘어 유지된다.

## 9. 종료와 실패

포인트 부족·중복 학습·효과 생성 실패에서는 목록과 비용을 변경하지 않는다. 소비도 가득 찬 체력이나 부적합한 Fragment를 거부한다.

Loadout의 `EndPlay`는 Inventory 구독을 해제하고 서버에서 보관한 장비·스킬 효과를 제거한다. 화면을 닫는 시점과 기능 소유자가 종료되는 시점은 다르다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 수치를 직접 증가 | 작은 성장 기능이 단순 | 원인별 제거·저장 복원·중복 방지 직접 관리 |
| 지속 효과 핸들 추적 | 장비·훈련의 원인을 구분 | 효과 기간·핸들·ASC 수명 이해 필요 |
| Pawn에 학습 저장 | 한 객체에서 조회 | 재스폰마다 복원 필요 |

현재 스킬 목록은 작고 고정된 프로토타입이다. 일반적인 분기 스킬 트리·선행 조건 편집기를 제공하지 않는다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 비용 부족·같은 스킬 재학습 | 거부, 상태 유지 |
| 학습 성공 | 효과 한 번, 비용 한 번 |
| 체력 가득 찬 상태에서 소비 | 수량 유지 |
| 학습·장비 후 사망·재스폰 | 성장 유지, 새 생명의 체력 초기화 |
| 별도 프로세스 저장 복원 | 학습 상태와 실제 Attribute 일치 |

기존 성장·재스폰·저장 검사는 ProgressionFoundation과 [SessionFoundation](../SessionFoundation.md)에 기록돼 있다. 이번에는 실행하지 않고 커밋과 기록을 대조했다.

## 12. 이해 확인

**소비 효과도 활성 핸들의 IsValid만 검사하면 될까?**

즉시 효과는 활성 상태로 남지 않을 수 있다. 적용 성공과 지속 효과의 핸들 수명을 구분해야 한다.

**재스폰 때 최대 체력을 다시 기본값으로 덮으면 어떻게 될까?**

유지해야 할 학습·장비 효과와 충돌한다. 현재 구현은 효과가 반영된 최대 체력을 기준으로 현재 체력을 복구한다.
