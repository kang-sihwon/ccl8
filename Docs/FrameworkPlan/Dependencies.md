# Framework 의존 정리 현황

상위 문서는 [FrameworkPlan](../FrameworkPlan.md)이다.

## 2026-10-11 의존 정리 현황

2026-10-11 전체 소스 검토에서 지적한 역방향 의존 다섯 개를 `CCL` 모듈 안에서 정리했다. 이는 "착수 후 진행 순서"에 있는 이동 절차 3번의 일부를 미리 수행한 것이며, 모듈·플러그인 분리 착수가 아니다.

| 이전 의존 | 정리 방식 |
|---|---|
| 전투 판정이 Agent 컴포넌트를 직접 호출 | Combat이 `ICCLHitReceiver`를 정의하고 Agent가 구현한다 |
| Agents가 Campaign Actor를 생성 | Agent 세계는 인구 변경만 알리고, Campaign의 `UCCLVillagePopulationSubsystem`이 Actor를 만든다. Agent 쪽은 `ICCLAgentEmbodiment`와 `ICCLAgentWorldHost`로 표현·세계 설정을 받는다 |
| 환경 세계 스냅샷 헤더가 Session을 include | 세계 스냅샷은 이름 있는 구간만 소유하고, 플레이어·보관함은 `ICCLWorldParticipant`로 자기 구간을 수집·검증·복원한다 |
| Items가 Combat의 Fighter를 호출 | Items가 `ICCLEquipmentPresenter`를 정의하고 Fighter가 구현한다 |
| AbilitySystem과 Combat의 상호 의존 | `UCCLCombatAbility`를 Combat으로 옮겨 AbilitySystem이 다른 게임 폴더를 include하지 않는다 |

폴더 단위의 순환은 아직 남아 있다. 적 캐릭터는 Combat 폴더에 있지만 Agent 컴포넌트를 조합하므로 Combat과 Agents가 서로 include한다. 환경 세계 시뮬레이션은 생활 시뮬레이션과 음식 인벤토리를 함께 확정하므로 Environment와 Agents, Environment와 Items도 서로 include한다. 분리를 착수하면 적 캐릭터를 콘텐츠 계층으로 옮기고 환경·생활·인벤토리의 공통 시간 계약을 먼저 나눠야 한다. 측정 기준은 하위 폴더 사이의 `#include` 방향이며 2026-10-11 작업 트리에서 확인했다.

### 단계 1-2의 전제와 분리 비용

결정 22의 단계 1-2 계획은 아래 의존 조사와 분리 비용 측정을 근거로 한다. 의존 설명은 2026-10-11 검토한 작업 트리 기준이며, 수치의 재현 기준은 표 아래에 구분한다.

- `UI/Core`와 `Map`은 같은 폴더의 헤더와 엔진 헤더만 include한다. 다른 게임 폴더는 include하지 않는다.
- `UI` 폴더 최상위에는 게임 화면이 함께 있다. `SCCLInventoryWidget.cpp`는 Items, `CCLMapScreen.cpp`는 Map, `CCLSessionMenuScreen.cpp`는 Session을 include한다. 이 화면들은 `KSIUI`로 옮기지 않고 `CCL`에 남긴다.
- Items는 저장 참여 계약이 Environment 폴더에 있어 Environment를 include한다 (`CCLInventorySnapshot.cpp`, `CCLInventoryPersistenceSubsystem.h`). 음식과 줍기 아이템도 환경 세계를 include한다 (`CCLInventoryFood.cpp`, `CCLFoodEnvironment.cpp`, `CCLWorldPickup.cpp`).
- Session은 Campaign을 include한다 (`CCLGameInstance.cpp`, `CCLPlayerPersistenceSubsystem.cpp`).

| 항목 | 측정값 | 분리할 때 필요한 작업 |
|---|---|---|
| 리플렉션 선언(`UCLASS`·`USTRUCT`·`UENUM`·`UINTERFACE`) | 307개 | 옮기는 타입의 경로가 `/Script/CCL.X`에서 바뀐다 |
| `CCL_API` 사용 | 323곳 | 옮기는 타입의 export 매크로를 새 모듈의 것으로 바꾼다 |
| `/Script/CCL`을 참조하는 에셋 | 454개 중 100개 | 옮긴 타입마다 CoreRedirects를 넣고 실제 로드로 확인한다 |
| `Config/`의 `/Script/CCL.` 참조 | 5곳 | 설정 섹션 이름과 클래스 경로를 바꾼다 |

소스와 설정 수치는 커밋 `ef44376d21811df02b5cf28b2c50105047921e2c`에 고정한다. 아래 명령은 저장소 루트의 PowerShell에서 실행하며 순서대로 307, 323, 5를 반환한다. 리플렉션 수치는 선언 매크로의 일치 건수이며 export 수치는 `CCL_API` 토큰의 사용 건수다. 옮길 타입 수나 리다이렉트 수를 그대로 뜻하지 않는다.

```powershell
$FrameworkRevision = 'ef44376d21811df02b5cf28b2c50105047921e2c'
(git grep -o -E '^[[:space:]]*U(CLASS|STRUCT|ENUM|INTERFACE)\(' $FrameworkRevision -- Source/CCL | Measure-Object -Line).Lines
(git grep -o -w 'CCL_API' $FrameworkRevision -- Source/CCL | Measure-Object -Line).Lines
(git grep -n -F '/Script/CCL.' $FrameworkRevision -- Config | Measure-Object -Line).Lines
```

에셋 수치는 2026-10-11 로컬 `Content/`의 `.uasset`·`.umap`을 대상으로 다시 확인했다. 다음 명령은 전체 파일 454개와 `/Script/CCL` 바이트열을 포함한 파일 100개를 반환한다. Git LFS의 실제 에셋을 받은 작업 트리에서 실행해야 하며, 포인터 파일을 검색한 수치와 구분한다. 이 바이트열 검사는 에디터에서 타입 참조와 로드를 검증하는 절차를 대신하지 않는다.

```powershell
(rg --files Content -g '*.uasset' -g '*.umap' | Measure-Object -Line).Lines
(rg -a -l -F '/Script/CCL' Content -g '*.uasset' -g '*.umap' | Measure-Object -Line).Lines
```

같은 날 후속 코드 수정을 포함한 작업 트리에서는 리플렉션 선언 308개와 `CCL_API` 330곳을 확인했다. 이 값은 위 커밋의 기준값에 합치지 않는다. 실제 분리 착수 시 대상 커밋과 측정 명령을 기록하고, 옮길 파일과 타입만 다시 집계한다.
