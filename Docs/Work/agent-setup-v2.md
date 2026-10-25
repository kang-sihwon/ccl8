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
  - 결과: `Tools/Agent/guard-commands.cjs`를 `.claude/settings.json`의 Bash·PowerShell PreToolUse에 등록했다. 차단 대신 `ask`로 답해 사용자 확인 뒤에만 실행되게 했다. 공통 지침의 "삭제·덮어쓰기 전 확인"과 같은 동작이다. 완료 조건은 훅 프로세스 시험으로 확인했고, 실제 세션에서 걸리는지는 설정을 다시 읽은 새 세션에서 확인한다. Codex에는 적용되지 않는다.
- [x] 5. 바닥선 검사 (1). diff에서 새 억제 주석, 비활성화된 자동화 테스트, 스모크 검사 삭제·시간 제한 증가, `checkNoEntry`·TODO 스텁을 찾는다. 완료 조건은 각 항목을 하나씩 넣은 시험 diff가 모두 걸리는 것이다.
  - 결과: `node Tools/Agent/floor-guard.cjs [--base <rev>]`. 추적하지 않는 새 파일도 포함하고 문서는 제외한다. 종료 코드는 0 통과, 1 발견, 2 실행 불가다. 다른 곳으로 옮긴 테스트·검사와 줄인 시간 제한은 발견으로 치지 않는다. 첫 실행에서 이 작업의 clang-format 시험에 들어 있던 조건부 `t.skip`을 찾아내 도구가 없으면 실패하도록 고쳤다.
- [x] 6. 진입점 (2). `AGENTS.md`를 정본으로 새로 쓰고 `CLAUDE.md`는 `@AGENTS.md`로 둔다. `Docs/README.md` 문서 지도와 `GLOSSARY.md` 초안을 만든다.
  - 결과: 매 턴 읽는 진입점이 `Docs/Guide.md` 13.7KB에서 `AGENTS.md` 약 4.1KB로 줄었다. 스킬 설명이 이미 맡는 스킬 라우팅 행은 뺐다. 문서 지도는 Guide의 83행을 문구 그대로 묶음별로 옮겼다. `GLOSSARY.md`에는 시스템·제작 용어 15개를 두고 서사 용어는 `Docs/Narrative/Glossary.md`에 남겼다. 규칙 문서, `README.md`, `ccl8-docs`·`ccl8-design` 스킬의 Guide 참조를 새 위치로 바꿨다. Guide에는 이행 중이라는 안내를 붙였다.
- [ ] 7. 규칙 문서 (3, 6). `Coding.md`(검사기로 옮긴 규칙 제외), `WritingGuide.md`, `VersionControl.md`, `AgentContextTools.md`, `Mentoring.md`, `SystemExplanationGuide.md`를 새로 쓰고 옛 `Docs/Guide.md`를 없앤다.
  - 남은 Guide 링크: `Docs/Learning/README.md:91`, `Docs/Learning/EnvironmentEvidence.md:7`. 학습 문서는 재작성 범위 밖이므로 Guide를 없앨 때 링크만 `Docs/README.md`로 바꾼다. `FrameworkPlan.md:477`은 과거 제출 범위의 기록이라 고치지 않는다.
- [ ] 8. 스킬 재구성 (2, 6, 7). 통합 구조의 모델 호출·사용자 호출 목록대로 스킬을 쓰고 기존 스킬을 교체한다. QA 보고서에서 절차로 쓰이던 실행 조건 표를 `ccl8-verify`로 옮긴다. 완료 조건은 스킬 검사 통과다.
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
