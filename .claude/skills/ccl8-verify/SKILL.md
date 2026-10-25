---
name: ccl8-verify
description: ccl8 게임 동작의 실행 검증에 쓴다. 게임 동작을 바꾼 코드·에셋 작업의 완료 보고 전, "스모크 돌려", "PIE로 확인", "실제로 되는지 봐 줘", 기능 회귀 확인이 신호다. 기능에 맞는 Tools/Validation 스모크나 Unreal MCP PIE를 고르고 증거와 증거 강도를 기록한다. 프로젝트 파일 생성과 빌드는 Docs/Coding.md를 따른다.
---

# ccl8 실행 검증

검증은 실제 실행의 증거로 끝난다. 빌드 통과, 코드 읽기, 이전 실행 기록은 이번 변경의 증거가 아니다. 빌드 순서와 성공 기준은 `../../../Docs/Coding.md`의 "코드 작업 완료 검증"이 소유한다. 이 스킬은 빌드 이후의 실행 검사를 맡는다.

## 절차

1. **범위**: [features/README.md](features/README.md)의 색인에서 변경과 관련된 기능을 모두 고른다. 기능 파일이 있으면 읽는다. 기능마다 스모크 스크립트나 MCP 경로 중 하나 이상이 정해지면 끝난다. 색인에 없는 기능이면 아래 "지도 갱신"부터 한다.
2. **Doctor**: 실행 전에 읽기 전용으로 확인한다. 모든 항목의 결과를 보고에 적는다.
   - `.local/agent-paths.json`의 `engineRoot` 아래에 `Binaries/Win64/UnrealEditor-Cmd.exe`가 있는가.
   - `Get-Process UnrealEditor*`에 이 프로젝트를 연 에디터가 있는가. 있으면 사용자 세션일 수 있으므로 종료하지 않는다. 같은 맵·포트를 쓰는 스모크는 미루고 보고한다.
   - `Binaries/Win64/UnrealEditor-CCL.dll`의 수정 시각이 Source를 바꾼 마지막 커밋(`git log -1 --format=%ci -- Source`)과 작업 트리 변경보다 나중인가. 아니면 Coding 절차로 빌드한 뒤 실행하거나 "바이너리가 소스보다 오래됨"을 결과 옆에 적는다. checkout으로 Source 파일의 수정 시각이 일괄 갱신되므로 파일 시각 대신 커밋 시각과 비교한다.
   - MCP 경로를 쓸 때만: `http://127.0.0.1:8000/mcp` 연결과 에디터의 Unreal MCP 서버 상태.
3. **Drive**: 기능별로 하나를 고른다.
   - **(A) 스모크**: `Tools/Validation/run_<name>.ps1`을 백그라운드로 실행한다. `-Mode`, `-Port`, 시간 제한 같은 인자는 스크립트 첫머리의 `param` 블록에서 확인한다. 시간 제한 인자의 이름은 스크립트마다 `-TimeoutSeconds` 또는 `-StartupTimeoutSeconds`로 다르다. 네트워크 모드가 있는 스크립트는 변경이 닿는 모드를 모두 실행한다(Standalone, Listen, Dedicated). 스크립트마다 기본 포트가 고정돼 있으므로(예: 세션 18791) 스모크를 동시에 돌리지 않는다.
   - **(B) MCP PIE**: 실제 키·마우스·Slate 조작이 필요할 때 쓴다. 준비, 실행 구성, 입력 주입, 판정, 정리는 [references/mcp-pie.md](references/mcp-pie.md)를 따른다. 도구의 클릭 성공 응답이 아니라 Pawn·GAS·HUD·저장 레코드의 실제 변화로 판정한다.
4. **Evidence**: 기능마다 다음을 남긴다.
   - 스모크: 스크립트 출력의 `PASS ...: <log>` 줄, 로그의 `CCL_<SUITE> PASS`와 `CHECK` 줄, `FAIL`·`Assertion failed:`·`Fatal error:`가 없다는 확인, 결과 폴더 `Saved/Tests/<Suite>/<시각>/`.
   - MCP: `Saved/Tests/MCP-<주제>-<날짜>/`에 상태 JSON·입력 기록·캡처·로그 사본.
   - **증거 강도**를 색인의 분류로 적는다. `EI`(Enhanced Input 주입), `Ability`(Ability 입력 태그), `Slate`(Slate 이벤트), `internal`(Controller·Subsystem 직접 호출), `MCP`(PIE 실제 조작). 키 배치·입력 매핑·위젯 포커스를 바꾼 변경을 `internal`이나 `Ability` 스모크만으로 검증했다고 보고하지 않는다.
5. **Cleanup**: 이번 실행이 시작한 프로세스만 정리한다. 스모크 스크립트는 `finally`에서 자기 PID를 종료하므로, 실행 뒤 `Get-Process UnrealEditor*`로 남은 프로세스가 없는지만 확인한다. MCP 경로는 PIE 종료, 원래 에디터 맵 복귀, 임시 네트워크·창 설정 복원까지 한다. 증거 폴더는 지우지 않는다.
6. **보고**: 기능마다 실행 명령, 결과(PASS·FAIL·미실행), 증거 경로, 증거 강도, Doctor 경고, 확인하지 못한 범위를 적는다. FAIL은 로그의 첫 `FAIL` 줄을 인용한다. 소유 문서의 검증 상태 절에 기록할 결과는 `ccl8-docs`로 옮긴다.

## 지도 갱신

새 `run_*.ps1`이나 `-CCL...` 명령행 옵션으로 생성되는 smoke subsystem을 추가했으면 같은 작업에서 색인에 행을 추가한다. 실행 중 지도와 실제가 어긋나면 다음처럼 나눈다.

- **문서 차이**: 지도·기능 파일의 설명이 실제 경로와 다르다. 지도를 고친다.
- **도구 공백**: 동작은 정상인데 스모크나 MCP 절차로 조작할 수 없다. 공백을 기능 파일의 Gotchas에 적고 스크립트 보강은 별도 작업으로 제안한다.
- **제품 결함**: 동작 자체가 틀렸다. 지도를 고치지 않고 결함으로 보고한다.

입력 경로 분류는 `Source/CCL/Tests/`에서 `InjectInputForAction`, `AbilityInputTagPressed`, `Process(Mouse|Key)…Event` 사용 여부로 정한다.

## Gotchas

- 스모크 캡처는 실행별 폴더가 아니라 `Saved/Tests/Presentation/`에 같은 이름으로 덮어쓴다. 캡처를 증거로 쓰려면 실행 직후 수정 시각을 확인하고 결과 폴더로 복사한다.
- `run_campaign_smoke.ps1`, `run_combat_smoke.ps1`, `run_network_smoke.ps1`은 시작 대기 제한만 기본 600초다. Campaign은 서버 표식을 추가로 180초 더 기다린다. 셸 도구의 기본 제한보다 길어서 백그라운드 실행이 필요하다.
- `run_integration_smoke.ps1`과 `run_agent_world_smoke.ps1`은 기능 문서가 언급하지 않는다. 이 둘의 결과는 이 스킬의 보고에만 남는다.

## 검증 상태

- 2026-10-25: (A) 스모크 경로를 `run_integration_smoke.ps1`로 한 번 수행했다. UE 5.8.3, PASS, CHECK 9개, 결과 `Saved/Tests/Integration/20261025-132535/`, 캡처 3개 갱신, 종료 후 남은 에디터 프로세스 없음. Doctor에서 CCL DLL(2026-10-13 23:39)이 Source를 바꾼 마지막 커밋 `549d183`(2026-10-14 18:45)보다 오래된 것을 확인했고, 빌드 없이 실행했다.
- (B) MCP PIE 경로는 이 스킬 작성 시점에 Unreal MCP가 연결되지 않아 수행하지 않았다.
