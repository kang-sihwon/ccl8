# 마을 서비스: 상호작용 조건과 실제 자원 이전

마을의 대화·퀘스트·구매는 ExpeditionComponent가 콘텐츠 조건을 확인하고, 금화와 상점 재고는 공통 계정·경제 경로가 변경한다. `Source/CCL/Campaign/CCLExpeditionComponent.cpp`와 `Agents/CCLAccountComponent.cpp`를 따라 ‘구매 버튼을 눌렀다’가 실제 지급으로 이어지는 과정을 살펴본다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `2d39c5f7db4dfedadf4fe9825992cad63eb53044` |
| 도입·변경 기준 | `aea2debe066b2d95fd41cb9f83100e551a0bbadb` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전에는 ExpeditionComponent의 Coins를 직접 줄였다. 생활 경제 통합 뒤에는 AccountComponent가 실제 계정과 재고를 연결한다. 게임 규칙과 과거 검증은 [ContentFoundation](../ContentFoundation.md), 자원 계약은 [AgentFoundation](../AgentFoundation.md)이 소유한다.

## 2. 대화 표시와 거래 성공

NPC 옆에서 구매를 요청한 뒤 서버 응답이 올 때까지 플레이어가 멀어질 수 있다. UI 표시 가능 여부와 서버 거래 가능 여부는 각각 검사해야 한다. 대화창이 열려 있다는 이유로 서버가 거리 검사를 생략할 수 없다.

원정 승리도 개인 보상을 자동 지급하는 사건은 아니다. 플레이어가 수락했고 승리 후 돌아왔으며 아직 수령하지 않았는지를 확인해야 한다.

## 3. 개인 상태와 어댑터

`UActorComponent`는 Actor에 붙는 기능 객체다. ExpeditionComponent는 PlayerState에 있으므로 Character 재스폰을 넘어 개인 수락 상태를 유지한다. `COND_OwnerOnly`는 해당 복제 속성을 소유 연결에만 전달한다.

어댑터는 서로 다른 데이터 표현을 연결하는 코드다. 이 프로젝트의 플레이어 가방은 GUID별 아이템이고 생활 경제의 재고는 자원 태그별 수량이다. AccountComponent가 포션 구매를 두 표현 사이에서 연결한다. 어댑터가 있다고 모든 아이템의 자유 거래가 구현된 것은 아니다.

## 4. 이전의 구매

출처: `2d39c5f`, [CCLExpeditionComponent.cpp](../../Source/CCL/Campaign/CCLExpeditionComponent.cpp):48의 `UCCLExpeditionComponent::Buy`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLExpeditionComponent::Buy(ACCLVillageSteward* Steward)
{
    if (!CanInteract(Steward)) { return Report(false, TEXT("Approach the steward while out of combat.")); }
    if (Coins < 10) { return Report(false, TEXT("Need 10 coins for a recovery potion.")); }
    auto* Item = LoadObject<UCCLItemDefinition>(nullptr, TEXT("/Game/Progression/DA_RecoveryPotion.DA_RecoveryPotion"));
    auto* Player = CastChecked<ACCLPlayerState>(GetOwner());
    if (!Item || !Player->GetInventory()->Add(Item, 1).IsValid()) { return Report(false, TEXT("Purchase rejected: inventory full or supply unavailable.")); }
    Coins -= 10;
    return Report(true, TEXT("Purchased recovery potion. I: inventory, H: use selected item."));
}
```

먼저 아이템을 넣고 성공했을 때 Coins를 줄인다. 가방이 가득 차면 비용을 차감하지 않는 순서는 이미 있었다. 다만 판매자의 재고와 계정으로 돈을 이전하는 구조는 아니었다.

## 5. 변경 이유

결정 18은 돈을 계정이, 물품을 인벤토리가 소유하도록 정했다. 같은 돈을 Expedition과 생활 Agent가 각각 복사해 관리하면 어느 쪽이 원본인지 모호해진다.

현재는 퀘스트 조건을 Expedition에 유지하면서 경제 변경을 Account로 옮겼다. 이는 공용 경제 코드에 ‘동쪽 길의 보스를 잡았는가’를 넣지 않기 위한 책임 분리다.

## 6. 현재의 구매

출처: `50e5838`, [CCLExpeditionComponent.cpp](../../Source/CCL/Campaign/CCLExpeditionComponent.cpp):59의 `UCCLExpeditionComponent::Buy`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLExpeditionComponent::Buy(ACCLVillageSteward* Steward)
{
    if (!CanInteract(Steward)) { return Report(false, TEXT("Approach the steward while out of combat.")); }
    if (GetCoins() < 10) { return Report(false, TEXT("Need 10 coins for a recovery potion.")); }
    auto* Item = LoadObject<UCCLItemDefinition>(nullptr, TEXT("/Game/Progression/DA_RecoveryPotion.DA_RecoveryPotion"));
    auto* Player = CastChecked<ACCLPlayerState>(GetOwner());
    auto* Account = GetOwner()->FindComponentByClass<UCCLAccountComponent>();
    if (!Account || !Account->Purchase(Player->GetInventory(), Item, 10))
    {
        return Report(false, TEXT("Purchase rejected: inventory full or supply unavailable."));
    }
    return Report(true, TEXT("Purchased recovery potion. I: inventory, H: use selected item."));
}
```

`GetCoins`는 계정 잔액을 읽는다. `Purchase`는 실제 아이템, 소유 인벤토리, 가격과 잔액을 다시 확인한다. 클라이언트가 임의 가격이나 보상량을 정하는 계약이 아니다.

## 7. Purchase 내부와 보상

AccountComponent는 재고와 가방 공간을 확인한 뒤 가방의 변경 알림을 보류한다. 아이템을 추가하고 거래 요청에 외부 목적지 계정·지급 아이템 GUID를 기록한다. 경제 실행에 실패하면 가방 스냅샷을 복원한다.

출처: `50e5838`, [CCLAccountComponent.cpp](../../Source/CCL/Agents/CCLAccountComponent.cpp):148의 `UCCLAccountComponent::Purchase`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
    const bool bSucceeded = CCLEconomy::Execute(Economy, Request).bSucceeded != 0;
    if (!bSucceeded)
    {
        Inventory->Restore(Before);
    }

    RefreshView();
    Inventory->EndTransaction(bSucceeded);
    return bSucceeded;
}
```

위 발췌 앞에는 계정·재고·가격 검사와 지급 준비가 있다. `EndTransaction`은 알림 공개 시점을 제어하며 데이터베이스의 영속 트랜잭션을 제공하는 API는 아니다. 메모리 안의 가방·계정 변경을 함께 관찰하도록 만든 프로젝트 계약이다.

퀘스트 `Talk`는 Available을 Accepted로 바꾸고, Accepted 상태에서 월드 Victory를 확인한 뒤 `Reward`를 호출한다. 금고에서 계정으로 이전에 성공하면 Rewarded를 기록하고 훈련 포인트를 지급한다. 생활 주민과의 대화는 별도로 `DescribeLife`를 반환한다.

튜토리얼은 별도의 수행 이력을 저장하는 시스템이 아니다. `UCCLExpeditionComponent::GetTutorial`이 Quest와 Campaign Phase에서 안내 문장을 선택한다. Available이면 마을 준비, Accepted이면 동쪽 전투, Victory이면 귀환 수령, Rewarded이면 새 훈련 포인트 안내를 반환한다. HUD는 이 문장을 읽으므로 개인 수락·수령과 공유 승리 상태가 바뀌면 안내도 바뀐다.

## 8. 서버 판정과 로컬 표시

`CanInteract`는 서버 권한, 생존, Busy·Stagger와 NPC의 거리·시야를 확인한다. 거래 응답은 해당 플레이어의 대화 문맥으로 전달된다. 늦은 응답의 화면 표시 검사는 [HUD·대화](HUDDialogue.md)에서 다룬다.

월드 승리는 GameState가 공유하고 개인 Quest는 소유자에게 복제한다. 계정도 소유자 전용 조회 값을 복제하며 생활 시뮬레이션 전체를 클라이언트에 보내지 않는다.

## 9. 실패와 종료

잔액·상점 재고·가방 공간이 부족하면 구매가 실패한다. 보상 계정이 준비되지 않으면 Quest를 Rewarded로 바꾸지 않는다. 이미 받은 보상은 다시 지급하지 않는다.

화면을 닫는 것은 거래를 되돌리는 명령이 아니다. 서버에서 이미 확정한 결과와 로컬 창의 수명을 구분해야 한다. 재스폰 후 개인 Quest와 계정은 유지되지만 호스트 체크포인트의 저장 범위는 SessionSave의 제한을 따른다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 개인 Coins 직접 변경 | 단일 상점 구현이 간단 | 실제 판매자 재고·원장과 분리 |
| 공통 자원 계약 + 구매 어댑터 | 플레이어와 Agent의 자원 이전을 연결 | 가방 알림·실패 복원·외부 지급 기록이 필요 |
| 범용 퀘스트 그래프 | 많은 퀘스트 제작에 유리 | 현재 단일 원정에는 정의·저장·도구 비용이 큼 |

현재 구매 어댑터는 회복약과 정해진 가격을 검사한다. 범용 상점 시스템으로 일반화하는 작업은 남아 있다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 먼 거리에서 요청 | 거부 |
| 가방 가득 참·재고 소진 | 금화와 아이템 부분 변경 없음 |
| 원정 수락 전·승리 전 보상 요청 | 수령 조건 미충족 |
| 승리 뒤 두 번 수령 | 첫 요청만 보상 |
| 개인 재스폰·늦은 접속 | 개인 상태와 월드 승리를 각각 처리 |

기존 ContentFoundation과 AgentFoundation의 NPC 거래·보상 검사 기록을 읽었다. 이번 문서 작성에서 실행 검사는 새로 하지 않았다.

## 12. 이해 확인

**왜 구매 조건을 UI 버튼의 활성화로만 제한할 수 없을까?**

요청 시점의 거리·재고·잔액은 바뀔 수 있고 클라이언트 요청은 서버 판정을 대신하지 못한다.

**가방에 먼저 넣은 뒤 경제 실행이 실패하면 무엇을 해야 할까?**

이전 가방 상태로 복원하고 성공 알림을 공개하지 않아야 한다. 그렇지 않으면 돈을 지불하지 않은 지급이 남는다.
