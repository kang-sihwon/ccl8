---
name: ccl8-docs
description: ccl8의 Docs/ 문서를 새로 만들거나 고칠 때, 결정을 기록하고 Decisions.md를 갱신할 때, 개인 Obsidian 볼트의 노트를 정리할 때 쓴다. "문서로 남겨", "사양서에 반영", "결정으로 적어", "볼트에 기록"이 신호다. 문서의 종류, 소유, 위치, 등록을 정하고 문장 작성과 검토는 ccl8-writing에 맡긴다. 서사 문서는 ccl8-narrative가 맡는다.
---

# ccl8 문서 작업

작업 전에 `../../../Docs/WritingGuide.md`(문서 종류와 구조)를 읽는다. 소유 문서를 찾을 때는 `../../../Docs/README.md`(문서 지도), 결정의 현재 상태가 필요할 때는 `../../../Docs/Decisions.md`를 읽는다. 규칙의 정본은 WritingGuide이고 여기에 복제하지 않는다. 문장을 쓰고 다듬고 검토하는 절차는 `ccl8-writing` 스킬이 맡는다. `AGENTS.md`, `CLAUDE.md`, 스킬 문서를 고칠 때는 `writing-for-agents` 스킬도 함께 쓴다.

## 작업 모드

- **작성·개정**: WritingGuide의 "문서의 종류와 소유" 표로 종류와 소유 문서를 정한다. 표로 정할 수 없을 때만 사용자에게 묻는다. 개요와 초안을 한 응답에 함께 낸다. 새 문서는 가장 가까운 부모 지도에 한 줄 등록한다. 다른 문서가 링크하는 제목을 바꾸면 그 링크도 함께 고친다. 기존 문서를 다시 쓰거나 합치거나 옮겼으면 `node Tools/Agent/preserve.cjs --old <옛 문서>... --new <새 문서>...`가 통과해야 한다. 빠진 항목은 되살리고, 일부러 뺀 항목만 허용 파일에 넣은 뒤 그 항목과 이유를 커밋 본문에 적는다.
- **결정 기록**: `../../../Docs/WritingGuide/DecisionRecords.md`의 네 단계를 따른다. `Docs/decisions/`에 새 번호의 파일을 만들고, `Docs/Decisions.md`는 생성기로 다시 만들고, 확정 수치는 사양서로 옮긴다. 과거 결정의 본문은 고치지 않는다.
- **결정 상태 재검토**: 주제 하나의 결정 상태를 다시 맞출 때는 ① `Tools/Graphify/graphify_ccl8.cmd query canonical "<주제>"`로 관련 결정 후보를 찾고(명령을 쓸 수 없거나 `stale`이면 재빌드하지 않고 `Docs/decisions/`에서 `topic`과 "대체"·"보완" 검색으로 계속한다), ② 후보 파일의 본문을 읽어 이후 결정의 번복·부분 번복·미결을 확인하고, ③ 머리말의 관계 키와 `status`, `summary`를 고친 뒤 생성기를 돌린다. 결정 본문에 없는 수치나 이유를 채우지 않는다.
- **볼트 정리**: 볼트 `CLAUDE.md`와 대상 노트·MOC를 먼저 다시 읽는다. 자동으로 불러오지 않기 때문이다. Obsidian 문법은 볼트 규칙과 이웃 노트가 이미 쓰는 것만 쓰고, 문법 세부는 `references/obsidian-markdown/GUIDE.md`에서 찾는다. 볼트에서 `git status`를 확인해 이 작업 밖의 미커밋 변경이 있으면 병합하지 않고 보고한다. 편집 뒤에는 `git diff`만 확인한다. 볼트의 pull, commit, push는 사용자가 요청할 때만 한다.

## 끝내기 전

`ccl8-writing`의 납품 점검을 한다. 그 위에 결정과 수치에 근거 결정 번호가 있는지 확인하고, `node Tools/Agent/docs.cjs check`로 링크, 앵커, 문서 지도 도달을 확인한다.

## 권한 경계

이 스킬로 고치는 것은 `Docs/`, `README.md`, `AGENTS.md`, `CLAUDE.md`, `GLOSSARY.md`와 `.claude/`, `.agents/`, `.codex/`의 스킬 문서다. 코드, 설정, 에셋 파일은 고치지 않는다. 커밋과 push는 `../../../Docs/VersionControl.md`의 커밋 주체 규칙을 따른다. 서사 정본(`World.md`, `Scenario.md`)의 확정 영역은 사용자 승인 없이 바꾸지 않는다.
