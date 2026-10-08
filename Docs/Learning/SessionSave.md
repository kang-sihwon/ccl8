# 세션과 저장: 화면 수명과 체크포인트 복원을 분리하기

GameInstance는 맵 이동을 넘어 접속 결과와 보류 중인 저장 레코드를 보관한다. 공용 UI는 로컬 플레이어별 메뉴를 관리하고 Codec은 복원할 값의 유효성을 검사한다. `Source/CCL/Session/`에서 메뉴 열기부터 새 월드에 체크포인트를 적용하는 흐름을 이해한다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `b5d9495d99bcfb903a7f5ef205f9f5b2cfcfe45e` |
| 도입·변경 기준 | `494a0ac977c7037d8ca732eeb27edbc317162f1f` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

메뉴는 `b5d9495`에서 GameInstance가 직접 Slate를 만들고 입력을 지정했다. `494a0ac`에서 공용 UI로 이전했다. 저장은 `f19fe61`에서 도입된 뒤 아이템·장비·탄약·Agent 상태를 수용했다. 저장 버전은 커밋 기준 현재 5이며 정책의 정본은 [SessionFoundation](../SessionFoundation.md)이다.

## 2. 메뉴와 월드 이동의 경계

게임 중 메뉴를 열어도 다른 플레이어와 월드는 계속 움직인다. 메뉴는 해당 로컬 플레이어의 입력을 조정하며 서버 시간을 멈추는 기능이 아니다.

불러오기는 현재 전투 Actor의 모든 상태를 덮어쓰는 작업도 아니다. 레코드를 검증하고 새 Campaign 월드를 연 뒤 PlayerState와 ASC가 준비되면 값을 적용한다.

## 3. GameInstance, 레코드와 Codec

`GameInstance`는 같은 게임 인스턴스의 맵 이동을 넘어 유지되는 UObject다. 월드 Actor 포인터가 그 수명만큼 유효하다는 뜻은 아니다. 오래 유지할 데이터는 값과 ID로 분리한다.

Codec은 저장 바이트와 값 레코드를 변환하는 프로젝트 코드다. JSON 앞의 형식 식별자와 CRC는 우발적인 손상을 찾는다. CRC를 변조 방지나 인증으로 해석하지 않는다.

`OpenLevel`, `ClientTravel`과 플랫폼 저장 슬롯은 UE4에도 있던 엔진 계약이다. LocalPlayer별 공용 메뉴로의 이전은 프로젝트 변경이다.

## 4. 이전 메뉴의 직접 입력 관리

출처: `b5d9495`, [CCLGameInstance.cpp](../../Source/CCL/Session/CCLGameInstance.cpp):64의 `UCCLGameInstance::ShowMenu`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
void UCCLGameInstance::ShowMenu()
{
    if (Menu.IsValid() || !GetGameViewportClient() || !GetFirstLocalPlayerController()) { return; }
    auto* PC = GetFirstLocalPlayerController();
    if (auto* CCL = Cast<ACCLPlayerController>(PC))
    {
        CCL->CloseInventory();
        CCL->CloseDialogue();
    }

    PC->FlushPressedKeys();
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);
    PC->bShowMouseCursor = true;
```

이 함수는 이후 버튼 트리와 Slate 본문도 만들고 `FInputModeUIOnly`를 직접 적용했다. 한 메뉴에는 흐름이 단순하지만 다른 UI와 입력 복구를 조정해야 한다.

## 5. 이전 이유와 저장 확장

결정 17은 화면 수명·입력·문맥을 공용화하도록 정했다. GameInstance는 메뉴 행동과 세션을 제공하고 화면은 표시를 맡는다. 결정 18의 생활 통합은 저장에 AgentSimulation과 AccountId를 추가했다.

옛 Coins 값과 계정 잔액을 동시에 원본으로 사용하면 중복 지급이 생길 수 있다. 현재 저장은 AccountId가 있을 때 Coins를 0으로 기록하고 옛 저장의 금화만 이관에 사용한다.

## 6. 현재 메뉴와 불러오기

출처: `50e5838`, [CCLGameInstance.cpp](../../Source/CCL/Session/CCLGameInstance.cpp):89의 `UCCLGameInstance::ShowMenuForPlayer`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
    if (auto* CCL = Cast<ACCLPlayerController>(PC))
    {
        CCL->CloseInventory();
        CCL->CloseDialogue();
    }

    PC->FlushPressedKeys();
    auto* Context = NewObject<UCCLSessionMenuContext>(this);
    Context->Session = this;
    Context->Controller = PC;
    Context->bInCampaign = GetWorld()->GetGameState<ACCLCampaignState>() != nullptr;
    MenuHandles.Add(PC->GetLocalPlayer(), UI->OpenView(CCLUITags::View_SessionMenu, Context, PC));
}
```

앞쪽에서는 중복 메뉴·관리자 유효성을 검사하고 사라진 로컬 플레이어 항목을 제거한다. 해당 컨트롤러의 인벤토리·대화를 닫은 뒤 문맥과 화면 핸들을 연결한다. 실제 본문은 `UCCLSessionMenuScreen`이 만들고 입력은 CommonUI가 적용한다.

불러오기의 월드 전환은 다음 함수가 맡는다.

출처: `50e5838`, [CCLGameInstance.cpp](../../Source/CCL/Session/CCLGameInstance.cpp):254의 `UCCLGameInstance::LoadSession`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLGameInstance::LoadSession(bool bHost)
{
    TArray<uint8> Bytes;
    FCCLSessionRecord Record;
    if (!UGameplayStatics::LoadDataFromSlot(Bytes, SaveSlot(), 0) || !FCCLSessionCodec::Decode(Bytes, Record))
    { Status = TEXT("Cannot load: checkpoint missing, damaged, incompatible or references unavailable content."); return false; }
    Pending = Record;
    GetSubsystem<UCCLAgentSessionStore>()->ResetSession();
    bPendingRestore = 1;
    Status = TEXT("Loading checkpoint ...");
    HideMenu();
    UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/Campaign"), true, bHost ? TEXT("listen") : TEXT(""));
    return true;
}
```

파일을 읽었어도 Decode가 실패하면 새 월드를 열지 않는다. 검증한 Record는 Pending에 보관하고 이전 생활 세션을 초기화한 뒤 맵 이동을 시작한다.

## 7. 바이트 검사와 실제 적용

`FCCLSessionCodec::Decode`는 크기·식별자·CRC·버전과 필드를 읽고 옛 형식을 현재 값으로 변환한다. `Validate`는 알려진 정의, 수량·GUID·가방 칸·장비 점유·스킬·체크포인트 조합을 확인한다. 형식 1-4의 탄창은 빈 상태로 이관한다.

`AfterMap`은 복원 타이머를 시작하고 `TryRestore`는 PlayerState·Pawn·ASC Avatar 준비를 기다린다. `ApplyRecord`가 Inventory, Loadout, Expedition, Agent 스냅샷과 계정 연결, 보급품·처치 상태를 적용한다.

이 전체 과정은 현재 월드를 수정했다가 되돌리는 단일 트랜잭션이 아니다. 일부 적용 후 실패하면 `TryRestore`가 메뉴로 돌아간다. ‘레코드 사전 검증’과 ‘실행 중 적용 성공’을 구분해야 한다.

## 8. 접속과 저장 권한

`Join`은 주소 문법을 검사한 뒤 `ClientTravel`을 요청한다. 반환값 true는 연결 요청을 시작했다는 뜻이며 실제 접속 성공을 보장하지 않는다. 네트워크·이동 실패는 엔진 이벤트로 별도 수신한다.

`SaveSession`은 살아 있고 행동 중이 아닌 권위 호스트와 유효한 캠페인을 요구한다. 호스트의 개인 체크포인트를 저장하며 손님의 개인 인벤토리와 재접속 신원까지 보존하지 않는다. 화면 품질·창 모드는 로컬 `GameUserSettings`로 적용·저장한다.

## 9. 취소·종료·오류 복구

맵 이동 전 `BeforeMap`이 메뉴를 닫는다. GameInstance 종료 때 맵·네트워크 델리게이트를 해제한다. 특정 플레이어 메뉴 닫기와 맵 이동을 위한 모든 메뉴 닫기는 서로 다른 함수다.

잘못된 저장 파일은 실패했다고 자동 덮어쓰지 않는다. Agent 스냅샷은 Unreal 구조체 기반 로컬 데이터이며 신뢰할 수 없는 외부 파일을 안전하게 역직렬화하는 보안 경계는 제공하지 않는다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 모든 Actor 상태 직렬화 | 정확한 중단 시점 복원 목표 | 타이머·참조·이동·버전 호환이 복잡 |
| 값 기반 체크포인트 | 재시작 상태를 설명·검증하기 쉬움 | 적 위치·공격 타이머는 재계산 |
| 메뉴가 직접 입력 적용 | 한 화면의 코드가 단순 | 화면 중첩·플레이어 분리 비용 |
| 공용 메뉴 핸들 | 공통 수명·입력 계약 재사용 | 문맥·등록과 실패 확인 필요 |

현재 선택은 호스트 체크포인트다. 온라인 계정 서비스와 방 검색·NAT 중계가 구현된 세션 서비스로 해석하지 않는다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 잘못된 주소·없는 포트 | 문법 거부 또는 비동기 실패 표시 |
| 손상 CRC·미래 버전·중복 GUID | 복원 거부 |
| 양손 점유·탄창 범위 오류 | 검증 거부 |
| 저장 후 별도 프로세스 재개 | 장비·학습·처치·계정 상태 일치 |
| 적용 중 실패 | 메뉴로 복귀, 성공 표시 없음 |
| 두 로컬 플레이어 메뉴 | 요청자의 화면만 제어 |

기존 근거는 SessionFoundation, UIFoundation과 AgentFoundation의 저장·세션 표다. 이번에는 커밋 소스와 기록을 대조했고 새 저장·접속·빌드 검사는 수행하지 않았다.

## 12. 이해 확인

**GameInstance에 Actor 포인터를 넣으면 맵 이동 후에도 안전할까?**

GameInstance와 Actor의 수명이 다르다. 값 레코드와 ID를 보관하고 새 월드의 객체에 다시 연결해야 한다.

**Decode 성공이면 복원 전체도 성공일까?**

아니다. 자산·Pawn·ASC·생활 상태 적용이 뒤따른다. 현재는 준비를 기다리고 적용 실패 시 새 월드를 떠나는 별도 경로가 있다.
