# 에이전트 도구 설치·복구

에이전트 도구의 위치, 설치, 사용 명령, 복구, 진단을 소유한다. 어떤 일에 어떤 도구를 쓰는지는 [AGENTS.md](../AGENTS.md)의 길잡이가 정한다. 도구를 실행할 수 있다고 해서 코드·설정을 고칠 권한이 생기지는 않는다. 편집 권한은 [Coding](Coding.md)이 정한다.

도구 후보의 조사와 채택·제외 근거는 [AgentToolingResearch](AgentToolingResearch.md)가 소유한다.

## 하위 문서

| 문서 | 읽는 때 |
|---|---|
| [기기 준비와 복구](AgentContextTools/Setup.md) | 새 기기를 설정하거나 플러그인, Unreal MCP, 볼트를 복구할 때 |
| [색인 도구](AgentContextTools/Indexes.md) | Graft와 Graphify를 설치, 재구성, 진단할 때 |

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

에이전트가 스스로 부르지 않는 사용자 호출 스킬은 `SKILL.md` 머리말에 `disable-model-invocation: true`(Claude)를 두고, 같은 폴더의 `agents/openai.yaml`에 `allow_implicit_invocation: false`(Codex)를 둔다. 목록은 `ccl8-help` 스킬에 있다. `graphify`는 `graphify_ccl8.cmd setup`이 `.claude/skills/`와 `.codex/skills/`에 설치하고 다시 쓰는 도구 관리 스킬이라 정본 폴더 밖에 둔다. Claude에서는 사용자 호출 스킬이 모델의 스킬 목록에 나타나지 않는 것을 2026-10-25에 확인했다. Codex에서 숨겨지는지는 실행해 보지 않았다.

스킬 정본은 `.agents/skills`이고 `.claude/skills`는 `skills.cjs sync`로 복사해 맞춘다. 링크로 공유하지 않는 이유는 Git이 Windows 디렉터리 연결을 추적하지 않고, 심볼릭 링크는 개발자 모드와 `core.symlinks` 설정이 있어야 해서 다른 환경에서 깨지기 때문이다.

저장소 루트는 `CCL.uproject`와 `.git`이 함께 있는 폴더다. 위치와 폴더명은 기기마다 달라도 되고, `ccl8-*` 스킬과 `graphify_ccl8.cmd`의 이름은 폴더명과 무관하다. 공유 도구는 실행 파일 위치로 프로젝트 경로를 계산한다. 엔진 경로는 `Engine/Build/Build.version`이 있는 `Engine` 폴더다. 엔진 인덱스는 기존 외부 디렉터리를 참조하고 자동 생성하지 않는다.

폴더를 옮겼으면 `.codex/config.toml`의 두 Graft 서버에서 `args`와 `cwd`를 새 저장소 루트로 고치고 에이전트를 재시작한다. Unreal MCP 항목은 그대로 둔다. 실제 경로의 제출 기준은 [VersionControl의 기기별 경로와 로컬 설정](VersionControl.md#기기별-경로와-로컬-설정)을 따른다.

## 검사 도구와 훅

`Tools/Agent/`의 검사 도구는 소스를 고치지 않고 결과만 보고한다. 언제 실행해야 하는지는 [AGENTS.md](../AGENTS.md)의 "끝내기 전"과 [Coding](Coding.md)의 "코드 작업 완료 검증"이 정한다.

| 명령 | 하는 일 |
|---|---|
| `node Tools/Agent/skills.cjs check` | 스킬 머리말, 참조 파일, 스킬 간 링크, 설명의 사용 조건, 사용자 호출 표시와 Codex 정책 파일의 일치, 두 스킬 폴더의 일치를 검사한다 |
| `node Tools/Agent/skills.cjs sync [--write]` | `.agents/skills`에서 `.claude/skills`로 복사할 파일을 출력하고 `--write`일 때만 복사한다. 삭제하지 않는다 |
| `node Tools/Agent/decisions.cjs check` | 결정 파일의 머리말·번호·관계, `Docs/Decisions.md`와 생성 결과의 일치, 저장소의 모든 `결정 N` 링크를 검사한다 |
| `node Tools/Agent/decisions.cjs generate [--write]` | 결정 파일의 머리말로 `Docs/Decisions.md`를 만든다. `--write`가 없으면 출력만 한다 |
| `node Tools/Agent/decisions.cjs next` | 새 결정의 번호와 파일 이름 형식을 출력한다 |
| `node Tools/Agent/docs.cjs check [--summary]` | `Docs/`, 루트 문서, 스킬의 상대 링크와 앵커를 검사하고, `Docs/`의 문서가 문서 지도에서 링크로 도달되는지 본다. 학습 문서의 알려진 깨진 링크는 코드의 예외 목록으로 넘기고, 고쳐진 항목이 목록에 남으면 보고한다 |
| `node Tools/Agent/preserve.cjs --old <path>... --new <path>... [--base <rev>] [--rename <old>=<new>]... [--allow <file>]` | `<rev>`(기본 `HEAD`)의 옛 문서에 있던 수치, `결정 N`, 코드 경로, 링크 대상 가운데 작업 트리의 새 문서 묶음에 없는 것을 처음 나온 위치와 함께 출력한다. 링크는 문서 위치를 기준으로 풀어 비교하므로 옮긴 문서도 비교할 수 있고, 옮긴 대상은 `--rename`으로 맞춘다. 허용 파일의 줄은 `<category> <token>  # 이유`다. 종료 코드는 0 보존, 1 빠짐, 2 실행 불가다 |
| `node Tools/Agent/cpp-rules.cjs check [--summary] [paths...]` | Coding의 기계 검사 규칙 위반을 보고한다 |
| `node Tools/Agent/cpp-rules.cjs format [--base <rev>]` | `<rev>`(기본 `HEAD`) 이후 바뀐 줄에 `.clang-format`을 적용했을 때의 차이를 검사한다 |
| `node Tools/Agent/floor-guard.cjs [--base <rev>]` | diff의 새 억제 주석, 비활성화된 테스트, 삭제된 스모크 검사, 늘어난 시간 제한, 미구현 스텁을 찾는다. 추적하지 않는 새 파일을 포함하고 문서는 제외한다 |
| `Tools/Agent/guard-commands.cjs` | Claude Code PreToolUse 훅. 파괴적 Git·삭제 명령이면 `ask`로 답해 사용자 확인을 받는다 |

`cpp-rules.cjs`는 clang-format을 `CLANG_FORMAT` 환경 변수, `PATH`, `vswhere`로 찾은 Visual Studio의 LLVM 도구(19.1.5) 순으로 찾는다. 엔진 `Extras/clang-format`에는 실행 파일이 없고 실험용 설정만 있다.

확인 훅은 `.claude/settings.json`의 Bash·PowerShell PreToolUse에 등록돼 있다. 2026-10-25 Claude 데스크톱 앱의 실제 세션에서 `git tag -d no-such-tag`에 `ask` 응답이 나오고 사용자 승인 뒤에만 실행되는 것을 확인했다. 훅은 명령 문자열 전체를 패턴으로 보므로, 따옴표 안의 시험 문자열에 패턴이 들어 있어도 확인을 요청한다. 안전한 쪽의 오탐이라 그대로 둔다. Codex에는 이 훅이 없다.

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
