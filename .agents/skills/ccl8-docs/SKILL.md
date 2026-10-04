---
name: ccl8-docs
description: ccl8의 Docs/ 문서를 새로 쓰거나 고칠 때, 결정을 기록해 Decisions.md를 갱신할 때, 검토 보고서를 쓸 때, 별도 Obsidian 볼트의 노트를 정리할 때 쓴다. "문서로 남겨", "정리해 둬", "사양서에 반영", "결정으로 적어", "볼트에 기록" 같은 요청이 신호다. 서사는 ccl8-narrative, 로드맵 검토는 roadmap-review가 맡고, 코드·설정·에셋 파일은 별도 명시 요청을 따른다.
---

# ccl8 문서 작업

작업 전에 `../../../Docs/Guide.md`(문서 지도), `../../../Docs/WritingGuide.md`(문장·표기·문서 종류·결정 기록 방법), `../../../Docs/Decisions.md`(현재 결정)를 읽는다. 문장 규칙과 물결표 금지의 정본은 WritingGuide이고 여기에 복제하지 않는다.

참고 스킬과 우선순위: 문서 종류별 수정 규칙과 문장·출처 규칙은 `technical-writing`, `CLAUDE.md`·`AGENTS.md`·스킬 문서의 작성법은 `writing-for-agents`, 볼트 노트의 Obsidian 문법은 `obsidian-markdown`을 참고한다. 우선순위는 WritingGuide, 이 문서, 참고 스킬 순이다. 명시 예외가 둘 있다. `technical-writing`의 em dash 금지는 `Docs/DesignLog.md` 제목 형식(`날짜 — 결정 N`)에 적용하지 않고, `결정 N`과 커밋 해시의 인용은 그 스킬의 "전달 이력 금지" 규칙의 예외로 허용한다.

## 작업 모드

- **작성·개정**: WritingGuide의 "문서의 종류와 소유" 표로 종류와 소유 문서를 정한다. 표로 정할 수 없을 때만 질문한다. 개요와 초안을 한 응답에 함께 낸다. 한국어로 쓰고 코드와 식별자는 원문 표기를 유지한다. 새 문서는 `Docs/Guide.md`의 문서 지도에 한 줄 등록한다.
- **결정 기록**: WritingGuide "결정을 기록하는 방법"의 네 단계를 따른다. `Docs/DesignLog.md` 끝에 새 번호로 붙이고, `Docs/Decisions.md`의 해당 주제 표를 갱신하고, 확정 수치는 사양서로 옮긴다. 과거 항목은 고치지 않는다.
- **Decisions.md 재구성**: 주제 하나를 다시 정리할 때는 ① `Tools/Graphify/graphify_ccl8.cmd query canonical "<주제>"`로 관련 결정 후보를 찾고(명령을 쓸 수 없거나 `stale`이면 재빌드하지 않고 DesignLog에서 제목과 "번복" 검색으로 계속한다), ② 후보 절을 원문에서 읽어 이후 절의 번복·부분 번복·미결을 확인하고, ③ 행마다 `유효`, `유효(부분)`, `번복 N → M`, `완료`, `미확인`을 붙인다. 결정 로그에 없는 수치나 이유를 채우지 않는다.
- **볼트 정리**: 볼트 `CLAUDE.md`와 대상 노트·MOC를 먼저 다시 읽는다 (자동 로드되지 않는다). Obsidian 문법은 볼트 규칙과 이웃 노트가 이미 쓰는 것만 쓴다. 볼트에서 `git status`를 확인해 이 작업 밖의 미커밋 변경이 있으면 병합하지 않고 보고한다. 편집 후에는 `git diff`만 확인한다. 볼트의 pull, commit, push는 사용자가 요청할 때만 한다.

## 검토

WritingGuide "검토 순서" 1-7을 적용한다. 그 위에 다음을 확인한다. 물결표가 없는지. 판정 기준이 GitHub 렌더링인지 (`[[ ]]`, `==`, `%%`, callout 같은 Obsidian 전용 문법을 `Docs/`에 쓰지 않는다). 결정과 수치에 근거 결정 번호가 있는지. 검토 보고서를 쓸 때는 `reviewing-technical-prose`의 심각도 어휘(BLOCKER, WARNING, OBS)를 보고서 안에서 정의한 뒤 쓴다.

## 권한 경계

이 스킬로 고치는 것은 `Docs/`, `README.md`, `CLAUDE.md`, `AGENTS.md`, `.claude/`, `.agents/`, `.codex/`의 스킬 문서다. 코드, 설정, 에셋 파일은 고치지 않는다. 고친 문서의 commit·push는 `Docs/VersionControl.md`의 커밋 주체와 원격 연결 조건을 따른다. 서사 정본(`World.md`, `Scenario.md`)의 확정 영역은 사용자 승인 없이 바꾸지 않는다.
