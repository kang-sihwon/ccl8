# 실험 메뉴

실험 메뉴의 입력, 시간 표시, 배율 조작을 바꿀 때 읽는다.

## 실험 메뉴 조작과 검증

두 환경 실험장의 메뉴는 **F7**로 열고 닫는다. 닫기 버튼을 사용한 뒤에도 F7로 다시 연다. 메뉴가 닫힌 동안 화면 왼쪽 아래의 HUD에 같은 키를 표시한다. PIE에서 마우스를 해제했다면 게임 화면을 클릭해 입력 초점을 돌린다.

메뉴의 게임 시간, 세계 시간과 완료 세계 시각은 `01시 23분 45초` 형식으로 표시한다. 초 미만은 버리고, 누적 시간이 하루를 넘으면 `25시 00분 00초`처럼 계속 표시한다. 시간 배율과 미처리 대기량은 별도 항목이며 대기량은 소수 초를 유지한다. 표시 문자열만 변환하므로 계산·저장·복제는 기존 초 단위를 사용한다. 구현은 `Source/CCL/Environment/CCLExperimentScreen.cpp`의 `FormatElapsedTime`과 화면 텍스트 바인딩에 있다.

격리 실험 맵은 **시간 정지**, **60배 감소**, **60배 증가**와 배율 입력창을 제공한다. 조작 대상이 **세계 시간**일 때 입력 후 Enter 또는 **배율 적용**을 누르면 서버에 요청한다. 0 이상의 유한한 정수·소수·지수 표기를 받으며 0은 세계 시간 정지다. 기본 배율은 60배이고 감소의 최솟값은 0배다. 조작 담당자만 변경할 수 있고, 자동 검사 실행 중에는 서버가 요청을 거부한다. 입력값은 `CCLExperimentPlayerController`의 RPC와 `CCLExperimentDirector::Execute`를 거쳐 공통 시계에 적용한다. 기존 저장의 배율·이력 형식은 유지한다.

세계 배율로 입력한 시간이 계산 예산을 넘으면 대기가 쌓인다. 빠른 공전 관찰에는 아래 천체 관측창의 독립 관측 배율을 사용한다. 세계 배율 직접 입력의 검증 범위는 숫자 적용·잘못된 입력 거부·서버 게시 값이며, 고배율의 지속 계산 성능과는 구분한다.

실험 메뉴는 빈 공간 클릭과 자식 위젯이 처리하지 않은 키를 받아 게임 입력으로 전달하지 않는다. 최초 포커스는 보이는 닫기 버튼이며 배율 입력 후 Enter를 눌러도 입력칸의 포커스를 유지한다. 게임 뷰포트에 키보드 포커스가 돌아오면 `RequestRefreshFocus`로 활성 메뉴의 포커스를 복원한다. 이 API는 최하위 활성 화면만 갱신하므로 다른 메뉴의 포커스를 빼앗지 않는다. 엔진 근거는 UE 5.8.3의 `<Engine>/Plugins/Runtime/CommonUI/Source/CommonUI/Public/CommonActivatableWidget.h`다. 지형 편집 도구의 메뉴 밖 클릭은 화면이 직접 처리하는 기존 경로를 유지한다.

`CCLExperimentPlayerController`는 게임 입력의 열기를, `CCLExperimentScreen`은 메뉴가 입력을 소유할 때의 닫기를 처리한다. CommonUI의 메뉴 정책과 닫힌 뒤 게임 입력 복구는 유지한다. UE 5.8.3의 F8은 에디터의 `PossessEjectPlayer`에 배정돼 있으므로 실험 메뉴에는 사용하지 않는다. 근거는 `<Engine>/Source/Editor/UnrealEd/Private/Kismet2/DebuggerCommands.cpp`와 `<Engine>/Source/Runtime/Engine/Private/GameViewportClient.cpp`이다.

회귀 검사 `Tools/Validation/run_experiment_menu_smoke.ps1`은 실제 Slate 클릭과 키 입력으로 닫기 버튼, F7 재열기, F7 닫기와 두 번째 재열기를 실행한다. 키를 누른 채 발생하는 반복 입력이 메뉴를 다시 닫지 않는지도 검사한다. 메뉴 상태, 이동·시점 입력 정책과 플레이어 소유권을 확인한다. `-Map EnvironmentScenario` 또는 `-Map EnvironmentPlayground`를 지정하며, `-PIE`를 추가하면 실제 에디터 뷰포트에서 검사한다. 기본은 Standalone이다.

## 검증 기록

| 날짜와 범위 | 결과 | 미검증 |
|---|---|---|
| 2026-10-10 메뉴 열기·닫기 | 프로젝트 파일 생성, Editor 빌드, 두 맵의 PIE와 Standalone 성공. 두 번의 재열기와 닫힌 화면의 F7 안내를 확인했고 PIE는 조종 해제로 전환되지 않았다 | |
| 세계 배율 직접 입력 | 두 맵 Standalone 통과. Scenario의 실제 적용 버튼으로 0.5·6000·60000·600000·0배와 잘못된 문자열 거부를 확인했고, Enter로 3600배를 적용했다. 서버의 음수 거부와 Playground의 입력창·버튼 비활성화도 확인했다 | PIE, 네트워크, 패키지 |
| 2026-10-12 배속 버튼 | Scenario Standalone·PIE의 실제 Slate 클릭으로 0·10·20·30·40·50·60·70·80배와 역순 감소, 0배 추가 감소를 확인했다. 당시 버튼은 10배 단위였고 현재 버튼은 ±60배다. Playground Standalone에서는 세 버튼의 비활성화를 확인했다 | 패키지 재생성, 네트워크 |
| 2026-10-12 입력·배율 보완 | Standalone의 빈 공간 클릭·키 입력 격리, 메뉴 닫기·재열기, 관측 배율 144·288 입력과 ±60·정지의 양쪽 입력값 일치, 세계 배율 불변과 세계 시각 복귀를 확인했다 | 네트워크, 패키지 |

추가 PIE 검사는 화면 포커스가 잡혔지만 게임 입력 모드와 이동·시점 차단이 적용되지 않아 실패했다. 종료 시 CommonUI가 `ViewportWidget was null`과 자식 해제 ensure를 기록했다. PIE 입력 정책 실패의 원인과 해결은 미확인이며 Standalone 통과를 PIE 통과로 확대하지 않는다. 후속 [Unreal MCP PIE QA](../../QA/EnvironmentPIEQA-2026-10-12.md)가 재현값, 코드 원인 분석, 미적용 수정안을 소유한다.
