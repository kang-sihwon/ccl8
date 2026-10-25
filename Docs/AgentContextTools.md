# 에이전트 도구 설치·복구

에이전트 도구의 위치, 설치, 사용 명령, 복구, 진단을 소유한다. 어떤 일에 어떤 도구를 쓰는지는 [AGENTS.md](../AGENTS.md)의 길잡이가 정한다. 도구를 실행할 수 있다고 해서 코드·설정을 고칠 권한이 생기지는 않는다. 편집 권한은 [Coding](Coding.md)이 정한다.

도구 후보의 조사와 채택·제외 근거는 [AgentToolingResearch](AgentToolingResearch.md)가 소유한다.

## 공유 설정과 로컬 설정

| 위치 | 역할 | Git |
|---|---|---|
| `.agents/skills/` | 스킬 정본. 상류 라이선스 포함 | 추적 |
| `.claude/skills/` | Claude용 스킬 사본. `node Tools/Agent/skills.cjs sync --write`로 맞춘다 | 추적 |
| `.codex/skills/graphify/` | Graphify의 Codex 전용 배포본 | 추적 |
| `.claude/settings.json` | 공유 권한, PreToolUse 확인 훅, graft 훅, 상태 표시 | 추적 |
| `.claude/helpers/`, `.claude/commands/` | graft 훅·상태 표시 스크립트, `/trace` 명령 | 추적 |
| `.mcp.json` | Claude용 프로젝트·엔진 Graft와 Unreal MCP. 저장소 루트에서 실행 | 추적 |
| `.codex/config.example.toml` | Codex MCP 설정 예시 | 추적 |
| `Tools/Agent/` | Graft 래퍼, 로컬 설정 스크립트, 검사 도구와 그 시험 | 추적 |
| `.codex/config.toml` | 기기별 Codex MCP 설정 | 제외 |
| `.claude/settings.local.json` | 기기별 엔진 읽기·볼트 접근 | 제외 |
| `.local/agent-paths.json` | 엔진·인덱스·런타임·선택적 볼트 경로. 형식은 `Tools/Agent/agent-paths.example.json` | 제외 |
| `.local/graft-runtime/` | 이 저장소 전용 Graft 런타임 | 제외 |

에이전트가 스스로 부르지 않는 사용자 호출 스킬은 `SKILL.md` 머리말에 `disable-model-invocation: true`(Claude)를 두고, 같은 폴더의 `agents/openai.yaml`에 `allow_implicit_invocation: false`(Codex)를 둔다. 목록은 `ccl8-help` 스킬에 있다. `graphify`는 `graphify_ccl8.cmd setup`이 `.claude/skills/`와 `.codex/skills/`에 설치하고 다시 쓰는 도구 관리 스킬이라 정본 폴더 밖에 둔다.

저장소 루트는 `CCL.uproject`와 `.git`이 함께 있는 폴더다. 위치와 폴더명은 기기마다 달라도 되고, `ccl8-*` 스킬과 `graphify_ccl8.cmd`의 이름은 폴더명과 무관하다. 공유 도구는 실행 파일 위치로 프로젝트 경로를 계산한다. 엔진 경로는 `Engine/Build/Build.version`이 있는 `Engine` 폴더다. 엔진 인덱스는 기존 외부 디렉터리를 참조하고 자동 생성하지 않는다.

폴더를 옮겼으면 `.codex/config.toml`의 두 Graft 서버에서 `args`와 `cwd`를 새 저장소 루트로 고치고 에이전트를 재시작한다. Unreal MCP 항목은 그대로 둔다. 실제 경로의 제출 기준은 [VersionControl의 기기별 경로와 로컬 설정](VersionControl.md#기기별-경로와-로컬-설정)을 따른다.

## 새 기기 준비

Git·Git LFS, Node.js 20 이상, npm, ripgrep을 설치한다. Graft는 기존 패치와 호환되는 0.18.0을 쓴다. 아래 명령은 저장소 루트의 PowerShell에서 실행한다.

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

Python 경로는 실제 설치 위치로 바꾼다. 네이티브 바인딩이 필요하므로 `--ignore-scripts` 설치는 완료가 아니다. 설치가 중간에 실패했으면 같은 Python 환경에서 `npm rebuild --prefix .local/graft-runtime --cache .local/npm-cache`로 복구한 뒤 패치를 적용한다.

개인 볼트를 연결하려면 최초 설정 때 `-VaultRoot '<vault>'`를 추가한다. 로컬 설정이 이미 있으면 스크립트는 덮어쓰지 않고 멈춘다. 기존 값을 검토해 직접 병합하거나, 전체 인수를 다시 지정해 `-Force`로 다시 만든다.

Codex는 신뢰한 프로젝트의 `.codex/config.toml`로 전역 MCP를 프로젝트 범위에서 덮어쓰고, Claude는 `.mcp.json`의 서버를 불러온다. 설정을 바꾼 뒤에는 두 에이전트를 재시작하고 MCP 목록과 실제 조회를 확인한다. 실행 중인 세션은 새 설정을 반영하지 않는다.

## Graft

스킬이 설명하는 `graft <인수>`는 이 프로젝트에서 `node Tools/Agent/graft.cjs cli <인수>`로 실행한다. 래퍼가 프로젝트 루트와 런타임을 고정하므로 전역 CLI가 없어도 된다.

```powershell
node Tools/Agent/graft.cjs cli map
node Tools/Agent/graft.cjs cli ask '심볼 또는 질문' --source
node Tools/Agent/graft.cjs engine --probe 'UObjectBase GetFName'
```

`graft/`는 이 저장소의 그래프 캐시다. 다른 프로젝트의 그래프를 복사하지 않는다. 그래프를 처음부터 다시 만들 때는 사용자 요청을 받고 `node Tools/Agent/graft.cjs cli build .`를 실행한다. Graft 0.18.0의 프로젝트 MCP는 그래프가 없으면 `tools/list`에 빈 목록을 돌려주고, 그래프를 만든 뒤 MCP를 다시 불러오면 도구가 나타난다.

엔진 인덱스는 `Tools/Graft/build_ue58_graft_indexes.cmd`가 로컬 경로 설정을 읽어 만든다. 큰 작업이므로 요청받은 범위에서만 실행하고, 외부 인덱스를 다른 작업이 쓰는지 먼저 확인한다.

`Tools/Graft/graft-orbis-local.patch`는 Graft 0.18.0의 파서 메모리, 대량 파일 열거, 확장자 필터를 보정한다. 이름과 내부 표식은 출처 확인용으로 보존했다. 실행 경로는 ccl8의 도구 사본과 로컬 런타임뿐이다. 라이선스는 `Tools/Graft/LICENSE`에 있다.

Graft 0.18.0은 캐시에 초기화 기록이 없으면 세션 시작·CLI·MCP 실행에서 전역 설정을 다시 쓸 수 있다. `configure_runtime.cjs`가 이 프로젝트 런타임에 관리 모드 가드를 적용하고, ccl8 진입점은 가드를 확인해 설정 재작성과 백그라운드 자동 갱신을 막는다. 가드가 없으면 진입점은 실행을 멈추므로 런타임을 다시 설치하면 가드도 다시 적용한다.

Claude 훅과 상태 표시는 같은 런타임 탐색기를 쓰고, 패키지가 없으면 원인을 stderr에 남긴 채 작업을 막지 않는다. 사용자 전역에도 같은 Graft 훅이 있으면 두 번 실행될 수 있다. 실제 중복이 확인되면 전역 훅 정리는 사용자와 별도로 진행한다.

## Graphify

사용자 설치의 `uv`, `graphify`, `claude`를 재사용한다.

```powershell
./Tools/Graphify/graphify_ccl8.cmd status
# 새 PC에서 설치와 스킬 복구가 필요할 때만:
./Tools/Graphify/graphify_ccl8.cmd setup
# 요청한 분석에 그래프가 필요할 때:
./Tools/Graphify/graphify_ccl8.cmd build canonical
./Tools/Graphify/graphify_ccl8.cmd query canonical '문서 간 관계 질문'
```

`canonical`은 운영·사양 문서, `research`는 Narrative의 Research·Drafts·Reviews 문서가 대상이다. 결과는 `.graphify-local/`에만 저장한다. `status`가 `missing`이나 `stale`을 보여도 정상이며 자동으로 빌드하거나 유료 호출을 하지 않는다. `setup`은 전역 도구 설치와 프로젝트 스킬 재생성을 포함한 복구 명령이므로 상태 확인에 쓰지 않는다.

## 검사 도구와 훅

`Tools/Agent/`의 검사 도구는 소스를 고치지 않고 결과만 보고한다. 언제 실행해야 하는지는 [AGENTS.md](../AGENTS.md)의 "끝내기 전"과 [Coding](Coding.md)의 "코드 작업 완료 검증"이 정한다.

| 명령 | 하는 일 |
|---|---|
| `node Tools/Agent/skills.cjs check` | 스킬 머리말, 참조 파일, 스킬 간 링크, 설명의 사용 조건, 사용자 호출 표시와 Codex 정책 파일의 일치, 두 스킬 폴더의 일치를 검사한다 |
| `node Tools/Agent/skills.cjs sync [--write]` | `.agents/skills`에서 `.claude/skills`로 복사할 파일을 출력하고 `--write`일 때만 복사한다. 삭제하지 않는다 |
| `node Tools/Agent/cpp-rules.cjs check [--summary] [paths...]` | Coding의 기계 검사 규칙 위반을 보고한다 |
| `node Tools/Agent/cpp-rules.cjs format [--base <rev>]` | `<rev>`(기본 `HEAD`) 이후 바뀐 줄에 `.clang-format`을 적용했을 때의 차이를 검사한다 |
| `node Tools/Agent/floor-guard.cjs [--base <rev>]` | diff의 새 억제 주석, 비활성화된 테스트, 삭제된 스모크 검사, 늘어난 시간 제한, 미구현 스텁을 찾는다. 추적하지 않는 새 파일을 포함하고 문서는 제외한다 |
| `Tools/Agent/guard-commands.cjs` | Claude Code PreToolUse 훅. 파괴적 Git·삭제 명령이면 `ask`로 답해 사용자 확인을 받는다 |

`cpp-rules.cjs`는 clang-format을 `CLANG_FORMAT` 환경 변수, `PATH`, `vswhere`로 찾은 Visual Studio의 LLVM 도구(19.1.5) 순으로 찾는다. 엔진 `Extras/clang-format`에는 실행 파일이 없고 실험용 설정만 있다.

확인 훅은 명령 문자열 전체를 패턴으로 보므로, 따옴표 안의 시험 문자열에 패턴이 들어 있어도 확인을 요청한다. Codex에는 이 훅이 없다.

## 플러그인 복구 목록

플러그인 패키지, 로그인, 설치 캐시는 저장소에 복사하지 않는다. 같은 PC에서는 사용자 설치를 재사용하고 새 PC에서는 각 앱에서 다시 설치·연결한다. 아래는 2026-10-04 사용자 설정에서 읽은 복구용 목록이며 설치·연결 성공 목록이 아니다.

- Codex 활성 ID: `superpowers@openai-curated`, `coderabbit@openai-curated`, `scite@openai-curated`, `biorender@openai-curated`, `github@openai-curated`, `browser@openai-bundled`, `codex-app-tools@openai-bundled`, `visualize@openai-bundled`, `computer-use@openai-bundled`, `code-review@openai-bundled`, `documents@openai-primary-runtime`, `pdf@openai-primary-runtime`, `spreadsheets@openai-primary-runtime`, `presentations@openai-primary-runtime`, `template-creator@openai-primary-runtime`
- Codex 사용자 스킬 `grill-me`, `pdf`와 앱 번들 스킬은 사용자·앱 범위에 둔다.
- Claude 사용자 설정에는 `enabledPlugins`가 없었고 동기화된 로컬 매니페스트에서 `engineering` 1.2.0을 확인했다. 새 PC에서는 Claude 플러그인 화면에서 확인한다.

## Unreal MCP

`CCL.uproject`는 `ModelContextProtocol`, `EditorToolset`, `AutomationTestToolset` 플러그인을 활성화하고, 두 에이전트의 Unreal MCP 주소는 `http://127.0.0.1:8000/mcp`다. 에디터가 실행 중이어야 연결된다. 설정이 있다는 것과 연결에 성공했다는 것은 다른 상태로 기록한다.

## 볼트

### Obsidian 활용 절차

설계, 구현, 기술 조사, 문제 해결에서는 개인 Obsidian 볼트를 기존 지식의 출발점이자 검증한 지식의 저장소로 쓴다. 볼트 정리 요청이나 `/trace` 호출이 없어도 아래 절차를 작업에 포함한다. 문구 수정이나 Git 전달처럼 기술 지식을 다루지 않는 작업은 생략할 수 있다.

1. **시작:** `.local/agent-paths.json`의 `vaultRoot`로 연결을 확인하고 볼트 지침을 읽는다. `Index.md`, 해당 주제의 MOC(노트 색인), 관련 노트 순으로 좁혀 읽고, 이번 작업에 관련된 설명·주의점·미확인 항목을 파악한다.
2. **검증:** 노트의 확인일과 엔진 버전을 본다. 판단에 쓰는 엔진 동작은 `graft_ue58`와 현재 소스, 필요하면 실행으로 확인하고, 프로젝트 코드의 위치와 호출 관계는 `graft`로 확인한다. 노트와 현재 근거가 다르면 차이를 확인해 갱신하고, 검증하지 못한 내용은 미확인으로 남긴다.
3. **기록:** 주제 하나를 마칠 때 다른 프로젝트에서도 쓸 수 있고 다시 알아내는 데 비용이 드는 원리, 호출·수명 흐름, 함정, 버전 차이, 문제 해결 근거를 기존 노트에 반영한다. 맞는 노트가 없으면 주제 단위로 만들어 MOC에 등록하고 관련 노트를 연결한다. 형식과 출처·확인일 표기는 볼트 지침을 따른다. ccl8 고유 결정, 진행, 시험 결과는 이 저장소 `Docs/`에 적는다.
4. **완료 보고:** 참고한 노트와 작성·갱신한 노트를 링크로 밝히고 무엇을 활용하거나 바꿨는지 적는다. 관련 노트나 새로 축적할 지식이 없으면 그렇다고 적는다. 접근 실패나 미확인으로 기록하지 못했으면 이유와 남은 항목을 적는다. 연결 확인만으로 활용·기록을 완료했다고 보고하지 않는다.

볼트 경로가 없거나 접근할 수 없으면 경로를 추측하지 않고 프로젝트 작업을 계속하며, 볼트 활용이 빠진 이유를 보고한다. 쓰기 전에 볼트의 Git 상태와 대상 노트의 변경을 확인하고 기존 사용자 변경을 보존한다. 편집 뒤에는 diff와 노트·MOC 연결을 확인한다. 볼트의 pull, commit, push는 사용자가 요청한 범위에서만 한다.

## 도구 구성 검증

도구를 설치·이전·수정한 뒤 다음을 실행한다. 엔진 전체 빌드는 도구 검증에 포함하지 않는다.

```powershell
node --test Tools/Agent/Tests/runtime.test.cjs
node Tools/Agent/graft.cjs doctor
node Tools/Agent/graft.cjs engine --list-shards
./Tools/Graphify/graphify_ccl8.cmd status
node Tools/Agent/skills.cjs check
```

`Tools/Agent/`의 검사 도구를 고쳤으면 `node --test Tools/Agent/Tests/*.test.cjs`로 전체 시험을 실행한다. 이 Node 24 환경에서는 폴더 경로를 넘기면 시험을 찾지 못한다.
