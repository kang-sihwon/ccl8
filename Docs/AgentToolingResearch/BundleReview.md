# 스킬 묶음 통합 검토

2026-10-25에 세 스킬 묶음을 어떻게 받아들일지 판단한 기록이다. 왜 어떤 스킬이나 규칙을 가져오거나 뺐는지 찾을 때 읽는다. 이 판단으로 정한 이행 방식은 [결정 28](../decisions/0028-agent-workflow-restructure.md)이고, 이행은 2026-10-27에 끝났다. 현재 구성은 [AgentContextTools](../AgentContextTools.md)가 소유한다.

세 묶음 중 하나를 그대로 설치하지 않는다. mattpocock의 호출 구분·글쓰기·진단 체계를 뼈대로 쓰고, pstack의 검증·자기개선 스킬과 addy의 품질 바닥선을 ccl8 조건에 맞춰 옮긴다. 세 묶음 모두 웹·TypeScript·GitHub 흐름의 비중이 커서 UE·Windows·1인 개발에 옮길 수 있는 범위는 일부다.

## 조회 범위

원문을 저장소 밖 임시 폴더에 내려받아 읽었다. 설치와 설정 변경은 하지 않았다.

| 묶음 | 기준 커밋 | 전부 읽은 것 | 훑어본 것 |
|---|---|---|---|
| `mattpocock/skills` | `5beb35b` (2026-10-19) | 스킬 27개와 보조 파일, `.agents/` 메타 문서·ADR, `.out-of-scope/` | 사람용 `docs/` 페이지, CHANGELOG |
| `addyosmani/agent-skills` | `aeac41d` (2026-10-13, 0.6.13) | 스킬 24개, 페르소나 4개, 커맨드, 훅 문서, `references/` | `scripts/run-evals.js`, `scripts/lib/skill-lint.js`, 평가 사례 |
| `cursor/plugins`의 `pstack` | `99b609a` (2026-10-24, 0.15.32) | README, 안내서 10장, Claude Code 대응표, 원칙 24개, 스킬 전부, `poteto-mode`와 핵심 플레이북 | GitHub·Cursor 전용 플레이북, `orch`·`watch-pr`·`statechart` 스크립트, Slack 자동화 `benny` |

[이식본 검토](SkillReview.md#pstack-검토-2026-10-25)는 파일 9개를 기준으로 했고, 이번에는 상류 원본을 읽어 `correct`, `reflect`, `benchmark-checklist`, `blast-radius`, `show-me-your-work`가 채택 후보로 추가됐다.

## 세 묶음의 성격

| 항목 | mattpocock | addy | pstack |
|---|---|---|---|
| 운영 방식 | 작은 스킬을 조합하고 사용자가 흐름을 고른다 | 정의부터 출시까지 단계별 스킬과 커맨드 | `poteto-mode`가 플레이북을 골라 다른 스킬을 호출 |
| 호출 | 사용자 호출과 모델 호출을 구분한다 | 모두 모델 호출이다 | 대부분 사용자 호출이다 |
| 강점 | 에이전트용 문서 작성법, 진단 루프, 용어집·ADR 기준, 회고 | 품질 바닥선과 diff 검사, 스킬 평가·검사 스크립트, 위임 금지 규칙 | 검증 스킬 생성·유지, 측정값 검증, 반복 실수의 구조화, 결정 로그 |
| ccl8 적합도 | 높음 | 중간. 웹 전용 스킬이 많다 | 중간. GitHub·Cursor 흐름과 다중 모델 기본값이 많다 |

## 가져온 것

| 영역 | 출처 | 가져오는 내용 |
|---|---|---|
| 진입점 | mattpocock `retro`, `writing-for-agents` | 항상 읽는 문서에는 규칙과 길잡이만 둔다. 문서 지도는 필요할 때 읽는다 |
| 호출 구분 | mattpocock `invocation`, `ask-matt` | 에이전트가 스스로 찾아야 하는 스킬만 모델 호출로 둔다. 나머지는 사용자 호출로 두고 안내 스킬 하나가 목록을 맡는다 |
| 정렬과 용어 | mattpocock `grilling`, `domain-modeling` | 질문은 "응"이 추천안을 받아들이게 쓴다. 프로젝트 용어집을 두고 ADR은 되돌리기 어렵고, 맥락 없이 놀랍고, 실제 대안이 있었던 결정에만 쓴다 |
| 설계 | mattpocock `codebase-design`, pstack `architect` | 깊은 모듈 용어와 삭제 시험, 호출자 사용 예를 먼저 쓰는 방식, 대안 필수, 에이전트 기여자를 가정한 설계 경고 목록 |
| 진단 | mattpocock `diagnosing-bugs`, pstack 원칙 | 실패를 재현하는 짧은 루프를 먼저 만든다. UE에서는 스모크 스크립트, 자동화 테스트, MCP PIE, 저장 세계 재생이 루프 후보다 |
| 리뷰 | mattpocock `code-review`, addy 심각도, pstack `interrogate` | 규칙 축과 사양 축을 나눠 보고, 심각도 접두어와 판정 분류(조치, 검토, 참고, 기각)를 쓴다 |
| 품질 바닥선 | addy `constraint-driven-development` | 새 억제 주석, 비활성화된 테스트, 스모크 검사 삭제나 시간 제한 증가, 미구현 스텁을 diff에서 찾는 검사 |
| 실행 검증 | pstack `create-verification-skill`, `maintain-verification-skill` | 정리 뒤 증거 보존 확인, 유지 결과 분류(clean, changed, blocked)와 문서 차이·도구 공백·제품 결함의 구분 |
| 측정·영향 | pstack `benchmark-checklist`, `blast-radius` | 성능 수치를 보고하기 전 일곱 질문, 변경의 안전 근거 하나를 실행으로 증명 |
| 자기개선 | mattpocock `retro`, pstack `correct`, `reflect` | 반복 실수를 구조, 타입·검사, 테스트, 문서 순으로 막고 규칙과 강제 수단을 표로 짝짓는다. 스킬 수정은 오래 유효하고 행동을 바꾸는 것만 받는다 |
| 세션 연속성 | pstack `pause-safely`, `session-pickup`, addy `context-engineering` | 중단 지점의 상태·검증 명령·미결을 파일로 남기고 재개할 때 이를 정본으로 읽는다 |
| 장시간 작업 | pstack `show-me-your-work` | 결정마다 근거 위치가 있는 한 줄 기록을 덧붙이기만 한다 |
| 학습 | mattpocock `teach`, pstack `teach` | 학습 목적·학습 기록 구조와 그림을 하나씩 쌓는 설명 방식 |
| 스킬 품질 | addy `skill-lint.js` | 머리말, 링크, 두 스킬 폴더의 일치, 설명의 사용 조건을 검사한다 |
| 안전 장치 | mattpocock `git-guardrails-claude-code` | 파괴적 Git·삭제 명령을 실행 전에 막는 훅 |

## 제외한 것

- addy의 웹 전용 스킬: `frontend-ui-engineering`, `browser-testing-with-devtools`, `api-and-interface-design`의 REST 부분, `ci-cd-and-automation`, `shipping-and-launch`, `observability-and-instrumentation`, 웹 성능·보안 점검표.
- pstack의 GitHub·Cursor 흐름: Babysit, Shipping, Autopilot, Orchestrate, Bugbot 대응, `benny`, `make-bot-ui`, `typescript-best-practices`, `ketchup-video`.
- mattpocock의 `triage`, `wizard`, `pr`, `setup-matt-pocock-skills`, 시험 단계 글쓰기 스킬, TypeScript 전용 스킬.
- addy의 "적용 가능성이 조금이라도 있으면 스킬을 호출한다"는 규칙과 세션 시작 시 안내 스킬 주입 훅. 기본 스킬 선택 위에 선택 단계가 하나 더 생긴다.
- 원격 CI가 생기면 addy `ci-cd-and-automation`과 pstack Babysit을 다시 검토한다.

## 충돌과 해결

| 충돌 | 해결 |
|---|---|
| pstack "사람을 기다리지 않는다"와 사용자 전역 규칙 "삭제·덮어쓰기 전 확인" | 사용자 규칙이 우선한다. pstack의 항상 멈추는 동작 목록과 훅으로 경계를 정한다 |
| pstack의 다중 모델 검토 기본값 | Claude Code 서브에이전트는 opus, sonnet, haiku, fable만 지정할 수 있다. 다른 계열의 검토는 요청 시 Codex CLI 읽기 전용 실행으로만 한다 |
| addy의 항상 TDD | 저렴한 자동화 테스트가 있을 때만 테스트를 먼저 쓴다. 나머지는 `ccl8-verify`로 검증한다 |
| 서브에이전트 사용 범위 | 스킬에 명시된 읽기 전용 위임만 허용한다. 근거는 결정 28이다 |
