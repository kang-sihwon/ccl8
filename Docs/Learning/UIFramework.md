# 공용 UI: 화면 종류와 열린 인스턴스를 분리하기

공용 UI는 LocalPlayer마다 화면 등록·열기·닫기·비동기 요청·문맥 수명을 관리한다. `Source/CCL/UI/Core/`의 코드를 따라 `OpenView` 한 줄 뒤에서 실제 위젯이 생기고 정리되는 과정을 이해한다. 기능 화면의 구체 예시는 [InventoryUI](InventoryUI.md)를 먼저 읽어도 좋다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `6a9cecb09da31030d54f28c1da59c73a393f402d` |
| 도입·변경 기준 | `b5d9495d99bcfb903a7f5ef205f9f5b2cfcfe45e` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전에는 기능별 화면이 뷰포트와 입력을 직접 관리했다. `b5d9495`은 공용 기반의 최초 도입이며 현재에는 기능 이전·표시 요청·재진입 보완이 포함된다. 계약의 정본은 [UIFoundation](../UIFoundation.md)이다.

## 2. 여러 화면과 늦은 로드

인벤토리, 메뉴와 대화는 같은 플레이어의 입력·화면 영역을 공유한다. 아직 로드되지 않은 화면 클래스를 기다리는 동안 맵이 바뀌거나 요청자가 사라질 수도 있다.

화면을 생성하는 함수만 공통으로 묶어서는 이런 상태를 관리하기 어렵다. 등록 자체, 비동기 요청, 열린 화면을 서로 다른 식별자로 추적해야 한다.

## 3. LocalPlayer, 문맥과 핸들

`ULocalPlayerSubsystem`은 로컬 플레이어 수명의 관리자 기반이다. 월드와 컨트롤러는 교체될 수 있어 관리자 수명과 각 화면 범위가 같지 않다. Subsystem·CommonUI API는 [엔진 근거](EngineEvidence.md)를 참고한다.

화면 태그는 종류이고 ViewHandle은 열린 인스턴스다. RegistrationHandle은 태그와 클래스·레이어 정책의 등록을 식별하고 RequestHandle은 아직 열리지 않은 요청을 식별한다.

문맥은 이번 화면에 필요한 기능 상태를 담는 UObject다. UObject 화면과 문맥은 GC 참조로, 안에 담은 Slate 본문은 공유 포인터로 관리한다. `NewObject`의 Outer만 지정했다고 모든 사용 참조가 보장되는 것은 아니다.

## 4. 이전의 닫기

이전 인벤토리는 컨트롤러가 직접 위젯과 입력을 정리했다.

출처: `6a9cecb`, [CCLPlayerController.cpp](../../Source/CCL/CCLPlayerController.cpp):238의 `ACCLPlayerController::CloseInventory`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void ACCLPlayerController::CloseInventory()
{
    if (!bInventoryOpen && !InventoryWidget.IsValid())
    {
        return;
    }

    bInventoryOpen = 0;
    if (EquipmentCamera)
    {
        EquipmentCamera->DestroyComponent();
        EquipmentCamera = nullptr;
    }

    if (InventoryWidget.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
    {
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(InventoryWidget.ToSharedRef());
    }

    InventoryWidget.Reset();
    if (IsLocalController())
    {
        if (FSlateApplication::IsInitialized())
        {
            FSlateApplication::Get().CancelDragDrop();
        }

        FlushPressedKeys();
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());
    }
}
```

한 화면의 처리를 찾기에는 쉽다. 하지만 모든 화면에 이 코드를 복제하면 인벤토리 닫기가 남아 있는 메뉴의 입력 상태까지 덮어쓸 수 있다. 이는 구조를 설명하는 가정이며 별도의 실제 버그 기록은 아니다.

## 5. 공통 계약을 도입한 이유

결정 17은 장르 독립적인 관리자와 기능별 문맥, 중첩 표시 요청을 요구했다. 장비·퀘스트의 서버 규칙은 기능 계층에 남긴다.

공용 Core가 인벤토리의 아이템 종류를 몰라도 레이어·클래스·문맥 타입·입력 정책을 검증할 수 있도록 등록 데이터가 경계를 만든다. 대가로 잘못된 등록을 거부하는 검증 코드가 필요하다.

## 6. OpenView의 실제 처리

`OpenView`는 등록과 소유자·문맥·월드를 검사하고 미로드 클래스와 같은 등록의 열기 재진입을 거부한다. 단일 창 교체 중 닫기 콜백이 등록을 바꿀 수 있어 정의를 복사하고 교체 후 다시 검증한다.

아래는 화면 기록이 생긴 다음의 연결 함수다.

출처: `50e5838`, [CCLUISubsystem.cpp](../../Source/CCL/UI/Core/CCLUISubsystem.cpp):420의 `UCCLUISubsystem::AttachScreen`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLUISubsystem::AttachScreen(FCCLUIViewHandle Handle)
{
    const auto* Entry = Views.Find(Handle.Id);
    if (!Entry || !Root)
    {
        return false;
    }

    const FCCLUIOpenView Snapshot = *Entry;
    auto* Screen = Root->AddScreen(Snapshot.Definition, [this, Snapshot, Handle](UCCLScreen& Instance)
    {
        Instance.BindContext(Snapshot.Context, Handle, Snapshot.Definition.InputPolicy);
        Instance.OnCloseRequested.AddUObject(this, &ThisClass::CloseView);
    });

    if (auto* Current = Views.Find(Handle.Id); Current && Screen && Screen->GetViewHandle() == Handle)
    {
        Current->Screen = Screen;
        Current->World = GetWorld();
        ApplyPresentation(Handle, 0.f);
        // A feature hook may close the new view while applying an existing request.
        return IsViewOpen(Handle);
    }

    if (Root && Screen)
    {
        Root->RemoveScreen(Screen);
    }

    return false;
}
```

Root가 실제 위젯을 추가할 때 문맥과 입력 정책, 닫기 요청을 연결한다. 표시 정책 콜백이 새 화면을 즉시 닫을 수 있으므로 마지막에 `IsViewOpen`을 다시 확인한다. 위젯 생성에 성공했다는 이유만으로 유효한 열린 핸들을 반환하지 않는다.

## 7. 루트와 비동기 요청

`UCCLUIRoot::AddScreen`은 레이어 정책에 따라 동시 패널·스택·큐와 확장 지점을 사용한다. CommonUI의 컨테이너는 위젯을 풀에서 재사용할 수 있어 초기화 콜백에서 새 문맥을 연결한다. 클래스 생성·활성화·문맥 연결은 서로 다른 시점이다.

`RequestOpenView`는 Soft Class 경로를 StreamableManager에 요청하고 Pending에 요청자·문맥·등록·월드를 보관한다. 로드 콜백은 ready만 표시하고 관리자의 처리 경로에서 아직 유효한 요청인지 확인한다.

`CancelRequest`는 Pending을 먼저 제거하고 로드 핸들을 취소한 뒤 실패 완료를 알린다. 이미 취소된 요청의 늦은 콜백은 Pending 항목을 찾지 못하므로 새 화면을 열지 않는다.

## 8. 입력과 범위

`UCCLScreen::GetDesiredInputConfig`가 정책을 CommonUI 입력 설정으로 바꾼다. Inherit 화면은 새 입력 요구를 내지 않고 Menu는 이동·시점 입력을 막는다. 게임 행동의 최종 허용은 여전히 게임 규칙과 서버가 검사한다.

World 범위 화면은 맵 정리 때 닫고 Player 범위 화면은 새 루트에 열린 순서대로 다시 연결한다. 컨트롤러 교체도 루트 연결을 다시 확인해야 하는 사건이다.

## 9. 닫기와 재사용

출처: `50e5838`, [CCLUISubsystem.cpp](../../Source/CCL/UI/Core/CCLUISubsystem.cpp):348의 `UCCLUISubsystem::CloseView`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void UCCLUISubsystem::CloseView(FCCLUIViewHandle Handle)
{
    FCCLUIOpenView View;
    if (!Views.RemoveAndCopyValue(Handle.Id, View))
    {
        return;
    }

    if (Root && View.Screen)
    {
        TGuardValue<uint8> RemovingGuard(bRemovingScreen, 1);
        Root->RemoveScreen(View.Screen);
    }

    OnViewClosed.Broadcast(Handle);
}
```

기록을 먼저 제거하므로 같은 핸들의 중복 닫기는 아무 작업도 하지 않는다. Root의 제거 경로는 문맥과 기능 자원을 해제한다. 화면 객체가 풀에 남아 있어도 이번 연결은 끝난다.

`UCCLScreen::ReleaseContext`는 뒤로 가기 바인딩, 기능 구독, 닫기 델리게이트, 포커스와 표시 상태를 정리한다. 모든 창 닫기 중 새 창 열기는 별도 가드로 거부한다. 일반 닫기 완료 콜백에서 다음 화면을 여는 동작과 구분한다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 화면마다 직접 관리 | 작은 UI의 흐름이 짧음 | 입력·월드 이동·취소를 중복 구현 |
| 관리자 + CommonUI | 활성화·입력·풀 계약 재사용 | 등록·문맥·핸들 이해 필요 |
| 자체 입력 스택 | 정책 자유도 | 포커스·캡처·게임패드 입력 유지 부담 |

UMG·Slate는 UE4에도 있었고 이 프로젝트의 공용 관리자는 자체 코드다. CommonUI를 사용했다는 사실과 모든 장르 UI가 완성됐다는 주장은 구분한다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 잘못된 문맥·레이어 | 생성 전 거부 |
| 미로드 화면 요청 후 취소 | 늦게 열리지 않음 |
| 닫기 콜백 중 같은 등록 재진입 | 교체 가드 적용 |
| 표시 콜백에서 즉시 닫기 | OpenView 결과도 무효 |
| 풀 재사용·맵 이동 | 이전 구독 제거, 허용 범위의 순서 복원 |
| 두 LocalPlayer | 핸들·문맥 분리 |

기존 RegistryContracts·UIFoundation·UIVisual 기록은 UIFoundation에 있다. 이번에는 소스와 기록을 대조했으며 UI 실행은 새로 하지 않았다.

## 12. 이해 확인

**로드 요청 핸들을 화면 핸들로 사용하면 왜 곤란할까?**

요청은 취소·실패할 수 있고 화면이 아직 없을 수 있다. 각각의 완료·수명을 구분해야 한다.

**풀에 남은 화면에서 Destruct만 기다리면 충분할까?**

객체 파괴 전에 다음 문맥으로 재사용될 수 있다. 닫을 때 이전 구독과 기능 자원을 해제해야 한다.
