# 2026-10-11 전체 검토 수정과 UE 5.8.3 재검증

> 2026-10-27 [DeliveryPlan](../DeliveryPlan.md)의 같은 이름 절에서 본문을 그대로 옮긴 시점 기록이다. 상대 링크의 대상만 새 위치에 맞췄다.

2026-10-11 전체 소스 검토의 지적 15개를 수정하고 UE 5.8.3 소스 엔진에서 다시 검증했다. 단계 2-7의 전투·캠페인·성장·콘텐츠·세션 기록은 UE 5.9.0 노트북 결과였다. 아래 표는 그 기능을 현재 설치본에서 다시 실행한 결과다. 설계 변경은 각 소유 문서의 2026-10-11 절에 있다.

| 검사 | 결과 | 로컬 근거 |
|---|---|---|
| 프로젝트 파일 생성·`CCLEditor Win64 Development` | 최종 코드에서 두 단계 모두 성공 | 세션 임시 로그, 저장소에 보존하지 않음 |
| CCL 자동 검사 | 154/154, 경고·실패·미실행 0 | `Saved/Tests/Automation/20261011-132651-391/report/index.json` |
| 전투 Standalone·Dedicated | 통과 | `Saved/Tests/CombatSmoke/Standalone-20261011-131140`, `Dedicated-20261011-131205` |
| 캠페인 기본·콘텐츠·성장·Dedicated | 4개 구성 통과 | `Saved/Tests/CampaignSmoke/`의 20261011-131249, 131337, 131433, 131533 실행 |
| 개인 세션 저장·재개와 방 생성·접속 | 통과 | `Saved/Tests/SessionSmoke/20261011-131646`, `20261011-131725` |
| 이동·사망·재스폰 Dedicated·Listen | 통과 | `Saved/Tests/NetworkSmoke/Dedicated-20261011-131754`, `Listen-20261011-131832` |
| UI 1280×720 | 통과 | `Saved/Tests/UIVisual/1280x720-20261011-131906` |
| 다중 플레이어 세계 복원 | 통과 | `Saved/Tests/LoadoutRestore/20261011-132035-625` |
| 환경 Scenario Dedicated와 관측자 재접속 | 통과 | `Saved/Tests/Environment/EnvironmentScenario-Dedicated-20261011-132104-300` |
| 환경 Playground Standalone과 실험 초기화 | 통과 | `Saved/Tests/Environment/EnvironmentPlayground-Standalone-20261011-132239-483` |

스모크 14개는 기능 수정을 마친 빌드에서 실행했다. 그 뒤 들여쓰기·한 줄 문장 펼치기와 GameplayCue 탐색 경로 설정만 바꿨다. 형식 변환 스크립트는 파일마다 공백·중괄호를 뺀 내용이 원본과 같은지 확인했다. 최종 코드에서 생성·빌드와 자동 검사를 다시 통과했다.

첫 스모크 실행에서는 세 가지 실패가 나왔다. 개인 세션 검사는 계정 재개방을 일반 초기화 경로에 넣은 수정이 기존 계약을 어겨 실패했다. 재개방을 복원 확정의 명시적 호출로 옮긴 뒤 통과했다. 환경 Dedicated 검사는 게임 역할이 모두 통과했지만, 검사 스크립트가 기록 중인 로그를 쓰기 공유 없이 열어 예외로 끝났다. `run_environment_smoke.ps1`의 로그 읽기를 고친 뒤 통과했다. 이동 Dedicated 검사는 한 번 "Dead character moved"로 실패했고 같은 코드의 재실행에서 통과했다. 사망 직전 입력에 대한 서버 위치 보정이 사망 확인 뒤 도착하는 경합으로 추정하며, 원인은 확인하지 않았다. 실패 기록은 `Saved/Tests/NetworkSmoke/Dedicated-20261011-130301`에 있다.

Windows 게임 패키지는 이번에 다시 만들지 않았다. 따라서 패키지 실행 결과는 [Packaging](../Packaging.md)의 기존 기록에 머문다. `LocalPlayerClassName`, 아이템 카탈로그 설정과 재접속 신원 연결이 들어간 패키지는 아직 검증하지 않았다. 렌더링 화면 검사와 지연·손실 구성도 이번에는 실행하지 않았다.
