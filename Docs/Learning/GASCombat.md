# GAS 전투: 자원 수명과 한 번의 공격을 분리하기

현재 전투는 PlayerState의 ASC가 능력과 자원을 유지하고, Character의 Fighter와 CombatComponent가 현재 생명의 실행을 연결한다. 이 장의 목표는 `Source/CCL/AbilitySystem/`과 `Combat/`에서 입력, 예측, 적중 검증, 효과 적용, 취소가 이어지는 이유를 설명하는 것이다. 발사체는 [무기 행동](Weapons.md), 적의 선택은 [적 AI](EnemyAI.md)에서 다룬다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `c8c6e4a27234bf2a78ca4c6f49c230e6badd193a` |
| 도입·변경 기준 | `4238990815cc932a6f38c55f947be78065389651` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전 기준에는 양수 피해로 바로 죽는 이동 검증용 Character가 있었다. `4238990`에서 GAS 전투가 도입됐고 `ed9a763`에서 근접과 발사체가 공통 적중 적용 경로를 사용하게 됐다. 후자의 전체 해시는 [무기 행동](Weapons.md)에 있다. 수치와 과거 검증은 [CombatFoundation](../CombatFoundation.md)이 소유한다.

## 2. 공격 한 번에 필요한 판단

공격 버튼을 누르면 자기 화면에서는 빠르게 반응해야 하지만 피해는 서버에서 한 번만 확정해야 한다. 회피·패링 유효 구간과 스태미나 비용도 함께 처리해야 한다. 플레이어가 죽는 순간에는 아직 기다리는 공격 타이머가 다음 생명을 건드려서는 안 된다.

결정 11은 이 문제를 ASC의 지속 수명과 Pawn의 생명 수명으로 나눴다. 여기서 ‘능력이 남는다’와 ‘실행 중인 공격이 남는다’는 서로 다른 요구다.

## 3. ASC, Attribute, Ability와 Effect

`AbilitySystemComponent`(ASC)는 부여된 능력, 실행과 효과를 관리한다. `AttributeSet`은 체력·스태미나 같은 수치의 정의와 변경 규칙을 제공한다. `GameplayAbility`는 공격·가드처럼 시작과 종료가 있는 행동이고 `GameplayEffect`는 수치와 태그를 적용하는 정의다. GAS는 UE4에도 있었으며 이 프로젝트의 도입 자체가 UE5에서 새로 생긴 기능은 아니다.

`OwnerActor`는 ASC의 논리적 소유자이고 `AvatarActor`는 현재 행동하는 몸이다. 플레이어는 PlayerState와 Character가 각각 그 역할을 맡는다. 적은 자신의 Actor가 두 역할을 맡는다.

`LocalPredicted`는 소유 클라이언트가 행동을 먼저 예측 실행하는 정책이다. 적의 체력을 클라이언트가 확정한다는 뜻은 아니다. `AbilityTask`는 지연·몽타주·입력 해제처럼 Ability 실행 중 기다릴 작업을 캡슐화한다.

## 4. 이전의 피해 처리

이전 Character는 유효한 양수 피해에 `Die`를 직접 호출했다.

출처: `c8c6e4a`, [CCLCharacter.cpp](../../Source/CCL/CCLCharacter.cpp):53의 `ACCLCharacter::TakeDamage`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
float ACCLCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (!HasAuthority() || IsDead() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.f)
    {
        return 0.f;
    }
    const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (AppliedDamage > 0.f)
    {
        Die();
    }
    return AppliedDamage;
}
```

현재는 피해가 체력 효과로 들어가고 체력 변경 알림에서 사망을 판정한다. 이렇게 연결하면 직접 피해와 무기 효과가 같은 체력에 반영되며 HUD도 동일한 Attribute를 읽을 수 있다.

## 5. 변경 이유

[DesignLog](../DesignLog.md)의 결정 8·10·11은 회피·가드·패링, 현행 엔진 시스템 사용, PlayerState 소유 ASC를 요구한다. 장르 공통 적중 검증과 소울라이크 방어 정책을 분리한다는 방향은 결정 9와 CombatFoundation에 기록돼 있다.

코드에 대한 분석으로 보면 공통 CombatComponent가 가드 각도나 패링 비용까지 알면 다른 피해 대상에 재사용하기 어려워진다. 현재는 검증한 적중 문맥을 정책에 전달하고, 정책이 적용할 효과와 결과 태그를 정한다.

## 6. ASC를 새 몸에 연결

서버의 `PossessedBy`와 클라이언트의 `OnRep_PlayerState`는 다음 함수로 들어온다.

출처: `50e5838`, [CCLCharacter.cpp](../../Source/CCL/CCLCharacter.cpp):188의 `ACCLCharacter::InitializeAbilitySystem`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void ACCLCharacter::InitializeAbilitySystem()
{
    auto* ASC = Cast<UCCLAbilitySystemComponent>(GetAbilitySystemComponent());

    if (!ASC || BoundASC.Get() == ASC)
    {
        return;
    }

    if (BoundASC.IsValid())
    {
        BoundASC->GetGameplayAttributeValueChangeDelegate(UCCLHealthSet::GetHealthAttribute()).Remove(HealthChanged);
    }

    ASC->InitAbilityActorInfo(GetPlayerState(), this);
    Fighter->Initialize(ASC);
    if (auto* State = GetPlayerState<ACCLPlayerState>())
    {
        State->GetLoadout()->SyncAvatar(true);
    }
    BoundASC = ASC;
    HealthChanged = ASC->GetGameplayAttributeValueChangeDelegate(UCCLHealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}
```

같은 ASC라면 중복 구독을 피하고, 다른 ASC라면 이전 체력 델리게이트를 해제한다. `InitAbilityActorInfo` 다음에 Fighter 초기화와 장비 동기화를 실행한다. 체력 델리게이트는 해당 ASC를 기록한 뒤 연결한다.

Fighter의 `Initialize`는 능력·세트·생명 자원을 준비한다. `UCCLAbilitySet::GrantTo`는 서버에서 AttributeSet과 Ability를 부여하고 클래스 중복을 검사한다. 공유 DataAsset 자체에 플레이어별 현재 체력을 쓰지 않는다.

## 7. 공격과 적중의 내부 연결

`UCCLCombatAbility::ActivateAbility`는 비용을 확정한 뒤 Busy 효과와 행동 상태를 설정한다. 공격에서는 준비, 유효, 회복 시간을 정의에서 읽어 Task로 창을 열고 닫는다. 가드는 입력 해제 Task를 기다리고 회피는 RootMotion Task와 무적 효과를 사용한다.

서버 CombatComponent는 공격 ID, 현재 Avatar, 거리, 차폐와 중복 대상을 확인한다. 다음은 정책 호출 직전 부분이다. 앞선 권한·거리·차폐 검사는 이 발췌에 생략돼 있다.

출처: `50e5838`, [CCLCombatComponent.cpp](../../Source/CCL/Combat/CCLCombatComponent.cpp):99의 `UCCLCombatComponent::ResolveHit`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
    const UCCLCombatDefinition* Definition = ActiveDefinition;

    if (!Definition->HitRule)
    {
        return FGameplayTag();
    }

    FCCLHitContext Context{Source, Target, SourceASC, TargetASC, Definition, Hit, AttackId};
    // Reserve before policy side effects: cancellation can synchronously re-enter the resolver.
    HitActors.Add(Target);
    const FGameplayTag Outcome = CCLHit::Apply(Context);
    OnHitResolved.Broadcast(Target, Outcome);
    return Outcome;
}
```

대상을 먼저 `HitActors`에 넣는 순서에 주목하자. 효과나 Ability 취소는 동기적으로 다른 콜백을 부를 수 있으므로, 부수 효과 전에 중복 적중을 예약한다.

`CCLHit::Apply`는 정의에 지정한 HitRule의 `Resolve`를 호출한다. 현재 소울라이크 정책은 `UCCLDuelHitRule`이다. 정책은 사망·팀·무적을 검사한 뒤 정면 패링, 가드와 일반 피해를 판단한다. 효과가 있는 결과라면 Spec에 수치를 넣고 적용 성공을 확인한다. 단순히 UI에 ‘적중’ 문구를 보냈다는 이유로 체력이 줄지는 않는다.

## 8. 예측과 권한

근접 Ability는 로컬 예측을 사용하지만 피해 정책의 적용은 서버다. 장비의 손 선택과 입력은 같은 PlayerState에 연결된 경로로 전달한다. 자세한 소스 ID 행동은 Weapons에서 다룬다.

체력과 스태미나는 별도 세트다. 대상에 스태미나가 없다고 공통 적중 자체를 거부하지 않는다. 방어 정책이 필요한 자원을 조건부로 검사한다. 복제 조건도 각 AttributeSet과 컴포넌트가 소유하므로 모든 자원이 모든 관찰자에게 동일하게 노출된다고 가정하지 않는다.

## 9. 취소와 다음 생명 보호

능력 종료 시 공격 창과 생명 효과의 핸들을 정리한다.

출처: `50e5838`, [CCLCombatAbility.cpp](../../Source/CCL/AbilitySystem/CCLCombatAbility.cpp):154의 `UCCLCombatAbility::EndAbility`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void UCCLCombatAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (HasCurrentAvatar())
    {
        EndWindow();
        Fighter->SetAction(ECCLCombatAction::None);

        if (ActorInfo->IsNetAuthority() && BusyEffect.IsValid())
        {
            ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(BusyEffect);
        }
    }

    BusyEffect.Invalidate();
    WindowEffect.Invalidate();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    Fighter = nullptr;
    ActivatedAvatar.Reset();
}
```

`HasCurrentAvatar`는 능력을 시작한 몸과 현재 ActorInfo의 Avatar를 비교한다. 이전 Pawn에서 늦게 끝난 능력이 새 Pawn의 상태를 지우는 것을 방지하는 경계다.

Fighter의 `EndLife`도 능력·입력과 생명 효과를 정리한다. 무한 지속 장비·학습 효과까지 모두 지우는 방식이면 재스폰 후 성장 유지 계약과 충돌하므로 생명 효과의 분류가 필요하다.

## 10. 대안과 비용

| 선택 | 이점 | 비용 |
|---|---|---|
| Pawn 소유 ASC | Pawn 파괴로 수명 이해가 쉬움 | 재스폰 때 부여·성장을 다시 구성 |
| PlayerState 소유 ASC | 능력·장비·성장을 유지 | Avatar 재연결과 생명 효과 정리가 필요 |
| 직접 RPC·타이머 전투 | 작은 공격 하나를 빠르게 작성 | 예측·효과·취소·복제를 직접 유지 |

현재는 GAS의 계약을 사용하므로 학습할 타입과 핸들이 늘어난다. 짧은 코드보다 예측과 취소 경계가 일관적인지를 기준으로 평가해야 한다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 공격 창에 같은 대상 재진입 | 공격 ID당 한 번 처리 |
| 벽 뒤 대상·오래된 공격 ID | 거부 |
| 정면·후면 방어, 비용 부족 | 정책에 맞는 결과와 자원 변경 |
| 가드 중 입력 해제·사망 | 유지 효과·실행 종료 |
| 재스폰 직후 이전 콜백 | 새 Avatar 상태 보존 |
| 스태미나 없는 체력 대상 | 공통 적중 경로 사용 |

이번에는 코드와 기존 검증 기록을 대조했다. CombatFoundation과 [AgentFoundation](../AgentFoundation.md)의 전투 네트워크 검사 기록을 참고했으며 현재 설치본에서 실행 검사를 새로 수행하지 않았다.

## 12. 이해 확인

**ASC가 살아 있는데 공격은 왜 끝내야 할까?**

부여된 능력의 수명과 실행 중인 생명의 수명이 다르기 때문이다. 이전 공격 창과 입력은 새 Character에 이어지면 안 된다.

**정책을 호출하기 전에 중복 대상을 기록하는 이유는 무엇일까?**

정책의 부수 효과가 동기적으로 재진입할 수 있기 때문이다. 결과 적용 뒤에 기록하면 그 사이 같은 대상이 다시 처리될 여지가 생긴다.
