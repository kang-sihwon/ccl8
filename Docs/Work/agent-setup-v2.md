# 에이전트 작업 체계 재구성

규칙 문서를 백업하고 새로 쓴 뒤 게임 문서를 새 형식으로 옮기는 작업의 지도다. 결정은 [결정 28](../decisions/0028-agent-workflow-restructure.md), 채택·제외 근거는 [스킬 묶음 통합 검토](../AgentToolingResearch.md#스킬-묶음-통합-검토-2026-10-25)가 소유한다. 이 작업이 끝나기 전까지 기존 규칙 문서가 유효하다.

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
- [x] 9. 결정 파일 전환 (7). 변환 스크립트로 결정 1-28을 옮기고 Decisions 생성기를 만든 뒤 링크를 바꾼다. 본문은 원문 그대로다. 완료 조건은 목적지의 결정 항목이다.
  - 결과: `Docs/decisions/0001-production-scope.md`부터 `0028-agent-workflow-restructure.md`까지 28개 파일이 있다. 본문은 DesignLog의 각 절과 대조해 28건 모두 같다. 차이는 제목이 `##`에서 `#`이 된 것과 상대 링크 앞에 `../`가 붙은 것뿐이다. `node Tools/Agent/decisions.cjs check`가 "28 decision(s), 0 error(s)"를 낸다. 이 검사는 머리말, 번호 순서, 관계 대상, `Docs/Decisions.md`와 생성 결과의 일치, 저장소의 모든 `결정 N` 링크, 없어진 DesignLog로 가는 링크를 본다.
  - 링크: 일회성 변환기로 34개 문서의 DesignLog 링크를 바꿨다. `[결정 N]`은 해당 결정 파일로, `[결정 7과 결정 13]`은 링크 두 개로 나눴고, 문서 이름 링크 `[DesignLog]`는 표기를 둔 채 `Decisions.md`로 바꿨다. 변환기는 저장소에 두지 않았다. DesignLog는 사용자 확인 뒤 `git rm`으로 지웠다.
  - 규칙: WritingGuide의 결정 기록 절차를 결정 파일과 생성기 기준으로 다시 썼고, em dash 예외를 결정 파일 제목으로 옮겼다. `AGENTS.md`의 "끝내기 전"에 `decisions.cjs check`를 넣었다. 문서 지도, 용어집, SystemExplanationGuide, AgentContextTools와 `ccl8-docs`, `ccl8-writing`, `ccl8-decision-log`, `ccl8-work`, `ccl8-narrative` 스킬의 DesignLog 참조도 바꿨다. `AgentToolingResearch.md`의 DesignLog 언급은 과거 상태를 서술한 기록이라 두었다.
  - 시험: `node --test Tools/Agent/Tests/*.test.cjs` 24개 통과(`decisions.test.cjs` 3개 추가). `skills.cjs check` 오류 0, `floor-guard.cjs` 발견 0.
  - 바꾼 문서의 링크를 검사하다 이번 변경 전부터 깨져 있던 링크 5개를 찾았다. `Docs/Learning/InventoryUI.md`의 `Source/CCL/UI/Core/` 경로 4개는 모듈 분리로 옮겨진 파일이고, `Docs/Learning/EnvironmentFollowupEvidence.md`의 `Packaging.md#2026-10-11-이후-변경의-패키지-미검증` 앵커는 지금 없다. 학습 문서는 범위 밖이라 고치지 않았다.
- [x] 10. 보존 검사 스크립트 (9). 옛 문서의 수치, `결정 N`, 경로, 링크가 새 문서에 모두 있는지 비교하고 빠진 항목을 출력한다.
  - 결과: `node Tools/Agent/preserve.cjs --old <path>... --new <path>... [--base <rev>] [--rename <old>=<new>]... [--allow <file>]`. 옛 문서는 `<rev>`(기본 `HEAD`)에서, 새 문서는 작업 트리에서 읽는다. 여러 문서를 합치거나 나눈 경우도 묶음끼리 비교한다. 종료 코드는 0 보존, 1 빠짐, 2 실행 불가다. 사용법은 AgentContextTools의 검사 도구 표에, 재작성 때 통과해야 한다는 규칙은 `ccl8-docs`의 작성·개정 모드에 넣었다.
  - 비교 규칙: 수치는 날짜, `1280x720`, 지수, `%`와 영문 단위를 포함한 문자열 그대로, `결정 8·10·11` 같은 묶음 표기는 결정 하나씩, 공백 없는 경로형 코드 표기는 문자열로, 링크는 문서 위치로 푼 저장소 경로와 앵커로 비교한다. 옛 링크에 앵커가 없으면 같은 파일로 가는 아무 링크나, 경로는 그 파일로 가는 링크나 그 경로로 끝나는 더 긴 경로(`helpers/` 대신 `.claude/helpers/`)를, 링크 대상이 새 묶음 안의 문서이면 링크가 없어도 보존으로 친다. 코드 블록 안에서는 수치만 본다.
  - 실제 문서 확인: DesignLog와 옛 Decisions(`bcf4dfd`)를 새 Decisions와 결정 파일 28개에 비교하면 `--rename Docs/DesignLog.md=Docs/Decisions.md`로 빠짐 0이다. 처음 실행에서 DesignLog 머리말의 WritingGuide 링크가 빠진 것을 찾아 생성되는 Decisions.md 머리말에 되살렸다. 조각 7의 규칙 문서 6개를 재작성 전 `e33868e`와 비교하면 VersionControl·SystemExplanationGuide는 0이다. 나머지 8건은 모두 의도한 변경이다: Coding의 `Engine/Build/Build.version`(`<Engine>/` 자리표시자로 씀)·`Source/CCL/`(AGENTS.md가 소유)·DesignLog 링크, WritingGuide의 `Docs/DesignLog.md`, AgentContextTools의 `5.8`·`2026-10-05`·`5.8.1`(조사 기록은 AgentToolingResearch가 소유), Mentoring의 `AGENTS.md`(결정 2 링크로 바꿈).
  - 시험: `node --test Tools/Agent/Tests/*.test.cjs` 29개 통과(`preserve.test.cjs` 5개 추가). 시험을 쓰다가 `120ms`처럼 영문 단위가 붙은 수치를 놓치는 결함을 찾아 고쳤다.
- [x] 11. QA 보고서 이동 (8). QA 보고서를 내용 변경 없이 `Docs/QA/`로 옮기고 폴더 README에 시점 기록임을 적는다.
  - 결과: `EnvironmentPIEQA-2026-10-12.md`, `FullGameplayMCPQA-2026-10-12.md`, `GameplayQAFixes-2026-10-12.md`를 `git mv`로 `Docs/QA/`에 옮겼다. 본문은 상대 링크의 대상만 새 위치에 맞췄다. 세 보고서에서 바뀐 줄은 7·8·3줄씩 18줄이고, 링크 대상을 가리고 비교하면 지운 줄과 넣은 줄이 모두 짝을 이룬다. 보존 검사는 세 보고서와 문서 지도 모두 빠짐 0이다(`--rename`으로 세 경로 대응).
  - `Docs/QA/README.md`에 시점 기록이라는 점, 내용을 고치지 않고 정정은 새 보고서로 쓴다는 규칙, 이름 형식과 목록을 적었다. 문서 지도의 "날짜 붙은 QA·검토 보고서" 절은 "QA 보고서"와 "날짜 붙은 검토 보고서"로 나눴다. 들어오는 링크는 `EnvironmentPlan.md`, `ContentFoundation.md`, 문서 지도, `ccl8-verify`의 `references/mcp-pie.md`를 고쳤고, `features/combat.md`의 경로 표기도 `Docs/QA/`로 바꿨다.
  - 일회성 링크 변환기가 디렉터리 링크 끝의 `/`를 지우는 결함이 있어, 관계없는 문서 다섯 곳(사본 포함 8곳)의 `examples/`, `decisions/` 링크가 잠시 바뀌었다. 커밋 전에 되돌렸고 최종 diff에는 없다.
- [x] 12. 게임 문서 재작성 (10, 11). 문서나 소유 묶음 하나씩 새로 쓰고 보존 검사를 통과시킨 뒤 커밋한다. `Docs/Learning/`은 제외한다.
  - 문서마다 하는 일: (1) WritingGuide 표로 종류를 정하고, 첫 문단에 결론과 이 문서가 소유하는 것을 쓴다. (2) 확정 내용에는 근거 결정 링크를 붙이고, 제안은 "(제안)"으로, 미기록·미확인은 "미확인" 절로 모은다. (3) 다른 문서가 소유하는 내용은 요약과 링크만 남긴다. (4) 다른 문서가 링크하는 제목은 유지하거나 링크를 함께 고친다. (5) `preserve.cjs --old <옛> --new <새>`가 통과하고, 일부러 뺀 항목은 허용 파일에 넣어 커밋 본문에 이유를 적는다. (6) `decisions.cjs check`와 상대 링크 검사가 통과한다. (7) 문서나 소유 묶음 하나를 커밋 하나로 남긴다.
  - 묶음 순서: 방향(Direction, GameDesign, ArtDirection, Setup), 시스템 사양(MultiplayerFoundation, CombatFoundation, CampaignFoundation, ContentFoundation, ProgressionFoundation, SessionFoundation, UIFoundation, AgentFoundation, Packaging), 코드 구조(FrameworkPlan, CodeStructurePlan), 자연 환경(EnvironmentPlan과 하위 문서 8개), 도구(AgentToolingResearch), 현황(LearningLog). `NarrativeConcept`는 서사 문서이므로 조각 13에서 다룬다.
  - 진행: 방향 묶음 완료. 네 문서 모두 보존 검사 빠짐 0이다.
    - Direction: 확정 범위 표에 근거 결정을 붙이고, 결정 19 링크를 넣고, 제안과 미확인을 각각 절로 모았다. 2026-10-26 사용자가 이 형식을 승인했다.
    - GameDesign: 결정 3·5·8을 "확정한 것" 표로 모으고 나머지 절을 제안으로 표시했다. "남은 사양"은 미정 목록으로 바꾸고, 그 뒤 프로토타입 수치와 성장·저장 구현을 맡은 CombatFoundation, ProgressionFoundation, SessionFoundation 링크를 붙였다.
    - ArtDirection: 확정은 결정 1의 스타일 공유뿐임을 첫 문단에 적고, 시각 기준과 제작 방식을 제안으로, 착수하지 않은 시안·미정 디자인·미측정 성능 목표를 미정 목록으로 모았다.
    - Setup: 2026-10-04 초기 구성의 이력 문서라 WritingGuide의 이력 규칙대로 본문을 두고, 시점 기록이라는 안내와 현재 소유 문서(AgentContextTools, VersionControl) 링크만 붙였다.
  - 진행: 시스템 사양 묶음 9개 완료. 공통으로 날짜 붙은 실행 기록은 "검증 상태" 아래 날짜별 절로 모으고, 현재 검증 방법은 별도 절로 나누고, 스모크 실행은 `ccl8-verify`를 가리키게 했다.
    - MultiplayerFoundation: 코드와 어긋난 문장 세 개를 고쳤다. 기본 시작 맵은 지금 FrontEnd(게임)와 Campaign(에디터·서버)이고, `ACCLPlayerState`가 ASC와 게임 컴포넌트를 소유하며(결정 11), 체력은 CombatFoundation이 소유한다. 빠짐 0.
    - CampaignFoundation: 보스 체력 240이 ContentFoundation의 적 정의 에셋에 있다고 적었다. "단계 3의 종료 입력을 단계 6에서 교체" 문장은 전달 이력이라 빼고 허용 목록에 넣었다.
    - CombatFoundation: 아이템 설명을 코드에 맞췄다. `UCCLItemDefinition`은 `UKSIItemDefinition` 파생형이고 `FInstancedStruct` Fragment를 쓰며(결정 15), 전투 데이터는 `FCCLItemFragment_Weapon`에 있고 `UCCLItemFragment_Combat`은 옛 에셋 호환용이다. "맵 생성 완료를 뜻하지 않는다" 문장을 지웠다. 빠짐 0.
    - ContentFoundation: 기능 계약을 대화·상점과 퀘스트·적으로 나눴다. `5.9`가 `UE 5.9.0` 제목으로 합쳐진 것만 허용 목록에 넣었다.
    - AgentFoundation: 결정 18·26 링크와 검증 날짜(2026-10-08)를 붙였다. "손님의 재접속 신원은 지원하지 않는다"는 2026-10-08 상태로 적고, 2026-10-11에 서버 발급 신원과 세계 플레이어 프로필이 추가됐다는 사실과 SessionFoundation 링크를 붙였다(결정 21). 빠짐 0.
    - ProgressionFoundation: 현재 계약(아이템 정의, 장비와 태그 설정, 화면과 입력, 콘텐츠 초안, 일관성)과 2026-10-07 기록을 나눴다. Fragment 루트를 `FKSIItemFragmentData`로 고치고, "단계 5 보상"·"단계 6 저장"은 ContentFoundation·SessionFoundation 링크로 바꿨다. 입력 키와 가방 16칸은 코드로 확인했다. "단계 6" 미래형 문장의 `6`만 허용 목록에 넣었다.
    - UIFoundation: 계약 절(경계, 수명, 공개 API, 입력, 표시 제어, 배치, 기능 연결, 검증 방법)과 2026-10-07-08 기록을 나눴다. 기록은 기존 "이전과 검증" 제목 아래 주제별 절이다. 빠짐 0.
    - Packaging: 첫 문장이 최신으로 가리킨 전투 모듈 패키지 뒤에 세계 모듈 패키지가 있어 둘 다 가리키게 고쳤다. 패키지 구성, `package_windows.ps1` 옵션(기본 출력 `Saved/Packages/` 포함), 실행 방법을 앞에 모으고 기록은 날짜순으로 "패키지 기록" 아래에 뒀다. 다른 문서가 링크하는 제목 6개를 유지했다. 빠짐 0.
    - SessionFoundation: 다른 문서가 링크하는 절이 8개이고 뒤 절이 앞 절의 계약을 바꾸는 날짜순 기록이라 절 순서를 두었다. 첫머리에 "현재 계약" 표를 넣어 주제마다 지금 계약과 정한 절을 가리키게 하고, 낡은 문장 세 곳에 바뀐 절을 가리키는 주석을 붙였다. 형식 번호는 코드로 확인했다(개인 세션 8, 세계 스냅샷 7). 빠짐 0.
    - 이 묶음에서 남은 개선: SessionFoundation의 절 안에 남은 "아직 구현하지 않았다" 문장들은 현재 계약 표로만 정정했다. 절 본문을 계약별로 다시 나누려면 링크하는 8곳을 함께 옮겨야 한다.
  - 진행: 코드 구조 묶음 완료. 두 문서 모두 보존 검사 빠짐 0.
    - FrameworkPlan: 보류된 계획으로 시작해 단계 1-4 기록이 붙은 문서라 단계 기록과 링크된 제목은 두었다. 첫 문단을 결과(KSI 모듈 11개, `Source/`로 확인)와 소유 범위로 바꾸고, 착수 전처럼 읽히던 계획 문장 다섯 곳을 지금 상태로 고쳤다.
    - CodeStructurePlan: 2026-10-11 검토 결과와 미착수 계획이라 지적은 그대로 두었다. 첫 문단에 검토 결론을 쓰고, 2026-10-26 코드 대조 결과(D1의 대체 저장은 아직 `Deinitialize`, D2의 역참조는 `CCLWorldSimulationSubsystem.cpp:270`·`:292`)와, 기준 커밋 뒤 FrameworkPlan 단계 2-4가 끝나 "단계 2 이전" 순서 제안이 반영되지 않았다는 사실을 덧붙였다.
  - 진행: 자연 환경 묶음 완료. 9개 문서 모두 보존 검사 빠짐 0.
    - 하위 문서 7개(Water, Nutrition, Weather, Terrain, Snow, Laboratories, Fire): 첫 문단을 결과와 소유 범위로 시작하고 KSI 이동 안내를 그 뒤로 옮겼다. 작업 전달 이력 문장을 뺐다. 지금 없는 소스 경로 4곳(`CCLEcologyNutrition.cpp`, `CCLWeatherModel`, `CCLSurfaceSimulation`, `CCLCombustionModel`)에 `git ls-files`로 확인한 KSIEnvironment 위치를 덧붙였다. "단계 5에서 멈춘다"(Snow), "단계 8에서 이어간다"(Fire), "자연 날씨·불·Server 타깃은 후속"(Water)은 각 소유 문서를 가리키게 고쳤다. Laboratories는 사용법을 앞에 두고 후속 작업 기준 두 절을 한 절로 모았다.
    - EnvironmentEcology·EnvironmentPlan: 날짜순 기록이고 링크된 제목이 각각 9개, 17개라 절 순서를 두었다. Ecology는 성과 목록 문단을 영역별 "구현 현황" 표로, Plan은 단계 0-9의 "단계별 현황" 표로 바꿔 각 영역·단계의 정본 절이나 문서를 가리키게 했다. 상태는 결정 19의 진행 기록(단계 0-7 완료, 8 진행)과 단계 9 관련 절을 따른다. Plan의 "다음 작업자는 Guide…"는 AGENTS.md로 바꿨다.
    - 이번 변경 전부터 깨져 있던 학습 문서 링크를 더 찾았다: `Docs/Learning/GASCombat.md:205`의 `EnvironmentFire.md#현재-코드의-후속-열-피해-연결`(제목 없음), `Docs/Learning/EnvironmentMapConfiguration.md`의 `Source/CCL/Editor/CCLEnvironmentFXLibrary.cpp`, `Docs/Learning/UIFramework.md`의 `Source/CCL/UI/Core/CCLUISubsystem.cpp` 두 곳. 학습 문서는 범위 밖이라 고치지 않았다.
  - 진행: 도구·현황 묶음 완료. AgentToolingResearch는 2026-10-05 조사 문장에 날짜를 붙이고, 제안이 어디까지 반영됐는지 2026-10-26 "반영 상태" 표를 넣었다(Unreal MCP·Toolset 활성화는 `CCL.uproject`로 확인, `ccl8-ue-editor`·`ccl8-asset-pipeline`은 만들지 않음). LearningLog는 Mentoring 양식의 사본을 링크로 바꿨다. 두 문서 모두 빠짐 0.
  - 결과: 조각 12의 대상 게임 문서 26개(방향 4, 시스템 사양 9, 코드 구조 2, 자연 환경 9, 도구·현황 2)를 모두 다뤘다. 문서마다 `preserve.cjs` 보존 검사가 빠짐 0으로 통과했고, 의도한 제거 3건(Campaign·Progression의 전달 이력, Content의 합쳐진 표기)은 커밋 본문에 이유와 함께 적었다. 날짜순 기록이고 링크된 제목이 많은 문서(SessionFoundation, EnvironmentEcology, EnvironmentPlan, FrameworkPlan, CodeStructurePlan)는 절을 다시 나누지 않고 첫머리 결론, 현재 상태 표, 낡은 문장 주석으로 정리했다. 재작성 중 코드와 대조해 낡은 사실을 고친 곳은 각 문서의 커밋 본문에 있다. `NarrativeConcept`는 조각 13에서 다룬다.
- [x] 13. 서사 정본 (10). `World.md`, `Scenario.md`의 확정 영역은 글자를 바꾸지 않고 구조만 바꾼다. 독립 검토의 수정 제안 목록을 사용자에게 제출한다.
  - 결과: `World.md`, `Scenario.md`에는 확정 내용이 없었다. 기존 문장은 그대로 두고 소유 문단과 빈 "확정한" 절만 더했다. Workflow, CurrentWork, NarrativeConcept는 결론을 앞에 두고 지운 Guide를 AGENTS.md로 바꾸고 결정 1·2와 VersionControl 링크를 붙였다. Glossary는 그대로 뒀다. 다섯 문서 모두 보존 검사 빠짐 0이다. 상대 경로를 링크로 바꾼 경우를 보존으로 인정하도록 `preserve.cjs`를 고쳤다.
  - 2026-10-26 읽기 전용 독립 검토(세션 모델)가 수정 제안 10건을 냈고 사용자에게 제출했다. 정본 확정 영역을 건드리는 제안은 없다. 반영은 사용자 승인 뒤에 한다.
    1. (높음) 스킬에 독립 검토의 읽기 전용 위임이 적혀 있지 않아 AGENTS.md 경계상 요청 없이 실행할 수 없다. CurrentWork의 검토자와 승인자도 섞여 있다.
    2. (높음) Research·Drafts·Reviews의 경로, 파일 이름, 종료 뒤 보존 규칙이 없고 NarrativeConcept의 "필요해지면"과 Workflow의 기록 의무가 어긋난다.
    3. (높음) 정본화 승인을 누가 어디에 남기는지 없다. 정본화마다 결정 파일을 만들고 정본 항목에 `결정 N`을 붙이는 안이다.
    4. (중간) 상태 어휘 세 벌(Glossary 항목 상태, Workflow 단계, 검토 판정)의 대응이 없고 판정 `APPROVED`가 사용자 승인과 헷갈린다.
    5. (중간) 단계에 초안 작성 단계가 없고 `CANON_CONFLICT` 뒤의 다음 단계가 없다.
    6. (중간) 스킬이 Workflow가 소유한다고 적은 항목(작업 방식 선택, 저작권, 문서 소유권)이 Workflow에 없고, 시작 때 읽을 목록이 스킬과 Workflow에 따로 있다.
    7. (중간) NarrativeConcept의 "다음 문서는 설정을 확정하지 않는다"가 바로 아래 World·Scenario 정본과 어긋난다.
    8. (중간) WritingGuide의 문서 종류 표에 서사 정본 종류가 없다.
    9. (중간) Workflow의 "이전 프로젝트의 설정, 작품 수 할당량과 일회성 예외"가 무엇인지 설명이 없다.
    10. (낮음) "한 도구"의 뜻, 시작 전 상태값, 필드 이름의 영어·한국어 혼용.
  - 2026-10-26 사용자가 10건 모두 반영을 지시했다("서사 수정 제안 10건을 모두 반영해 줘"). 반영 결과는 다음과 같다. 정본 확정 영역은 바꾸지 않았다.
    - 스킬에 독립 검토의 읽기 전용 위임을 적고 읽을 목록을 Workflow "시작" 절 하나로 모았다(1, 6).
    - Workflow에 역할, 기록 위치, 단계와 판정, 승인과 정본 반영 절을 두었다. `DRAFTING` 단계 추가, 판정 `APPROVED`를 `PASS`로 변경, 판정별 다음 단계, 단계와 항목 상태의 대응, 폴더별 파일 이름(`YYYY-MM-DD-<english-kebab>.md`)과 종료 뒤 처리, 정본화마다 결정 파일을 만들고 정본 항목에 근거 결정을 붙이는 규칙이다(2-5).
    - "이전 프로젝트" 문장은 Orbis로 밝히고, 할당량과 예외의 원래 내용은 저장소에 기록이 없다고 적었다(9). 근거는 Setup의 Orbis 이전 기록이다.
    - CurrentWork는 검토자와 승인자를 나누고 필드 이름을 한국어로 맞췄다(1, 10). NarrativeConcept는 "확정하지 않는다"의 대상을 절차 문서로 좁혔다(7). WritingGuide 문서 종류 표에 정본 종류를 더했다(8).
- [ ] 14. 날짜 붙은 검토 보고서 정리 (12). 결과를 소유 사양서로 옮긴다. 삭제는 대상 목록을 사용자에게 확인받은 뒤 한다.
  - 대상: `AgentReview-2026-10-08.md`, `UIReview-2026-10-08.md`, `CodeDesignFixes-2026-10-11.md`.
  - 진행: 결과 이동 완료. 세 보고서 모두 보존 검사에서 보고서 자신을 가리키던 링크만 빠졌다.
    - CodeDesignFixes: 장비 확정 순서·GAS 근거·기능 서비스와 2026-10-11 검증 기록은 ProgressionFoundation, 카탈로그 이름 유일성은 SessionFoundation, 선택적 시간 진행 인터페이스는 EnvironmentPlan, 표면 발행본 수명과 비용은 EnvironmentSnow로 옮겼다. 클래스 다이어그램은 관계를 각 소유 문서의 문장으로 옮기고 뺐다.
    - AgentReview: 데이터 위치 표, 현재 이름의 클래스 다이어그램, 직접 확인 절차를 AgentFoundation으로 옮겼다. 지도 서브시스템 이름을 KSI 이름으로 고쳤다.
    - UIReview: 구현 커밋 표, 최종 생성·빌드 로그, 직접 확인 절차, 2026-10-08 당시 한계를 UIFoundation으로 옮겼다.
    - CodeStructurePlan, FrameworkPlan, `Docs/Learning/AgentExecution.md`, `Docs/Learning/Map.md`의 링크를 새 위치로 바꿨다. `Docs/Learning/Map.md`의 `Source/CCL/Map/CCLMapSystem.cpp` 링크 두 곳은 이전부터 깨져 있었다.
    - 남은 일: 사용자 확인 뒤 세 파일 삭제와 Docs/README.md의 "날짜 붙은 검토 보고서" 절 삭제.
- [ ] 15. 계획 문서 정리 (12). StudyRoadmap은 방향과 일정, DeliveryPlan은 제출 절차만 남기고 다음 작업은 `Docs/Work/`로 옮긴다.

## 정한 것

- [결정 28](../decisions/0028-agent-workflow-restructure.md): 백업 방식, 재작성 범위와 순서, 결정 파일 전환, 읽기 전용 위임, 작업 지도, QA·학습·서사 문서의 처리.
- 두 스킬 폴더는 복사로 맞춘다. 정본은 `.agents/skills`다. Git은 Windows 디렉터리 연결을 추적하지 않고, 심볼릭 링크는 개발자 모드와 `core.symlinks` 설정이 있어야 해서 다른 환경에서 깨진다.
- clang-format은 Visual Studio 2022의 LLVM 도구(19.1.5)를 `vswhere`로 찾는다. 다른 실행 파일은 `CLANG_FORMAT` 환경 변수로 지정한다. 엔진 `Extras/clang-format`에는 실행 파일이 없고 실험용 설정만 있다.
- 검사 도구의 시험은 `node --test Tools/Agent/Tests/*.test.cjs`로 실행한다. 이 Node 24 환경에서는 폴더 경로를 넘기면 시험을 찾지 못한다.
- 결정 파일 이름은 `NNNN-<english-kebab>.md`다. 2026-10-26 사용자가 영어 kebab-case를 골랐다. 근거: 사용자 지시 "slug는 영어 kebab-case로 해 줘(예: 0028-agent-workflow-restructure.md)".
- 결정 파일 머리말에는 결정 28이 적은 상태(`status`, 진행이 있으면 `progress`)와 관계(`refines`, `amends`, `supersedes_part`, `supersedes`, `lifts_hold`)에 더해 `topic`, `summary`, `owners`를 둔다. 옛 Decisions 표의 주제·현재 내용·소유 문서를 생성 결과에 남기려면 이 세 칸이 필요했다. 결정 28의 "상태·번복 관계만"을 넓게 읽은 것이므로 사용자가 다르게 정하면 머리말과 생성기를 고친다. 근거: `Tools/Agent/decisions.cjs`의 `REQUIRED`.
- 결정 사이의 관계는 나중 결정의 머리말에만 적고, 반대 방향("결정 7에서 보완")은 생성기가 계산한다. 근거: `Tools/Agent/decisions.cjs`의 `relationText`.
- 날짜 붙은 문서 여섯 개 가운데 `EnvironmentPIEQA-2026-10-12.md`, `FullGameplayMCPQA-2026-10-12.md`, `GameplayQAFixes-2026-10-12.md`는 QA 보고서, `AgentReview-2026-10-08.md`, `UIReview-2026-10-08.md`, `CodeDesignFixes-2026-10-11.md`는 검토 보고서다. 근거: 2026-10-26 사용자 지시 "세 개를 QA 보고서로 Docs/QA/에 옮기고, AgentReview, UIReview, CodeDesignFixes는 검토 보고서로 둬".

## 아직 모르는 것

- 개인 볼트에 재사용 지식을 반영할 범위. 이번 기록 때 Obsidian MCP가 연결되지 않아 볼트를 참고하지 못했다.

## 범위 밖

- 새 설정으로 소스 코드를 바꾸는 작업. 이 지도가 끝난 뒤 별도 작업으로 시작한다.
- `Docs/Learning/` 재작성과 QA 재검증. 소스 변경 뒤에 한다.
- GitHub Issues와 CI 도입.
