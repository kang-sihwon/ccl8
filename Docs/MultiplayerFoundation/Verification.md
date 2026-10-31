# 멀티플레이 검증

이동, 사망, 재스폰을 고친 뒤 무엇을 검사할지, 설치본별로 어디까지 확인됐는지 볼 때 읽는다. 실행 순서와 증거 강도 기록은 `ccl8-verify` 스킬을 따른다. 엔진 경로는 Git에서 제외한 `.local/agent-paths.json`을 읽고, 도구는 자신이 시작한 프로세스만 종료한다.

## 검사 방법

네트워크 검사는 `UnrealEditor-Cmd -game`과 `-server`의 별도 프로세스를 `-nullrhi`로 실행하고, 입력은 Enhanced Input Action에 주입한다. 테스트용 `UCCLNetworkSmokeSubsystem`은 개발 구성에서 `-CCLSmoke=driver` 또는 `witness`를 명시했을 때만 생성되며, 실패하면 `CCL_SMOKE FAIL`, 통과하면 `CCL_SMOKE PASS`를 기록한다. 구간 판정은 월드 게임 시간으로 하고 전체 중단 제한은 실제 시간으로 둔다.

게임 실행 로그에는 `EditorToolset`의 `AgentSkill`, `ToolsetRegistry`의 `PythonTestRunner` Python 초기화 오류가 남는다. 해당 엔진 플러그인은 수정하지 않았고 기본 흐름 검사는 통과했다.

## 2026-10-06 결과

UE 5.8.2 설치본(Changelist 56702186)과 UE 5.8.1 설치본(Changelist 0)에서 `CCLEditor Win64 Development` 빌드가 성공했고 아래가 모두 통과했다.

| 실행 | 검증 내용 |
|---|---|
| Standalone | 이동 입력, 생존 중 재도전 거부, 사망 후 이동 차단, 반복 재도전, 카메라 대상과 입력 복구, KillZ 사망, 모든 시작 위치가 막힌 상태의 대기 후 재시도(`CCL Spawn blocked`) |
| Dedicated Server + 클라이언트 2개 | 원격 이동, 사망과 새 Pawn 복제, 다른 플레이어의 Pawn 유지 |
| Listen Server + 원격 클라이언트 | 원격 클라이언트 사망·재스폰, 호스트 Pawn 유지 |
| Listen 호스트 재스폰 | 호스트 사망·재스폰, 원격 플레이어 Pawn 유지 |

같은 날 사용자가 UE 5.8.2의 Standalone에서 이동, 점프, 마우스 회전, 사망, 카메라의 지형 충돌, 리스폰이 정상 동작함을 수동으로 확인했다.

시간 기준은 `<Engine>/Source/Runtime/Engine/Classes/Engine/World.h`의 `GetTimeSeconds`와 `<Engine>/Source/Runtime/Engine/Private/LevelTick.cpp`의 월드 Tick에서 확인했다. `GetTimeSeconds`는 월드의 `TimeSeconds`를 반환하고 월드 Tick은 게임 DeltaSeconds를 누적한다.

## 미검증

이 기본 흐름 검사에 포함하지 않은 것이다.

- UE 5.8.1 설치본에서 화면을 보며 하는 조작감 확인.
- 패키징한 게임과 전용 서버, 패킷 손실 환경, 재접속. 이후 결과는 [Packaging](../Packaging.md)과 [CombatFoundation](../CombatFoundation/Verification.md)에 있다.
- 최종 외형과 조작감 수치, 접속 인원과 협동·대전 여부는 정하지 않았다.

## 엔진 근거와 대안

아래 줄 번호는 UE 5.8.3에서 다시 확인했다.

UE 5.8.2의 `<Engine>/Source/Runtime/Engine/Private/GameModeBase.cpp:1068`에서 `HandleStartingNewPlayer_Implementation`은 조건을 확인한 뒤 `RestartPlayer`를 호출한다. 같은 파일의 `:1241`은 시작 위치 선택, `:1264`는 Pawn 확인과 생성, `:1362`는 `Possess`를 맡는다. 이미 Pawn이 있으면 그 Pawn을 쓰므로 재스폰 전에 이전 Pawn을 정리한다.

`<Engine>/Source/Runtime/Engine/Private/Components/CharacterMovementComponent.cpp:8907`의 `ReplicateMoveToServer`와 `:9137`의 `CallServerMovePacked`를 포함한 엔진 이동 경로를 쓴다. 별도 위치 RPC와 보간기는 이동 규칙을 자유롭게 만들 수 있지만 예측·보정 구현과 검증 부담이 커진다.

입력은 `<Engine>/Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputSubsystemInterface.h:265`의 `AddMappingContext`를 쓴다. UE4에서 흔히 쓰던 `DefaultInput.ini` Action/Axis 이름 바인딩과 다르다. GameMode, PlayerController, Character의 역할 분리는 UE5만의 신규 구조가 아니다.
