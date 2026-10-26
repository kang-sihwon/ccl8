# 종합 QA 수정과 검증

[종합 QA 보고서](FullGameplayMCPQA-2026-10-12.md)의 다섯 지적 사항에 수정 사항을 반영했다. 최초 메뉴 입력 적용, 종료 중 입력 대상 수명, 전투실험장 방패 배치, 생애 실험 안내와 NPC 대상 선택이 작업 범위다. 보고서의 미확인 항목 전체를 이번 수정의 검증 완료로 취급하지 않는다.

## 변경 범위와 검증 기준

| 항목 | 변경 범위 | 검증 기준 |
|---|---|---|
| F-01 | 공용 입력 라우터가 포커스 변경 뒤 활성 화면의 정책을 다시 평가 | 두 환경 맵의 최초 메뉴에서 이동·시점 차단, 닫기·재열기에서 입력 복원 |
| F-02 | 입력 대상이 준비되기 전이나 종료 중이면 설정 적용 보류 | 메뉴가 열린 상태와 닫힌 상태의 PIE 종료에서 무효 뷰포트·PlayerInput 오류 없음 |
| F-03 | 기존 CombatPlayground에 방패 픽업과 획득·오른손 장착 안내 추가 | 기본 배치에서 획득·장착·가드·해제 가능 |
| F-04 | 구역 07의 정의와 상태 문구를 실제 조작 범위에 맞춤 | 음식·생애 수동 조작과 식생·저장 자동 검사의 범위 구분 |
| F-05 | HUD와 서버에서 접근 가능한 가장 가까운 NPC를 같은 함수로 선택 | 겹친 범위·차폐·거리 변경에서 대상 선택 일치 |

입력 설정은 CommonUI가 소유하며 매 틱 이동 차단 카운트를 덧붙이지 않는다. 포커스 변경 콜백에서는 갱신을 예약하고 다음 틱에 처리해 Slate 포커스 변경 중 재진입을 피한다. 단순 지연 시간으로 최초 열기를 늦추는 방법보다 수명 처리가 늘지만 포커스 복귀에도 같은 경로를 사용할 수 있다.

NPC 선택은 거리 우선으로 정한다. 조준 방향 우선보다 카메라 조건이 단순하지만 두 NPC가 교차하면 대상이 바뀔 수 있다. 대화·구매 입력 시점에 접근 가능 여부를 서버가 다시 검증한다.

## 검증 상태

UE 5.8.3에서 프로젝트 파일 생성과 `CCLEditor Win64 Development` 빌드에 성공했다. 첫 빌드의 `UPlayerInput` 정의 누락을 보완한 뒤 통과했고, 마지막 안내 문구 수정 뒤에도 두 명령을 다시 실행했다. 원본 로그는 `Saved/QAWork/generate.log`, `Saved/QAWork/build.log`에 있다.

| 검사 | 결과 | 증거 |
|---|---|---|
| 두 환경 맵의 최초 메뉴와 F7 재열기 | PASS. 클릭 없이 뷰포트 포커스를 전달한 뒤 이동·시점 차단, 닫기·재열기와 시각 배율 입력 확인 | `Saved/Tests/ExperimentMenu/EnvironmentScenario-PIE-20261012-201341-292/`, `EnvironmentPlayground-PIE-20261012-201342-052/` |
| Campaign UI PIE | PASS. 대화, 메뉴, 풀 재사용, 재스폰, 표시 제어 중첩과 입력 복원 | `Saved/Tests/UIVisual/1280x720-20261012-201506/game.log` |
| 관련 자동 검사 | 11개 성공, 실패·경고 0개. NPC 거리·차폐·이동, 방패 획득·장착·손 동작 복귀, 기존 전투·UI·장비 계약 포함 | `Saved/Tests/Automation/20261012-201342-412/report/index.json` |
| Standalone 전투 스모크 | PASS. 정면 가드의 체력 보존·스태미나 소모, 가드 붕괴, 후속 피해, 패링·회피, 사망·재스폰 | `Saved/Tests/CombatSmoke/Standalone-20261012-201549/driver.log` |
| 실제 에셋 갱신과 재실행 | PASS. 방패 4개, 안내 4개를 보존하고 두 환경 맵의 Director가 수정한 구역 07 정의를 참조함을 확인 | `Saved/QAWork/guard-glyphs.log` |

같은 에디터에서 EnvironmentScenario와 EnvironmentPlayground를 각각 메뉴 열림·닫힘 상태로 종료하고, CombatPlayground까지 총 5회 PIE를 실행했다. 입력 차단과 닫기 후 복원이 유지됐으며 종료 오류는 없었다. 결과는 `Saved/QAWork/pie-lifecycle.json`, 로그는 `Saved/QAWork/lifecycle-verified.log`에 있다.

방패 안내는 저장 후 맵을 다시 읽고 1280×720 PIE 캡처에서 글자와 방향을 확인했다. 안내는 “E 방패 줍기 / I 장비”, “오른손 방패 장착”, “우클릭 유지: 가드”다. 오프라인 폰트의 포함 문자를 배치 도구에서 검사하며, Actor와 TextRenderComponent를 수정 대상으로 표시해 외부 Actor 패키지에도 저장한다. 캡처는 `Saved/QAWork/guard-practice.png`, 결과는 `Saved/QAWork/guard-preview.json`에 있다.

환경과 Campaign의 위 PIE 검사 종료 로그에는 `Failed to commit change!`와 `Null player input`이 없다. 두 실행 스크립트도 이 문구를 실패로 판정한다. 첫 환경 검사 시도는 오프스크린 창에 포커스가 없어 실패했다. 검사 준비에서 Slate 포커스 이벤트를 명시적으로 전달하고 다음 틱에 확인하도록 보완했으며, 최초 화면을 닫고 다시 여는 우회는 사용하지 않았다.

## 구현 근거와 남은 확인

공용 입력 수명은 [KSIUIActionRouter](../../Source/KSIUI/Private/KSIUIActionRouter.cpp)가 처리한다. `IsInputTargetReady`는 World 종료 상태, 뷰포트 위젯, Controller와 PlayerInput을 확인하며 적용하지 못한 정책을 엔진 캐시에 기록하지 않는다. UE의 `CommonUIActionRouterBase.cpp:1866`은 기반 적용 함수에서 입력 설정 캐시와 이동·시점 차단을 함께 관리한다. `UIActionRouterTypes.cpp:1685`는 에디터 창 포커스 조건을 검사한다. 엔진 경로는 `<Engine>/Plugins/Runtime/CommonUI/Source/CommonUI/Private/Input/` 기준이다.

NPC 선택은 [FindNearestReachable](../../Source/CCL/Campaign/CCLVillageSteward.cpp)를 HUD, 대화 RPC와 구매 RPC가 함께 사용한다. 서버가 요청을 처리할 때의 거리와 차폐로 다시 선택한다. 지연이 있는 원격 클라이언트에서 이동 중인 NPC의 표시와 서버 선택이 같은 프레임에 일치하는지는 이번에 검증하지 않았다.

방패 배치와 설명 에셋은 [configure_gameplay_qa.py](../../Tools/Validation/configure_gameplay_qa.py)로 갱신했다. 기존 CombatPlayground를 다시 만들지 않고 도구가 소유한 이름의 Actor를 재사용한다. 획득·장착 자동 검사와 전투 스모크는 분리된 검사이며, 새 PIE에서 E·I·우클릭을 연속 조작한 전체 수동 검사를 대신한다고 주장하지 않는다. 가드 조작 안내는 [CombatFoundation](../CombatFoundation.md#입력과-방향)을 따른다.

실제 OS 창 전환에서 이미 누른 이동키·마우스 축 입력의 경계, 외부 네트워크, 패키지와 게임패드는 이번 수정에서 재검증하지 않았다. 종합 QA 원문에 적힌 장시간 환경 실행과 그 밖의 미확인 범위도 유지한다. 재사용할 CommonUI 포커스·캐시·입력 대상 수명 지식은 개인 볼트의 `LocalPlayer-UI-Contexts`에 반영했다.
