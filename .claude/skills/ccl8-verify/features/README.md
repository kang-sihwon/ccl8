# ccl8 기능 검증 지도

먼저 이 색인에서 기능을 고르고, 기능 파일이 있으면 그 파일의 절차를 따른다. 모든 스크립트는 `Tools/Validation/`에 있다. 결과는 `Saved/Tests/<Suite>/<시각>/`에 남는다. 통과 조건은 종료 코드 0과 로그의 `PASS` 표식이며 `FAIL`, `Assertion failed:`, `Fatal error:`가 없어야 한다.

입력 경로 표기는 다음과 같다. 강도는 위에서 아래로 약해진다.

| 표기 | 의미 |
|---|---|
| `MCP` | Unreal MCP PIE에서 실제 키·마우스·Slate 조작 |
| `EI` | Enhanced Input Action에 입력 주입 |
| `Slate` | Slate 마우스·키 이벤트를 위젯에 전달 |
| `Ability` | ASC에 Ability 입력 태그를 직접 전달. 키 매핑은 거치지 않음 |
| `internal` | Controller·Subsystem 함수를 직접 호출하고 상태를 확인 |

모드 열의 `S/L/D`는 `-Mode Standalone|Listen|Dedicated`를 뜻한다. 기본값은 Standalone이며 `run_terrain_network_smoke.ps1`만 Listen과 Dedicated를 받고 기본값이 Dedicated다.

## 게임 흐름

| 기능 | 스크립트 | 맵 | 모드 | PASS 표식 | 입력 | 소유 문서 | 기능 파일 |
|---|---|---|---|---|---|---|---|
| 이동·사망·재도전 | `run_network_smoke.ps1` | MultiplayerPlayground | S/L/D, `-Port`, `-DriverIsHost` | `CCL_SMOKE PASS` | `EI` | MultiplayerFoundation | [network](network.md) |
| 최소 전투 | `run_combat_smoke.ps1` | CombatPlayground | S/L/D, `-Port`, `-DriverIsHost`, `-Impaired` | `CCL_COMBAT PASS` | `Ability` | CombatFoundation | [combat](combat.md) |
| 투사체 무기 | `run_projectile_smoke.ps1` | CombatPlayground | 단일 | `CCL_PROJECTILE PASS` | `internal` | Packaging | [combat](combat.md) |
| 인벤토리 복제 | `run_inventory_network.ps1` | CombatPlayground | 서버+클라이언트 | `CCL_INVENTORY_NET PASS` | `internal` | FrameworkPlan, Packaging | 없음 |
| 원정·콘텐츠·성장 | `run_campaign_smoke.ps1` | Campaign | S/L/D, `-Progression`, `-Content`, `-Impaired`, `-DriverIsHost`, `-Rendered` | `CCL_CAMPAIGN SERVER PASS`, `CCL_CAMPAIGN CLIENT PASS` | `Ability` | CampaignFoundation, ContentFoundation, ProgressionFoundation | [campaign](campaign.md) |
| 도입 연출·지도 | `run_integration_smoke.ps1` | Campaign | 단일 | `CCL_INTEGRATION PASS` | `internal` | 없음 | [campaign](campaign.md) |
| 시작 메뉴·저장·재개 | `run_session_smoke.ps1` | 프로젝트 기본 맵 | 기본은 write·read, `-Network`, `-Failure`, `-Rendered` | `CCL_SESSION_TEST PASS <역할>` | `Slate` | SessionFoundation | [session](session.md) |
| Loadout 복원 | `run_loadout_restore.ps1` | EnvironmentScenario | 단일 | `CCL_LOADOUT_RESTORE PASS` | `internal` | SessionFoundation | [session](session.md) |
| UI 화면 | `run_ui_smoke.ps1` | Campaign | 단일 | `CCL_UI PASS` | `Slate` | Packaging | 없음 |
| 공용 UI 기반 | `run_ui_foundation.ps1` | Campaign | 단일 | `CCL_UI_FOUNDATION PASS` | `internal` | UIFoundation | 없음 |
| 세계 저장·진행 | `run_world_smoke.ps1` | Campaign | S/L/D | `CCL_WORLD_SMOKE PASS` | `internal` | EnvironmentPlan | 없음 |
| 생활 Agent 세계 | `run_agent_world_smoke.ps1` | Campaign | 단일 | `CCL_AGENT_WORLD PASS` | `internal` | 없음 | 없음 |

## 환경 실험장

| 기능 | 스크립트 | 맵 | 모드 | PASS 표식 | 입력 | 소유 문서 | 기능 파일 |
|---|---|---|---|---|---|---|---|
| 실험장 구역 전체 | `run_environment_smoke.ps1` | EnvironmentPlayground, EnvironmentScenario | S/L/D | `CCL_EXPERIMENT_SMOKE PASS` | `Slate` | EnvironmentPlan, EnvironmentWeather | [environment-lab](environment-lab.md) |
| 실험 메뉴·전용 레벨 | `run_experiment_menu_smoke.ps1` | 위 두 맵 | 단일 | `CCL_EXPERIMENT_SMOKE PASS` | `Slate` | EnvironmentPlan | [environment-lab](environment-lab.md) |
| 영구 지형 | `run_terrain_smoke.ps1` | EnvironmentScenario | 단일 | `CCL_TERRAIN_SMOKE PASS` | `internal` | EnvironmentTerrain | [environment-lab](environment-lab.md) |
| 지형 조작 장면 | `run_terrain_scenario_smoke.ps1` | 위 두 맵 | 단일 | `CCL_TERRAIN_SCENARIO PASS` | `Slate` | EnvironmentTerrain | [environment-lab](environment-lab.md) |
| 지형 복제 | `run_terrain_network_smoke.ps1` | EnvironmentScenario | L/D | `CCL_TERRAIN_NETWORK PASS` | `internal` | EnvironmentTerrain | 없음 |
| 지형 위 이동 | `run_terrain_travel_smoke.ps1` | EnvironmentScenario | S/L/D | `CCL_TERRAIN_TRAVEL PASS` | `internal` | EnvironmentTerrain, FrameworkPlan | 없음 |
| 물 | `run_water_smoke.ps1` | EnvironmentScenario | S/L/D | `CCL_WATER_SMOKE PASS` | `Slate` | EnvironmentPlan | 없음 |
| 깊은 눈 | `run_snow_smoke.ps1` | EnvironmentScenario | S/L/D | `CCL_SNOW_SMOKE PASS` | `internal` | EnvironmentPlan | 없음 |
| 날씨 | `run_weather_smoke.ps1` | EnvironmentScenario | S/L/D | `CCL_WEATHER_SMOKE PASS` | `internal` | EnvironmentWeather | 없음 |
| 연소 | `run_fire_smoke.ps1` | EnvironmentScenario | S/L/D | `CCL_FIRE_SMOKE PASS` | `internal` | EnvironmentFire | 없음 |
| 화재 구역 | `run_fire_zones.ps1` | 위 두 맵 | S/L/D | `CCL_FIRE_ZONE PASS` | `internal` | EnvironmentFire | 없음 |
| 열 | `run_thermal_smoke.ps1` | 위 두 맵 | S/L/D | `CCL_THERMAL PASS` | `internal` | EnvironmentFire | 없음 |
| 생태계 | `run_ecology_smoke.ps1` | 위 두 맵 | 단일 | `CCL_ECOLOGY PASS` | `internal` | EnvironmentEcology | 없음 |
| 생태계 행동 | `run_ecology_behavior.ps1` | 위 두 맵 | S/L/D | `CCL_ECOLOGY_BEHAVIOR PASS` | `internal` | EnvironmentEcology | 없음 |
| 생태계 표현 전환 | `run_ecology_representation.ps1` | 위 두 맵 | S/L/D | `CCL_ECOLOGY_REPRESENTATION PASS` | `internal` | EnvironmentEcology, FrameworkPlan | 없음 |
| 생태계 공간 | `run_ecology_space.ps1` | 위 두 맵 | S/L/D | `CCL_ECOLOGY_SPACE PASS` | `internal` | EnvironmentEcology, EnvironmentNutrition | 없음 |
| 생태계 구역 | `run_ecology_zone.ps1` | 위 두 맵 | S/L/D | `CCL_ECOLOGY_ZONE PASS` | `Slate` | EnvironmentEcology, EnvironmentNutrition | 없음 |
| 생물 생애 | `run_agent_biology.ps1` | Campaign, 위 두 맵 | 단일, `-Map` | `CCL_AGENT_BIOLOGY PASS` | `internal` | EnvironmentEcology | 없음 |
| 생애 구역 | `run_biology_zone.ps1` | 위 두 맵 | S/L/D | `CCL_BIOLOGY_ZONE PASS` | `Slate` | EnvironmentEcology | 없음 |
| 식생 | `run_vegetation_smoke.ps1` | 위 두 맵 | 단일 | `CCL_VEGETATION PASS` | `internal` | EnvironmentEcology | 없음 |
| 식생 구역 | `run_vegetation_zone.ps1` | 위 두 맵 | S/L/D | `CCL_VEGETATION_ZONE PASS` | `internal` | EnvironmentEcology | 없음 |
| 음식 | `run_food_smoke.ps1` | 위 두 맵, Campaign | 단일 | `CCL_FOOD PASS` | `internal` | EnvironmentEcology, Packaging | 없음 |
| 음식 환경 | `run_food_environment.ps1` | 위 두 맵 | S/L/D | `CCL_FOOD_ENVIRONMENT PASS` | `Slate` | EnvironmentEcology | 없음 |
| 음식 줍기 | `run_food_pickup.ps1` | 위 두 맵 | S/L/D | `CCL_FOOD_PICKUP PASS` | `internal` | EnvironmentEcology | 없음 |
| 음식 구역 | `run_food_zone.ps1` | 위 두 맵 | S/L/D | `CCL_FOOD_ZONE PASS` | `Slate` | EnvironmentEcology | 없음 |

## 맵 실행 밖의 검사

| 기능 | 스크립트 | 형태 | 소유 문서 |
|---|---|---|---|
| 자동화 테스트 | `run_automation.ps1 -Filter <테스트 경로>` | 에디터 Automation, 기본 필터 `CCL.Equipment.TagContracts`, 시간 제한 최대 1800초 | ContentFoundation, EnvironmentPlan 외 |
| 공용 모듈 독립성 | `run_framework_isolation.ps1` | 최소 프로젝트에서 모듈별 빌드·실행 | FrameworkPlan |
| Windows 패키지 | `package_windows.ps1` | 패키지 생성. 일부 스모크는 `-PackagedExecutable`로 패키지를 실행 | Packaging |

소유 문서는 `Docs/` 아래 같은 이름의 문서다. "없음"은 2026-10-25 기준으로 그 스크립트를 언급하는 기능 문서가 없다는 뜻이다.
