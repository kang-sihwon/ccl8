# 무기 행동과 발사체: 발사 순간의 상태를 보존하기

무기의 행동은 아이템 GUID를 Source ID로 등록하고 GAS 능력을 실행한다. 발사체는 발사 당시의 피해·팀·효과를 보존해 발사자 Actor가 사라져도 적중을 처리한다. `Source/CCL/Actions/`와 `Combat/CCLProjectile.cpp`에서 능력 소유와 날아가는 탄환의 수명을 나누는 이유를 배운다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `68672713e47b19b0d4adb51bdc21c7c9bbe78eb6` |
| 도입·변경 기준 | `ed9a763db54edd93c7fe802b887ee96b553bb0df` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전 기준에는 근접 GAS와 무기 정의가 있었지만 현재의 발사체 실행 경로는 없었다. `ed9a763`이 Source 기반 Action과 지속 발사체를 도입했다. 구조 계약은 [AgentFoundation](../AgentFoundation.md), 장비 소유는 [ProgressionFoundation](../ProgressionFoundation.md)이 소유한다.

## 2. 발사자보다 오래 사는 탄환

플레이어가 총을 발사한 직후 무기를 바꾸거나 죽어도 이미 날아간 탄환의 피해가 다른 무기의 값으로 바뀌어서는 안 된다. 반대로 아직 완료하지 않은 장전은 취소되면 탄약을 옮기면 안 된다.

현재 구현은 능력을 준 Source, 능력 실행, 발사된 Actor, 개별 아이템 탄창을 각각 구분한다.

## 3. SpecHandle과 Source ID

`FGameplayAbilitySpecHandle`은 ASC에 부여된 능력의 식별자다. `FCCLActionSource`는 Source ID와 행동 태그별 SpecHandle을 묶는 프로젝트 타입이다. 같은 Ability 클래스를 쓰는 무기도 소스를 구분해서 제거할 수 있다.

`GameplayEffectSpec`은 실행에 사용할 효과 인스턴스 정보다. `DuplicateObject`는 원본 UObject 정의를 복제한다. 발사체가 공유 DataAsset을 직접 수정하지 않도록 복사본을 만든다.

`UProjectileMovementComponent`는 발사체 이동을 제공한다. GAS·ProjectileMovement 자체는 UE4에도 있었으며 Source ID 계약은 프로젝트가 추가한 구조다.

## 4. 이전의 공통 능력 부여

이전 AbilitySet은 같은 클래스가 이미 부여돼 있으면 건너뛰었다.

출처: `6867271`, [CCLAbilitySet.cpp](../../Source/CCL/AbilitySystem/CCLAbilitySet.cpp):23의 `UCCLAbilitySet::GrantTo`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
    for (const auto& Grant : Abilities)
    {
        if (!Grant.Ability || ASC.FindAbilitySpecFromClass(Grant.Ability))
        {
            continue;
        }

        FGameplayAbilitySpec Spec(Grant.Ability, 1, INDEX_NONE, Source);
        Spec.GetDynamicSpecSourceTags().AddTag(Grant.InputTag);
        ASC.GiveAbility(Spec);
    }
```

이는 기본 공격·회피 같은 공통 능력을 한 번 부여하는 데 맞는다. 개별 무기의 행동을 별도로 추가·해제하려면 클래스 중복 검사만으로는 소유를 표현하기 어렵다.

## 5. 변경 이유

결정 18과 AgentFoundation은 플레이어·NPC·적이 공통 Action/GAS 진입점을 사용하고 근접·투사체 실행기를 연결하도록 정했다. 소스별 등록은 개별 장비 교체 때 그 장비가 준 능력만 제거하기 위한 구조다.

발사체의 캡처 전략이 선택된 세부 논의는 별도 결정 로그에 없다. 발사자 수명과 발사 이후 피해의 독립성은 구현과 발사자 제거 검사가 뒷받침한다.

## 6. Source 등록과 제거

`RegisterSource`는 권한·ID·정의·중복 행동 태그를 검사하고 능력을 부여해 핸들을 기록한다. `Execute`는 해당 Source의 해당 행동 핸들을 찾아 활성화한다.

출처: `50e5838`, [CCLActionComponent.cpp](../../Source/CCL/Actions/CCLActionComponent.cpp):61의 `UCCLActionComponent::RemoveSource`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void UCCLActionComponent::RemoveSource(FGuid Id)
{
    const int32 Index = Sources.IndexOfByPredicate([Id](const FCCLActionSource& Source) { return Source.Id == Id; });
    if (Index == INDEX_NONE)
    {
        return;
    }

    const auto Handles = Sources[Index].Handles;
    Sources.RemoveAt(Index);
    if (auto* ASC = GetASC(); ASC && GetOwner()->HasAuthority())
    {
        for (const auto& Pair : Handles)
        {
            ASC->CancelAbilityHandle(Pair.Value);
            ASC->ClearAbility(Pair.Value);
        }
    }
}
```

먼저 소스 목록에서 제거하고 복사한 핸들로 실행을 취소·해제한다. 취소 콜백이 재진입해도 같은 Source가 여전히 등록된 것으로 보이지 않게 한다. 기존 예측 근접 입력은 `RequestInput` 어댑터를 통해 ASC로 전달한다.

## 7. 실제 발사와 공통 적중

발사 시에는 다음 값을 탄환에 캡처한다.

출처: `50e5838`, [CCLProjectile.cpp](../../Source/CCL/Combat/CCLProjectile.cpp):55의 `ACCLProjectile::Launch`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void ACCLProjectile::Launch(UAbilitySystemComponent* ASC, const UCCLCombatDefinition* Definition,
    FVector Velocity, float Gravity)
{
    if (!HasAuthority() || !ASC || !Definition || Velocity.ContainsNaN() || !FMath::IsFinite(Gravity))
    {
        Destroy();
        return;
    }

    SourceASC = ASC;
    ShotDefinition = DuplicateObject<UCCLCombatDefinition>(Definition, this);
    CapturedEffect = ASC->MakeOutgoingSpec(Definition->DamageEffect, 1.f, ASC->MakeEffectContext());
    if (const auto* Fighter = GetOwner() ? GetOwner()->FindComponentByClass<UCCLFighterComponent>() : nullptr)
    {
        CapturedTeam = Fighter->Team;
    }
    if (ASC->HasAttributeSetForAttribute(UCCLOffenseSet::GetAttackBonusAttribute()))
    {
        ShotDefinition->Damage += ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute());
    }

    Movement->ProjectileGravityScale = Gravity;
    Movement->Velocity = Velocity;
    Movement->MaxSpeed = 0;
}
```

피해 보너스도 발사 시 복사한 정의에 반영한다. 적중 때 현재 무기의 보너스를 다시 더하면 이미 캡처한 값을 중복 적용할 수 있으므로 DetachedShot 경로를 구분한다.

출처: `50e5838`, [CCLProjectile.cpp](../../Source/CCL/Combat/CCLProjectile.cpp):81의 `ACCLProjectile::Impact`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void ACCLProjectile::Impact(UPrimitiveComponent* HitComponent, AActor* Other,
    UPrimitiveComponent* OtherComponent, FVector Impulse, const FHitResult& Hit)
{
    if (!HasAuthority() || bResolved || Other == GetOwner())
    {
        return;
    }

    bResolved = 1;
    auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other);
    if (CapturedEffect.IsValid() && ShotDefinition && TargetASC && IsValid(Other))
    {
        FCCLHitContext Context{IsValid(GetOwner()) ? GetOwner() : this, Other,
            IsValid(SourceASC) ? SourceASC.Get() : nullptr, TargetASC, ShotDefinition, Hit, 0};
        Context.bDetachedShot = 1;
        Context.CapturedEffect = CapturedEffect;
        Context.CapturedTeam = CapturedTeam;
        Context.IncomingDirection = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
        CCLHit::Apply(Context);
    }

    Destroy();
}
```

발사자가 유효하지 않으면 발사체 자체를 문맥 Source로 사용하고 캡처한 효과로 공통 `CCLHit::Apply`에 전달한다. Target의 현재 Avatar와 정책은 여전히 검증한다. 발사자가 사라졌다고 모든 방어 검사를 건너뛰는 구조가 아니다.

## 8. 서버와 탄약 상태

현재 발사체 이동·충돌·피해는 서버가 수행한다. 클라이언트의 발사체는 충돌을 끄고 복제 이동을 표시한다. 기존 근접의 LocalPredicted 정책과 모든 총기 동작의 예측을 동일시하지 않는다.

LoadedAmmo는 Inventory의 GUID별 상태다. 발사는 `ConsumeShot`으로 탄창을 줄이고, 장전 완료 경로는 `Reload`에서 가방 탄약과 탄창의 후보 배열을 만든 뒤 복원한다. 탄약은 공유 무기 정의에 저장하지 않는다.

## 9. 취소와 단일 적중

`bResolved`는 첫 충돌에서 설정되며 적중 처리 뒤 Actor를 제거한다. 벽처럼 ASC가 없는 대상과 충돌해도 탄환을 제거한다. 유효하지 않은 발사 인자와 수명 만료도 종료 경로다.

무기 해제는 Source의 능력을 취소하지만 이미 발사된 탄환의 캡처 상태를 소급해서 지우지 않는다. 장전은 완료 때 탄약을 이전하므로 중단 시 미리 지급한 탄창을 되돌리는 경로에 의존하지 않는다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 즉시 Hitscan | 객체·비행 상태가 적음 | 비행 시간·낙차 표현이 다름 |
| 발사자 상태를 적중 때 조회 | 캡처 데이터가 적음 | 교체·사망이 이미 발사한 피해에 영향 |
| 서버 발사체 + 발사 시 캡처 | 발사 이후 상태가 명확 | Actor 복제·충돌 비용, 예측 표현 추가 과제 |

현재 외형·총성·총구 애니메이션은 최종 콘텐츠가 아니다. 이 구조가 모든 무기 유형의 완성도를 보장하지 않는다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 발사 직후 발사자 제거 | 유효 대상에 캡처된 피해 적용 |
| 중복 충돌 | 한 번 처리 |
| 무기 교체·해제 | 해당 Source 능력만 정리 |
| 장전 중 취소 | 완료 전 탄약 이전 없음 |
| 저장·복원 | GUID별 탄창 유지, 범위 초과 거부 |

기존 ProjectileSmoke와 장비·전투 검사 기록은 AgentFoundation에 있다. 이번에는 코드와 기록을 대조했으며 발사 테스트를 새로 수행하지 않았다.

## 12. 이해 확인

**발사체가 현재 Loadout을 조회하면 어떤 문제가 생길까?**

발사 후 무기 교체로 피해·팀 정보의 기준이 바뀔 수 있다. 현재는 발사 당시 상태를 캡처한다.

**Source ID와 Ability 클래스는 왜 둘 다 필요할까?**

클래스는 행동 구현의 종류이고 Source ID는 그 능력을 제공한 개별 물건이다. 같은 구현을 여러 소스가 제공할 때 제거 대상을 구분한다.
