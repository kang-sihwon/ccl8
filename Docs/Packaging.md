# Windows 검토용 패키지

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
