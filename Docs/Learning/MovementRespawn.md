# 이동과 재스폰: 플레이어와 한 번의 생명을 분리하기

플레이어가 죽어도 접속과 동료의 진행은 유지된다. 서버는 죽은 Character만 교체하고 PlayerController는 새 Pawn을 조작한다. 이 장에서는 `Source/CCL/CCLGameModeBase.cpp`, `CCLCharacter.cpp`, `CCLPlayerController.cpp`를 따라 스폰 실패까지 포함한 한 번의 재도전을 이해한다. 전투 자원의 수명은 [GAS 전투](GASCombat.md)에서 이어서 다룬다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `3258f4867c7bd47263917d8422f6ee8c8f545fb9` |
| 도입·변경 기준 | `c8c6e4a27234bf2a78ca4c6f49c230e6badd193a` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

`3258f48`의 초기 프로젝트와 `c8c6e4a`의 이동·개별 재스폰 도입을 비교한다. 이전에 완성된 재스폰 시스템이 있었던 것은 아니다. 아래 이전 코드는 첫 구현의 실제 사망 처리이고, 현재는 GAS 전투가 연결된 상태다. 사양과 과거 실행 기록은 [MultiplayerFoundation](../MultiplayerFoundation.md)이 소유한다.

## 2. 재도전에서 보존할 대상

두 플레이어가 싸우다가 한 명만 죽었다고 하자. 맵을 다시 열면 살아 있는 플레이어와 처치한 적도 초기화될 수 있다. 결정 5는 사망한 플레이어만 재스폰하도록 정했다.

따라서 재도전은 월드 재시작과 다른 작업이다. 요청자의 현재 Pawn을 확인하고, 죽은 Pawn을 해제한 다음 안전한 시작 위치에서 새 Pawn을 만들어야 한다. 시작 위치가 막혔다면 새 Pawn이 없는 상태도 정상적인 대기 상태로 표현해야 한다.

## 3. Pawn, Controller와 복제

`Pawn`은 조작 대상이고 `PlayerController`는 그 대상을 소유·조작하는 연결이다. `Possess`는 Controller와 Pawn을 연결한다. `GameMode`는 서버에서 스폰 규칙을 실행하고, `PlayerState`는 플레이어의 지속 상태를 담는다. 이 역할 분리는 UE4에도 있었다.

`UFUNCTION(Server, Reliable)`로 선언한 RPC는 소유 클라이언트의 요청을 서버 구현으로 전달한다. Reliable은 재스폰 허용을 보장한다는 뜻이 아니다. 서버는 생존 여부와 요청 간격을 다시 확인한다.

`ReplicatedUsing`은 복제 값이 도착했을 때 실행할 함수를 지정한다. 서버의 일반 변수 대입이 클라이언트의 복제 콜백을 서버에서도 자동 실행하는 것은 아니므로 이 프로젝트는 사망 표현 함수를 서버에서 직접 호출한다.

이동은 `UCharacterMovementComponent`의 예측·보정 경로를 사용한다. 입력은 UE5 프로젝트의 Enhanced Input Action과 Mapping Context로 구성한다. UE4에서 흔히 쓰던 이름 기반 Action/Axis 바인딩과 구분하되, 카메라의 SpringArm 자체를 UE5 신규 기능으로 설명하지 않는다.

## 4. 최초 구현의 사망

이동 검증 단계에서는 양수 피해를 받으면 즉시 죽었다. 체력을 조금씩 줄이는 규칙은 아직 없었다.

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

입력 값과 서버 권한을 확인한 뒤 `Die`를 호출한다. 이 코드는 ‘피해가 생겼을 때 재스폰 흐름이 동작하는가’를 검사하기에는 충분하다. 방어와 여러 번의 피격을 구현하려면 피해량을 자원에 반영하는 과정이 필요하다.

## 5. 변경 계기와 유지한 책임

결정 11은 체력과 스태미나를 GAS로 관리하고 플레이어 ASC를 PlayerState에 두도록 정했다. 현재 `TakeDamage`는 체력 변경 효과를 적용하며, 체력 알림이 0 이하를 확인하면 `Die`로 들어간다. 재스폰의 주체는 계속 GameMode다.

이 분리는 체력 시스템이 늘어나도 스폰 위치 검사와 재시도 정책을 바꾸지 않게 한다는 장점이 있다. 이는 코드 구조에 대한 분석이다. 기록된 요구의 근거는 [DesignLog](../DesignLog.md)의 결정 5와 결정 11이다.

## 6. 현재의 재도전

서버는 요청자가 죽었거나 이전 스폰에 실패한 경우에만 다시 스폰한다.

출처: `50e5838`, [CCLGameModeBase.cpp](../../Source/CCL/CCLGameModeBase.cpp):83의 `ACCLGameModeBase::RequestRetry`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void ACCLGameModeBase::RequestRetry(APlayerController* Player)
{
    if (!HasAuthority() || !IsValid(Player))
    {
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();

    if (const double* NextRetry = NextRetryTimes.Find(Player); NextRetry && Now < *NextRetry)
    {
        return;
    }

    ACCLCharacter* Character = Cast<ACCLCharacter>(Player->GetPawn());

    if (Character && Character->IsDead())
    {
        PendingRespawns.Add(Player);
        Player->UnPossess();
        Character->Destroy();
    }
    else if (Player->GetPawn() || !PendingRespawns.Contains(Player))
    {
        return;
    }

    NextRetryTimes.Add(Player, Now + 0.5);
    RestartPlayer(Player);
}
```

죽은 Character를 대기 집합에 등록한 뒤 `UnPossess`와 `Destroy`를 호출한다. 생성에 실패하면 Controller는 살아 있고 Pawn만 없다. 다음 요청은 대기 집합을 근거로 허용된다.

살아 있는 Pawn이 있으면 거부한다. Pawn이 없더라도 대기 집합에 없는 임의 Controller까지 무조건 생성해 주지는 않는다. 요청 간격은 게임 시간으로 제한한다.

## 7. 실제 스폰과 이동 연결

`RestartPlayer`는 `FindPlayerStart`로 새 위치를 확인한 뒤 `RestartPlayerAtPlayerStart`를 호출한다. 프로젝트의 위치 검사는 Character 기본 캡슐 크기로 겹침을 확인한다. 실패했을 때 엔진이 이전 StartSpot으로 되돌아가는 경로를 피하려는 이유는 함수의 소스 주석에도 남아 있다.

출처: `50e5838`, [CCLGameModeBase.cpp](../../Source/CCL/CCLGameModeBase.cpp):24의 `ACCLGameModeBase::RestartPlayer`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void ACCLGameModeBase::RestartPlayer(AController* NewPlayer)
{
    if (!IsValid(NewPlayer))
    {
        return;
    }

    // The base RestartPlayer falls back to the cached StartSpot when no free start exists.
    // Require a freshly checked start so a blocked checkpoint remains retryable.
    if (AActor* StartSpot = FindPlayerStart(NewPlayer))
    {
        RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
    }

    if (IsValid(NewPlayer->GetPawn()))
    {
        PendingRespawns.Remove(NewPlayer);
        UE_LOG(LogTemp, Display, TEXT("CCL Spawn Controller=%s Pawn=%s NetMode=%d"),
            *GetNameSafe(NewPlayer), *GetNameSafe(NewPlayer->GetPawn()), static_cast<int32>(GetNetMode()));
    }
    else
    {
        PendingRespawns.Add(NewPlayer);
        UE_LOG(LogTemp, Warning, TEXT("CCL Spawn blocked Controller=%s; retry is available"), *GetNameSafe(NewPlayer));
    }
}
```

새 Pawn이 생겼는지를 마지막에 검사하므로 위치를 골랐다는 사실과 생성 성공을 구분한다. 새 Character의 카메라·이동 컴포넌트는 새 객체의 수명을 따르고, Controller의 입력 처리는 현재 `GetPawn()`을 조회한다.

입력 연결은 `ACCLPlayerController::SetupInputComponent`에 있다. 런타임에 만든 Axis2D Action에 WASD를 매핑하며 Negate는 부호를, Swizzle은 입력 축의 위치를 바꾼다. 이동 Action은 여러 키를 누른 값을 누적한다. `Move`는 ControlRotation의 Yaw만으로 전방·우측 벡터를 만들고 `AddMovementInput`에 축 값을 넘긴다. 카메라를 위로 올렸다고 캐릭터가 하늘 방향으로 걷는 구조가 아니다.

Character 생성자는 몸의 회전을 Controller Yaw에 직접 고정하지 않고 CharacterMovement의 이동 방향 회전을 사용한다. SpringArm은 Pawn의 ControlRotation을 따라가고 Camera는 그 끝에 붙는다. `Look`이 Controller의 Yaw·Pitch를 바꾸므로 시점 방향과 이동에 따른 몸 방향을 구분할 수 있다. 점프는 시작과 해제·취소 이벤트를 따로 연결하고 Controller 종료 때 자기 Mapping Context를 제거한다.

## 8. 권한과 생명 경계

입력은 로컬 플레이어에서 시작하지만 `RequestRetry`는 서버에서만 실행된다. 클라이언트가 원하는 스폰 좌표나 남의 Pawn을 넘기는 계약도 없다. 현재 R 요청은 살아 있을 때 장전을 시도하고, 죽었을 때 GameMode에 재도전을 전달한다. 이 후속 연결은 `ACCLPlayerController::ServerRequestRetry_Implementation`에서 확인할 수 있다.

Character의 사망은 `bDead`를 복제하고 이동·충돌을 정리한다. 인벤토리와 학습 효과는 PlayerState에 남는다. 반면 이전 생명의 AbilityTask와 효과는 정리해야 하므로 Pawn 교체만으로 전투 정리가 끝난다고 생각하면 안 된다.

## 9. 실패와 접속 종료

막힌 시작 위치에서는 대기 상태를 유지한다. 장애물이 사라진 뒤 다시 요청하면 복구할 수 있다. 자동으로 무한 재시도하는 루프는 이 함수에 없다.

접속 종료 시 `Logout`이 `PendingRespawns`와 `NextRetryTimes`에서 Controller를 제거한다. 사망 처리의 중복 호출은 `Die`의 권한·사망 검사로 막는다.

## 10. 대안과 비용

| 방식 | 유효한 상황 | 비용 |
|---|---|---|
| 월드 다시 열기 | 완전한 싱글플레이 재시작 | 생존자와 월드 진행도 초기화됨 |
| Pawn만 교체 | 현재 개별 재도전 | 지속 상태와 생명 상태의 분리가 필요 |
| 기존 Pawn을 되살리기 | 객체 교체를 줄이고 싶은 게임 | 이동·충돌·태그·타이머를 빠짐없이 초기화해야 함 |

현재 방식은 새 Pawn의 초기화를 재사용하지만 Controller·PlayerState에 남은 참조와 효과를 함께 검증해야 한다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 살아 있을 때 재도전 요청 | 새 Pawn을 만들지 않음. 현재 R은 장전 경로도 확인 |
| 한 플레이어 사망·재도전 | 해당 Pawn만 교체, 생존자 Pawn 유지 |
| 모든 시작 위치 차단 | 실패 대기, 차단 해제 후 요청으로 복구 |
| 재도전 후 이동·회전 | 새 Pawn과 카메라를 조작 |
| 대기 중 접속 종료 | Controller별 대기·간격 기록 제거 |

기존 실행 근거는 MultiplayerFoundation의 NetworkSmoke 표에 있다. 이번 문서 작성에서는 커밋 소스와 기록을 대조했으며 게임 실행·빌드를 새로 수행하지 않았다.

## 12. 이해 확인

**Pawn이 없으면 모두 재스폰해도 될까?**

대기 집합이 실제 스폰 실패나 사망 후 재도전 상태를 증명한다. 이 구분을 없애면 초기화 중인 Controller까지 재도전으로 취급할 수 있다.

**장비가 재스폰 뒤 유지되는 이유는 무엇일까?**

소유 데이터와 효과가 PlayerState 수명을 따르기 때문이다. 새 Character에는 기존 ASC와 장비 표현을 다시 연결한다.
