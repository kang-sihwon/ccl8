# Loadout 복원 경계

인벤토리, 장비, 스킬, GAS 효과를 실패 없이 함께 복원하는 계약이다. 구현은 `Source/CCL/Items/CCLLoadoutRestore.cpp`와 `Source/CCL/Session/CCLGameInstance.cpp`에 있다. 검증은 `Tools/Validation/run_loadout_restore.ps1`로 2026-10-10 UE 5.8.3의 EnvironmentScenario Standalone에서 했다.

## 인벤토리·장비 복원 준비와 실행 검증

기존 효과를 먼저 지운 뒤 `Equip`을 반복하면 나중 항목의 오류가 앞의 장비와 스킬을 일부 적용한 채 남긴다. 그래서 준비 단계에서 전체 장착 점유와 정의, 스킬 효과를 검사하고, 확정 단계에서는 장비를 하나씩 교환하는 UI 경로를 쓰지 않는다. 준비 자료와 상태 보류 비용이 늘지만 세계와 개인 상태의 실패 경계를 대조할 수 있다.

- **입력 검사:** `UCCLLoadoutComponent::RestoreState`가 항목, 장착 점유, 스킬, 포인트를 함께 검사한다. 장착 항목은 가방 슬롯 밖에 있어야 하고 양손 장비는 양쪽 참조를 모두 가져야 한다. 누락된 아이템, 중복 부위, 고립된 장착 항목, 잘못된 스킬과 포인트는 상태 변경 전에 거부한다. 후보를 먼저 복사하므로 호출자가 현재 배열 자체를 넘겨도 잃지 않는다.
- **허용 효과:** 비주기, 비중첩 Infinite 효과의 수치 가산 Modifier만 복원한다. 현재 체력과 스태미나를 직접 수정하는 Modifier, Execution, GameplayEffect Component, Cue, Attribute 기반 계산, 커스텀 계산은 거부한다. 정적 수치 또는 SetByCaller로 계산한 결과와 대상 AttributeSet도 본다. 조건부 효과를 더하려면 가역적인 준비와 취소 계약도 구현해야 한다.
- **교체 순서:** 새 효과를 모두 적용한 뒤 기존 효과를 없앤다. 도중 GAS가 거부하면 새 핸들만 없애고 기존 핸들, 개인 항목, 포인트, 부상을 유지한다. `FScopedAggregatorOnDirtyBatch`로 Attribute 계산 알림을 묶고, 인벤토리 알림은 항목, 장착 목록, 스킬, 포인트, 아바타 동기화 뒤 한 번 게시한다.
- **무기 부여:** 복원 성공 시 이전 무기 행동 부여를 정리해 같은 GUID가 과거 정의를 참조하지 않게 한다. 다음 무기 조작에서 복원된 정의로 능력을 준다.
- **체력:** `bResetHealth=false`는 부상을 유지하고 최종 최대 체력이 낮아지면 그 값으로 제한한다. 기존 개인 체크포인트는 기본값 `true`로 완전 체력을 유지한다.

이 계약은 임의 GAS 적용 델리게이트의 외부 부작용을 되돌리는 트랜잭션이 아니다. Attribute 알림 묶음이 효과 적용 델리게이트 자체를 지연하지 않고, 인벤토리의 트랜잭션도 모든 외부 변경을 막는 잠금이 아니다. 같은 Loadout의 조작과 복원 재진입은 거부하지만 외부 코드가 다른 상태를 수정하는 경우는 보장하지 않는다.

UE 5.8.3 근거는 `<Engine>/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/` 아래의 `Private/AbilitySystemComponent.cpp:1037`(적용 Query 거부), 2160줄(효과 적용 델리게이트), `Public/GameplayEffectAggregator.h:392`(Attribute 알림 보류)다. 이 확인만으로 해당 기능이 UE5에서 처음 도입됐다고 주장하지 않는다.

| 검사 | 결과 |
|---|---|
| 실제 PlayerState와 GAS 복원 | 실패 원본과 효과 핸들 보존, 가득 찬 가방에서 양손 장비 교체, 중복 보너스 방지, 부상과 완전 체력 정책, 이전 무기 부여 제거 통과 |
| 별도 프로세스의 개인 세션 재개 | 음식 이력, 장비, 스킬, 부분 체크포인트, 메뉴 왕복, 승리 재개 통과 |
| CCL 자동 검사 | 140/140, 경고, 실패, 미실행 0 |

## 다중 플레이어 복원 준비 경계

`FCCLLoadoutRestoreBatch::Execute`는 여러 Loadout의 효과 적용과 세계 복원을 한 동기 호출로 묶는다. 인자는 요청 배열, `CommitWorld`, `FinalizeProfiles`다. 플레이어를 순서대로 완전히 복원하는 방식보다 후보 메모리와 일괄 알림 관리가 늘지만, 뒤 플레이어의 GAS 거부로 앞 플레이어만 바뀌는 실패를 막는다.

1. 후보 배열을 복사하고 모든 대상의 인벤토리 알림과 Loadout 조작을 보류한다.
2. 새 효과를 전부 준비한다. GAS가 거부하면 세계 복원을 호출하지 않는다.
3. `CommitWorld`를 호출한다. 실패하면 모든 대상의 새 효과를 없앤다. `CommitWorld`는 실패할 때 자기 세계 상태를 보존해야 하고 플레이어 후보나 효과를 수정하면 안 된다.
4. 준비된 항목, 장비, 스킬을 확정한다. 항목 교체는 검증한 복사본을 받는 비공개 `RestoreValidated` 경로로 한다.
5. Attribute 일괄 갱신이 끝난 뒤 아바타, 체력 정책, 개인 상태 후처리를 적용하고 인벤토리 알림을 게시한다.

요청과 준비 자료는 같은 게임 스레드 호출 안에서만 산다. GAS의 일괄 갱신 잠금은 전역이므로 이미 잠금이 열린 상태에서는 거부한다. 중첩 잠금 안에서 체력을 확정하면 아직 계산하지 않은 최대 체력을 쓸 수 있기 때문이다. `UCCLLoadoutComponent::RestoreState`는 한 요청으로 이 경계를 쓰고, `UCCLGameInstance::ApplyRecord`는 `Agents->Restore`를 `CommitWorld`에 연결한다.

엔진 근거는 같은 모듈의 `Private/GameplayEffectAggregator.cpp:687`의 `FScopedAggregatorOnDirtyBatch::EndLock`이다. 전역 잠금 수가 0일 때만 Dirty Aggregator를 게시한다. 일괄 갱신은 모든 ASC의 Attribute 변경 콜백을 동시에 관측하게 만드는 트랜잭션이 아니고, 여기서 검증한 최종 관측 경계는 Loadout 인벤토리 알림이다.

| 검사 | 결과 |
|---|---|
| 두 PlayerState와 실제 세계 복원 | Standalone 통과, 종료 코드 0 |
| 개인 세션의 별도 프로세스 재개 | 쓰기, 읽기, 메뉴 재진입, 완료 체크포인트 통과 |
| 기존 CCL 자동 검사 | 143/143, 경고, 실패, 미실행 0 |

- 뒤 대상의 두 번째 GAS 효과 거부에서 세계 복원 호출은 0회였고 양쪽 기존 효과 핸들, 항목, 포인트, 부상이 유지됐다.
- 효과를 모두 준비한 뒤 손상된 세계 파일을 넘겨도 같은 값과 세계 Epoch, 시각을 유지했다.
- 정상 파일은 Epoch를 교체하고 저장 시각으로 복원했다. 두 최대 체력 100, 125를 계산한 뒤 후처리에서 체력 50, 62.5를 적용했다.
- 인벤토리 알림은 대상마다 한 번 발생했고 두 대상의 최종 값을 관측했다. 중복 대상, 중복 아이템, 외부 GAS 잠금은 세계 변경 전에 거부했다.

같은 서버 월드의 두 PlayerState로 실행했으므로 두 원격 클라이언트의 복제 검증이 아니다.
