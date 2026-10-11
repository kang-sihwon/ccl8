# 자연환경 학습 문서의 비교 근거

2026-10-12 공용 모듈 분리 이후의 위치는 [Framework 파일 이동표](../FrameworkPlan.md#코드-위치의-전후-비교)를 따른다. 아래 발췌·이름·줄 번호·해시는 기록 당시 기준을 유지하며 소스 링크는 현재 대응 파일을 연다.

2026-10-11 후속 커밋을 비교한 기준은 [후속 비교 근거](EnvironmentFollowupEvidence.md)에 있다. 아래의 작업 트리·해시·검증 횟수는 최초 작성 시점의 기록이다.

이 문서는 학습 장이 어떤 브랜치·코드·검증 기록을 읽었는지 보관한다. 구현·게임 규칙의 정본은 [Guide](../Guide.md)가 연결하는 사양 문서다. 이력 비교의 전체 경로는 [환경 메인](Environment.md)에서 시작한다.

## 브랜치와 작업 트리

2026-10-10 조사 시작 기준 로컬 브랜치는 `main` 하나다. HEAD는 `b5d141dd314b3f4fe2997c8ad1c5d43742199217`, 로컬 원격 추적 `origin/main`은 `54123fbb2ed500c16bceefaa3778d775d525eb4a`였다. 공통 조상은 해당 origin/main과 같고 main은 그보다 27커밋 앞섰다. 분기한 기능 브랜치가 없으므로 의미 있는 도입 커밋 사이를 비교했다. 원격 저장소의 최신 상태를 fetch한 결과는 아니다.

작업 트리에는 코드·설정·에셋의 미커밋 변경과 신규 파일이 있었다. 날씨·불·환기 소스와 후속 생태계·플레이어 저장 코드가 함께 존재한다. 문서 제목이 Document로 시작한다고 해당 기능의 소스가 그 커밋에 들어 있다고 판단하지 않았다. `5a640bf`부터 인용하는 환경 검증 커밋은 문서 기록이며 해당 신규 소스의 기준은 아래 작업 트리 해시다.

이 작업은 브랜치 전환·reset·stash·pull·push를 수행하지 않는다. 문서 이외의 변경은 기존 작업 상태로 유지한다. 아래 기준은 새 문서 커밋이 만들어져 HEAD가 움직여도 바꾸지 않는다.

## 커밋별 근거

| 짧은 이름 | 전체 SHA | Git 제목 |
|---|---|---|
| `50e5838` | `50e583859c832d6cb325e5e76beb8356e8c13959` | Fix missing gameplay tag include in UI root |
| `aab8661` | `aab86619b813db2af3d72365cae10fc6a15b0bdc` | Record environment preflight and pending clock validation |
| `c58afa4` | `c58afa4fba976a9789bd256771da30d8cbaa7813` | Add world clock core and environment preflight checks |
| `fe5c143` | `fe5c14344161f5d3b07d68265ad042a61115e604` | Add atomic world and life time advancement |
| `64a7549` | `64a7549ca15a852a9ddd47f983f6dcf272ef82e2` | Integrate server world time and versioned life snapshots |
| `7307465` | `730746518ac64c030f0b22e707c8701b618834f0` | Add environment experiment maps and validated server controls |
| `4458dea` | `4458deace5b5ea2cd8cdc3a211ffcd5b81bfaa67` | Add deterministic celestial and shelter state foundations |
| `5f62e1e` | `5f62e1ef76b3ee765e6f5f5b614e90e7e4fca4dc` | Connect celestial and shelter experiments to replicated world state |
| `a28772c` | `a28772c186f52da4f19beb812f886519298b4f49` | Add transactional terrain storage and chunk mesh extraction |
| `92d1b38` | `92d1b38cc95d4fd3236ad9568f87d2f7868aa538` | Add atomic terrain collision publication and handoff notes |
| `0408d94` | `0408d9488755823ca69d8844dcc7792dda610630` | Integrate terrain, water, snow and gameplay UI updates |
| `a817082` | `a817082fc8dfbbc63f2bed292a70f39cd22f8cc5` | Improve environment lab controls and surface visualization |
| `5a640bf` | `5a640bfb193c3762f7ce320459c60eb1450cb81e` | Document natural weather model and validation |
| `a5c79b0` | `a5c79b00da238bb8ec699a0188c14c28aeb7a949` | Document weather presentation and combustion core validation |
| `ab195b1` | `ab195b19b967416c9e204f3e83a8f4a04c63f994` | Document surface combustion coupling and network validation |
| `54123fb` | `54123fbb2ed500c16bceefaa3778d775d525eb4a` | Document conservative room ventilation model and tests |
| `ac0deb5` | `ac0deb55497bc975f80638d374e5fbd2422a1fb3` | Document coupled fire surface and room environment validation |
| `39005ee` | `39005ee55453e28b8397864ef97d5154d1e4c060` | Document geometry-aware ventilation and runtime validation |
| `9e3570f` | `9e3570feba6a96f820135df1d6980e43c9d6f4ed` | Document fire experiment controls and rendered validation |
| `c182af3` | `c182af3f98f0bf6fb60fe72acf177bdd4d51455e` | Document thermal damage integration and validation |
| `b5d141d` | `b5d141dd314b3f4fe2997c8ad1c5d43742199217` | Document session profile finalization before restore notifications |

물과 눈은 `0408d94` 하나에 함께 기록됐다. 이전 물 전용 중간 소스가 별도 커밋에 있다고 가정하지 않는다. 단계별 작업 순서와 당시 실행 조건은 각각 EnvironmentWater·EnvironmentSnow의 기록을 사용한다. 신규 시스템의 최초 구현 이전에는 코드가 없었다고 명시하며 가상의 이전 코드를 만들지 않는다.

## 작업 트리 인용의 식별값

`WT-2026-10-10`은 문서 작성 중 로컬 사본으로 고정한 파일이다. 원본 바이트의 SHA-256을 아래에 보관했다. 코드 블록은 그 사본에서 연속 구간을 뽑고 들여쓰기만 정리했다. 줄 번호도 그 사본 기준이다. 링크를 클릭하면 이후 수정됐을 수 있는 현재 파일이 열린다.

| 저장소 상대 경로 | 조사 시 Git 상태 | SHA-256 |
|---|---|---|
| `Source/CCL/Environment/CCLAirSimulation.cpp` | 미추적 신규 파일 | `56b584267908da4e857d44b74ff0eb977a9c8eb3f1519f405bfc7afa05901510` |
| `Source/CCL/Environment/CCLCombustionModel.cpp` | 미추적 신규 파일 | `b2a820380abddda9f267febd375119c270f793df49e7629fee1740f23514e2bb` |
| `Source/CCL/Environment/CCLExperimentPlayerController.cpp` | 미커밋 변경 | `37ad6d271df194cbaf96375c10e5ef152d7d00e21c6959c3b71887055cbb1447` |
| `Source/CCL/Environment/CCLSurfaceFire.cpp` | 미추적 신규 파일 | `98e036154ada573d9fda12c6b80d4793df3df6c09075fe28f35ade662ac9662f` |
| `Source/CCL/Environment/CCLSurfaceQuery.cpp` | 미커밋 변경 | `4dae9eca0c7772803b35669612ed375be0e71d91b0982c860556ea8ced19b190` |
| `Source/CCL/Environment/CCLSurfaceSimulation.cpp` | 미커밋 변경 | `9d3a9e74090d5b695b2aab3fb0906adcc19e757bd2f6254ae8bce0325d74f673` |
| `Source/CCL/Environment/CCLThermalExposure.cpp` | 미추적 신규 파일 | `b30b05a8ee3b4c096b950fa77093843ffc3556f40b261764470ed552fea7fab9` |
| `Source/CCL/Environment/CCLWeatherModel.cpp` | 미추적 신규 파일 | `d5c8a331b7270af358bf970a80fc7c9d455958e808f6902f72eea7f0d62127af` |
| `Source/CCL/Environment/CCLWorldAdvance.cpp` | 미커밋 변경 | `31873634723c51a6dfb15b918a340b80ea214bdd671b96535ca3417b764511af` |
| `Source/CCL/Environment/CCLWorldSimulationSubsystem.cpp` | 미커밋 변경 | `84c183c8057c7d7d0bea980db01fa268152833f74ad2f3aa97ceb9168cefe8b5` |
| `Source/CCL/Environment/CCLWorldSnapshot.cpp` | 미커밋 변경 | `49bbe72cece7e624ed827a85d01e912c85d8703e57576b2001787b0e54600730` |

해시는 해당 바이트의 식별값이며 파일 전체를 복원하는 자료는 아니다. 공유 문서에는 핵심 발췌를 포함했고 전체 로컬 조사 사본은 `Saved/LearningDocs/2026-10-10/snapshot/`에 두었다. Saved는 Git 제출 대상이 아니다. 코드를 제출하지 않은 상태에서 다른 기기가 위 미커밋 파일 전체를 Git으로 가져올 수 있다고 설명하지 않는다.

## 엔진 확인

설치본 `<Engine>/Build/Build.version`을 읽어 Major 5, Minor 8, Patch 3을 확인했다. 이 값은 과거 프로젝트 커밋이 모두 같은 설치본에서 빌드됐다는 증거가 아니다.

| UE 5.8.3 소스 | 확인한 내용 |
|---|---|
| `<Engine>/Source/Runtime/Engine/Private/Components/CharacterMovementComponent.cpp:8116` | `ForceClientAdjustment`는 `ServerLastClientAdjustmentTime`을 초기화한다 |
| 같은 파일 `:10401` | `bForceClientUpdate` 또는 위치 오차 판정으로 보정 분기에 들어간다 |
| 같은 파일 `:8674` | 보정 후 이동 재실행에서 `PrepMoveFor`를 호출한다 |

첫 진입점은 엔진 Graft로 찾고 실제 설치 소스와 대조했다. CMC·RPC·GAS 자체와 프로젝트의 공통 시간·후보 확정은 구분했다. 그 밖의 기존 게임 엔진 근거는 [EngineEvidence](EngineEvidence.md)에 있다. 이번 작업은 외부 최신 엔진 API 조사가 아니라 설치 소스와 프로젝트 이력의 비교다.

## 기존 실행 보고서와 이번 확인

아래 로컬 JSON을 이번 작성 중 열어 succeeded·failed·경고·미실행 값을 읽었다. 새 자동 검사 실행 결과가 아니다. 네 보고서의 failed·succeededWithWarnings·notRun·inProcess는 모두 0이었다.

| 기존 검증 구간 | 성공 개수 | 이번에 읽은 로컬 보고서 |
|---|---|---|
| 물 | 48 | `Saved/Tests/Automation/20261009-222257-126/report/index.json` |
| 눈 | 52 | `Saved/Tests/Automation/20261009-233306-271/report/index.json` |
| 날씨·차폐 | 58 | `Saved/Tests/Automation/20261010-115547-766/report/index.json` |
| 불·환기·실험장 | 79 | `Saved/Tests/Automation/20261010-132752-448/report/index.json` |

각 실행의 맵·네트워크·렌더링·표현 수정 전후 범위는 소유 문서의 표를 따른다. 자동 검사 개수를 시스템 완성도나 사용자 학습 성취의 점수로 사용하지 않는다. 새 프로젝트 파일 생성·빌드·게임 실행·화면 캡처·패키징·성능 측정은 하지 않았다.

## 작성과 검토 범위

문서는 기존 학습 형식을 유지하며 문제, UE 개념, 이전 구현, 변경 이유, 현재 내부 연결, 실패·종료, 비용, 확인 방법과 질문·해설을 담는다. 새 설계 결정을 확정하지 않았으므로 Decisions·DesignLog의 과거 결정을 수정하지 않는다.

공통 시간과 표면 보존의 기존 Obsidian 노트 `UE5/Gameplay/World-Time-Commit-Boundaries.md`, `Conservative-Surface-Flow.md`를 참고하고 실제 프로젝트 소스와 대조했다. 볼트에는 별도 미커밋 변경이 있어 병합·커밋하지 않았다. 이번 설명에서 재사용한 원리는 기존 노트에 이미 있으며 신규 일반 지식 노트는 추가하지 않았다.

문서 23개의 상대 링크 362개와 이번에 추가한 실제 코드 발췌 35개를 검사했다. 전체 커밋을 Git에서 해석하고 발췌를 해당 커밋 또는 고정 사본과 대조했다. 파일·문서 앵커 누락과 발췌 불일치는 0건이었고 `git diff --check`도 통과했다. 새 문서는 Guide의 문서 지도에 등록했다. 맞춤법·문체·용어는 WritingGuide를 기준으로 검토하고 인용 코드는 유지했다.

고정 사본 이후 별도 작업에서 `CCLExperimentDirector.cpp`, `CCLExperimentTerrain.cpp`, `CCLWorldSimulationSubsystem.cpp/.h`, `CCLExperimentSmokeSubsystem.cpp/.h`가 바뀐 것을 마지막 대조에서 확인했다. 이 문서의 작업 트리 발췌는 위 해시의 사본을 유지한다. 이후 변경을 새로 실행·검증한 것처럼 포함하지 않는다.
