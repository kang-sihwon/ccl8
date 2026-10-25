# 이동·사망·재도전

서버가 캐릭터 생성과 사망을 결정하고 각 플레이어는 자기 캐릭터만 조작한다. 사망한 플레이어만 재도전하고 생존자의 Pawn과 진행은 유지된다. 소유 문서는 `Docs/MultiplayerFoundation.md`다.

## 하위 기능

- `move`: 카메라 yaw 기준 이동, 3인칭 카메라, 점프
- `death`: 개발용 사망, KillZ 낙하 사망, 사망 후 이동 차단
- `retry`: 생존 중 재도전 거부, 사망 후 재도전, 반복 재도전
- `spawn-blocked`: 모든 시작 위치가 막혔을 때 대기 후 재시도
- `replication`: 원격 이동, 새 Pawn 복제, 다른 플레이어 Pawn 유지

## 사용자 진입 경로

- `/Game/Maps/MultiplayerPlayground`에서 WASD·마우스·Space로 조작한다.
- K로 사망하고 R로 재도전한다. 콘솔 `CCLDie`, `CCLRetry`도 같은 경로를 호출한다.
- 멀티플레이는 Listen 호스트와 원격 클라이언트, 또는 Dedicated 서버와 클라이언트 2개로 접속한다.

## 실행

스모크(`EI`)는 아래 네 구성을 모두 실행할 때 기능 전체를 덮는다. 변경이 서버 판정에만 닿으면 Dedicated와 Listen을 우선한다.

```powershell
.\Tools\Validation\run_network_smoke.ps1 -Mode Standalone
.\Tools\Validation\run_network_smoke.ps1 -Mode Dedicated
.\Tools\Validation\run_network_smoke.ps1 -Mode Listen -Port 18778
.\Tools\Validation\run_network_smoke.ps1 -Mode Listen -Port 18779 -DriverIsHost
```

- 기대 결과: 출력 `PASS <Mode>`, 각 로그의 `CCL_SMOKE PASS`, Standalone 로그의 `CCL Spawn blocked` 뒤 PASS.
- 증거: `Saved/Tests/NetworkSmoke/<Mode>-<시각>/`의 driver·witness·server 로그.
- MCP 경로: 키 배치나 카메라 조작감을 바꿨을 때 쓴다. 키 입력 뒤 Pawn 위치·ViewTarget·입력 차단 상태를 기록한다.

## Gotchas

- 스모크는 `-nullrhi`로 실행하므로 화면 증거가 없다. 카메라 가림이나 표시 변경은 MCP 화면 캡처로 확인한다.
- 네 구성이 서로 다른 포트를 쓰지만 순서대로 실행한다. 동시 실행은 기본 포트 18777에서 충돌할 수 있다.
