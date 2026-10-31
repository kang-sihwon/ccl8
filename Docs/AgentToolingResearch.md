# AI 주도 제작을 위한 도구 조사

AI 주도 제작에 쓸 도구는 UE 내장 Unreal MCP를 먼저 쓰고, ccl8의 빌드·플레이·에셋 검증 절차를 프로젝트 스킬로 보강하는 방향으로 정했다. 이 문서는 도구 후보의 조사와 채택·제외 근거를 소유한다. 도구 설치와 설정의 정본은 [AgentContextTools](AgentContextTools.md)다.

2026-10-05 조사는 후보를 찾은 기록이다. 그 뒤 반영된 것은 아래 반영 상태를 따른다.

## 하위 문서

| 문서 | 읽는 때 |
|---|---|
| [외부 스킬 후보 검토](AgentToolingResearch/SkillReview.md) | 외부 스킬 후보의 채택과 제외 근거를 찾을 때 |
| [스킬 묶음 통합 검토](AgentToolingResearch/BundleReview.md) | 스킬 묶음 세 개에서 무엇을 가져오고 뺐는지 찾을 때 |

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

## 내장 Unreal MCP의 근거

엔진 내부 식별자는 `ModelContextProtocol`, 플러그인 화면의 이름은 `Unreal MCP`다. 실험 기능이라 실제 제공 도구와 동작은 소규모 테스트로 확인한다. [Epic Unreal MCP 문서](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor)

`ToolsetRegistry`가 공통 기반이며 로컬 매니페스트는 PythonScriptPlugin과 EditorScriptingUtilities를 의존성으로 선언한다. `AllToolsets`는 전체 기능을 둘러볼 대안이지만, 실제 프로젝트는 사용할 기능부터 추가하는 편이 의존성을 파악하기 쉬워 최소 조합을 골랐다.

아래는 2026-10-05에 로컬 소스에서 확인한 근거다. 경로는 엔진 루트 기준이다.

| 근거 경로 | 확인 내용 |
|---|---|
| `Engine/Plugins/Experimental/ModelContextProtocol/ModelContextProtocol.uplugin` | Epic 제작, Experimental, 기본 비활성 |
| `Engine/Plugins/Experimental/ToolsetRegistry/ToolsetRegistry.uplugin` | EditorOnly, Python·EditorScriptingUtilities 의존 |
| `Engine/Plugins/Experimental/Toolsets/EditorToolset/Content/Python/editor_toolset/toolsets/` | 액터·씬·에셋·Blueprint·머티리얼·데이터 에셋 조작 코드 |
| `Engine/Plugins/Experimental/Toolsets/EditorToolset/Source/EditorToolset/Private/EditorAppToolset.h:223` | 에디터 화면 캡처 함수 |
| 같은 파일 `:365` | StartPIE 함수. StopPIE·실행 상태 조회도 선언됨 |
| `Engine/Plugins/Experimental/Toolsets/AutomationTestToolset/Source/AutomationTestToolset/Public/AutomationTestToolset.h:40` | 자동화 테스트 탐색 도구 |

### 설정 생성기의 주의점

- `ModelContextProtocolClientConfig.cpp:159` 부근은 소스 빌드의 기본 출력 위치를 `FPaths::RootDir()`로 정한다. 이 환경에서는 엔진 상위 루트에 설정이 생길 수 있으므로 ccl8 설정에 직접 병합한다.
- 같은 파일 `:118`의 TOML 생성기는 기존 파일이 있으면 거부한다. 현재 ccl8의 Codex 설정에는 Graft가 있으므로 새 서버 항목만 추가해야 한다.
- Claude JSON 생성기는 기존 항목 병합을 지원하지만, Graft 설정과 프로젝트 경로를 보존했는지 diff로 확인한다.

위 파일의 전체 경로는 `Engine/Plugins/Experimental/ModelContextProtocol/Source/ModelContextProtocolEngine/Private/ModelContextProtocolClientConfig.cpp`다.

### 엔진에 포함된 작업 지침

EditorToolset의 `Content/Python/editor_toolset/skills/`에서 BlueprintBasicsSkill, MaterialBasicsSkill, DefaultOutdoorLightingSkill을 확인했다. 이는 저장소의 SKILL.md와 다른 UE AgentSkill 형식이며, 서버 연결 후 검색·로드 방법을 검증해서 사용한다.

식생·반복 소품 배치가 필요해지면 PCGToolset과 Skill_PCGGraphGeneration을 추가 후보로 삼는다. 로컬 Content/Skills에도 관련 에셋이 있다. 수작업 배치로 충분한 작은 장면에서는 PCG 설정 비용과 비교한다. [Epic PCG·MCP 작업 지침](https://dev.epicgames.com/documentation/unreal-engine/working-with-pcg-and-llms-using-unreal-mcp-in-unreal-engine)

## 아트 제작과 외부 MCP

Blender MCP는 소품 제작, 메시 수정과 반입 준비를 AI가 직접 처리할 때 유용한 후속 후보다. Blender 공식 Lab MCP와 ahujasid의 커뮤니티 MCP는 서로 다른 프로젝트다. 공식 검색 결과에서 Lab MCP의 Blender 5.1 이상 요구를 확인했지만 공식 본문·저장소 직접 열기는 접근 오류가 나서 상세 설치 검증은 미완료다. 커뮤니티 저장소는 직접 열어 별도 프로젝트임을 확인했다. [Blender 공식 MCP 안내](https://www.blender.org/lab/mcp-server/), [커뮤니티 MCP](https://github.com/ahujasid/mcp-for-blender)

사용할 Blender 버전과 서버를 정한 뒤 선택한 스킬이 요구하는 도구 이름과 기능이 맞는지 확인한다. 연결 확인은 간단한 소품 생성, 저장, 재열기, UE 반입으로 진행하는 것을 제안한다. 캐릭터 리깅, 애니메이션, 최종 아트 품질까지 자동 보장되는 것은 아니다.

이미지 생성 도구는 현재 사용 가능하므로 콘셉트, 아이콘과 시각 초안에는 추가 플러그인이 필수는 아니다. 3D 모델·리그·게임용 애니메이션 제작은 별도 경로로 다룬다. 외부 이미지·이미지→3D 생성 서비스의 비교는 [컨셉 이미지 도구 비교 테스트](ConceptImageToolTest.md)가 소유한다.

외부 Unreal MCP 중 ChiR24/Unreal_mcp는 UE 5.8 지원과 에디터·PIE 작업을 표방한다. 다만 문서는 사전 배포 버전의 별도 플러그인 설치를 안내한다. 내장 MCP에 부족한 구체적 작업이 발견됐을 때 비교할 후보로 남긴다. ccl8에서 빌드·실행 검증하지 않았다. [저장소와 설치 안내](https://github.com/ChiR24/Unreal_mcp)

플러그인 디렉터리에서 Unreal·Blender·Meshy 관련어를 검색했지만 직접 맞는 설치 항목은 이번 결과에 없었다. 검색 결과가 전체 카탈로그를 보장하지 않으므로 해당 제품 자체가 없다고 결론내리지 않는다.
