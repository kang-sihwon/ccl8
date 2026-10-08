# 로컬 연출: 카메라·음향·조명·UI를 한 수명으로 정리하기

`UCCLCinematicSubsystem`은 짧은 로컬 연출의 카메라·소리·조명과 UI 제어를 묶는다. 이 장의 핵심은 화면을 바꾸는 코드보다 취소 시 자신이 만든 자원과 요청만 해제하는 순서다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `ed9a763db54edd93c7fe802b887ee96b553bb0df` |
| 도입·변경 기준 | `0931fbfb5c96c4da24acb9d2ed44243fc0e6171f` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전 기준에는 `Source/CCL/Presentation/CCLCinematicSubsystem.cpp`가 없었다. `0931fbf`에 도입된 연출 경로와 현재 코드를 비교한다. 사양의 근거는 결정 18, UI 계약은 [UIFoundation](../UIFoundation.md)이다.

## 2. 해결할 문제

보스나 시설을 보여 주는 동안 HUD를 숨기고 카메라를 이동했는데 캐릭터가 죽거나 맵이 바뀔 수 있다. 카메라만 되돌리고 소리·조명·입력 차단이 남으면 연출은 끝나지 않은 셈이다.

동시에 다른 UI 요청이 존재할 수 있다. 연출 종료 때 모든 숨김 요청을 지우는 대신 자신이 발급받은 Handle을 해제해야 한다.

## 3. ViewTarget과 자원 수명

PlayerController의 ViewTarget은 카메라 계산 대상으로 쓰는 Actor다. `SetViewTargetWithBlend`는 대상을 바꾸면서 보간한다. 이전 대상과 연출용 CameraActor를 따로 기억해야 복원할 수 있다.

`TWeakObjectPtr`는 대상이 파괴될 수 있음을 표현하는 약한 참조다. 이전 Pawn·Controller가 끝까지 살아 있다고 가정하지 않는다. 생성한 CameraActor와 Component는 명시적 Cancel 경로에서 정리한다.

Procedural SoundWave는 코드가 만든 음성 샘플을 재생하는 자료다. 현재 연출은 간단한 합성 음과 PointLight를 사용한다. 완성된 음악·Niagara·Sequencer 연출을 구현했다고 설명하지 않는다. 이 장의 카메라·Actor·Component 수명 문제는 UE4에도 적용된다.

## 4. 최초 재생 시작

재생은 로컬 Controller와 Pawn, 유한한 초점·재생 시간을 확인한다.

출처: `0931fbf`, [CCLCinematicSubsystem.cpp](../../Source/CCL/Presentation/CCLCinematicSubsystem.cpp):51의 `UCCLCinematicSubsystem::Play`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
FGuid UCCLCinematicSubsystem::Play(APlayerController* Controller, FVector Focus, const FString& Title, float Duration)
{
    if (!Controller || !Controller->IsLocalController() || !Controller->GetPawn() || Focus.ContainsNaN() || !FMath::IsFinite(Duration) || Duration <= 0)
    {
        return {};
    }
    Cancel(Active);
    auto* UI = CCLGameUI::Get(Controller);
    if (!UI)
    {
        return {};
    }
    Observer = Controller;
    PreviousTarget = Controller->GetViewTarget();
    InitialPawn = Controller->GetPawn();
    Active = FGuid::NewGuid();
    FocusPoint = Focus;
    Elapsed = 0;
```

새 연출을 시작할 때 기존 Active 연출을 취소한다. 이 구조는 다중 연출의 동시 혼합을 위한 스택이 아니라 Subsystem당 하나의 Active 연출이다.

## 5. Handle로 취소를 제한한 이유

UI는 [연출 요청](UIPresentation.md)의 Handle 합성 규칙을 이미 사용한다. 로컬 연출도 자기 카메라·음향·UI 요청의 수명을 함께 관리한다.

코드에서 분석하면, 호출자가 가진 오래된 Handle로 새 연출을 취소하지 못하게 하면 비동기 종료나 늦은 입력이 다음 연출을 끊는 문제를 줄일 수 있다. 특정 취소 버그가 실제 발생했다는 기록으로 해석하지 않는다.

## 6. 현재 취소의 소유권 검사

다음은 Handle 검사부터 ViewTarget 복원까지의 경로다.

출처: `50e5838`, [CCLCinematicSubsystem.cpp](../../Source/CCL/Presentation/CCLCinematicSubsystem.cpp):122의 `UCCLCinematicSubsystem::Cancel`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLCinematicSubsystem::Cancel(FGuid Handle)
{
    if (!Handle.IsValid() || Handle != Active)
    {
        return false;
    }
    if (auto* Controller = Observer.Get())
    {
        if (auto* UI = Controller->GetLocalPlayer() ? Controller->GetLocalPlayer()->GetSubsystem<UCCLUISubsystem>() : nullptr)
        {
            UI->ReleasePresentation(UIHandle);
            UI->CloseView(Overlay);
        }
        if (Controller->GetViewTarget() == Camera)
        {
            Controller->SetViewTarget(PreviousTarget.IsValid() ? PreviousTarget.Get() : Controller->GetPawn());
        }
    }
    if (IsValid(Audio))
    {
        Audio->Stop();
        Audio->DestroyComponent();
    }
    if (IsValid(Light))
    {
        Light->Destroy();
    }
    if (IsValid(Camera))
    {
        Camera->Destroy();
    }
    Audio = nullptr;
    Sound = nullptr;
    Light = nullptr;
    Camera = nullptr;
    Active.Invalidate();
    return true;
}
```

현재 ViewTarget이 자신의 Camera일 때만 이전 대상 또는 현재 Pawn으로 복원한다. 다른 시스템이 이미 카메라를 가져갔다면 Cancel이 그 선택을 덮지 않는다.

후반부에서는 Audio를 멈추고 Component를 파괴하며 Light와 Camera를 정리한다. 관련 참조와 Active Handle도 초기화한다.

## 7. Play 안쪽에서 만드는 것

CameraActor를 생성하고 FOV와 후처리 값을 설정한 뒤 ViewTarget을 전환한다. UI에는 HUD·메뉴·대화 숨김과 게임 입력 차단 요청을 넣고 연출 Overlay를 연다.

PointLight는 초점 근처의 빛을 만들고, AudioComponent는 Camera를 Outer로 생성해 등록한 뒤 합성 SoundWave를 재생한다. Outer를 지정했다는 이유로 취소 시 Stop·DestroyComponent가 불필요해지지 않는다.

Tick은 경과 시간에 따라 카메라 위치와 빛을 갱신한다. 연출의 카메라·음향은 로컬 표현이며 서버 전투 판정이나 캠페인 상태를 대신 소유하지 않는다.

## 8. 입력과 게임플레이의 경계

UI Presentation의 입력 차단은 해당 로컬 플레이어의 입력 정책이다. 시간 정지·서버 무적·다른 플레이어 정지를 자동으로 뜻하지 않는다.

관찰자, 시작 Pawn, 이전 ViewTarget을 구분한다. 리스폰으로 Pawn이 바뀌었다면 이전 연출을 계속 적용할 대상이 달라졌으므로 Tick에서 취소한다.

## 9. 자동 종료와 생성 실패

재생 시간이 끝나거나 Controller·Camera가 무효가 되면 취소한다. Pawn 교체나 체력 0도 취소 조건이다. Subsystem Deinitialize 역시 Active 연출을 취소한다.

Camera 생성 실패 시 유효한 Active Handle을 유지하지 않는다. UI 요청을 해제할 때 Overlay와 Presentation은 각각 자기 Handle을 사용한다. 오래된 Cancel을 반복해도 현재 Active와 일치하지 않으면 처리하지 않는다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 현재 C++ 단일 연출 | 수명·실패 경로를 직접 추적하기 쉬움 | 아트 편집과 복잡한 시퀀스에 제한 |
| Sequencer 기반 | 카메라·음향·이벤트의 제작 도구 활용 | 재생 종료·중단·외부 상태 복구 계약 필요 |
| 여러 연출 혼합 관리자 | 겹치는 카메라·효과 조정 가능 | 우선순위·혼합·소유권 복잡도 |

현재 구조의 다음 단계로 Sequencer를 쓸 수 있어도 Cancel 책임 자체가 사라지는 것은 아니다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 자연 종료 | 카메라·소리·빛·UI 요청 정리 |
| 연출 중 사망·Pawn 교체 | 이전 Pawn용 연출 취소 |
| 새 연출 후 오래된 Handle 취소 | 새 Active 유지 |
| 다른 시스템이 ViewTarget 변경 | Cancel이 새 카메라 선택을 덮지 않음 |
| 다른 HUD 숨김 요청 존재 | 자신의 요청만 해제 |

관련 통합 검사는 [CCLIntegrationSmokeSubsystem.cpp](../../Source/CCL/Tests/CCLIntegrationSmokeSubsystem.cpp)에 있다. 이번에는 코드와 기존 기록을 읽었으며 카메라 화면·음향을 새로 실행해 확인하지 않았다.

## 12. 이해 확인

**Cancel에서 무조건 플레이어 Pawn으로 ViewTarget을 바꾸면 왜 문제가 될까?**

다른 연출이나 카메라 시스템이 이미 새 대상을 선택했을 수 있다. 자신의 Camera가 아직 사용 중일 때만 복원해야 한다.

**UI를 숨겼으니 게임도 멈췄다고 가정해도 될까?**

아니다. 화면 표시와 로컬 입력 정책을 바꾼 것이다. 월드 시간·서버 전투 중단은 별도 기능이다.
