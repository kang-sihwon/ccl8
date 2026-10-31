# 멀티플레이 기본 흐름

`Source/CCL/`에 스폰, 이동과 카메라, 사망 후 개별 재스폰을 구현했고 Standalone, Dedicated Server, Listen Server에서 자동 검사로 확인했다. 이 문서는 이 기본 흐름의 책임 배치, 인터페이스, 조작을 소유한다. 근거는 [결정 5](decisions/0005-run-modes-and-respawn.md)다. 테스트 맵은 `Content/Maps/MultiplayerPlayground.umap`이다.

## 하위 문서

| 문서 | 읽는 때 |
|---|---|
| [검증](MultiplayerFoundation/Verification.md) | 검사 방법, 설치본별 결과, 엔진 근거가 필요할 때 |

## 목표와 제약

서버가 캐릭터의 생성과 사망을 결정하고 각 클라이언트는 자신이 소유한 캐릭터를 조작한다. Standalone도 같은 권위 경로를 쓴다. 사망한 플레이어만 재도전하며 생존자의 캐릭터와 진행은 유지한다. 평소 작업은 Standalone에서 하고 서버 검증은 Dedicated Server와 Listen Server에서 한다.

서버 검증에는 플레이어 2명을 썼으며 출시 인원 제한이 아니다. 테스트 맵에 배치한 PlayerStart 4개도 콘텐츠 사양이 아닌 검증용 배치다. 공격과 체력은 [CombatFoundation](CombatFoundation.md), 저장은 [SessionFoundation](SessionFoundation.md)이 소유한다. 동료 부활, 호스트 이탈 복구, 방 검색 UI는 이 흐름에 없다.

조작감 확인용으로 3인칭 카메라, 키보드·마우스 입력, 단순 도형 캐릭터를 쓴다. 기존 마네킹을 연결하는 대안은 사람 비례와 애니메이션 확인에 유리하지만, 기본 흐름에는 애니메이션 의존을 넣지 않았다.

## 책임과 수명

입력은 PlayerController에 두고 재도전 때 Character를 교체한다. 사망한 Pawn은 재도전 전까지 남아 있어 플레이어가 같은 시점에서 재도전을 고를 수 있다.

| 타입 | 책임 | 생성과 소멸 | 네트워크 권위 |
|---|---|---|---|
| `ACCLGameModeBase` | 시작 위치 검증, 재스폰 허용, 스폰 실패 대기 상태와 재시도 간격 관리 | 월드 생성·종료, 접속 종료 때 플레이어별 대기 기록 제거 | 서버 전용 |
| `ACCLPlayerController` | Enhanced Input 구성, 현재 Pawn 조작, 재도전 RPC | 접속부터 종료까지 유지 | 입력은 소유 클라이언트, RPC 판정은 서버 |
| `ACCLPlayerState` | 플레이어 ASC와 능력치, 인벤토리·장비·원정·계정 컴포넌트 소유. 재스폰 사이에 유지한다 | 접속부터 종료까지 유지 | 서버 권위. ASC 소유는 [결정 11](decisions/0011-minimal-combat-and-asc-lifetime.md) |
| `ACCLCharacter` | 이동, 카메라 구성, 현재 생명의 사망 상태 | 스폰부터 다음 재도전 또는 접속 종료까지 | `bDead`는 서버 결정 후 복제 |
| `UCharacterMovementComponent` | 이동 예측과 서버 이동 처리 | Character의 컴포넌트 | 엔진 이동 경로 |
| `USpringArmComponent`, `UCameraComponent` | 카메라 충돌 검사와 플레이어 시점 | Character의 컴포넌트 | 로컬 시점 |
| `ACCLHUD` | 조작 안내와 재도전 메시지 | 로컬 플레이어 HUD | 로컬 표시 |

사망은 `uint8 bDead`를 `ReplicatedUsing`으로 전달한다. 서버에서도 `OnRep_Dead`의 공통 표현 경로를 실행해 이동을 멈추고 캡슐 충돌을 해제한다.

## 인터페이스와 처리 흐름

공개 진입점은 `ACCLCharacter`의 `IsDead`, `Die`, `ACCLPlayerController`의 `CCLRetry`, `CCLDie`, `ACCLGameModeBase`의 `RequestRetry`다. 선언과 기본값은 `Source/CCL/CCLCharacter.h`, `CCLPlayerController.h`, `CCLGameModeBase.h`가 소유한다.

`CCLRetry`는 대상 Pawn이나 위치를 인자로 받지 않는 서버 RPC로 전달된다. 서버는 요청자의 현재 Pawn을 검사해 살아 있으면 거부한다. 사망한 Pawn은 소유를 해제하고 제거한 뒤 `RestartPlayer`로 교체한다. 새 Pawn 생성에 실패하면 요청자는 서버의 대기 집합에 남고 다음 입력으로 다시 시도할 수 있다. 재시도 간격은 반복 요청을 제한하는 구현 기본값이며 전투 사양이 아니다.

`FindPlayerStart_Implementation`은 캡슐이 들어갈 공간을 확인한다. 시작 위치가 모두 막혔으면 실패를 기록하고 대기 상태를 유지한다. 프로젝트의 `RestartPlayer`는 검사를 통과한 위치로 `RestartPlayerAtPlayerStart`를 호출하고, 엔진 기본 구현이 이전 `StartSpot`으로 되돌아가는 경로를 쓰지 않는다. 체크포인트는 이 시작 위치 정책을 확장해 붙인다.

`ACCLCharacter::Die`는 서버에서만 실행하고 이미 죽었으면 돌아온다. `bDead`를 세운 뒤 현재 생명의 전투 상태를 끝내고, 남은 체력을 0으로 맞추고, `OnRep_Dead`와 `ForceNetUpdate`로 표현과 복제를 처리한다.

사망 진입점은 양수 피해, 월드의 KillZ 아래 낙하, 개발용 `CCLDie`다. 체력과 피해 계산은 CombatFoundation이 소유한다. 개발용 사망 요청의 처리와 K 키 바인딩은 Shipping·Test 구성에서 비활성화된다.

입력 Action과 Mapping Context는 PlayerController가 런타임에 만들고 종료 때 해제한다. `CCL.Build.cs`에 `EnhancedInput` 의존성이 있다. 별도 입력 에셋 편집 없이 시험할 수 있지만, 디자이너가 키 배치를 바꾸려면 에셋이나 설정으로 옮겨야 한다.

## 조작과 실행

| 입력 | 동작 |
|---|---|
| WASD | 카메라 yaw를 기준으로 이동 |
| 마우스 | 3인칭 카메라 회전 |
| Space | 점프 |
| K | 개발용 사망 |
| R | 사망 또는 스폰 실패 후 재도전 |
| 콘솔 `CCLDie`, `CCLRetry` | 같은 사망·재도전 경로 호출 |

`Tools/Validation/create_playground.py`가 Unreal 에디터에서 맵을 만들고 저장한다. 바닥, 점프용 단차, 카메라 가림 확인용 벽과 시작 위치가 있다. 이 도구는 기존 맵을 덮어쓰지 않는다. 수동 확인은 에디터에서 `MultiplayerPlayground`를 열고 Standalone으로 플레이한다.
