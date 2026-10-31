---
number: 28
date: 2026-10-25
title: "에이전트 작업 체계의 재구성"
topic: "에이전트 작업 체계"
status: "유효"
progress: "2026-10-27 이행 완료"
summary: "규칙 문서를 태그 `agent-setup-v1`로 백업하고 새로 쓴 뒤 게임 문서를 내용 보존 검사와 함께 새 형식으로 옮긴다. 결정은 1건당 파일로 나누고 Decisions는 생성한다. 스킬에 명시된 읽기 전용 위임만 요청 없이 허용한다. 재개 지점은 `Docs/Work/`에 둔다"
owners: "[AgentToolingResearch](../AgentToolingResearch/BundleReview.md), [AgentContextTools](../AgentContextTools.md), [WritingGuide](../WritingGuide/DecisionRecords.md#결정을-기록하는-방법), [진행 중인 작업](../Work/README.md)"
---

# 2026-10-25 — 결정 28: 에이전트 작업 체계의 재구성

### 배경

사용자가 mattpocock/skills, addyosmani/agent-skills, cursor/plugins의 pstack을 코드까지 읽고, 처음 구성한다는 기준으로 현재 설정과 장점을 합친 체계를 요청했다. 비교 결과와 채택·제외 근거는 [AgentToolingResearch](../AgentToolingResearch/BundleReview.md)가 소유한다. 현재 설정은 매 턴 `Docs/Guide.md` 13.5KB와 스킬 15개의 설명 약 5.2KB를 불러오고, `.claude/skills`와 `.agents/skills`를 손으로 동기화하며, 기계적으로 검사할 수 있는 C++ 배치 규칙을 문장으로만 두고 있다. 사용자는 grilling으로 열린 결정을 정했다.

### 결정

규칙 문서를 Git 태그 `agent-setup-v1`과 브랜치로 백업한 뒤 새로 쓴다. 작업 트리에는 옛 규칙의 사본을 두지 않는다. 대상은 `CLAUDE.md`, `AGENTS.md`, `Docs/Guide.md`, `Coding.md`, `WritingGuide.md`, `Mentoring.md`, `VersionControl.md`, `AgentContextTools.md`, `SystemExplanationGuide.md`와 `.claude/`, `.agents/`, `.codex/`의 지침·스킬이다. graft 훅의 기능은 유지한다. 소스 코드는 새 설정이 완성된 뒤 그 설정으로 변경한다.

게임 문서도 새 형식으로 다시 쓰되 기획 내용은 보존한다. 문서마다 수치, `결정 N`, 경로, 링크의 보존을 스크립트로 확인하고 문서나 소유 묶음 하나를 커밋 하나로 남긴다. 날짜가 붙은 검토 보고서는 결과를 소유 사양서로 옮기고 삭제한다. QA 보고서는 내용을 바꾸지 않고 `Docs/QA/`로 옮겨 시점 기록으로 표시하고, 절차로 쓰이던 부분은 `ccl8-verify`로 옮긴다. `Docs/Learning/`은 이번 재작성에서 제외하고 소스 변경 뒤 시스템별로 새로 쓴다. 서사 정본의 확정 영역은 글자를 바꾸지 않고 구조만 바꾸며, 독립 검토의 수정 제안은 사용자 승인 뒤에 반영한다. 진행 순서는 규칙 문서, 결정 파일 전환, 게임 문서다.

결정 기록은 결정 1건당 파일 1개(`Docs/decisions/NNNN-*.md`)로 바꾼다. 번호와 본문은 그대로 옮기고 상태·번복 관계만 머리말로 붙인다. `Docs/Decisions.md`는 그 머리말에서 생성하고, 다른 문서의 `결정 N` 표기는 유지한 채 링크만 바꾼다. 이 결정 28이 전환 전 마지막 DesignLog 항목이다.

스킬에 명시된 읽기 전용 위임(대량 탐색, 독립 리뷰, 검증 병렬 실행)은 요청 없이 허용한다. 탐색은 Explore 에이전트, 리뷰와 검증은 세션 모델로 하고 시작할 때 한 줄로 알린다. 파일을 쓰는 위임, 다른 세션 시작, 예약 작업과 Codex CLI 교차 리뷰는 사용자가 요청할 때만 한다.

진행 중인 작업의 재개 지점은 `Docs/Work/README.md` 한 곳에 둔다. 여러 세션에 걸치는 작업마다 지도 파일 하나에 목적지, 차단 관계가 있는 진행 조각, 정한 것, 아직 모르는 것, 범위 밖을 적는다. 끝난 작업은 결과를 소유 사양서로 옮긴 뒤 지도를 삭제한다. GitHub Issues는 보류한다.

새 규칙 문서가 완성되기 전까지는 기존 규칙 문서가 유효하다.

### 트레이드오프

전면 재작성은 문서 사이의 중복과 매 턴 불러오는 양을 한 번에 정리하지만, 게임 문서까지 다시 쓰는 비용과 기획 내용을 잃을 위험이 생긴다. 보존 검사 스크립트와 문서별 커밋으로 그 위험을 확인 가능한 범위로 줄인다. 결정 파일 분할은 에이전트가 결정 하나만 읽게 하고 손으로 맞추던 표를 없애지만, 지금까지의 인용 링크를 바꾸는 이전 작업이 필요하다. 읽기 전용 위임만 허용하면 독립 리뷰와 병렬 검증을 얻는 대신 쓰기 작업의 병렬화는 포기한다. 작업 지도는 재개 지점을 하나로 모으지만 여러 세션이 동시에 일을 가져가는 운용에는 부족하며, 그 운용을 시작할 때 티켓 파일 분리를 다시 검토한다.
