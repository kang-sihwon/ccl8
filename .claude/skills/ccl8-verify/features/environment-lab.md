# 환경 실험장

환경 실험은 두 경로로 한다. 통합 실험장 `/Game/Maps/EnvironmentPlayground`, `/Game/Maps/EnvironmentScenario`는 여러 기능을 구역별로 비교한다. 전용 레벨 `/Game/Maps/Tests/<기능>`은 시험 하나와 표본 수를 골라 0배속에서 새로 시작한다. 소유 문서는 `Docs/EnvironmentPlan.md`(구역과 단계), `Docs/EnvironmentLaboratories.md`(전용 레벨)이며, 기능별 계산은 날씨·지형·물·눈·불·생태계 문서가 소유한다.

## 하위 기능

- `zones`: 통합 실험장의 구역 1-11 자동 검사
- `menu`: F7 실험 화면, 시간 배율 적용·정지·초기화, 전용 레벨 선택
- `lab`: 전용 레벨의 시험 종류·표본 수 배치와 저장 슬롯
- `terrain`: 지형 굴착·충돌·경로와 조작 장면
- 기능별 계산(물, 눈, 날씨, 불, 생태계, 식생, 음식)은 색인의 각 스크립트가 맡는다.

## 사용자 진입 경로

- 통합 실험장: 맵을 열고 Play, F7로 실험 화면을 열어 구역 이동·조건·배율을 고른다.
- 전용 레벨: `/Game/Maps/Tests/Ecology` 같은 레벨을 열고 Play한다. F7의 **전용 레벨 선택**으로 기능을 바꾼다. 생태계·식생·생애·부패는 **시험 종류**와 **표본 수 (1-16)**를 고르고 **선택한 시험만 새로 배치**를 누른다.
- 시간은 배율 입력 뒤 **배율 적용**으로 바꾼다. 0은 정지, 1은 실시간, 60은 빠른 관찰이다.

## 실행

- 구역 전체(`Slate`): `.\Tools\Validation\run_environment_smoke.ps1 -Mode Standalone -Map EnvironmentScenario`. `-Map EnvironmentPlayground`, `-Mode Listen|Dedicated`, 재시작 `-Restart`, 맵 이동 `-Travel`, 재접속 `-Reconnect`, 지연 `-PacketLagMs`, 손실 `-PacketLossPercent`, 화면 `-Rendered`를 조합한다.
- 메뉴(`Slate`): `.\Tools\Validation\run_experiment_menu_smoke.ps1`. 전용 레벨 경로는 `-Lab`, 에디터 PIE 실행은 `-PIE`를 더한다.
- 지형(`internal`, 조작 장면은 `Slate`): `run_terrain_smoke.ps1`, `run_terrain_scenario_smoke.ps1`.
- 기대 결과: `CCL_EXPERIMENT_SMOKE PASS`, 지형은 `CCL_TERRAIN_SMOKE PASS`, `CCL_TERRAIN_SCENARIO PASS`.
- 증거: `Saved/Tests/Environment/`, `Saved/Tests/ExperimentMenu/<Map>-<Standalone|PIE>-<시각>/`의 로그와 캡처. 전용 레벨의 MCP 기록은 `Saved/Laboratories/`에 둔다.
- 계산 규칙만 바꿨으면 `run_automation.ps1 -Filter CCL.Environment`로 자동화 테스트를 함께 돌린다.
- MCP 경로: 배율 조작, 관측창, 표본 배치처럼 화면을 보며 판정하는 변경에 쓴다. 판정 기준은 `Docs/EnvironmentPIEQA-2026-10-12.md`와 `Docs/FullGameplayMCPQA-2026-10-12.md`의 환경 절을 따른다.

## Gotchas

- 자동 검사 중에는 장면 시간과 무관하게 즉시 결과를 내는 항목이 있다. 장면의 시간 진행을 관찰한 결과와 구분해서 보고한다.
- 천체 관측창이 열려 있으면 배율이 관측창에 적용된다. 세계 시간을 바꾸는 검사 전에 관측창을 닫는다.
- 스모크 스크립트는 전용 레벨을 맵 인자로 받지 않는다. 전용 레벨 경로는 메뉴 스모크의 `-Lab`과 MCP로만 확인된다.
- 고배율(60·120·600배) 수위 검증은 소유 문서의 기록 시점(초기·0.5초·15초)을 따른다. PASS 하나로 수위 이상의 해결을 판정하지 않는다.
