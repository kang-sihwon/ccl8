# AI 주도 제작을 위한 도구 조사

AI 주도 제작에 쓸 도구는 UE 내장 Unreal MCP를 먼저 검증하고, ccl8의 빌드·플레이·에셋 검증 절차를 프로젝트 스킬로 보강하는 방향으로 정했다. 범용 스킬 대량 설치보다 에디터에서 수정하고 실제 결과를 확인하는 연결이 우선이다. 이 문서는 도구 후보의 조사, 채택·제외 근거, 도입 제안을 소유한다. 2026-10-05 조사와 2026-10-25 스킬 묶음 검토는 각 날짜의 기록이다.

2026-10-05 조사 당시에는 새 스킬·MCP·플러그인을 설치하거나 활성화하지 않았다. 검색 CLI는 임시 npm 캐시에서 실행했다. 그 조사는 실제 게임 제작 능력이나 새 MCP 연결 성공을 검증한 기록이 아니다.

## 하위 문서

| 문서 | 읽는 때 |
|---|---|
| [스킬 검토 기록](AgentToolingResearch/SkillReview.md) | 외부 스킬 묶음의 채택·제외 근거를 찾을 때 |

## 반영 상태

2026-10-26 저장소에서 확인한 상태다. 도구 설치·설정의 정본은 [AgentContextTools](AgentContextTools.md)다.

| 제안 | 상태 | 근거 |
|---|---|---|
| 내장 Unreal MCP와 최소 Toolset 활성화 | 활성화됨. `ModelContextProtocol`, `EditorToolset`, `AutomationTestToolset`을 Editor 타깃에 켰다. 연결에는 에디터 실행이 필요하다 | `CCL.uproject`, [AgentContextTools의 Unreal MCP](AgentContextTools/Setup.md#unreal-mcp) |
| MCP로 PIE 시작·조작·캡처 검증 | 2026-10-12 QA에서 사용 | [QA 보고서](QA/README.md) |
| `ccl8-verify` | 작성. MCP PIE 절차는 스킬의 `references/mcp-pie.md`로 옮겼다 | `.agents/skills/ccl8-verify/` |
| `ccl8-ue-editor`, `ccl8-asset-pipeline` | 만들지 않음 | `.agents/skills/`에 없음 |
| Blender 설치·MCP | 확인한 기록 없음 | |
| 스킬 묶음 통합 | 결정 28로 확정했고 2026-10-27 재구성을 마쳤다. 도구와 스킬 구성은 AgentContextTools가 소유한다 | [결정 28](decisions/0028-agent-workflow-restructure.md), [AgentContextTools](AgentContextTools.md) |
| GitHub CI | 도입하지 않음 | 재구성 지도의 범위 밖 |

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

## 아트 제작과 외부 MCP

Blender MCP는 소품 제작, 메시 수정과 반입 준비를 AI가 직접 처리할 때 유용한 후속 후보다. Blender 공식 Lab MCP와 ahujasid의 커뮤니티 MCP는 서로 다른 프로젝트다. 공식 검색 결과에서 Lab MCP의 Blender 5.1 이상 요구를 확인했지만 공식 본문·저장소 직접 열기는 접근 오류가 나서 상세 설치 검증은 미완료다. 커뮤니티 저장소는 직접 열어 별도 프로젝트임을 확인했다. [Blender 공식 MCP 안내](https://www.blender.org/lab/mcp-server/), [커뮤니티 MCP](https://github.com/ahujasid/mcp-for-blender)

사용할 Blender 버전과 서버를 정한 뒤 선택한 스킬이 요구하는 도구 이름·기능과 맞는지 확인해야 한다. 연결 확인은 간단한 소품 생성, 저장, 재열기와 UE 반입으로 진행하는 것을 제안한다. 캐릭터 리깅·애니메이션과 최종 아트 품질까지 자동 보장되는 것은 아니다.

이미지 생성 도구는 현재 사용 가능하므로 콘셉트, 아이콘과 시각 초안에는 추가 플러그인이 필수는 아니다. 3D 모델·리그·게임용 애니메이션 제작은 별도 경로로 다룬다. 외부 이미지·이미지→3D 생성 서비스의 비교는 [컨셉 이미지 도구 비교 테스트](ConceptImageToolTest.md)가 소유한다.

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

2026-10-25에 `ccl8-verify` 초안을 `.claude/skills/ccl8-verify/`와 `.agents/skills/ccl8-verify/`에 작성했다. 스모크 실행 경로의 검증 결과와 MCP 경로의 미검증 상태는 스킬의 `SKILL.md`에 적는다.

## 제안하는 도입 순서

1. ccl8 UE 프로젝트가 생기면 내장 MCP와 최소 Toolset을 활성화하고 두 에이전트에서 연결을 확인한다.
2. 테스트 맵에 액터 생성·수정·저장·재조회, 화면 캡처, PIE 시작·종료를 순서대로 검증한다.
3. 그 결과를 ccl8-ue-editor와 ccl8-verify에 담고, 실제 게임 테스트를 추가한다.
4. 아트 제작 방식이 정해지면 Blender 설치·MCP·내보내기 스킬을 소품 하나로 검증한다.
5. PCG, 추가 AI·애니메이션 Toolset, GitHub CI는 사용하는 기능에 맞춰 도입한다.

이 순서는 2026-10-05의 도입 제안이며 설치 승인이나 구현 완료 기록이 아니다. 그 조사에서는 전역 설정, 엔진 파일, MCP 설정과 기존 스킬을 변경하지 않았다. 이후 반영된 항목은 위 "반영 상태"를 따른다.
