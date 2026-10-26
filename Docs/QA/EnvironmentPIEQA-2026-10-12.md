# 환경 실험장 PIE QA: 2026-10-12

> 후속 수정과 재검증은 [종합 QA 수정과 검증](GameplayQAFixes-2026-10-12.md)에 있다. 아래 내용은 최초 검사 당시의 기록이다.

**전체 판정: 수정 필요.** 시간 배율 조작과 천체 관측은 아래 검사 항목에서 정상이다. 에디터 뷰포트 PIE를 처음 시작하면 메뉴가 활성화돼도 CommonUI 입력 정책과 이동·시점 차단이 적용되지 않는다. 메뉴를 닫고 F7로 다시 열면 정상 상태로 돌아온다. 메뉴가 열린 상태에서 PIE를 종료할 때도 입력 정리 오류가 남는다.

이 문서는 환경 실험 메뉴의 포커스, ±60배 조작, 배율 직접 입력, 천체 관측과 값 동기화를 검증한 기록이다. 환경 시스템 전체나 프로젝트 전체의 정상 동작을 인증하지 않는다. 실패 항목의 수정 방법은 제안이며 코드에는 적용하지 않았다.

## 실행 조건과 근거

| 항목 | 조건 |
|---|---|
| 실행 일시 | 2026-10-12 17:08부터 17:30까지, KST |
| 엔진·소스 기준 | UE 5.8.3, 프로젝트 HEAD `b322d5e` |
| 실행 상태 | 열려 있던 에디터와 로드된 모듈 사용. 이번 QA에서 재빌드하지 않음 |
| 맵 | `/Game/Maps/EnvironmentScenario`, `/Game/Maps/EnvironmentPlayground` |
| 실행 방식 | Scenario는 뷰포트 PIE와 별도 창 PIE, Playground는 뷰포트 PIE. 모두 Simulate 비활성 |
| 조작 | Unreal MCP의 StartPIE·StopPIE, Slate 클릭·키 입력·화면 캡처, 레벨 로드 |
| 상태 진단 | MCP로 에디터 콘솔에서 읽기 전용 Python 실행. Controller의 입력 차단·Pawn 위치를 조회하고 CommonUI 트리와 입력 정책을 덤프 |
| 종료 상태 | PIE 종료 확인 후 원래 `EnvironmentScenario` 에디터 맵으로 복귀 |

시작 전에 있던 `.mcp.json`, `CCL.uproject`, `Config/DefaultGame.ini` 변경은 유지했다. 실행 관측은 현재 에디터 상태의 결과이며, 코드 원인 분석은 위 HEAD의 소스를 기준으로 한다.

증거는 로컬 `Saved/Tests/MCP-QA-20261012/`에 보관했다. 이 폴더는 Git 제출 대상이 아니다. 아래 경로는 저장소 상대 경로이며, 로그를 인용할 때 기기별 절대 경로는 생략했다.

| 파일 | 내용 |
|---|---|
| `snapshots.json` | 조작 전후 Slate 상태와 기록 시각. 탐색 중의 중간 상태도 포함 |
| `editor.log` | 실행 종료 시점의 에디터 로그 사본. 이번 QA 이전 로그도 포함하므로 실행 시각으로 구분 |
| `probe.py`, `probes.jsonl` | 읽기 전용 진단 코드와 6회 측정값 |
| `syncshot.png`, `144shot.png` | 두 입력창의 288배·144배 일치와 관측·세계 배율 표시 |
| `highshot.png`, `limitshot.png` | 고배율 공전 화면과 관측 시간 상한에서의 자동 정지 |
| `viewport.png` | 뷰포트 PIE의 메뉴 화면 |

## 정상 항목

아래의 정상 판정은 해당 조작과 관측값에 한정한다. 화면 클릭 성공 응답만으로 판정하지 않고 후속 표시·캡처·상태 조회로 확인했다.

| QA 항목 | 실행과 확인 결과 | 판정 |
|---|---|---|
| PIE 시작·종료 | 두 맵에서 실제 PIE 진입·종료를 확인 | 정상, 종료 로그는 F-02 참조 |
| 세계 배율 ±60·정지 | Scenario에서 버튼으로 `0 → 60 → 120 → 60 → 0 → 0`. 0에서 감소해도 음수가 되지 않음 | 정상 |
| 세계 배율 직접 입력 | Enter로 144·288·0.5·0 적용. 적용 버튼으로 `6e1`을 입력해 60 확인 | 정상 |
| 잘못된 세계 입력 | `abc` 거부, 오류 안내 표시, 기존 60배 유지 | 정상 |
| 천체 관측 열기 | 구역 01에서 열면 조작 대상이 천체 관측으로 전환 | 정상 |
| 두 입력창 동기화 | 관측창에서 144배, 메뉴에서 288배, 관측창 적용 버튼으로 다시 144배. 양쪽 입력창·관측 배율 표시가 일치 | 정상 |
| 관측 ±60·정지 동기화 | 288에서 증가 348, 감소 288, 정지 0, 추가 감소 0. 양쪽 표시가 함께 갱신 | 정상 |
| 관측 입력 검증 | 0.5 허용. `abc`, `-1`, `1e309` 거부와 오류 표시. 유효한 값 적용 시 오류 해제 | 정상 |
| 고배율 공전 | `1e10` 적용 후 관측 시각과 Star 주위 World 위치 변화 확인 | 정상 |
| 세계와 관측의 분리 | 관측 배율을 바꿔도 세계 배율 0과 세계 시각 00시 20분 00초 유지 | 정상 |
| 관측 시간 상한 | `1e20` 적용 후 다음 진행이 최대 `1e12`초를 넘으면 자동 0배, 상한 안내, 마지막 유효 자세 유지 | 정상 |
| 세계 시각 복귀 | 복귀 버튼으로 세계 시각을 따르는 상태로 돌아옴 | 정상 |
| 메뉴·구역 전환 | 메뉴 닫기·재열기와 구역 02 전환 시 관측창이 닫히고 조작 대상이 세계 시간으로 복귀 | 정상 |
| Playground 조작 제한 | 세계 배율 조작은 비활성. 관측을 열면 로컬 배율 조작이 활성화돼 600000배 적용. 구역 변경 후 세계 조작은 다시 비활성 | 정상 |
| 별도 창 PIE 메뉴 입력 | Scenario 메뉴에서 빈 영역 클릭 후 I·W를 눌러도 인벤토리가 열리지 않음. 이동·시점 차단 값 모두 true | 정상, 검사한 입력에 한정 |
| 닫기·F7 재열기 | 두 맵에서 재열기 성공. 뷰포트 PIE도 재열기 후 Menu 정책과 이동·시점 차단 true 확인 | 정상 |

## 실패 항목

심각도는 **BLOCKER**를 이번 변경의 정상 판정을 막는 결함, **WARNING**을 실행을 막지는 않지만 수정해야 하는 결함으로 정의한다.

### F-01 · BLOCKER: 뷰포트 PIE 최초 메뉴의 입력 정책 누락

**현상:** `EnvironmentScenario`와 `EnvironmentPlayground`를 뷰포트 PIE로 시작하면 실험 메뉴가 활성화되고 닫기 버튼에 포커스가 있어도 `ActiveInputConfig`가 없다. Controller의 `IsMoveInputIgnored()`와 `IsLookInputIgnored()`도 모두 false다. 기대 상태는 Menu 정책과 두 차단 값 true다.

재현 순서는 다음과 같다.

1. 환경 맵을 열고 `PlayMode_InViewPort`, `bSimulate=false`로 PIE를 시작한다.
2. 자동으로 열린 실험 메뉴를 닫지 않은 상태에서 `CommonUI.DumpActivatableTree`, `CommonUI.DumpInputConfig`와 Controller 차단 값을 조회한다.
3. Scenario에서 빈 영역을 클릭하고 I·W를 누른 뒤 다시 조회한다. 입력 정책은 여전히 없다.
4. 닫기 버튼을 누른 뒤 F7로 다시 열고 조회한다. Menu 정책과 두 차단 값 true로 바뀐다.

| 측정 조건 | KST | 입력 정책 | 이동·시점 차단 | 근거 |
|---|---|---|---|---|
| Scenario 별도 창 PIE | 17:23:29 | Menu | true / true | `probes.jsonl:1`, `editor.log:3277` |
| Scenario 뷰포트 PIE 최초 | 17:24:02 | 없음 | false / false | `probes.jsonl:2`, `editor.log:3393` |
| 같은 세션에서 빈 영역·I·W 입력 후 | 17:24:49 | 없음 | false / false | `probes.jsonl:3`, `editor.log:3433` |
| 같은 세션에서 닫기·F7 후 | 17:25:37 | Menu | true / true | `probes.jsonl:4`, `editor.log:3482` |
| Playground 뷰포트 PIE 최초 | 17:26:30 | 없음 | false / false | `probes.jsonl:5`, `editor.log:3625` |
| Playground 닫기·F7 후 | 17:27:54 | Menu | true / true | `probes.jsonl:6`, `editor.log:3690` |

실패 상태에서도 CommonUI 트리의 Root와 실험 메뉴는 활성 상태였다. Scenario에서 I·W를 누른 전후 Pawn 위치는 `(-550, -270, 98.15)`로 같았다. 따라서 **실제 캐릭터 이동이나 인벤토리 입력 유출을 재현했다고 판단하지 않는다.** 확인된 결함은 메뉴가 요구하는 입력 정책과 차단이 누락된다는 점이다. 메뉴 자체의 키 처리 때문에 짧은 키 입력이 막혀도 이 정책 누락을 정상으로 볼 수 없다.

#### 코드 원인

확인된 프로젝트 경로는 다음과 같다.

- [CCLExperimentPlayerController.cpp:27](../../Source/CCL/Environment/CCLExperimentPlayerController.cpp#L27)의 `PlayerTick`은 Director와 Pawn 준비 후 메뉴를 자동으로 연다. 에디터의 게임 창 포커스 준비를 기다리지 않는다.
- [KSIUISubsystem.cpp:395](../../Source/KSIUI/Private/KSIUISubsystem.cpp#L395)의 `EnsureRoot`는 Root를 뷰포트에 추가하고 활성화한다.
- [KSIScreen.cpp:18](../../Source/KSIUI/Private/KSIScreen.cpp#L18)의 메뉴 입력 설정은 이동·시점 차단을 요구한다. 설정 정의가 없는 문제가 아니다.
- [CCLExperimentScreen.cpp:95](../../Source/CCL/Environment/CCLExperimentScreen.cpp#L95)의 `OnManagedTick`은 게임 뷰포트가 키보드 포커스를 가졌을 때 `RequestRefreshFocus()`만 요청한다. 입력 정책 재적용은 하지 않는다.

UE 5.8.3의 `<Engine>/Plugins/Runtime/CommonUI/Source/CommonUI/Private/`에서 확인한 동작은 다음과 같다.

| 엔진 위치 | 확인한 동작 |
|---|---|
| `Input/UIActionRouterTypes.cpp:97`, `:1685` | 에디터에서 게임 창이 사용자 포커스 경로에 없으면 `ApplyLeafmostNodeConfig`가 조기 반환 |
| `Input/UIActionRouterTypes.cpp:1925` | `HandleRequestRefreshLeafmostFocus`는 포커스만 갱신하며 입력 설정을 적용하지 않음 |
| `Input/CommonUIActionRouterBase.cpp:1600` | `RefreshUIInputConfig`는 기존 `ActiveInputConfig`가 있을 때만 적용. 설정이 없는 초기 상태를 단독으로 복구하지 못함 |
| `Input/CommonUIActionRouterBase.cpp:1866` | 실제 입력 설정 적용에서 Controller 차단과 Menu 진입 시 눌린 키 정리를 수행 |

**원인 추론, 신뢰도 높음:** 최초 자동 열기가 게임 창 포커스 준비보다 빨라 엔진의 적용 경로가 건너뛰어지고, 이후 프로젝트가 포커스만 복원하므로 입력 설정이 비어 있는 상태가 유지된다. 닫기·재열기로 다시 활성화할 때는 설정이 적용된다. 관측값과 코드 흐름이 일치하지만, 최초 조기 반환 지점에 중단점이나 계측을 추가한 것은 아니므로 그 순간의 호출 경로까지 직접 확정하지는 않았다.

#### 수정 방법 제안

프로젝트의 CommonUI 라우터에서 게임 창 포커스가 준비됐을 때 현재 활성 화면의 입력 설정을 다시 평가하도록 한다. 기존 [KSIUIActionRouter.cpp:12](../../Source/KSIUI/Private/KSIUIActionRouter.cpp#L12)의 `RefreshPresentationRouting()`은 현재 Root를 다시 설정하는 경로가 있으므로 재사용 후보가 된다.

포커스 변경 콜백 안에서 즉시 재진입시키기보다 다음 틱으로 예약하고, 해당 LocalPlayer의 현재 활성 Root와 최하위 화면을 기준으로 적용한다. 최초 설정 누락이나 게임 창 포커스 복귀를 계기로 실행하며 매 프레임 `SetIgnoreMoveInput`을 반복 호출하지 않는다. `RequestRefreshFocus()` 추가 호출이나 설정이 없는 상태의 `RefreshUIInputConfig()` 호출만으로 끝내면 원인이 남는다.

수정 후에는 두 맵에서 최초 PIE 메뉴를 닫지 않아도 Menu 정책과 두 차단 값 true가 되는지 확인해야 한다. 메뉴를 열기 전부터 누른 W, 마우스 시점, 게임패드, 다른 메뉴와의 포커스 우선순위, 종료 후 재시작도 회귀 검사에 포함한다. 현재 확인된 임시 우회는 **닫기 버튼 후 F7 재열기**이며, Scenario의 별도 창 PIE에서도 정상 정책을 확인했다.

### F-02 · WARNING: PIE 종료 중 무효한 입력 대상에 설정 적용

**현상:** 메뉴가 열린 뷰포트 PIE를 종료하면 아래 경고와 오류가 발생한다. PIE는 종료되고 에디터도 계속 동작했다.

```text
Failed to commit change! ViewportWidget was null.
[RemoveTagsFromInputMode] Null player input
[AppendTagsToInputMode] Null player input
```

종료 시각 17:13:12, 17:25:55, 17:28:43, 17:30:54에 같은 유형을 확인했다. 근거는 `editor.log:2749`, `:3490`, `:3698`, `:3883`부터 이어지는 로그다. 이번 실행에서는 과거 기록에 있던 자식 해제 ensure를 새로 재현하지 않았다.

#### 코드 원인

[CCLExperimentPlayerController.cpp:88](../../Source/CCL/Environment/CCLExperimentPlayerController.cpp#L88)의 `EndPlay`는 실험 화면을 닫고 등록을 해제한다. [KSIUISubsystem.cpp:348](../../Source/KSIUI/Private/KSIUISubsystem.cpp#L348)의 `CloseView`와 [KSIUIRoot.cpp:150](../../Source/KSIUI/Private/KSIUIRoot.cpp#L150)의 `RemoveScreen`이 화면을 제거하면 활성 입력 트리가 바뀐다. Root의 기본 설정은 [KSIUIRoot.cpp:24](../../Source/KSIUI/Private/KSIUIRoot.cpp#L24)의 Game 정책이다.

엔진 `<Engine>/Plugins/Runtime/CommonUI/Source/CommonUI/Private/Input/CommonUIActionRouterBase.cpp:1866`의 `ApplyUIInputConfig`는 뷰포트와 입력 모드에 변경을 적용한다. `:1997`에서 뷰포트 위젯이 없다는 경고를 출력하고, `:2009`와 `:2010`에서는 Enhanced Input의 입력 모드 태그를 갱신한다. 이 태그 호출은 뷰포트 위젯이 유효한 경우에만 실행되는 구조가 아니다.

**원인 추론, 신뢰도 높음:** PIE 정리 중 화면 제거가 입력 트리의 Game 정책 복귀를 유발하고, 이미 뷰포트 또는 PlayerInput이 정리된 시점에도 적용을 시도한다. 프로젝트의 `ResetRoot`·`OnWorldCleanup` 가드는 새 화면 생성을 막지만 라우터의 입력 설정 적용까지 차단하지 않는다. 로그와 정리 코드를 근거로 한 분석이며 종료 순간의 호출 스택은 수집하지 않았다.

#### 수정 방법 제안

일반적인 메뉴 닫기와 World·LocalPlayer 종료를 구분한다. 종료 시에는 입력 대상이 유효할 때 트리를 해제하도록 순서를 조정하거나, 프로젝트 라우터의 종료 상태에서 뷰포트·PlayerInput에 대한 적용을 생략하도록 수명 처리를 보완한다. 실제로 재정의할 엔진 훅과 종료 순서는 구현 단계에서 확인해야 한다.

로그만 숨기거나 모든 입력 차단을 무조건 초기화하지 않는다. 일반 메뉴 닫기는 Game 입력을 복원해야 하고, 다음 PIE 세션은 새 Root와 Menu 정책을 정상 구성해야 한다. 수정 후 두 PIE 방식에서 메뉴가 열린 경우·닫힌 경우의 종료와 반복 재시작을 검사한다.

## 판정 범위와 남은 검사

세계 배율 0은 이미 접수된 고배율 계산 대기를 없애는 명령이 아니다. 탐색 중 높은 세계 배율을 입력한 뒤 0으로 바꿨을 때 대기 처리가 이어지는 것을 관측했고, 새 PIE에서 대기가 없는 상태로 버튼 검사를 다시 수행했다. 이는 [기존 계산 대기 설명](../EnvironmentPlan.md#실험-메뉴-조작과-검증)과 일치한다. [KSIWorldClock.cpp:101](../../Source/KSIWorldTime/Private/KSIWorldClock.cpp#L101)의 `FKSIWorldClock::ChangeTimeScale`은 배율을 바꾸고, 같은 파일 `:155`의 `Prepare`는 접수된 구간의 배율을 사용한다. 이번 보고서의 정지 정상 판정을 대기 작업 취소로 해석하면 안 된다.

144배와 288배 입력은 검증했지만 실제 10분·5분을 기다려 게임 하루가 지나는 장시간 검사는 하지 않았다. `1e10`에서도 관측 시각과 공전 변화만 확인했으며, 고배율의 실시간 처리율을 계측하지 않았다. 키를 계속 누른 상태, 게임패드, 네트워크, 패키지, 저장 복원과 날씨·지형·눈·물·불·생태계 전체 회귀도 이번 검사에 포함하지 않았다. Graft 설치·빌드는 PIE 기능 검사 대상이 아니다.

프로젝트 코드 조회는 Graft 후 실제 소스로 확인했다. UE 그래프 도구가 제공되지 않아 엔진 원인은 설치된 UE 5.8.3 소스를 직접 확인했다. 개인 볼트는 `.local/agent-paths.json`의 `vaultRoot`가 비어 있어 조회·기록하지 않았다.

## 후속 검사

같은 날 17:40 이후 수행한 환경 두 맵의 1-11번, 전투실험장, 멀티플레이실험장, Campaign의 실제 플레이 결과는 [종합 PIE QA](FullGameplayMCPQA-2026-10-12.md)에 기록했다. 위 판정은 당시 입력·천체 검사 범위의 기록으로 유지한다.
