# AI 주도 제작을 위한 도구 조사

2026-10-05 기준 권고는 UE 내장 Unreal MCP를 먼저 검증하고, ccl8의 빌드·플레이·에셋 검증 절차를 프로젝트 스킬로 보강하는 것이다. 범용 스킬 대량 설치보다 에디터에서 수정하고 실제 결과를 확인하는 연결이 우선이다.

이 문서는 조사 결과와 도입 제안이다. 새 스킬·MCP·플러그인을 설치하거나 활성화하지 않았다. 검색 CLI는 임시 npm 캐시에서 실행했다. 실제 게임 제작 능력이나 새 MCP 연결 성공을 검증한 기록은 아니다.

## 현재 상태

| 구분 | 확인 결과 | ccl8 판단 |
|---|---|---|
| 엔진 | 로컬 `Engine/Build/Build.version`에서 5.8.1 확인 | 설치된 버전의 소스를 기준으로 한다 |
| Graft | 프로젝트의 Claude·Codex 설정에 프로젝트·엔진 서버 존재 | 코드 검색에 유지한다. 에디터 조작을 대신하지 않는다 |
| Graphify | 문서에 도구와 복구 절차가 있음 | 문서 관계 분석용으로 유지한다. 이번에는 그래프 생성·조회 미실시 |
| 공통 스킬 | 설계·문서·서사·문장 검토·학습용 스킬 구성 | 제작 실행과 게임 검증 절차 보강이 필요하다 |
| GitHub·Context7 | 플러그인 조회에서 installed=true, 현재 도구 목록에도 관련 기능 존재 | 추가 설치보다 기존 연결을 활용한다. 저장소 접근과 실제 조회 성공은 별도 확인한다 |
| 이미지 생성·컴퓨터 조작·브라우저·코드 리뷰 | 현재 세션에 관련 도구 또는 스킬 제공 | 같은 기능의 추가 플러그인은 우선순위가 낮다 |
| Superpowers | 사용자 설정에 활성화 항목과 로컬 캐시 존재. 플러그인 조회는 installed=false | 표면별 상태가 불일치한다. 재설치 전에 실제 로딩 여부를 확인한다 |
| Blender | PATH와 일반 설치 위치에서는 실행 파일을 확인하지 못함 | 다른 위치 설치 가능성은 남아 있다. 버전·경로와 사용할 MCP를 먼저 확인한다 |

AgentContextTools의 2026-10-04 목록은 복구용 설정 기록이다. 현재 설치·연결 상태 전체를 증명하지 않는다. 공식 skill-installer의 설치 표시 역시 사용자 스킬 폴더 기준이므로 앱 번들·시스템 스킬과 구분해야 한다.

## 최우선 후보: UE 내장 Unreal MCP

Epic 공식 문서와 로컬 소스 양쪽에서 확인했다. 엔진 내부 식별자는 `ModelContextProtocol`, 플러그인 화면의 이름은 `Unreal MCP`다. 현재 소스에 포함돼 있으므로 외부 패키지 설치보다 ccl8 프로젝트에서 활성화하고 연결하는 작업이 필요하다. 실험 기능이므로 실제 제공 도구와 동작은 소규모 테스트로 확인한다. [Epic Unreal MCP 문서](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor)

첫 조합으로 `ModelContextProtocol`, `EditorToolset`, `AutomationTestToolset`을 제안한다. `ToolsetRegistry`가 공통 기반이며 로컬 매니페스트는 PythonScriptPlugin과 EditorScriptingUtilities를 의존성으로 선언한다. `AllToolsets`는 전체 기능을 둘러볼 대안이지만, 실제 프로젝트는 사용할 기능부터 추가하는 편이 의존성을 파악하기 쉽다.

로컬 소스에서 확인한 근거는 다음과 같다. 경로는 엔진 루트 기준이며 게임 저장소에 엔진 파일을 복사하지 않는다.

| 근거 경로 | 확인 내용 |
|---|---|
| `Engine/Plugins/Experimental/ModelContextProtocol/ModelContextProtocol.uplugin` | Epic 제작, Experimental, 기본 비활성 |
| `Engine/Plugins/Experimental/ToolsetRegistry/ToolsetRegistry.uplugin` | EditorOnly, Python·EditorScriptingUtilities 의존 |
| `Engine/Plugins/Experimental/Toolsets/EditorToolset/Content/Python/editor_toolset/toolsets/` | 액터·씬·에셋·Blueprint·머티리얼·데이터 에셋 조작 코드 |
| `Engine/Plugins/Experimental/Toolsets/EditorToolset/Source/EditorToolset/Private/EditorAppToolset.h:223` | 에디터 화면 캡처 함수 |
| 같은 파일 `:365` | StartPIE 함수. StopPIE·실행 상태 조회도 선언됨 |
| `Engine/Plugins/Experimental/Toolsets/AutomationTestToolset/Source/AutomationTestToolset/Public/AutomationTestToolset.h:40` | 자동화 테스트 탐색 도구 |

이 기능들은 마을 배치, Blueprint·DataAsset 구성과 검토 화면 확보에 유용할 것으로 판단한다. 소스 존재만 확인했으며 현재 ccl8에서 실행하지 않았다. 전투 입력 자동화나 실제 게임 승리 조건은 별도 테스트가 필요하다.

### 연결할 때의 프로젝트 특이점

현재 ccl8에는 .uproject가 없다. 프로젝트 생성 후 에디터에서 플러그인 로딩과 서버 시작을 검증해야 한다. 서버는 로컬 연결로 유지하고 에디터 조작 요청은 순서대로 실행한다.

설정 생성 명령을 그대로 실행하기 전에 다음 소스 차이를 반영한다.

- `ModelContextProtocolClientConfig.cpp:159` 부근은 소스 빌드의 기본 출력 위치를 `FPaths::RootDir()`로 정한다. 이 환경에서는 엔진 상위 루트에 설정이 생길 수 있으므로 ccl8 설정에 직접 병합한다.
- 같은 파일 `:118`의 TOML 생성기는 기존 파일이 있으면 거부한다. 현재 ccl8의 Codex 설정에는 Graft가 있으므로 새 서버 항목만 추가해야 한다.
- Claude JSON 생성기는 기존 항목 병합을 지원하지만, Graft 설정과 프로젝트 경로를 보존했는지 diff로 확인한다.

위 파일의 전체 경로는 `Engine/Plugins/Experimental/ModelContextProtocol/Source/ModelContextProtocolEngine/Private/ModelContextProtocolClientConfig.cpp`다.

### 엔진에 포함된 작업 지침

EditorToolset의 `Content/Python/editor_toolset/skills/`에서 BlueprintBasicsSkill, MaterialBasicsSkill, DefaultOutdoorLightingSkill을 확인했다. 이는 저장소의 SKILL.md와 다른 UE AgentSkill 형식이며, 서버 연결 후 검색·로드 방법을 검증해서 사용한다.

식생·반복 소품 배치가 필요해지면 PCGToolset과 Skill_PCGGraphGeneration을 추가 후보로 삼는다. 로컬 Content/Skills에도 관련 에셋이 있다. 수작업 배치로 충분한 작은 장면에서는 PCG 설정 비용과 비교한다. [Epic PCG·MCP 작업 지침](https://dev.epicgames.com/documentation/unreal-engine/working-with-pcg-and-llms-using-unreal-mcp-in-unreal-engine)

## 스킬 탐색 결과

사용자가 언급한 스킬 탐색에 해당하는 후보는 Vercel의 `find-skills`다. 공개 스킬을 찾아주는 안내 스킬이며 `npx skills find` CLI는 이 스킬을 설치하지 않아도 사용할 수 있다. Codex의 내장 `skill-installer`는 공식 목록 조회와 지정한 GitHub 경로 설치를 담당한다. [Vercel find-skills 원문](https://github.com/vercel-labs/skills/blob/main/skills/find-skills/SKILL.md), [공식 스킬 목록](https://github.com/openai/skills/tree/main/skills/.curated)

이번 조회는 다음과 같이 수행했다.

- 내장 `skill-installer/scripts/list-skills.py --format json`로 공식 목록을 조회했다. 이 목록에서 UE 전용 스킬은 찾지 못했다.
- npm에서 버전을 확인한 뒤 `skills@1.7.0 find unreal`, `find 'game development'`, `find blender`를 실행했다.
- 검색 결과에 무관한 항목도 많이 섞였다. 후반 검색에서 결과 출력 뒤 Windows libuv의 `UV_HANDLE_CLOSING` assertion으로 비정상 종료했다. 검색 일부 결과는 확보했지만 세 명령 모두 정상 종료했다고 보지 않는다.
- CLI 검색 결과에서 관련 후보를 추린 뒤 GitHub 원문을 직접 읽었다. 설치 수를 품질 근거로 사용하지 않았다.

| 후보 | 판단 | 근거와 조건 |
|---|---|---|
| vercel-labs/skills의 find-skills | 선택적 도입 | 향후 스킬 탐색 반복에 편리하다. 설치만으로 UE 제작 기능이 생기는 것은 아니다 |
| Superpowers의 systematic-debugging, verification-before-completion | 기존 설치 상태 확인 후 선별 활용 | 원인 확인과 실행 증거 중심의 절차가 유용하다. 전체 워크플로를 다시 설치하면 ccl8의 계획·승인 규칙과 중복될 수 있다 |
| arjun988/blender-skills의 unreal-export | Blender 채택 후 조건부 도입 | 단위·충돌·소켓·LOD·UE 반입 검증을 다룬다. 선행 스킬과 참조 파일이 있으므로 단일 파일만 복사해서는 충분하지 않다 |
| 같은 묶음의 asset-optimization | 수정 검토 후 조건부 도입 | 모델 정리 체크리스트는 유용하지만 고정 LOD 비율·UV 수치·이름 규칙은 ccl8 에셋 종류에 맞춰 검토해야 한다 |
| Jeffallan/claude-skills의 game-developer | 지금은 보류 | Unity와 Unreal을 함께 다루고, 고정 프레임 목표와 Unity 규칙이 섞여 있다. 기존 ccl8 설계 지침보다 추가 이득이 작다고 판단한다 |
| sickn33/agentic-awesome-skills의 unreal-engine-cpp-pro | 원문 그대로 도입하지 않음 | AddToRoot 일반 우선 권고와 비동기 로딩 절의 LoadSynchronous 예시를 검토해야 한다. 현재 엔진 소스와 프로젝트 규칙을 우선한다 |
| openai/skills의 gh-fix-ci | GitHub Actions 도입 후 재검토 | CI 실패 로그 조사용이다. 현재 원격·CI가 없고 gh 실행 파일도 PATH에서 확인하지 못했다 |

후보의 원문: [Superpowers 디버깅](https://github.com/obra/superpowers/blob/main/skills/systematic-debugging/SKILL.md), [검증 절차](https://github.com/obra/superpowers/blob/main/skills/verification-before-completion/SKILL.md), [unreal-export](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/unreal-export/SKILL.md), [asset-optimization](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/asset-optimization/SKILL.md), [game-developer](https://github.com/Jeffallan/claude-skills/blob/main/skills/game-developer/SKILL.md), [unreal-engine-cpp-pro](https://github.com/sickn33/agentic-awesome-skills/blob/main/skills/unreal-engine-cpp-pro/SKILL.md), [gh-fix-ci](https://github.com/openai/skills/blob/main/skills/.curated/gh-fix-ci/SKILL.md).

## 아트 제작과 외부 MCP

Blender MCP는 소품 제작, 메시 수정과 반입 준비를 AI가 직접 처리할 때 유용한 후속 후보다. Blender 공식 Lab MCP와 ahujasid의 커뮤니티 MCP는 서로 다른 프로젝트다. 공식 검색 결과에서 Lab MCP의 Blender 5.1 이상 요구를 확인했지만 공식 본문·저장소 직접 열기는 접근 오류가 나서 상세 설치 검증은 미완료다. 커뮤니티 저장소는 직접 열어 별도 프로젝트임을 확인했다. [Blender 공식 MCP 안내](https://www.blender.org/lab/mcp-server/), [커뮤니티 MCP](https://github.com/ahujasid/mcp-for-blender)

사용할 Blender 버전과 서버를 정한 뒤 선택한 스킬이 요구하는 도구 이름·기능과 맞는지 확인해야 한다. 연결 확인은 간단한 소품 생성, 저장, 재열기와 UE 반입으로 진행하는 것을 제안한다. 캐릭터 리깅·애니메이션과 최종 아트 품질까지 자동 보장되는 것은 아니다.

이미지 생성 도구는 현재 사용 가능하므로 콘셉트, 아이콘과 시각 초안에는 추가 플러그인이 필수는 아니다. 3D 모델·리그·게임용 애니메이션 제작은 별도 경로로 다룬다.

외부 Unreal MCP 중 ChiR24/Unreal_mcp는 UE 5.8 지원과 에디터·PIE 작업을 표방한다. 다만 문서는 사전 배포 버전의 별도 플러그인 설치를 안내한다. 내장 MCP에 부족한 구체적 작업이 발견됐을 때 비교할 후보로 남긴다. ccl8에서 빌드·실행 검증하지 않았다. [저장소와 설치 안내](https://github.com/ChiR24/Unreal_mcp)

플러그인 디렉터리에서 Unreal·Blender·Meshy 관련어를 검색했지만 직접 맞는 설치 항목은 이번 결과에 없었다. 검색 결과가 전체 카탈로그를 보장하지 않으므로 해당 제품 자체가 없다고 결론내리지 않는다.

## ccl8에 직접 만들 스킬 제안

아래 이름은 새로 작성할 후보이며 설치 가능한 기존 제품 이름이 아니다.

| 후보 | 반복 절차 | 완료 증거 |
|---|---|---|
| ccl8-ue-editor | 활성 프로젝트 확인, 기존 에셋 확인, 제한된 변경, 저장·재조회 | 목표 에셋의 저장 상태와 실제 화면 |
| ccl8-verify | 변경에 맞는 빌드·테스트·PIE·패키징 선택, 실패 재현과 결과 기록 | 실행 명령·로그·화면·재도전 결과 |
| ccl8-asset-pipeline | 원본 출처, 용도, 크기·축·충돌·리그·머티리얼 검사, 반입·LFS 확인 | 에디터 반입 결과와 해당 용도 검증 |

스킬은 반복 절차를 제공하고 실제 동작은 도구가 수행한다. 따라서 먼저 에디터 연결과 대표 작업을 확인한 뒤 검증된 절차를 스킬로 만드는 순서가 적절하다. [OpenAI 스킬·MCP 역할 설명](https://developers.openai.com/plugins/build/skills)

## 제안하는 도입 순서

1. ccl8 UE 프로젝트가 생기면 내장 MCP와 최소 Toolset을 활성화하고 두 에이전트에서 연결을 확인한다.
2. 테스트 맵에 액터 생성·수정·저장·재조회, 화면 캡처, PIE 시작·종료를 순서대로 검증한다.
3. 그 결과를 ccl8-ue-editor와 ccl8-verify에 담고, 실제 게임 테스트를 추가한다.
4. 아트 제작 방식이 정해지면 Blender 설치·MCP·내보내기 스킬을 소품 하나로 검증한다.
5. PCG, 추가 AI·애니메이션 Toolset, GitHub CI는 사용하는 기능에 맞춰 도입한다.

이 순서는 도입 제안이며 설치 승인이나 구현 완료 기록이 아니다. 이번 조사에서는 전역 설정, 엔진 파일, MCP 설정과 기존 스킬을 변경하지 않았다.
