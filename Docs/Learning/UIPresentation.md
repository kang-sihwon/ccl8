# UI 표시 제어: 중첩 요청을 해제해도 남은 정책을 지키기

UI 표시 제어는 현재 기본 상태에 살아 있는 요청들을 합성한다. 컷신 요청을 해제해도 사망 등 다른 요청이 남아 있다면 그 제한을 유지한다. `Source/CCL/UI/Core/CCLUISubsystem.cpp`와 `CCLScreen.cpp`에서 숨김·입력·갱신·페이드의 독립성을 이해한다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `bbe9b3bbd452710f5ee5079f59f2da760258acd3` |
| 도입·변경 기준 | `37dd9045730412708a9f28e7d21cc4858c386202` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전 공용 UI에는 현재의 범위·소유자 기반 표시 요청 기능이 없었다. `37dd904`에서 추가했고 `b6cc535`에서 생성 중 닫힘 등 수명 검사를 보완했다. 세부 계약과 기존 검사 결과는 [UIFoundation](../UIFoundation.md)이 소유한다.

## 2. 잘못된 복구의 예

컷신이 HUD를 숨긴 동안 사망 화면도 HUD 숨김을 요청했다고 가정하자. 컷신 종료가 ‘시작 전에 보이던 상태’를 그대로 복원하면 사망 요청이 남아 있어도 HUD가 나타날 수 있다.

이것은 구조를 설명하는 가정이다. 현재 구현은 과거 화면 상태를 복사해 되돌리는 대신 기본 상태와 남은 요청을 다시 계산한다.

## 3. 표시, 입력, 갱신과 활성화

`FCCLUIViewPresentation`은 가시성·UI 입력·표시 갱신·불투명도를 나눈다. `bBlockGameplay`는 로컬 플레이어 전체의 게임 입력을 제한하는 별도 옵션이다.

CommonUI의 비활성화는 스택에서 제거되는 경로로 이어질 수 있다. 잠깐 숨기는 동안 열린 핸들·문맥을 유지하려면 화면 닫기나 비활성화와 구분해야 한다. 이는 프로젝트 표시 정책이며 UE4에서 UE5로의 일괄 문법 변경이 아니다.

## 4. 최초 추가의 요청 해제

표시 요청이 처음 도입된 커밋부터 핸들을 제거한 뒤 재계산했다.

출처: `37dd904`, [CCLUISubsystem.cpp](../../Source/CCL/UI/Core/CCLUISubsystem.cpp):562의 `UCCLUISubsystem::ReleasePresentation`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void UCCLUISubsystem::ReleasePresentation(FCCLUIPresentationHandle Handle)
{
    FCCLUIPresentationRequest Removed;
    if (Presentations.RemoveAndCopyValue(Handle.Id, Removed))
    {
        RefreshPresentation(Removed.Definition.FadeSeconds);
    }
}
```

이전에 존재하지 않았던 기능을 설명하기 위해 가상의 전역 `SetVisible(true)` 구현을 전후 코드로 제시하지 않는다. 위 함수가 실제 최초 구현이다.

## 5. 도입 이유

결정 17은 여러 UI를 연출 중 함께 제어하되 요청 소유자와 범위를 구분하도록 정했다. 복구 순서가 달라도 남은 요청을 지켜야 한다는 계약도 포함한다.

현재 합성 규칙은 요청 하나가 숨김·차단을 요구하면 유지하고, 불투명도는 가장 낮은 제한을 선택한다. 합성은 읽는 순서에 따라 마지막 요청 하나가 이기는 단순 덮어쓰기가 아니다.

## 6. 현재의 합성

출처: `50e5838`, [CCLUISubsystem.cpp](../../Source/CCL/UI/Core/CCLUISubsystem.cpp):619의 `UCCLUISubsystem::ApplyPresentation`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void UCCLUISubsystem::ApplyPresentation(FCCLUIViewHandle Handle, float FadeSeconds)
{
    const auto* View = Views.Find(Handle.Id);
    if (!View || !View->Screen)
    {
        return;
    }

    FCCLUIViewPresentation State = View->BasePresentation;
    for (const auto& Pair : Presentations)
    {
        const auto& Request = Pair.Value.Definition;
        if (Pair.Value.Owner.IsValid() && (Request.Scope != ECCLUIScope::World || Pair.Value.World == GetWorld()) &&
            Request.Matches(View->Definition.Groups))
        {
            State.bVisible = State.bVisible && !Request.bHide;
            State.bInputEnabled = State.bInputEnabled && !Request.bDisableInput;
            State.bUpdatesEnabled = State.bUpdatesEnabled && !Request.bSuspendUpdates;
            State.Opacity = FMath::Min(State.Opacity, Request.Opacity);
            FadeSeconds = FMath::Max(FadeSeconds, Request.FadeSeconds);
        }
    }

    View->Screen->ApplyPresentation(State, FadeSeconds);
}
```

기본 상태에서 시작하고 유효한 소유자, 현재 월드 범위, 화면 그룹이 일치하는 요청만 반영한다. 게임 입력 차단은 `IsGameplayInputBlocked`에서 별도로 확인한다.

`SetBasePresentation`은 기본 상태를 바꾼다. 요청으로 숨긴 동안 기본 불투명도를 바꿔도, 요청 해제 후에는 새 기본값이 사용돼야 한다.

## 7. 페이드와 입력 라우팅

`UCCLScreen::ApplyPresentation`은 현재 불투명도에서 새 목표까지의 페이드를 준비한다. `TickManagedPresentation`이 실제 값을 진행하고 `UpdatePresentationVisuals`가 Visibility·Enabled·포커스 참여를 적용한다.

관리자가 페이드를 진행하므로 숨긴 위젯 자신의 Tick이 멈춰도 복구가 가능하다. 페이드가 끝나기 전에는 UI 입력을 차단하며, 상호작용 가능성이 바뀌면 CommonUI 라우팅을 다시 계산한다.

## 8. 수명과 게임 상태

요청은 PresentationHandle과 소유자를 갖는다. 소유자가 없어졌거나 월드 범위가 끝난 요청은 계산에서 제외되고 정리된다. 다른 LocalPlayer의 요청을 함께 적용하지 않는다.

갱신 정지는 화면 표시의 정책이다. HUD 모델은 최신 데이터를 유지하고, 인벤토리 미리보기는 캡처를 멈춘다. 서버의 전투·거리·생존 검사는 계속 진행한다.

## 9. 해제·재진입·복구

같은 요청 핸들을 두 번 해제하면 첫 번째만 제거된다. 일부 요청이 남으면 숨김·입력 차단이 유지된다. 복구 페이드 중 다시 숨김 요청이 들어오면 현재 불투명도에서 새 목표로 이동한다.

`ApplyPresentation`은 기능 콜백을 호출할 수 있다. 새 화면이 그 콜백에서 닫히는 경우 `AttachScreen`이 열린 상태를 다시 확인하도록 보완돼 있다. 효과 적용 함수가 항상 대상을 그대로 남긴다고 가정하지 않는다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 이전 표시 상태 저장·복원 | 단일 연출에는 단순 | 중첩과 해제 순서에 취약 |
| 참조 횟수 하나 | 단일 숨김 이유 처리에 적합 | 그룹·불투명도·갱신 정책 표현 부족 |
| 소유 핸들별 요청 합성 | 중첩·월드·플레이어 경계 명확 | 요청 정리·재계산·페이드 검사 필요 |

현재 규칙은 제한을 합성한다. 우선순위로 어떤 요청이 다른 요청을 강제 무시하는 일반 연출 언어는 아니다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 두 요청을 반대 순서로 해제 | 남은 제한 유지 |
| 동일 핸들 중복 해제 | 무해 |
| 숨긴 동안 기본값 변경·새 화면 생성 | 현재 요청을 반영 |
| 페이드 중 역전 | 현재 값에서 전환, 입력 조기 복구 없음 |
| 월드 종료·소유자 소멸 | 이전 요청 정리 |
| 숨김 중 사망 | 게임 규칙 계속 적용 |

기존 UIPresentation·UIFoundation 검사 결과를 읽었으며 이번 작성에서는 실제 페이드나 입력 검사를 새로 하지 않았다.

## 12. 이해 확인

**화면을 숨길 때 CloseView를 호출하면 어떤 차이가 생길까?**

닫기는 문맥과 기능 자원을 해제한다. 일시 숨김은 핸들과 문맥을 유지하고 표시 정책만 제한한다.

**숨긴 위젯의 Tick으로 복구하면 왜 위험할까?**

숨김 상태에서 Tick이 진행되지 않으면 복구 자체가 멈출 수 있다. 현재는 관리자가 페이드를 진행한다.
