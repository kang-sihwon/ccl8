# 캠페인 진행: 적의 사망을 월드 목표로 바꾸기

캠페인은 적 클래스가 승리를 직접 결정하지 않도록 Director와 GameState를 둔다. `Source/CCL/Campaign/CCLCampaignDirector.cpp`와 `CCLCampaignState.cpp`에서 사망 통지가 경비병 처치, 보스 생성, 승리와 늦은 접속자의 표시로 이어지는 과정을 이해한다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `30952c8bcf65b597d7ee9de5237a86ce42430fc2` |
| 도입·변경 기준 | `b1ee3a1c9c72c29b5781ef609475ad29784c5eb9` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전 기준은 개별 전투 검증 단계이며 캠페인 Director가 없다. `b1ee3a1`이 이 시스템의 최초 구현이다. 없는 이전 Director 코드를 가정하지 않고 최초 구현의 처리와 현재의 저장 연결을 비교한다. 사양은 [CampaignFoundation](../CampaignFoundation.md)이 소유한다.

## 2. 개인 사망과 공유 진행

일반 적을 처치한 뒤 플레이어가 죽었다고 이미 연 길까지 되돌아가면 개별 재스폰과 맞지 않는다. 월드 진행은 GameState에 남기고 개인 생명은 Character에서 교체해야 한다.

늦게 접속한 플레이어도 현재 보스 단계나 승리를 알아야 한다. 한 번 방송한 사망 이벤트만으로 UI를 만들면 그 이벤트 이전에 없었던 클라이언트는 진행을 복구하기 어렵다.

## 3. GameMode, GameState와 Director

GameMode는 서버 규칙을 실행하고 GameState는 클라이언트가 조회할 복제 상태를 담는다. 둘의 구분은 UE4에도 있었다. Director는 엔진 고정 역할이 아닌 프로젝트의 콘텐츠 연결 Actor다.

`TWeakObjectPtr`는 적을 강제로 살려 두지 않는 참조다. `TSet`의 처치 기록은 같은 적의 사망을 중복 반영하지 않게 한다. 적 수와 구간은 복제 상태로 전달하고 클라이언트가 자신에게 보이는 Actor 수로 승리를 추론하지 않는다.

## 4. 최초 구현의 사망 통지

Director 소속의 죽은 적인지와 중복 처리 여부를 확인한 뒤 진행을 바꾼다.

출처: `b1ee3a1`, [CCLCampaignDirector.cpp](../../Source/CCL/Campaign/CCLCampaignDirector.cpp):91의 `ACCLCampaignDirector::NotifyEnemyDefeated`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void ACCLCampaignDirector::NotifyEnemyDefeated(ACCLEnemyCharacter* Enemy)
{
    if (!HasAuthority() || !State.IsValid() || !IsValid(Enemy) || !Enemy->IsDead() || Defeated.Contains(Enemy) ||
        State->GetPhase() == ECCLCampaignPhase::Error || State->GetPhase() == ECCLCampaignPhase::Victory)
    {
        return;
    }

    if (Enemy == Boss.Get() && State->GetPhase() == ECCLCampaignPhase::Boss)
    {
        Defeated.Add(Enemy);
        State->SetProgress(ECCLCampaignPhase::Victory, 0);
        UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN victory"));
        return;
    }

    if (!Guards.Contains(Enemy))
    {
        return;
    }

    Defeated.Add(Enemy);
    const int32 Remaining = Guards.Num() - Defeated.Num();
    State->SetProgress(ECCLCampaignPhase::Road, Remaining);
    if (Remaining == 0)
    {
        Boss = SpawnEnemy(BossLocation, true);
        State->SetProgress(Boss.IsValid() ? ECCLCampaignPhase::Boss : ECCLCampaignPhase::Error, 0);
    }
}
```

이 함수에서 보스와 경비병은 진행에 미치는 결과가 다르다. 전투 적중 정책에는 이런 분기가 없다. 같은 EnemyCharacter를 전투 실험장에서도 사용할 수 있는 이유다.

## 5. 저장과 생활 통합의 계기

이후 세션 저장이 추가되면서 단순한 ‘현재 적 수’ 외에 어떤 경비병을 처치했는지 복원해야 했다. SessionFoundation은 처치 체크포인트와 승리, 남은 보급품을 저장하도록 정한다.

Agent 통합 뒤에는 살아 있는 적의 부상도 별도 생활 스냅샷으로 이어진다. 위치·공격 타이머까지 동일 시점으로 되감는 전체 월드 저장은 현재 정책이 아니다.

## 6. 현재의 진행 처리

출처: `50e5838`, [CCLCampaignDirector.cpp](../../Source/CCL/Campaign/CCLCampaignDirector.cpp):110의 `ACCLCampaignDirector::NotifyEnemyDefeated`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
void ACCLCampaignDirector::NotifyEnemyDefeated(ACCLEnemyCharacter* Enemy)
{
    if (!HasAuthority() || !State.IsValid() || !IsValid(Enemy) || !Enemy->IsDead() || Defeated.Contains(Enemy) ||
        State->GetPhase() == ECCLCampaignPhase::Error || State->GetPhase() == ECCLCampaignPhase::Victory)
    {
        return;
    }

    if (Enemy == Boss.Get() && State->GetPhase() == ECCLCampaignPhase::Boss)
    {
        Defeated.Add(Enemy);
        State->SetProgress(ECCLCampaignPhase::Victory, 0);
        UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN victory"));
        return;
    }

    if (!Guards.Contains(Enemy))
    {
        return;
    }

    Defeated.Add(Enemy);
    const int32 Remaining = Guards.Num() - Defeated.Num();
    State->SetProgress(ECCLCampaignPhase::Road, Remaining);
    if (Remaining == 0)
    {
        Boss = SpawnEnemy(BossLocation, true);
        State->SetProgress(Boss.IsValid() ? ECCLCampaignPhase::Boss : ECCLCampaignPhase::Error, 0);
    }
}
```

첫 구현과 비교해 보스 사망을 한 번만 승리로 반영하는 골격은 유지된다. 경비병을 식별하는 상태와 복원 경로가 추가돼도 클라이언트가 ‘승리했다’는 값을 서버에 보내는 인터페이스는 만들지 않는다.

## 7. 사망에서 화면까지

EnemyCharacter의 사망 통지를 Director가 받고, Director는 GameState의 `SetProgress`를 호출한다. 모든 경비병이 끝나면 보스를 생성하고, 보스가 죽으면 Victory를 기록한다. HUD는 이 상태에서 목표 문장을 만든다.

저장 복원에서는 `RestoreCheckpoint`가 처치 상태를 적용해 적 구성을 다시 만든다. 플레이어의 개인 퀘스트 수락·보상 수령은 [마을 서비스](VillageServices.md)의 ExpeditionComponent가 별도로 보관한다. 공유 승리와 개인 수령 여부를 섞지 않는다.

## 8. 권한과 수명

Director의 생성·집계는 서버 책임이고 GameState의 복제 결과는 읽기용이다. Director는 캠페인 월드 수명, PlayerState의 퀘스트는 플레이어 수명, Character는 한 번의 생명 수명을 따른다.

월드를 나가면 Director와 적 Actor는 종료된다. 다음 월드에서 보존할 상태는 Actor 포인터 대신 체크포인트 값으로 전달한다.

## 9. 생성 실패와 중복 통지

적 생성에 실패하면 Error 상태를 사용한다. 적이 없다는 이유로 Victory를 계산하면 생성 오류가 성공으로 위장되므로, 생성 완료와 처치 완료를 구분해야 한다.

Director 소속이 아닌 적, 아직 살아 있는 적, 이미 처리한 적의 통지는 거부한다. 저장 적용도 유효한 처치 마스크와 승리 조합을 검증한다. 세션의 전체 적용 실패는 [세션·저장](SessionSave.md)의 새 월드 복구 경로를 따른다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 클라이언트에서 적 수 집계 | UI 구현이 짧음 | 복제 가시성·늦은 접속에 따라 어긋남 |
| 적 클래스에 캠페인 진행 내장 | 직접 연결이 쉬움 | 전투 실험장과 다른 콘텐츠 재사용이 어려움 |
| Director + GameState | 콘텐츠 연결과 공개 상태를 분리 | 초기화·적 식별·실패 상태를 관리해야 함 |

현재는 작은 고정 캠페인에 맞춘 Director다. 범용 퀘스트 그래프나 다중 지역 진행을 모두 구현한 시스템으로 확대 해석하지 않는다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 경비병 한 명 처치 후 플레이어 재스폰 | 처치 진행 유지 |
| 같은 적 사망 통지 반복 | 집계 한 번 |
| 일반 적이 남은 상태 | 보스 단계 조기 진입 방지 |
| 승리 후 늦은 접속 | 같은 Victory 상태 수신 |
| 적 생성 실패 | Error, 잘못된 승리 없음 |

기존 검사와 내비게이션 결함 수정은 CampaignFoundation의 실행 근거에 있다. 이번 작성에서는 코드와 과거 기록을 대조했으며 게임 실행은 새로 하지 않았다.

## 12. 이해 확인

**Victory를 한 번 보내는 RPC만으로 충분할까?**

늦은 접속자가 현재 상태를 복원하기 어렵다. GameState에 상태를 남겨 복제해야 한다.

**개인 퀘스트 수령 상태를 GameState에 두지 않은 이유는 무엇일까?**

월드 승리는 공유하지만 수락·수령은 플레이어마다 다르기 때문이다. 개인 상태는 PlayerState의 기능 컴포넌트에 둔다.
