# HUD와 대화: 값의 변경과 화면 갱신을 연결하기

HUD는 기능 데이터를 모으고, 표시 모델과 문맥은 변경을 알리며, 화면은 그 알림을 받아 텍스트와 막대를 갱신한다. `Source/CCL/CCLHUD.cpp`, `UI/CCLCombatViewModel.cpp`, `UI/CCLHUDScreens.cpp`에서 폴링과 이벤트의 실제 경계를 읽는다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `494a0ac977c7037d8ca732eeb27edbc317162f1f` |
| 도입·변경 기준 | `bbe9b3bbd452710f5ee5079f59f2da760258acd3` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전 HUD는 Canvas에서 직접 배치·줄바꿈·그리기를 처리했다. `bbe9b3b`에서 능력치·일반 안내·대화를 공용 화면으로 나눴다. 계약·기존 실행 근거는 [UIFoundation](../UIFoundation.md)이 소유한다.

## 2. 값이 바뀌는 이유와 위치가 바뀌는 이유

체력은 GAS 변경 알림을 받을 수 있다. 하지만 ‘가까운 NPC가 누구인가’는 플레이어 이동에 따라 달라지므로 체력 알림만으로 갱신할 수 없다.

따라서 현재 구조는 모든 Tick을 제거하지 않는다. Attribute는 이벤트로 전달하고 근접 안내·퀘스트 문장은 기능 계층이 주기적으로 조회한다. 화면에는 달라진 문자열만 알린다.

## 3. ViewModel과 FieldNotify

ViewModel은 게임 데이터를 화면이 읽기 좋은 값으로 제공하는 객체다. `UCCLCombatViewModel`은 ASC 델리게이트를 구독해 값을 보관하고 `UE_MVVM_SET_PROPERTY_VALUE`로 FieldNotify를 발생시킨다. 매크로는 변경된 필드의 구독자에게 통지를 전달한다.

`AddWeakLambda`는 대상 UObject 수명을 고려하는 델리게이트 연결이다. 이것만으로 다른 ASC로 재연결할 때의 중복 구독을 해결하는 것은 아니므로 명시적 `Unbind`가 있다.

Canvas·UMG·델리게이트는 UE4에도 있었다. 이 장에서는 UE5 설치본의 MVVM 플러그인을 사용하며, 기존 HUD를 분리한 책임 변경은 프로젝트 자체 결정이다.

## 4. 이전 Canvas HUD

출처: `494a0ac`, [CCLHUD.cpp](../../Source/CCL/CCLHUD.cpp):27의 `ACCLHUD::DrawHUD`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
void ACCLHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas)
    {
        return;
    }

    // Use a 720p layout and scale text and spacing together on larger displays.
    const float UiScale = FMath::Max(0.5f, FMath::Min(Canvas->SizeX / 1280.f, Canvas->SizeY / 720.f));
    const float Width = Canvas->SizeX / UiScale;
    const float Height = Canvas->SizeY / UiScale;
    constexpr float Margin = 24.f;
    constexpr float PanelWidth = 560.f;
    const float LeftWidth = FMath::Min(650.f, Width - PanelWidth - Margin * 3.f);

```

이 함수는 이어서 문장 너비 측정·줄바꿈·그리기와 기능 상태 표시를 처리했다. 해상도에 맞춰 직접 그리는 방식이 잘못된 것은 아니다. 화면마다 독립된 표시 정책과 구독을 연결하려면 역할을 나눌 필요가 있었다.

## 5. 변경 이유

결정 17은 HUD·대화·능력치를 공용 화면으로 연결하고 기능별 변경 통지를 사용하도록 정했다. UIFoundation은 기존 위치 기반 안내의 조회를 기능 계층에 유지한다고 명시한다.

표시와 게임 상태 검사를 나누면 HUD를 숨겨도 대화 거리·생존 판정을 계속 수행할 수 있다. 숨김을 게임 규칙 정지로 해석하지 않는 것이 이 분리의 목적이다.

## 6. 현재의 값 연결

출처: `50e5838`, [CCLCombatViewModel.cpp](../../Source/CCL/UI/CCLCombatViewModel.cpp):13의 `UCCLCombatViewModel::Bind`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void UCCLCombatViewModel::Bind(UAbilitySystemComponent* InASC)
{
    if (ASC == InASC)
    {
        return;
    }

    Unbind();
    ASC = InASC;

    if (!ASC)
    {
        PublishValues();
        return;
    }

    const TArray<FGameplayAttribute> Attributes = {UCCLHealthSet::GetHealthAttribute(), UCCLHealthSet::GetMaxHealthAttribute(), UCCLStaminaSet::GetStaminaAttribute(), UCCLStaminaSet::GetMaxStaminaAttribute()};

    for (const auto& Attribute : Attributes)
    {
        Values.Add(Attribute, ASC->GetNumericAttribute(Attribute));
        Handles.Add(Attribute,
            ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(this,
                [this, Attribute](const FOnAttributeChangeData& Data)
                {
                    Values.FindOrAdd(Attribute) = Data.NewValue;
                    PublishValues();
                }));
    }

    PublishValues();
}
```

같은 ASC면 재연결하지 않고, 다른 ASC면 먼저 이전 핸들을 제거한다. 현재 값을 읽고 변경 델리게이트를 연결한 뒤 초기 표시도 갱신한다. 알림이 올 때까지 체력 UI가 빈 채로 기다리지 않는다.

HUD는 현재 Pawn의 ASC를 넘긴다. 재스폰으로 몸이 바뀌어도 플레이어의 같은 ASC를 사용하면 중복 구독을 만들지 않는다. ASC 자체가 바뀌거나 사라지는 경우도 별도로 처리한다.

## 7. 화면까지의 내부 연결

`PublishValues`가 필드 변경을 알리면 `UCCLVitalsScreen::OnContextBound`에서 등록한 구독이 `Refresh`를 호출한다. Refresh는 텍스트와 체력·스태미나 비율을 실제 위젯에 적용한다.

대화는 `UCCLDialogueContext::Update`에서 화자·본문이 달라질 때만 `OnChanged`를 방송한다. DialogueScreen은 그 문맥을 구독하고 `Speaker`와 `Body`에 표시한다. 서버 응답이 화면을 직접 생성·배치하는 구조는 아니다.

일반 HUD는 `ACCLHUD::RefreshContent`가 주변 Actor와 퀘스트 상태를 조회하고 `UCCLHUDContext::Update`에 문장을 넘긴다. 공용 UI 관리자는 NPC 검색이나 보상 조건을 알지 않는다.

## 8. 응답 유효성과 표시 갱신

서버의 거래·퀘스트 결과가 늦게 도착하면 컨트롤러는 현재 생존·거리·메뉴·인벤토리 상태를 다시 확인한다. 이미 다른 메뉴로 이동한 플레이어에게 뒤늦게 대화창을 열지 않기 위한 표시 검사다. 서버에서 확정한 거래를 취소하는 검사는 아니다.

`IsPresentationUpdating`가 false면 화면 갱신을 보류하지만 ViewModel과 문맥의 최신 데이터는 계속 갱신된다. 표시가 복구될 때 `OnPresentationChanged`가 Refresh를 실행한다.

## 9. 구독 해제

출처: `50e5838`, [CCLCombatViewModel.cpp](../../Source/CCL/UI/CCLCombatViewModel.cpp):60의 `UCCLCombatViewModel::Unbind`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void UCCLCombatViewModel::Unbind()
{
    if (ASC)
    {
        for (const auto& Entry : Handles)
        {
            ASC->GetGameplayAttributeValueChangeDelegate(Entry.Key).Remove(Entry.Value);
        }
    }

    Handles.Reset();
    Values.Reset();
    ASC = nullptr;
}
```

화면도 `OnContextReleased`에서 FieldNotify 또는 문맥 델리게이트를 제거한다. ViewModel의 ASC 구독과 Screen의 ViewModel 구독은 서로 다른 연결이므로 두 곳 모두 정리가 필요하다.

HUD 종료 시 열린 화면 핸들을 닫고 모델을 `Bind(nullptr)`로 해제한다. 풀 재사용 때 이전 문맥의 변경이 새 화면을 갱신하지 않아야 한다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 매 프레임 직접 그리기 | 값과 출력 흐름이 직관적 | 배치·조회·표시 정책이 한곳에 모임 |
| 모든 값을 이벤트화 | 변경 시점이 명확 | 위치·관찰 조건의 이벤트 설계도 필요 |
| 이벤트 + 기능별 주기 조회 | 데이터 특성에 맞게 갱신 | 어느 경로가 원본인지 명시해야 함 |

현재는 네이티브 C++로 UMG 트리를 만든다. UMG 기반이라는 이유만으로 모든 레이아웃이 디자이너 자산에서 편집되는 것은 아니다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 실제 GAS 체력 변경 | FieldNotify를 거쳐 표시 값 변경 |
| 같은 ASC 반복 Bind | 중복 알림 없음 |
| 새 ASC 연결 후 이전 ASC 변경 | 현재 화면에 영향 없음 |
| 숨김 중 값 변경 후 복구 | 최신 값 표시 |
| 대화 요청 후 사망·거리 이탈·인벤토리 열기 | 늦은 응답 표시 거부 |

기존 ViewModelLifetime·UIVisual 결과는 UIFoundation에 있다. 이번에는 코드와 기록을 대조했으며 렌더링 검사는 새로 하지 않았다.

## 12. 이해 확인

**이벤트 기반인데 HUD Tick이 남아 있는 이유는 무엇일까?**

거리·주변 Actor 같은 상태는 위치 변화로 달라지며 별도 기능 조회가 필요하기 때문이다. Attribute 갱신과 같은 방식으로 묶지 않았다.

**약한 람다를 쓰면 Unbind가 없어도 될까?**

객체 생존 중 다른 ASC로 바뀌는 경우 이전 연결이 남을 수 있다. 약한 참조와 명시적 연결 교체는 서로 다른 문제를 해결한다.
