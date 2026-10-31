# 기기 준비와 복구

새 기기 준비, 플러그인 복구, Unreal MCP, 볼트 연결을 소유한다. 상위 문서는 [AgentContextTools](../AgentContextTools.md)다.

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
