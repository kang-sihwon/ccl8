# Windows 검토용 패키지

최신 UE 5.8.3 Game·Server 결과는 [전투 모듈 분리 후 패키지](#전투-모듈-분리-후-패키지)를 따른다. 아래 최초 전달본과 실패 기록은 당시 상태를 보존한다.

단계 7은 Windows Development 패키지를 생성하고 에디터를 거치지 않는 실행 파일에서 전체 흐름을 검사한다. 이번 산출물은 기능 검토용이며 최종 아트·밸런스와 Shipping 배포본을 대신하지 않는다.

시작·원정·전투·이동 검사 맵을 쿠킹 목록에 지정하고 런타임 문자열로 불러오는 Combat·Progression 정의를 항상 포함한다. 에디터 MCP·도구 플러그인은 Editor 타깃으로 제한한다. 게임에는 편집·자동화 서버가 필요하지 않다.

## 생성과 검사

`Tools/Validation/package_windows.ps1`은 로컬 엔진 경로 설정을 읽어 UAT BuildCookRun을 실행한다. 생성 결과는 Git에서 제외한 `Saved/Packages/`에 둔다. 소스 엔진에서 첫 게임 타깃과 패키징 도구를 빌드해야 하므로 CCLEditor의 증분 빌드보다 오래 걸릴 수 있다. 컴파일 병렬 수는 4로 제한한다.

`-ArchiveDirectory`로 별도 출력 폴더를 지정할 수 있다. 장비 UI 검증본은 기존 전달본을 덮지 않고 `Saved/EquipmentValidationPackage/`에 생성했다. 첫 쿠킹에서는 Zen 저장소의 `Missing chunk references` 오류가 발생했다. `-SkipZenStore` 옵션으로 쿠킹 결과를 파일로 저장한 재시도는 UAT 종료 코드 0으로 통과했다. 이 옵션은 엔진 `CookCommandlet.cpp`의 `SkipZenStore` 분기를 사용하며 프로젝트의 기본 설정을 바꾸지 않는다. 게임 빌드 로그는 `Saved/StageValidation/Equipment-Package.log`, 재시도 결과는 `Equipment-Package-Final.log`다.

장비 UI 검증본의 실제 게임 실행 파일을 `run_ui_smoke.ps1 -PackagedExecutable`에 전달하고 `-Offscreen`으로 렌더링했다. 1280×720과 1920×1080에서 가방 이동·교환·취소, 장착·해제, 장비 8칸 표시와 NPC 대화를 확인했다. 화면은 `Saved/Tests/UIVisual/1280x720-20261007-172525/`, `1920x1080-20261007-172622/`에 있다. 이 검사는 게임 내부 Slate 이벤트를 사용하며 Windows의 수동 마우스 조작과 구분한다.

기존 `run_session_smoke.ps1`, `run_campaign_smoke.ps1`, `run_combat_smoke.ps1`, `run_network_smoke.ps1`에 `-PackagedExecutable`로 실제 게임 실행 파일을 전달해 같은 기능 계약을 검사한다. 게임 타깃의 검사 범위는 Standalone·Listen이며 Dedicated 전용 타깃 빌드와 구분한다. Development의 검사 서브시스템은 명시적인 테스트 인자에서만 실행된다. 일반 실행은 시작 화면으로 진입한다.

검사는 시작·저장·불러오기·접속·실패 표시, 전투·성장·콘텐츠·승리·개별 재스폰과 종료를 포함한다. 렌더링한 시작 화면과 실제 플레이 화면에서 누락된 메시·머티리얼·글꼴을 확인한다. 패키징 실행 전에 단계 6에서 통과한 결과를 패키징 성공으로 계산하지 않는다.

## 실행 방법

`Saved/Packages/Windows/CCL.exe`를 실행하면 시작 화면이 열린다. 다른 위치로 옮길 때는 실행 파일만 복사하지 않고 `Windows` 폴더 전체를 복사한다. 디버그 심볼을 포함한 폴더 크기는 약 3.16 GiB다.

검사에서 생성한 저장 파일·설정·로그·화면 캡처는 배포 폴더 밖의 `Saved/Tests/PackagedRuntime/`에 보존했다. 전달용 폴더에는 테스트 저장 상태가 없다. 일반 플레이는 `CCL_Checkpoint` 슬롯을 사용하며 자동 검사의 별도 슬롯과 구분한다.

## 검증 결과

2026-10-07 현재 노트북의 UE 5.9.0 소스 엔진에서 생성 BAT, CCLEditor, 게임·패키징 도구 빌드와 쿠킹·스테이징·아카이브가 성공했다. UAT는 종료 코드 0을 반환했다. 첫 게임·도구 빌드의 UBA 실행 시간은 약 27분, 전체 UAT 실행 시간은 약 38분이다.

아래 로그는 `Saved/StageValidation/`에 보존한다. 모두 패키징한 실행 파일에서 통과했다. 생성·에디터 빌드 로그는 각각 `Stage7-Generate.log`, `Stage7-EditorBuild.log`이고 패키징 로그는 `Stage7-Package.log`다.

| 검사 | 확인한 결과 | 로그 |
|---|---|---|
| 시작·저장·복원 | 실제 Enter 입력, 부분 진행·성장·승리 복원, 잘못된 저장 거부, 설정, 메뉴 복귀와 정상 종료 | `Stage7-Session.log` |
| 방 생성·접속 | 메뉴에서 호스트 생성, 원격 접속, 게스트 저장 거부와 정상 종료 | `Stage7-SessionNetwork.log` |
| 접속 실패 | 응답 없는 주소의 타임아웃을 메뉴에 표시하고 정상 종료 | `Stage7-SessionFailure.log` |
| 사용자 실행 파일 | 루트 `CCL.exe`에서 맵 인자 없이 시작·저장·복원, 자식 게임과 실행기의 정상 종료 | `Stage7-Bootstrap.log` |
| 성장 | Standalone·Listen 원격 조작, 소유자별 인벤토리, 장비·훈련의 실제 피해 반영, 재스폰·늦은 접속 | `Stage7-Progression.log`, `Stage7-NetworkProgression.log` |
| 콘텐츠 | 렌더링한 Standalone의 실제 AI·거래·퀘스트·보스·승리, Listen 호스트 조작과 패킷 지연 50 ms·손실 2% | `Stage7-ContentRendered.log`, `Stage7-NetworkContent.log` |
| 전투·이동 회귀 | Listen의 공격·방어·패링·회피·자원, 실제 이동 입력·카메라·개별 사망과 재스폰 | `Stage7-Combat.log`, `Stage7-Movement.log` |

패키징한 시작·저장 복원·메뉴 화면과 NPC·승리 화면을 확인했다. 캡처는 `Saved/Tests/PackagedRuntime/Tests/SessionVisual/`과 `CampaignVisual/`에 있다. 확인한 화면에서 메시·머티리얼·글꼴 누락은 발견하지 않았다. 스테이징된 엔진 설정에 자동 생성 SecurityToken 키가 포함되지 않은 것도 확인했다.

로그 검토에서는 의도한 접속 실패의 타임아웃과 검사 종료 후 서버를 닫을 때의 연결 종료 메시지를 구분했다. 게임플레이의 치명적 오류·어설션과 빌드·쿠킹 실패는 발견하지 않았다.

## 검증 범위와 남은 작업

이번 결과는 Windows 노트북 한 대에서 실행한 Development 패키지다. 패키지의 Standalone·Listen을 검증했고 Dedicated Server는 이전 단계의 에디터 실행 결과만 있다. Shipping 게임 타깃, 다른 PC, 외부 인터넷·NAT 환경은 검증하지 않았다. UE 5.8 호환성을 확인한 결과도 아니다.

Development에는 명시적인 인자로 동작하는 검사 서브시스템과 디버그 사망 입력이 남아 있다. 아트·애니메이션·밸런스는 프로토타입 수준이며 최종 조작감은 사용자 검토가 필요하다. 메뉴의 모든 마우스 조작과 창 모드 전환에 대한 수동 검사는 [SessionFoundation](SessionFoundation.md)의 미확인 항목을 따른다.

## 자연 환경의 Server 타깃 검증 계획

환경 통합은 `CCLServer`의 `TargetType.Server`와 Windows Development 게임 패키지에서 각각 확인한다. `package_windows.ps1 -Server`는 UAT의 `-server -noclient -serverconfig=Development -serverplatform=Win64`를 사용한다. 서버 패키지와 게임 패키지는 별도 아카이브 경로에 생성하고 여섯 검사 맵을 쿠킹 대상으로 명시한다. 실제 서버 실행 파일에 게임 클라이언트 두 개를 연결해 환경 상태와 조작을 검사한다. 타깃 선언과 빌드·쿠킹·실행 성공은 구분하며, 아직 이 계획은 패키지 성공을 뜻하지 않는다.


### 최초 Server 패키지와 시작 실패

2026-10-11 UE 5.8.3에서 `CCLServer Win64 Development`의 컴파일·링크와 여섯 맵의 WindowsServer 쿠킹·스테이징·아카이브가 성공했다. 빌드 영수증의 `TargetType`은 `Server`다. UAT는 `BUILD SUCCESSFUL`, 종료 코드 0과 총 3236.68초를 기록했다. 근거는 `Saved/EnvironmentStages/ServerTarget-VerifiedSource-Package.log`이며 산출물은 `Saved/EnvironmentServerPackage/WindowsServer/`다. 이 첫 바이너리에는 이후 작성한 지형 경계 최적화가 들어 있지 않다.

실제 패키지의 첫 실행은 실패했다. `Saved/Tests/Environment/EnvironmentScenario-Dedicated-20261011-002648-327/server.log`에서 맵 로딩과 19971 포트 개방 뒤 `ACCLVegetationPresentation::BeginPlay`의 `Clear`가 없는 레이블에 접근해 종료됐다. 결과 JSON은 `passed=false`다. 따라서 패키지 생성 성공을 실행 통과로 기록하지 않는다. Game 최초 빌드는 이 문제를 먼저 수정·재검증하기 위해 중단했고 중간 산출물은 보존했다.

Dedicated Server에서는 시각 요소 갱신 전에 서버 모드를 확인하도록 식생·생태 표현 경로를 수정한다. 작업장은 레이블이 없어도 권위 단계 갱신과 복제를 계속해야 하므로 표현 포인터만 선택적으로 사용한다. 표면의 서버 충돌과 내비게이션·생태 상태 계산은 유지한다. 엔진의 `UPrimitiveComponent::NeedsLoadForServer`는 충돌이 없고 `AlwaysLoadOnServer`도 꺼진 컴포넌트를 제외한다. 근거는 `<Engine>/Source/Runtime/Engine/Private/Components/PrimitiveComponent.cpp:2076`이다. 최종 생성·Editor 빌드와 재패키징 뒤 실제 Server의 기동·첫 접속·관측·재접속을 다시 검사한다.


### Server 실행 수정과 실제 네트워크 통과

표현 컴포넌트가 없는 서버 경로를 수정하고 프로젝트 생성·Editor 빌드를 통과했다. 로그는 `Saved/EnvironmentStages/ServerPresentation-GPF.log`, `ServerPresentation-ActualCompile.log`, `ServerPresentation-EditorBuild.log`다. 재패키징은 `ServerPresentation-ServerPackage.log`에서 93.71초·종료 코드 0으로 통과했다. 다음 실행은 서버가 기동됐지만 경로로 로딩하는 `DA_FoodSample`이 쿠킹에서 빠져 실험 준비를 끝내지 못했다. 이 실패는 `EnvironmentScenario-Dedicated-20261011-004014-729`에 보존했다.

`Config/DefaultGame.ini`의 패키징 목록에 두 환경 맵과 `/Game/Environment`를 추가했다. 음식 정의와 경로로 선택하는 표면 재질 등 환경 에셋을 포함한다. 맵에서 직접 참조하는 것만 쿠킹하는 방식보다 패키지 크기가 늘지만, 현재 실험장이 경로로 선택하는 에셋을 함께 전달한다. `Saved/EnvironmentStages/EnvironmentCook-ServerPackage.log`의 재쿠킹·아카이브 후 실제 `CCLServer.exe`에서 80ms 지연·2% 손실·조작자·관측자·관측자 재접속 검사를 통과했다. 결과는 `Saved/Tests/Environment/EnvironmentScenario-Dedicated-20261011-004432-782/result.json`이다. 서버는 실제 Server 타깃이고 클라이언트는 Editor 게임 모드다. 일반 Game 타깃 패키지는 아직 최초 컴파일 중이며 이 결과로 완료 처리하지 않는다. 구역 09의 후속 변경 전 검증 결과다.

## 전체 검토 직후의 패키지 미검증 기록

2026-10-11 전체 검토 직후에는 에디터에서 다시 검증했지만 Windows 게임 패키지를 다시 만들지 않았다. 이 절은 아래 집계 영양 상태 패키지 생성 전의 기록이다. `LocalPlayerClassName` 설정, `DA_ItemCatalog` 카탈로그와 적 정의 에셋, 재접속 신원 연결은 패키지에서 확인하지 않았다. 카탈로그와 적 정의는 `/Game/Progression`에 있어 기존 항상 쿠킹 목록에 들어간다. 에디터 재검증 결과는 [DeliveryPlan의 2026-10-11 재검증](DeliveryPlan.md#2026-10-11-전체-검토-수정과-ue-583-재검증)을 따른다.


## 집계 영양 상태를 포함한 패키지

2026-10-11 UE 5.8.3에서 캠페인 배치 식별 수정과 집계 영양 상태를 포함한 Windows Development Game·Server 패키지를 생성했다. Game의 첫 빌드는 엔진 게임 타깃과 셰이더의 최초 생성 비용을 포함해 BuildCookRun 3126.37초였으며 `Saved/EnvironmentStages/Nutrition-GamePackage.log`에 보존한다. 영양 상태의 후속 수정을 반영한 Game은 `Nutrition-FinalGamePackage.log`의 80.39초, Server는 `Nutrition-ServerPackage.log`의 275.19초로 성공했다. 두 최종 UAT 모두 `BUILD SUCCESSFUL`과 종료 코드 0을 기록했다. 빌드·실행 결과의 원본은 로컬 `Saved/`에 있으며 기기별 절대 경로는 생략했다.

검토 실행 파일은 다음 위치에 있다. Game의 사용자는 루트 실행 파일을 사용하며 검사 도구는 내부 실행 파일을 직접 사용한다. 다른 PC로 전달할 때는 해당 Windows 또는 WindowsServer 폴더 전체가 필요하다.

| 용도 | 경로 |
|---|---|
| Game 실행기 | `Saved/NutritionGamePackage/Windows/CCL.exe` |
| 검사에 사용한 Game | `Saved/NutritionGamePackage/Windows/CCL/Binaries/Win64/CCL.exe` |
| Dedicated Server | `Saved/NutritionServerPackage/WindowsServer/CCL/Binaries/Win64/CCLServer.exe` |

다음 검사는 영양 상태의 최종 수정 후, 구역 09 관찰 시간 수정 전 패키지에서 통과했다.

| 실행 | 확인한 범위 | 결과 폴더 |
|---|---|---|
| 개인 세션 | 체크포인트·새 프로세스 재개·승리 복원 | `Saved/Tests/SessionSmoke/20261011-171454` |
| Scenario 환경 재개 | 새 프로세스의 세계 저장 복원 | `Saved/Tests/Environment/EnvironmentScenario-Standalone-20261011-171449-449` |
| Playground 환경 이동 | 세계·지형의 맵 왕복 | `Saved/Tests/Environment/EnvironmentPlayground-Standalone-20261011-171457-848` |
| 생태 구역·공간 | Game 화면·새 프로세스 재개, 실제 Server와 Game의 첫·늦은 접속·80ms·2% | [영양 상태의 패키지 결과](EnvironmentNutrition.md#windows-gameserver-패키지의-생태-실행) |

환경 전체의 최초 Game Listen 실행에서는 구역 09의 물이 하류에 도달하기 전에 판정해 실패했다. 실패 로그를 보존하고 실험 관찰 시간에 게임 시간 조건을 추가했다. 계산식과 수량 임계값은 유지했으며 근거는 [순환 실험의 두 시간](EnvironmentPlan.md#순환-실험의-게임-시간과-세계-시간)을 따른다.

구역 09 관찰 시간 수정 후에도 Game·Server를 다시 빌드·쿠킹했다. `Saved/EnvironmentStages/Nutrition-CycleGamePackage.log`는 BuildCookRun 198.15초, `Nutrition-CycleServerPackage.log`는 138.02초와 각각 종료 코드 0을 기록했다. 위 실행 파일 경로에는 이 후속 결과가 들어 있다. Game Listen의 환경 전체·80ms 지연·2% 손실·관측자 재접속은 `Saved/Tests/Environment/EnvironmentScenario-Listen-20261011-173112-975/result.json`에서 통과했다.

실제 Server와 Game 클라이언트의 환경 전체 검사도 `Saved/Tests/Environment/EnvironmentScenario-Dedicated-20261011-173314-260/result.json`에서 통과했다. `server_target=true`, `packaged_client=true`이며 80ms 지연·2% 손실을 적용한 상태에서 조작자·관측자 접속과 관측자 재접속을 확인했다. 에디터 게임 모드로 Server를 대신한 결과가 아니다.

Game의 Playground 렌더링 전체 검사는 `Saved/Tests/Environment/EnvironmentPlayground-Standalone-20261011-173243-353/result.json`에서 통과했다. 같은 폴더의 `controls.png`, `overview.png`, `shelter-open.png`에서 안내·12개 구역 배치·열린 문을 확인했다. 생태 전용 검사의 두 화면도 [영양 상태 문서](EnvironmentNutrition.md)에 연결했다. 시험 영역 바깥의 검은 배경과 단순한 형상·재질은 그대로 남아 있다.

이 결과는 한 PC의 로컬 Development 검사다. Shipping, 다른 PC·외부 인터넷·NAT와 최종 아트 품질은 검증하지 않았다. 실험장 관측자의 재접속은 환경 상태 복제를 검사하며 모든 캠페인 원격 프로필 복원 조합의 검증으로 확대하지 않는다.


최신 패키지와 네트워크 검사를 재현하는 명령은 다음과 같다. 아카이브 폴더는 최초 전달본과 분리한다.

```powershell
Tools/Validation/package_windows.ps1 -ArchiveDirectory Saved/NutritionGamePackage
Tools/Validation/package_windows.ps1 -Server -ArchiveDirectory Saved/NutritionServerPackage
$gameBinary = 'Saved/NutritionGamePackage/Windows/CCL/Binaries/Win64/CCL.exe'
$serverBinary = 'Saved/NutritionServerPackage/WindowsServer/CCL/Binaries/Win64/CCLServer.exe'
Tools/Validation/run_environment_smoke.ps1 -Mode Dedicated -Map EnvironmentScenario -Port 20423 -PacketLagMs 80 -PacketLossPercent 2 -Reconnect -PackagedExecutable $gameBinary -ServerExecutable $serverBinary
```


프레임 계측은 기능 검사와 별개다. 18,000프레임 실행의 중단과 600프레임 재실행의 행 수 불일치로 최신 패키지의 완료 성능 수치는 아직 없다. 근거와 재검사 항목은 [프레임 계측의 미완료 범위](EnvironmentPlan.md#프레임-계측의-미완료-범위)를 따른다.


## Framework 분리 후 Game·Server 패키지

2026-10-12 `KSICore`·`KSIInventory` 분리와 선행 인벤토리·환경 보완을 포함한 Windows Development 패키지를 생성했다. 게임·전용 서버 모두 쿠킹·스테이징·아카이브까지 완료했다. 최초 패키지의 서버 글꼴 로딩 오류를 고친 최종 산출물이며, 아래 실행 기록은 에디터 실행과 구분한다. 제출 범위와 모듈 독립 검증은 [FrameworkPlan](FrameworkPlan.md#패키지-검증과-제출-범위)을 따른다.

| 용도 | 경로 |
|---|---|
| 사용자용 Game 실행기 | `Saved/FrameworkStage2GamePackage/Windows/CCL.exe` |
| 검사에 사용한 Game | `Saved/FrameworkStage2GamePackage/Windows/CCL/Binaries/Win64/CCL.exe` |
| 실제 Server 타깃 | `Saved/FrameworkStage2ServerPackage/WindowsServer/CCL/Binaries/Win64/CCLServer.exe` |

전달할 때는 실행 파일만 복사하지 않고 해당 `Windows` 또는 `WindowsServer` 폴더 전체를 복사한다. 이 산출물과 원본 로그는 Git에서 제외된 `Saved/`에 보존하며 GitHub에는 코드·설정·검증 도구와 문서를 제출한다.

### 생성과 실행 근거

아래 로그 이름의 공통 루트는 `Saved/Tests/FrameworkStage2/Packaging/`이다. 실행 도구 로그에는 실제 프로세스 로그·결과 폴더가 기록된다. 원본 로그의 기기별 절대 경로는 이 문서에서 저장소 상대 경로로 표기했다.

| 검사 | 결과 | 근거 |
|---|---|---|
| 프로젝트 파일 생성 | 성공 | `Generate-Final.log` |
| 최종 Editor 빌드 | 성공 | `Editor-Verified.log` |
| CCL 자동 검사 | 163개 성공, 경고·실패·미실행 0 | `Automation-Final.log`, `Saved/Tests/Automation/20261012-005306-157/report/index.json` |
| Game BuildCookRun | 78.56초, 종료 코드 0 | `Game-Package-Final.log` |
| Server BuildCookRun | 68.30초, 종료 코드 0 | `Server-Package-Final.log` |
| 인벤토리 확장 복제 | 실제 Server와 Game 2개. 음식·탄약 상태, 소유자 전용 복제, 변경 알림과 늦은 접속 성공 | `Inventory-Network-Final.log` |
| 음식 저장 | 새 Game 프로세스의 동일 시각·CRC 복원과 후속 노화 성공 | `Food-Final.log` |
| 개인·네트워크 세션 | 체크포인트 새 프로세스 복원, 방 생성·접속과 종료 성공 | `Session-Final.log`, `Session-Network-Final.log` |
| 장비·소비·훈련 | Game Listen의 원격 조작·개별 재스폰·늦은 접속 성공 | `Progression-Final.log` |
| UI | 1280×720의 장착·이동·소비·세션·대화 조작 성공. 장비 화면의 한글·슬롯·캐릭터 표시 확인 | `UI-Final.log`, `Saved/Tests/UIVisual/1280x720-20261012-005808/equipment-full.png` |
| 환경 네트워크 | 실제 Server·Game의 지연 80ms·손실 2%, 관측자 재접속과 환경 순환 성공 | `Environment-Network-Final.log` |
| 사용자용 실행기 | 루트 `CCL.exe`에서 시작·저장·새 프로세스 복원과 정상 종료 성공 | `Bootstrap-Final.log` |

빌드 영수증의 `TargetType`은 각각 Game·Server이며 패키지 내부 실행 파일은 `Binaries/Win64`의 해당 빌드 파일과 SHA-256이 일치한다. 근거는 같은 로그 폴더의 `CCL.target`, `CCLServer.target`, `package-binaries-final.json`이다. 검증한 소스 파일은 `verified-source.json`에 기록했다.

### 서버 생성자의 시각 에셋 로딩

최초 전용 서버의 기능 검사는 통과했지만 CDO 생성 시 화면용 글꼴을 찾지 못했다. `CCLVegetationPresentation`, `CCLWorkshop`, `CCLVillageSteward`의 생성자가 서버 패키지에 없는 `F_VegetationLabel`·`F_KoreanLabel`을 조회한 것이 원인이다. 최초 오류는 `Saved/Tests/InventoryNetwork/20261012-004624/server.log`에 남아 있다.

세 생성자는 `IsRunningDedicatedServer()`일 때 글꼴 조회를 건너뛴다. 게임의 글꼴 설정과 기본 컴포넌트 생성은 유지한다. 이 함수는 서버 전용 플랫폼을 먼저 판정한다 (`<Engine>/Source/Runtime/Core/Public/Misc/CoreMisc.h:152`, 앵커 `FPlatformProperties::IsServerOnly()`). `BeginPlay`의 시각 갱신 분기만으로는 그보다 앞선 생성자 조회를 막을 수 없다.

복제 검사 도구에도 CDO 생성자 오류를 실패 조건으로 추가했다. 수정 전 패키지가 실제로 거부된 기록은 `Inventory-Negative-CDO.log`와 `Saved/Tests/InventoryNetwork/20261012-005313/server.log`다. 최종 패키지에서는 이 오류가 사라지고 같은 복제 검사가 통과했다.

### 재현과 한계

```powershell
Tools/Validation/package_windows.ps1 -ArchiveDirectory Saved/FrameworkStage2GamePackage -SkipZenStore
Tools/Validation/package_windows.ps1 -Server -ArchiveDirectory Saved/FrameworkStage2ServerPackage -SkipZenStore
$gameBinary = 'Saved/FrameworkStage2GamePackage/Windows/CCL/Binaries/Win64/CCL.exe'
$serverBinary = 'Saved/FrameworkStage2ServerPackage/WindowsServer/CCL/Binaries/Win64/CCLServer.exe'
Tools/Validation/run_inventory_network.ps1 -PackagedExecutable $gameBinary -ServerExecutable $serverBinary
Tools/Validation/run_food_smoke.ps1 -PackagedExecutable $gameBinary
Tools/Validation/run_environment_smoke.ps1 -Mode Dedicated -Map EnvironmentScenario -Port 20433 -PacketLagMs 80 -PacketLossPercent 2 -Reconnect -PackagedExecutable $gameBinary -ServerExecutable $serverBinary
```

최종 쿠킹에는 에디터 미리보기의 `ETonemappingMethod=Default` 가져오기 경고와 환경 맵의 `FString` 기반 `FText` 저장 경고가 남았다. Game·Server 패키징 로그에서 확인했으며 경고 0인 빌드로 보고하지 않는다. 이번 검증에서 에셋은 재저장하지 않았다.

검증 환경은 Windows 한 대의 UE 5.8.3 Development다. UI 도구는 게임 내부 Slate 이벤트를 사용하며 운영체제 마우스의 수동 검사는 아니다. Shipping, 다른 PC·외부 인터넷, Iris 전환과 성능 목표 충족은 검증하지 않았다. 기능 PASS와 치명적 오류·어설션·생성자 오류를 함께 확인했다. 환경 검사에서 의도한 구역 09 취소 1건은 `CCL_EXPERIMENT FAIL`로 기록되며, 이어지는 취소 확인·원래 입력 복구와 최종 성공을 대조했다. 로그 판정 요약은 `Saved/Tests/FrameworkStage2/Packaging/final-results.json`에 있다.


## 전투 모듈 분리 후 패키지

2026-10-12 KSICombat 분리 후 Game·실제 Server 타깃의 Windows Development 빌드·쿠킹·스테이징·아카이브와 실행 검사를 완료했다. 공용 모듈·게임 정책의 경계, 자동 검사 164개와 독립 실행 27개 항목은 [Framework 단계 3](FrameworkPlan.md#단계-3-구현과-검증)을 따른다.

| 용도 | 경로 |
|---|---|
| 사용자용 Game 실행기 | `Saved/FrameworkStage3GamePackage/Windows/CCL.exe` |
| 검사에 사용한 Game | `Saved/FrameworkStage3GamePackage/Windows/CCL/Binaries/Win64/CCL.exe` |
| 실제 Server 타깃 | `Saved/FrameworkStage3ServerPackage/WindowsServer/CCL/Binaries/Win64/CCLServer.exe` |

전달할 때는 해당 Windows 또는 WindowsServer 폴더 전체를 복사한다. 패키지·검사 로그는 Git에서 제외된 Saved에 보존하며 GitHub에는 코드·설정·검증 도구·문서를 제출한다.

아래 로그의 공통 루트는 `Saved/Tests/FrameworkStage3/`다. 도구 로그에는 각 프로세스의 원본 로그 폴더가 기록된다. 기기별 절대 경로는 이 문서에서 저장소 상대 경로로 표기했다.

| 검사 | 결과 | 근거 |
|---|---|---|
| Game BuildCookRun | 347.48초, 종료 코드 0 | `Game-Package.log` |
| Server BuildCookRun | 307.31초, 종료 코드 0 | `Server-Package.log` |
| 전투 Standalone | 공격·가드·패링·회피·개별 재스폰 성공 | `Packaged-Combat-Standalone.log` |
| 실제 Dedicated | Server와 Game 클라이언트 2개, 지연 50ms·손실 2%, 관찰자 생존과 개별 재스폰 성공 | `Packaged-Combat-Dedicated.log` |
| Listen 원격·호스트 | 두 조작자 구성의 전투·재스폰 성공 | `Packaged-Combat-Listen.log`, `Packaged-Combat-Host.log` |
| 투사체·탄약 | 발사자 제거 후 적중, 단일 충돌·탄약 소비·저장 codec 성공 | `Packaged-Projectile.log` |
| 성장·게임 진행 | Listen에서 장비·소비·훈련·재스폰·보스·승리 성공 | `Packaged-Progression.log` |
| 사용자용 실행기·세션 | 루트 CCL.exe의 시작·저장·새 프로세스 복원·정상 종료 성공 | `Packaged-Session.log` |
| UI | 1280×720 실제 렌더링, 장비·이동·소비·세션·대화 조작 성공 | `Packaged-UI.log`, `Saved/Tests/UIVisual/1280x720-20261012-021053/equipment-full.png` |

장비 화면에서 한글·장비 8칸·캐릭터 표시를 확인했다. UI 검사는 게임 내부 Slate 이벤트와 오프스크린 렌더링을 사용하며 수동 마우스 조작 검사와 구분한다. Game·Server 빌드 영수증의 TargetType과 패키지 내부 실행 파일의 SHA-256도 확인했다. `CCL.target`, `CCLServer.target`, `package-binaries-final.json`은 패키지가 각각 최신 빌드와 같은 실행 파일임을 기록한다. 전체 검사·오류 분류는 `final-results.json`에 있다.

### 생성·재현 명령

```powershell
Tools/Validation/package_windows.ps1 -ArchiveDirectory Saved/FrameworkStage3GamePackage -SkipZenStore
Tools/Validation/package_windows.ps1 -Server -ArchiveDirectory Saved/FrameworkStage3ServerPackage -SkipZenStore
$gameBinary = 'Saved/FrameworkStage3GamePackage/Windows/CCL/Binaries/Win64/CCL.exe'
$serverBinary = 'Saved/FrameworkStage3ServerPackage/WindowsServer/CCL/Binaries/Win64/CCLServer.exe'
Tools/Validation/run_combat_smoke.ps1 -Mode Dedicated -Impaired -PackagedExecutable $gameBinary -ServerExecutable $serverBinary
Tools/Validation/run_combat_smoke.ps1 -Mode Listen -PackagedExecutable $gameBinary
Tools/Validation/run_combat_smoke.ps1 -Mode Listen -DriverIsHost -PackagedExecutable $gameBinary
Tools/Validation/run_projectile_smoke.ps1 -PackagedExecutable $gameBinary
Tools/Validation/run_campaign_smoke.ps1 -Mode Listen -Progression -PackagedExecutable $gameBinary
Tools/Validation/run_session_smoke.ps1 -PackagedExecutable 'Saved/FrameworkStage3GamePackage/Windows/CCL.exe'
Tools/Validation/run_ui_smoke.ps1 -PackagedExecutable $gameBinary -Offscreen
```

패키지 Dedicated 전투 검사는 Game과 Server 실행 파일을 모두 지정해야 한다. 한쪽 인자가 빠지면 도구가 실행을 거부한다. 전투·투사체 도구는 CDO 생성자의 누락 에셋 오류도 실패로 판정한다.

Game 쿠킹에는 기존 실험장 FText 저장 경고, Game·Server 쿠킹에는 미리보기의 ETonemappingMethod 기본값 경고가 남았다. 이 변경에서 실험장 에셋을 다시 저장하지 않았다. 최종 패키지 실행에서 치명적 오류·어설션·ensure와 CDO 생성자 오류는 발견하지 않았다. 일부 Editor 실행의 Python 도구 초기화 오류는 FrameworkPlan에 별도로 기록했다.

이 검증은 한 PC의 로컬 Windows Development 범위다. Shipping, 다른 PC·외부 인터넷·NAT·Iris와 성능 개선은 검증하지 않았다. 환경 전체의 장시간 계측과 이전 단계 전체 회귀를 다시 수행한 결과로 확대하지 않는다.
