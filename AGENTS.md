# ccl8

ccl8은 Orbis의 방향과 그래픽 스타일을 공유하는 단기 소울라이크 RPG다. UE 프로젝트는 `CCL.uproject`이고 게임 모듈은 `Source/CCL/`, 공용 모듈은 `Source/KSI*/`에 있다. 기본 엔진 기준은 UE 5.8.1이고 실제 검증에 쓴 설치본은 작업 문서에 적는다. 설치 경로와 도구 설정은 [AgentContextTools](Docs/AgentContextTools.md)가 소유한다.

## 세션 시작

1. `git status --short --branch`로 상태를 확인한다. 동기화는 [VersionControl](Docs/VersionControl.md)을 따른다.
2. [진행 중인 작업](Docs/Work/README.md)에서 이어갈 작업의 지도를 읽는다. 완료와 승인 상태는 지도와 Git에서 확인한다.
3. 작업에 필요한 문서는 [문서 지도](Docs/README.md)에서 찾는다.

## 협업

- AI가 기획, 아트 제안, 설계, 구현, 통합, 검증, 문서화를 주도한다. 사용자는 결과를 검토하고 방향을 지시한다. 파일 수정과 실행 경계는 [Coding](Docs/Coding.md)이 소유한다.
- 답은 한국어 반말로 간결하게 쓴다. 코드, 식별자, 코드 주석, 커밋 메시지는 영어로 쓴다. 사실, 추론, 제안, 미확인을 구분한다.
- 설계는 [Decisions](Docs/Decisions.md)의 확정 결정에서 출발한다. Orbis의 모듈 구성, 시스템 규모, 수치 목표는 ccl8에 맞는지 판단한 뒤에 쓴다.
- 엔진 동작은 현재 엔진 소스로 확인하고 `path:line`을 남긴다. UE4와 UE5의 차이를 구분하고, 설계 대안마다 트레이드오프를 붙인다.
- 확정한 결정은 [WritingGuide](Docs/WritingGuide.md)의 결정 기록 절차로 남긴다. 과거 결정은 새 결정으로 번복한다.
- 훈련이나 힌트를 요청받으면 [Mentoring](Docs/Mentoring.md) 절차를 쓴다.

## 경계

- 파일을 삭제하거나 덮어쓰기 전에 사용자에게 확인한다. Claude Code에서는 `Tools/Agent/guard-commands.cjs` 훅이 파괴적 Git·삭제 명령에 확인을 요청한다. Codex에는 이 훅이 없다.
- 엔진 폴더는 읽기만 한다.
- 서사 정본(`Docs/Narrative/World.md`, `Docs/Narrative/Scenario.md`)의 확정 영역은 사용자 승인 뒤에 바꾼다.
- 스킬에 적힌 읽기 전용 위임(대량 탐색, 독립 리뷰, 검증 병렬 실행)은 시작할 때 한 줄로 알리고 진행한다. 탐색은 Explore 에이전트, 리뷰와 검증은 세션 모델로 한다. 파일을 쓰는 위임, 다른 세션 시작, 예약 작업, Codex CLI 교차 리뷰는 사용자가 요청할 때 한다. 근거는 [결정 28](Docs/DesignLog.md)이다.

## 길잡이

| 필요한 것 | 쓸 것 |
|---|---|
| 프로젝트 코드의 위치·심볼·호출 관계 | `graft`. 결과가 없거나 부족하면 실제 파일 검색 |
| 엔진 내부 동작·API·셰이더 | `graft_ue58`. 부족한 범위는 엔진 소스 |
| 여러 문서의 관계·누락·영향 | `Tools/Graphify/graphify_ccl8.cmd`. 그래프는 단서다. 원문으로 확인하고, 오래됐다는 이유만으로 재빌드하지 않는다 |
| 코드 리뷰 | 수정 요청이 없으면 지적까지만 한다 |
| 일반 UE·도구 지식 | 개인 Obsidian 볼트. [활용 절차](Docs/AgentContextTools.md#obsidian-활용-절차)대로 시작할 때 읽고 끝날 때 검증한 지식을 반영한다. ccl8 고유 결정과 진행은 이 저장소에 적는다 |

스킬 정본은 `.agents/skills/`다. `.claude/skills/`는 `node Tools/Agent/skills.cjs sync`로 맞춘다.

## 끝내기 전

- 코드: [Coding](Docs/Coding.md)의 프로젝트 파일 생성과 빌드 결과를 각각 확인한다. `node Tools/Agent/cpp-rules.cjs format`과 `node Tools/Agent/floor-guard.cjs`가 통과해야 한다.
- 게임 동작: `ccl8-verify`로 실행 검증하고 증거 강도를 적는다.
- 스킬·지침: `node Tools/Agent/skills.cjs check`가 통과해야 한다.
- 여러 세션에 걸친 작업: 지도의 진행과 아직 모르는 것을 갱신한다.

## 이행 중

[재구성 지도](Docs/Work/agent-setup-v2.md)가 끝날 때까지 스킬과 결정 기록은 기존 형식이 유효하다. 스킬은 조각 8, 결정 파일은 조각 9에서 바뀐다.
