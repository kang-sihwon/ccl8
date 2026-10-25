# 에이전트 작업 체계 재구성

규칙 문서를 백업하고 새로 쓴 뒤 게임 문서를 새 형식으로 옮기는 작업의 지도다. 결정은 [결정 28](../DesignLog.md), 채택·제외 근거는 [스킬 묶음 통합 검토](../AgentToolingResearch.md#스킬-묶음-통합-검토-2026-10-25)가 소유한다. 이 작업이 끝나기 전까지 기존 규칙 문서가 유효하다.

## 목적지

다음이 모두 참이면 끝난다.

- 태그 `agent-setup-v1`이 옛 규칙 문서를 가리킨다.
- 새 진입점(`AGENTS.md`, `CLAUDE.md`)과 규칙 문서, 스킬이 통합 구조대로 있고 스킬 검사가 통과한다.
- 결정 1-28이 `Docs/decisions/`의 파일로 있고, `Docs/Decisions.md`가 생성 스크립트의 출력과 같으며, 모든 `결정 N` 링크가 해석된다.
- 재작성한 게임 문서마다 보존 검사가 통과했다.
- QA 보고서는 `Docs/QA/`에 있고, 날짜 붙은 검토 보고서는 결과를 옮긴 뒤 삭제됐다.

## 진행

각 조각의 괄호는 먼저 끝나야 하는 조각이다.

- [x] 1. 백업 (없음). 태그 `agent-setup-v1`과 같은 이름의 브랜치를 만든다. 완료 조건은 `git show agent-setup-v1:Docs/Guide.md`가 현재 문서를 출력하는 것이다. 원격 push는 실행 전에 사용자에게 확인한다.
  - 결과: 둘 다 `1d125d5`를 가리키고 완료 조건을 확인했다. 이름이 같아 Git이 `refname is ambiguous` 경고를 내므로 명령에는 `refs/tags/agent-setup-v1`이나 `refs/heads/agent-setup-v1`을 쓴다. 원격 push는 하지 않았다.
- [x] 2. 스킬 검사와 동기화 (1). 머리말, 참조 파일, 스킬 간 링크, 설명의 사용 조건, 두 스킬 폴더의 일치를 검사하는 스크립트를 만든다. 완료 조건은 현재 스킬에서 알려진 불일치(`graft`, `graphify`가 `.agents/skills`에 없음)를 검사가 찾아내는 것이다.
  - 결과: `node Tools/Agent/skills.cjs check`. 정본은 `.agents/skills`이고 `.claude/skills`를 비교한다. `sync`는 복사할 파일만 출력하고 `--write`일 때만 복사하며 삭제하지 않는다. 현재 오류 4건: `graft`, `graphify`가 `.agents/skills`에 없음, `graft` 설명에 사용 조건이 없음, `humanizer/references/structure-patterns.md:180`의 상대 경로가 한 단계 깊음. 경고는 `graphify` 본문 756줄이다. 수정은 조각 8에서 한다.
- [x] 3. C++ 규칙 검사 (1). clang-format 설정과 헤더 배치 검사 스크립트를 만들고 현재 소스의 위반 목록만 출력한다. 소스는 고치지 않는다. 완료 조건은 일부러 어긴 예제 파일에서 검사가 실패하고 고친 예제에서 통과하는 것이다.
  - 결과: `node Tools/Agent/cpp-rules.cjs check [--summary]`가 Coding의 기계 규칙 16개를 전체 소스에 적용한다. 2026-10-25 기준 60개 파일에서 183건이다(한 줄 블록 110, 연속 빈 줄 27, `GENERATED_BODY()` 뒤 빈 줄 26, 닫는 중괄호 앞 빈 줄 15, 중괄호 없는 본문 4, 헤더 `bool` 멤버 1). `.clang-format`은 바뀐 줄에만 `cpp-rules.cjs format [--base <rev>]`로 적용한다. 기존 파일은 namespace 들여쓰기와 이어진 줄 정렬이 섞여 있어 파일 전체에 적용하면 규칙과 무관한 차이가 7,800건을 넘기 때문이다. 영역 순서와 선언 묶음 분류는 검사하지 않고 리뷰에 남긴다.
- [x] 4. 안전 장치 훅 (1). 파괴적 Git·삭제 명령을 실행 전에 막는 PreToolUse 훅을 추가한다. 완료 조건은 시험 입력 `git reset --hard`가 막히고 `git status`가 통과하는 것이다.
  - 결과: `Tools/Agent/guard-commands.cjs`를 `.claude/settings.json`의 Bash·PowerShell PreToolUse에 등록했다. 차단 대신 `ask`로 답해 사용자 확인 뒤에만 실행되게 했다. 공통 지침의 "삭제·덮어쓰기 전 확인"과 같은 동작이다. 완료 조건은 훅 프로세스 시험으로 확인했다. Codex에는 적용되지 않는다.
  - 실제 세션 확인(2026-10-25, Claude 데스크톱 앱, auto 모드): `git tag -d no-such-tag`를 실행하려 하자 세션 기록에 `PreToolUse:Bash` 훅의 `ask` 응답이 남았고, 사용자 승인 뒤에 실행돼 `tag 'no-such-tag' not found`로 끝났다. 훅은 명령 문자열 전체를 보므로 시험용 JSON처럼 문자열 안에 패턴만 들어 있어도 확인을 요청한다. 안전한 쪽의 오탐이라 그대로 둔다.
- [x] 5. 바닥선 검사 (1). diff에서 새 억제 주석, 비활성화된 자동화 테스트, 스모크 검사 삭제·시간 제한 증가, `checkNoEntry`·TODO 스텁을 찾는다. 완료 조건은 각 항목을 하나씩 넣은 시험 diff가 모두 걸리는 것이다.
  - 결과: `node Tools/Agent/floor-guard.cjs [--base <rev>]`. 추적하지 않는 새 파일도 포함하고 문서는 제외한다. 종료 코드는 0 통과, 1 발견, 2 실행 불가다. 다른 곳으로 옮긴 테스트·검사와 줄인 시간 제한은 발견으로 치지 않는다. 첫 실행에서 이 작업의 clang-format 시험에 들어 있던 조건부 `t.skip`을 찾아내 도구가 없으면 실패하도록 고쳤다.
- [x] 6. 진입점 (2). `AGENTS.md`를 정본으로 새로 쓰고 `CLAUDE.md`는 `@AGENTS.md`로 둔다. `Docs/README.md` 문서 지도와 `GLOSSARY.md` 초안을 만든다.
  - 결과: 매 턴 읽는 진입점이 `Docs/Guide.md` 13.7KB에서 `AGENTS.md` 약 4.1KB로 줄었다. 스킬 설명이 이미 맡는 스킬 라우팅 행은 뺐다. 문서 지도는 Guide의 83행을 문구 그대로 묶음별로 옮겼다. `GLOSSARY.md`에는 시스템·제작 용어 15개를 두고 서사 용어는 `Docs/Narrative/Glossary.md`에 남겼다. 규칙 문서, `README.md`, `ccl8-docs`·`ccl8-design` 스킬의 Guide 참조를 새 위치로 바꿨다. Guide에는 이행 중이라는 안내를 붙였다.
  - 진입점 확인(2026-10-25, 새 세션): 시작 맥락에 `CLAUDE.md`(`@AGENTS.md`)와 `AGENTS.md` 본문만 들어왔고 `Docs/Guide.md`는 들어오지 않았다. 저장소와 사용자 전역 `CLAUDE.md`에 Guide를 `@`로 가져오는 줄이 없다. `AGENTS.md`의 Guide 언급은 코드 표기라 가져오기가 아니다.
- [x] 7. 규칙 문서 (3, 6). `Coding.md`(검사기로 옮긴 규칙 제외), `WritingGuide.md`, `VersionControl.md`, `AgentContextTools.md`, `Mentoring.md`, `SystemExplanationGuide.md`를 새로 쓰고 옛 `Docs/Guide.md`를 없앤다.
  - 결과: Coding은 검사기의 16개 규칙을 규칙 이름·근거 결정 표 하나로 줄이고, 리뷰로 남는 영역 순서·선언 묶음·단계 구분만 문장으로 뒀다. 완료 검증에 `cpp-rules.cjs format`과 `floor-guard.cjs`, `ccl8-verify`를 넣었다. 다른 문서가 링크하는 앵커 7개(Coding 5, AgentContextTools 1, VersionControl 1)는 제목을 유지했고, 스킬이 인용하는 WritingGuide의 절 이름과 단계 수도 유지했다.
  - 고친 낡은 내용: Mentoring의 "어느 모드에서든 코드는 사용자가 쓴다"(결정 2와 충돌, 훈련 과제에서만 사용자 구현으로 바꿈), VersionControl의 "원격 연결과 push를 하지 않는다"와 최초 구성 문장(지금은 `origin`이 있음), AgentContextTools의 "프로젝트 그래프와 Graphify 그래프가 없다"와 손으로 맞추던 스킬 목록(`skills.cjs`로 대체). AgentContextTools에 검사 도구와 확인 훅 절을 더해 크기가 12.8KB에서 13.5KB로 늘었고, 나머지 다섯 문서는 줄었다.
  - Guide는 사용자 확인 뒤 `git rm`으로 지웠다. 링크는 `Docs/Learning/README.md`, `Docs/Learning/EnvironmentEvidence.md`를 문서 지도로, 지도에 빠져 있던 `ccl8-narrative` 스킬의 읽기 목록을 `AGENTS.md`로 바꿨다. DesignLog, AgentToolingResearch, `FrameworkPlan.md:477`, 훅 시험 문자열의 Guide 언급은 과거 기록이거나 시험 입력이라 두었다.
  - 검증: 바꾼 문서와 링크하는 문서 15개의 상대 링크·앵커가 모두 해석된다. `skills.cjs check`는 조각 2에서 기록한 오류 4건만 내고 새 오류는 없다.
- [x] 8. 스킬 재구성 (2, 6, 7). 통합 구조의 모델 호출·사용자 호출 목록대로 스킬을 쓰고 기존 스킬을 교체한다. QA 보고서에서 절차로 쓰이던 실행 조건 표를 `ccl8-verify`로 옮긴다. 완료 조건은 스킬 검사 통과다.
  - 결과: `node Tools/Agent/skills.cjs check`가 오류 0, 경고 0으로 통과한다. 모델 호출 스킬은 10개(`graft`, `ccl8-design`, `ccl8-verify`, `ccl8-debug`, `ccl8-review`, `ccl8-docs`, `ccl8-writing`, `ccl8-narrative`, `grilling`, `writing-for-agents`)이고 매 턴 불러오는 설명은 15개 5,214바이트에서 3,260바이트로 줄었다. 사용자 호출 스킬은 10개(`ccl8-help`, `ccl8-work`, `ccl8-handoff`, `ccl8-decision-log`, `ccl8-retro`, `ccl8-teach`, `ccl8-benchmark`, `ccl8-blast-radius`, `roadmap-review`, `graphify`)다.
  - 문장 스킬 다섯 개와 `obsidian-markdown`은 사용자 확인 뒤 독립 스킬에서 빼고 `ccl8-writing/references/`, `ccl8-docs/references/`로 옮겼다. 원래 `SKILL.md`는 `GUIDE.md`로 바꾸고 머리말만 뺐으며, 서로를 가리키는 경로 다섯 곳만 새 위치로 고쳤다. MIT 라이선스 파일은 폴더마다 그대로 있다.
  - 사용자 호출은 Claude의 `disable-model-invocation: true`와 Codex의 `agents/openai.yaml`(`allow_implicit_invocation: false`)로 표시하고, `skills.cjs`가 둘이 맞는지 검사한다. Codex 형식은 사용자 Codex 설치의 플러그인 스킬 예시에서 확인했다. 이 세션에서 새 스킬을 만든 뒤 Claude가 보여 준 스킬 목록에는 모델 호출 스킬만 나타났다. Codex에서 숨겨지는지는 실행해 보지 않았다.
  - `graft`는 정본 폴더로 옮기고 설명에 사용 조건을 넣었다. `graphify`는 setup이 다시 쓰는 도구 관리 스킬이라 `skills.cjs`의 `TOOL_MANAGED`로 동기화·길이 검사에서 빼고, `graphify_ccl8.ps1`의 `Set-GraphifySkillScope`가 사용자 호출 표시와 Codex 정책 파일을 쓰게 했다. 그 함수만 떼어 두 번 실행해 두 사본에 적용되고 두 번째 실행은 아무것도 바꾸지 않음을 확인했다.
  - MCP PIE 절차는 `ccl8-verify/references/mcp-pie.md`로 옮겼다. 기능 파일 두 곳의 판정 기준 링크도 그쪽으로 바꿨다. `features/combat.md`의 F-03 언급은 과거 결함 기록이라 QA 보고서 경로로 남겼고, 조각 11에서 경로를 바꾼다.
  - 시험: `node --test Tools/Agent/Tests/*.test.cjs` 21개 통과(사용자 호출 일치, 도구 관리 스킬 시험 2개 추가). `floor-guard.cjs` 발견 0.
- [ ] 9. 결정 파일 전환 (7). 변환 스크립트로 결정 1-28을 옮기고 Decisions 생성기를 만든 뒤 링크를 바꾼다. 본문은 원문 그대로다. 완료 조건은 목적지의 결정 항목이다.
- [ ] 10. 보존 검사 스크립트 (9). 옛 문서의 수치, `결정 N`, 경로, 링크가 새 문서에 모두 있는지 비교하고 빠진 항목을 출력한다.
- [ ] 11. QA 보고서 이동 (8). QA 보고서를 내용 변경 없이 `Docs/QA/`로 옮기고 폴더 README에 시점 기록임을 적는다.
- [ ] 12. 게임 문서 재작성 (10, 11). 문서나 소유 묶음 하나씩 새로 쓰고 보존 검사를 통과시킨 뒤 커밋한다. `Docs/Learning/`은 제외한다.
- [ ] 13. 서사 정본 (10). `World.md`, `Scenario.md`의 확정 영역은 글자를 바꾸지 않고 구조만 바꾼다. 독립 검토의 수정 제안 목록을 사용자에게 제출한다.
- [ ] 14. 날짜 붙은 검토 보고서 정리 (12). 결과를 소유 사양서로 옮긴다. 삭제는 대상 목록을 사용자에게 확인받은 뒤 한다.
- [ ] 15. 계획 문서 정리 (12). StudyRoadmap은 방향과 일정, DeliveryPlan은 제출 절차만 남기고 다음 작업은 `Docs/Work/`로 옮긴다.

## 정한 것

- [결정 28](../DesignLog.md): 백업 방식, 재작성 범위와 순서, 결정 파일 전환, 읽기 전용 위임, 작업 지도, QA·학습·서사 문서의 처리.
- 두 스킬 폴더는 복사로 맞춘다. 정본은 `.agents/skills`다. Git은 Windows 디렉터리 연결을 추적하지 않고, 심볼릭 링크는 개발자 모드와 `core.symlinks` 설정이 있어야 해서 다른 환경에서 깨진다.
- clang-format은 Visual Studio 2022의 LLVM 도구(19.1.5)를 `vswhere`로 찾는다. 다른 실행 파일은 `CLANG_FORMAT` 환경 변수로 지정한다. 엔진 `Extras/clang-format`에는 실행 파일이 없고 실험용 설정만 있다.
- 검사 도구의 시험은 `node --test Tools/Agent/Tests/*.test.cjs`로 실행한다. 이 Node 24 환경에서는 폴더 경로를 넘기면 시험을 찾지 못한다.

## 아직 모르는 것

- 날짜 붙은 문서 여섯 개 중 어느 것이 QA 보고서인지. 후보는 `EnvironmentPIEQA-2026-10-12.md`, `FullGameplayMCPQA-2026-10-12.md`, `GameplayQAFixes-2026-10-12.md`다. `AgentReview-2026-10-08.md`, `UIReview-2026-10-08.md`, `CodeDesignFixes-2026-10-11.md`는 검토 보고서 후보다. 조각 11 전에 사용자와 정한다.
- 결정 파일 이름의 slug를 영어로 쓸지 한국어로 쓸지.
- 개인 볼트에 재사용 지식을 반영할 범위. 이번 기록 때 Obsidian MCP가 연결되지 않아 볼트를 참고하지 못했다.

## 범위 밖

- 새 설정으로 소스 코드를 바꾸는 작업. 이 지도가 끝난 뒤 별도 작업으로 시작한다.
- `Docs/Learning/` 재작성과 QA 재검증. 소스 변경 뒤에 한다.
- GitHub Issues와 CI 도입.
