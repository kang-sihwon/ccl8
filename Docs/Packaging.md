# Windows 검토용 패키지

Windows Development의 Game·실제 Server 패키지를 만들고 에디터를 거치지 않는 실행 파일에서 기능을 검사한다. 가장 최근 결과는 2026-10-12 UE 5.8.3의 [세계 모듈 분리 후 패키지](Packaging/Records.md#세계-모듈-분리-후-패키지)이고, 전투·투사체·UI 검사는 같은 날의 [전투 모듈 분리 후 패키지](Packaging/Records.md#전투-모듈-분리-후-패키지)가 마지막이다. 이 문서는 패키지 구성, 생성·실행 방법, 패키지별 검증 기록을 소유한다. [StudyRoadmap](StudyRoadmap.md)의 단계 7 사양이며, 산출물은 기능 검토용이라 최종 아트·밸런스와 Shipping 배포본을 대신하지 않는다.

## 하위 문서

| 문서 | 읽는 때 |
|---|---|
| [패키지 기록](Packaging/Records.md) | 지난 패키지의 생성·실행 결과와 한계를 확인할 때 |

## 패키지 구성

- 시작·원정·전투·이동 검사 맵, 두 환경 맵과 실험장 레벨을 `Config/DefaultGame.ini`의 쿠킹 목록(`MapsToCook`)에 지정한다.
- 런타임 문자열로 불러오는 `/Game/Combat`·`/Game/Progression` 정의와, 실험장이 경로로 고르는 `/Game/Environment` 에셋을 항상 포함한다(`DirectoriesToAlwaysCook`). 맵에서 직접 참조하는 것만 쿠킹하는 방식보다 패키지가 커지지만 경로로 고르는 에셋을 함께 전달한다.
- 에디터 MCP·도구 플러그인은 Editor 타깃으로 제한한다. 게임에는 편집·자동화 서버가 필요하지 않다.
- Development의 검사 서브시스템은 명시적인 테스트 인자에서만 실행되고, 일반 실행은 시작 화면으로 들어간다. Development에는 디버그 사망 입력도 남아 있다.

## 생성과 검사

`Tools/Validation/package_windows.ps1`은 로컬 엔진 경로 설정을 읽어 UAT BuildCookRun을 실행한다. 생성 결과는 Git에서 제외한 `Saved/` 아래에 둔다. 소스 엔진에서 첫 게임 타깃과 패키징 도구를 빌드해야 하므로 CCLEditor의 증분 빌드보다 오래 걸릴 수 있다. 컴파일 병렬 수는 4로 제한한다.

| 옵션 | 동작 |
|---|---|
| `-ArchiveDirectory <path>` | 출력 폴더를 지정한다. 기본값은 `Saved/Packages/`다. 기존 전달본을 덮지 않도록 패키지마다 다른 폴더를 쓴다 |
| `-Server` | UAT의 `-server -noclient -serverconfig=Development -serverplatform=Win64`로 `CCLServer`(`TargetType.Server`)를 만든다 |
| `-SkipZenStore` | 쿠킹 결과를 Zen 저장소 대신 파일로 저장한다. 엔진 `CookCommandlet.cpp`의 `SkipZenStore` 분기를 쓰며 프로젝트 기본 설정은 바꾸지 않는다 |
| `-SkipBuild` | 빌드를 건너뛰고 쿠킹부터 실행한다 |

기능 검사는 `run_session_smoke.ps1`, `run_campaign_smoke.ps1`, `run_combat_smoke.ps1`, `run_network_smoke.ps1`, `run_ui_smoke.ps1`, 환경·투사체·인벤토리 스모크에 `-PackagedExecutable`로 실제 게임 실행 파일을, 실제 Server 검사에는 `-ServerExecutable`로 서버 실행 파일을 함께 넘긴다. 패키지 Dedicated 검사는 두 실행 파일을 모두 지정해야 하며 한쪽이 빠지면 도구가 실행을 거부한다. 전투·투사체·인벤토리 복제 도구는 CDO 생성자의 누락 에셋 오류도 실패로 판정한다. 실행 순서와 증거 기록은 `ccl8-verify` 스킬을 따른다.

검사는 시작·저장·불러오기·접속·실패 표시, 전투·성장·콘텐츠·승리·개별 재스폰과 종료를 포함한다. 렌더링한 시작 화면과 실제 플레이 화면에서 누락된 메시·머티리얼·글꼴을 확인한다. 에디터에서 통과한 결과를 패키징 성공으로 세지 않고, 패키지 생성 성공을 실행 통과로 기록하지 않는다. UAT 종료 코드만으로 아카이브 전체 복사가 성공했다고 판정하지 않는다.

## 실행 방법

사용자는 패키지 루트의 `Windows/CCL.exe`를 실행하면 시작 화면이 열린다. 검사 도구는 `Windows/CCL/Binaries/Win64/CCL.exe`를 직접 쓴다. 다른 위치로 옮기거나 다른 PC에 전달할 때는 실행 파일만 복사하지 않고 해당 `Windows` 또는 `WindowsServer` 폴더 전체를 복사한다. 패키지와 로그는 Git에서 제외된 `Saved/`에 두고 GitHub에는 코드·설정·검증 도구·문서를 제출한다.

일반 플레이는 `CCL_Checkpoint` 슬롯을 쓰며 자동 검사의 별도 슬롯과 구분한다. 검사에서 생성한 저장 파일·설정·로그·화면 캡처는 배포 폴더 밖에 둔다.
