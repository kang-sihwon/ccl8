# 에이전트 도구 설치·복구

프로젝트 코드 검색은 Graft, 엔진 검색은 Graft UE 5.8, 문서 관계 분석은 Graphify를 사용한다. 스킬의 자동 선택은 [Guide](Guide.md)가 정한다. 코드·설정 편집 권한과 도구 실행 권한은 서로 다르다.

## AI 주도 제작용 확장 조사

2026-10-05 조사 결과와 설치·설정 기록의 차이는 [AgentToolingResearch](AgentToolingResearch.md)에 있다. UE 5.8.1 내장 Unreal MCP, 에디터 Toolset, 스킬 탐색과 Blender 연계를 비교했다. 조사 당시 후보는 설치·활성화하지 않았다. 현재 `CCL.uproject`에는 `ModelContextProtocol`, `EditorToolset`, `AutomationTestToolset`이 활성화되어 있고 두 에이전트의 Unreal MCP 주소는 `http://127.0.0.1:8000/mcp`다. 설정과 실제 연결 성공 여부는 구분하며 아래 복구용 플러그인 목록도 연결 성공 목록으로 해석하지 않는다.

## 공유 설정과 로컬 설정

| 위치 | 역할 |
|---|---|
| `.agents/skills/`, `.claude/skills/` | 두 에이전트의 공통 스킬, 라이선스 포함 |
| `.codex/skills/graphify/` | 기존 Graphify의 Codex 전용 배포본 |
| `.claude/settings.json`, `helpers/`, `commands/` | 공유 권한·훅·상태 표시·`/trace` |
| `.mcp.json` | Claude용 프로젝트·엔진 MCP, 저장소 루트에서 실행 |
| `.codex/config.example.toml` | Codex MCP 설정 예시 |
| `.codex/config.toml` | 기기별 프로젝트 MCP 설정, Git 제외 |
| `.claude/settings.local.json` | 기기별 엔진 읽기·볼트 접근, Git 제외 |
| `.local/agent-paths.json` | 엔진·인덱스·런타임·선택적 볼트 경로, Git 제외 |
| `.local/graft-runtime/` | 이 저장소에만 설치하는 Graft 런타임, Git 제외 |

저장소 루트는 `CCL.uproject`와 `.git`이 함께 있는 폴더이며 위치와 폴더명은 기기마다 달라도 된다. `ccl8-*` 스킬과 `graphify_ccl8.cmd`의 이름은 폴더명과 독립적으로 유지한다. 공유 도구의 프로젝트 경로는 실행 파일 위치를 기준으로 계산한다. 폴더를 옮겼으면 `.codex/config.toml`의 두 Graft 서버에서 `args`와 `cwd`를 새 저장소 루트로 수정한 뒤 에이전트를 재시작한다. 기존 Unreal MCP 항목은 보존한다. 엔진은 `Engine/Build/Build.version`이 있는 `Engine` 폴더를 지정한다. 엔진 인덱스는 기존 외부 디렉터리를 참조하며 자동 생성하지 않는다. `Tools/Agent/agent-paths.example.json`에 로컬 경로 형식이 있다.

실제 경로를 저장·제출하는 기준은 [VersionControl의 기기별 경로와 로컬 설정](VersionControl.md#기기별-경로와-로컬-설정)을 따른다. 새 기기 설정과 폴더 이동은 로컬 설정에 반영한다.

## 새 기기 준비

Git·Git LFS, Node.js 20 이상, npm, ripgrep을 설치한다. Graft는 기존 도구와 호환되는 0.18.0을 사용한다. 아래 명령은 저장소 루트의 PowerShell에서 실행한다.

```powershell
git lfs install --local
git lfs pull
# 네이티브 언어 바인딩 빌드에는 Python과 Visual C++ 빌드 도구가 필요하다.
# UE 소스 설치에 포함된 Python을 사용할 수도 있다.
$env:npm_config_python = '<Engine>/Binaries/ThirdParty/Python3/Win64/python.exe'
npm install --prefix .local/graft-runtime --cache .local/npm-cache --no-audit --no-fund @nanonets/graft@0.18.0
./Tools/Graft/apply_graft_orbis_patch.ps1 apply
./Tools/Agent/setup_local.ps1 -EngineRoot '<Engine>' -IndexRoot '<UE_5.8 indexes>'
node Tools/Agent/configure_runtime.cjs apply
node Tools/Agent/graft.cjs doctor
node Tools/Agent/graft.cjs engine --list-shards
```

Python 경로는 실제 설치 위치로 바꾼다. 네이티브 바인딩이 필요하므로 `--ignore-scripts`만으로 설치를 완료했다고 보지 않는다. 설치가 중간에 실패했으면 같은 Python 환경에서 `npm rebuild --prefix .local/graft-runtime --cache .local/npm-cache`로 복구한 뒤 패치를 적용한다.

개인 볼트를 연결하려면 최초 설정 때 `-VaultRoot '<vault>'`를 추가한다. 이미 로컬 설정이 있으면 스크립트가 덮어쓰지 않고 중단한다. 기존 값을 검토한 뒤 직접 병합하거나 전체 인수를 다시 지정해 `-Force`로 재생성한다.

Codex는 신뢰한 프로젝트의 `.codex/config.toml`로 기존 전역 MCP를 프로젝트 범위에서 덮어쓴다. Claude는 이 저장소에서 `.mcp.json`의 서버를 로드한다. 설정 뒤 두 에이전트를 재시작하고 MCP 목록과 실제 조회를 확인한다. 실행 중인 채팅이 새 설정을 즉시 반영했다고 가정하지 않는다.

## Graft 사용

스킬이 설명하는 `graft <인수>`는 이 프로젝트에서 `node Tools/Agent/graft.cjs cli <인수>`로 실행한다. 이 래퍼는 프로젝트 루트와 런타임을 고정하므로 전역 CLI가 없어도 동작한다.

```powershell
node Tools/Agent/graft.cjs cli map
node Tools/Agent/graft.cjs cli ask '심볼 또는 질문' --source
node Tools/Agent/graft.cjs engine --probe 'UObjectBase GetFName'
```

프로젝트 그래프가 아직 없으면 `map`의 미생성 결과는 정상이다. 사용자가 요청하면 `node Tools/Agent/graft.cjs cli build .`로 만든다. `graft/`는 저장소별 캐시이며 이전 프로젝트의 그래프를 복사하지 않는다.

Graft 0.18.0의 프로젝트 MCP는 그래프가 없을 때 `tools/list`에 빈 목록을 반환한다. 현재는 정상 상태다. 첫 그래프를 만든 뒤 에이전트의 MCP를 다시 로드하면 프로젝트 검색 도구가 표시된다. 엔진 MCP는 기존 인덱스가 있으므로 바로 조회할 수 있다.

`Tools/Graft/build_ue58_graft_indexes.cmd`는 로컬 경로 설정을 읽는다. 엔진 인덱스 생성은 큰 작업이므로 요청받은 범위에서만 수행한다. 현재 외부 엔진 인덱스는 공유하되 갱신 전 다른 사용 여부를 확인한다.

`graft-orbis-local.patch`의 이름과 내부 표식은 이전 도구 패치의 출처 및 검증용으로 보존했다. ccl8의 도구 사본과 로컬 런타임만 사용하며 이전 프로젝트 디렉터리를 실행 경로로 참조하지 않는다. 패치는 Graft 0.18.0의 파서 메모리·대량 파일 열거·확장자 필터를 보정한다. 라이선스는 `Tools/Graft/LICENSE`에 있다.

Claude 훅과 상태 표시는 같은 런타임 탐색기를 사용한다. 패키지가 없으면 원인을 stderr에 남기고 작업을 차단하지 않는다. 사용자 전역에도 동일한 Graft 훅이 있으면 중복 실행될 수 있다. 프로젝트 이전 작업은 전역 설정을 수정하지 않으며, 실제 중복이 확인되면 사용자 전역 훅 정리는 별도로 진행한다.

Graft 0.18.0은 캐시의 초기화 기록이 없으면 세션 시작·CLI·MCP 실행에서 전역 설정까지 자동으로 다시 쓸 수 있다. `configure_runtime.cjs`는 이 프로젝트의 런타임에 관리 모드 가드를 적용한다. ccl8 진입점은 가드를 확인하고 관리 모드를 켜므로 설정 재작성과 백그라운드 자동 갱신이 실행되지 않는다. 도구 설정은 저장소가 관리하며 런타임 재설치 후에는 가드를 다시 적용한다. 가드가 없으면 진입점은 실행을 중단한다.

## Graphify

기존 사용자 설치의 `uv`, `graphify`, `claude`를 재사용한다.

```powershell
./Tools/Graphify/graphify_ccl8.cmd status
# 새 PC에서 설치와 스킬 복구가 필요할 때만:
./Tools/Graphify/graphify_ccl8.cmd setup
# 요청한 분석을 위해 그래프 생성이 필요할 때:
./Tools/Graphify/graphify_ccl8.cmd build canonical
./Tools/Graphify/graphify_ccl8.cmd query canonical '문서 간 관계 질문'
```

`canonical`은 운영·사양 문서, `research`는 Narrative의 Research·Drafts·Reviews 문서를 대상으로 한다. 결과는 `.graphify-local/` 안에만 저장한다. 현재 그래프는 없으며 미생성 상태 조회는 정상이다. 자동 빌드·유료 호출은 하지 않는다. `setup`은 전역 도구 설치와 프로젝트 스킬 재생성이 포함된 복구 명령이므로 상태 확인을 대신해 실행하지 않는다.

## 플러그인과 공통 스킬 복구 목록

플러그인 패키지·로그인·설치 캐시는 저장소에 복사하지 않는다. 같은 PC의 사용자 설치를 재사용하며 새 PC에서는 각 앱에서 다시 설치·연결한다. 프로젝트 스킬은 저장소에서 복구된다.

2026-10-04 사용자 Codex 설정에서 활성화된 ID는 다음과 같다. 설치·연결 성공을 뜻하는 목록이 아니라 복구용 설정 목록이다.

- `superpowers@openai-curated`, `coderabbit@openai-curated`, `scite@openai-curated`, `biorender@openai-curated`, `github@openai-curated`
- `browser@openai-bundled`, `codex-app-tools@openai-bundled`, `visualize@openai-bundled`, `computer-use@openai-bundled`, `code-review@openai-bundled`
- `documents@openai-primary-runtime`, `pdf@openai-primary-runtime`, `spreadsheets@openai-primary-runtime`, `presentations@openai-primary-runtime`, `template-creator@openai-primary-runtime`

Claude 사용자 설정에는 `enabledPlugins` 항목이 없고 동기화된 로컬 매니페스트에서 `engineering` 1.2.0이 확인됐다. 새 PC에서 사용 가능 여부는 Claude 플러그인 화면에서 확인한다. Codex 사용자 스킬 `grill-me`, `pdf`와 앱 번들 스킬 역시 사용자·앱 범위를 유지한다.

프로젝트 공통 스킬은 `ccl8-design`, `ccl8-docs`, `ccl8-narrative`, `roadmap-review`, `grilling`, `writing-for-agents`, `technical-writing`, `humanizer`, `grammar-checker`, `style-guide`, `reviewing-technical-prose`, `obsidian-markdown`이다. `graft` 스킬은 Claude에, Graphify는 각 도구의 배포 형식에 맞춰 둔다. 공통 스킬은 두 위치를 함께 갱신하고 상류 라이선스를 보존한다.

## 볼트와 검증

### Obsidian 활용 절차

설계·구현·기술 조사·문제 해결에서는 Obsidian을 기존 지식의 참고와 검증 결과의 축적에 사용한다. 별도 볼트 정리 요청이나 `/trace` 호출을 기다리지 않고 다음 절차를 작업에 포함한다. 단순 문구 수정이나 Git 전달처럼 기술 지식을 다루지 않는 작업은 생략할 수 있다.

1. **시작:** `.local/agent-paths.json`의 `vaultRoot`로 연결을 확인하고 볼트 지침을 읽는다. `Index.md`, 해당 주제의 MOC(노트 색인), 관련 노트 순으로 좁혀 읽는다. 기존 설명·주의점·미확인 항목 중 이번 작업에 관련된 내용을 파악한 뒤 설계나 조사에 사용한다. 볼트 전체 검색 대신 색인의 항목과 심볼을 먼저 찾는다.
2. **검증:** 노트의 확인일과 엔진 버전을 살핀다. 이번 판단에 사용하는 엔진 동작은 `graft_ue58`와 현재 소스, 필요한 실행 검증으로 확인한다. 프로젝트 코드의 위치와 호출 관계는 `graft`로 확인한다. 노트와 현재 근거가 다르면 차이를 확인한 뒤 갱신하고, 검증하지 못한 내용은 미확인으로 남긴다.
3. **기록:** 한 주제의 작업을 마무리할 때 다른 프로젝트에서도 쓸 수 있고 다시 알아내는 데 비용이 드는 원리, 호출·수명 흐름, 함정, 버전 차이와 문제 해결 근거를 기존 노트에 반영한다. 적절한 노트가 없으면 주제 단위로 만들고 MOC에 등록하며 관련 노트를 연결한다. 노트 형식과 출처·확인일 표기는 볼트 지침을 따른다. ccl8 고유 결정·진행 상황·테스트 결과는 프로젝트 `Docs/`에 기록한다.
4. **완료 보고:** 실제로 참고한 노트와 작성·갱신한 노트를 링크로 밝히고, 무엇을 활용하거나 바꿨는지 짧게 적는다. 관련 노트가 없거나 새로 축적할 지식이 없으면 그 사실을 밝힌다. 접근 실패나 미확인 때문에 기록을 남기지 못했으면 이유와 남은 항목을 구분해 보고한다. 연결 확인만으로 검색·활용·작성을 완료했다고 보고하지 않는다.

볼트 경로가 없거나 접근할 수 없으면 임의의 경로를 추측하지 않고, 가능한 프로젝트 작업을 계속하며 볼트 활용이 빠진 이유를 보고한다. 쓰기 전 볼트의 Git 상태와 대상 노트 변경을 확인하고 기존 사용자 변경을 보존한다. 편집 후 diff와 노트·MOC 연결을 확인한다. ccl8 작업에서 볼트의 pull·commit·push는 사용자가 요청한 범위에서만 수행한다.

### 도구 구성 검증

검증 명령은 `node --test Tools/Agent/Tests/runtime.test.cjs`, Graft doctor·엔진 조회, Graphify status다. 엔진 전체 빌드는 도구 이전 검증에 포함하지 않는다.
