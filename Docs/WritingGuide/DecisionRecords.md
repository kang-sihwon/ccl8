# 결정 기록

상위 문서는 [ccl8 문서 작성 가이드](../WritingGuide.md)다.

## 결정을 기록하는 방법

결정 1건은 `Docs/decisions/NNNN-<english-kebab>.md` 파일 하나다. 근거는 [결정 28](../decisions/0028-agent-workflow-restructure.md)이고, 이름의 영어 kebab-case는 2026-10-26 사용자가 골랐다. `Docs/Decisions.md`는 결정 파일의 머리말에서 생성하므로 손으로 고치지 않는다.

머리말에는 결정 28이 적은 상태와 번복 관계에 더해 `topic`, `summary`, `owners`를 둔다. 옛 Decisions 표의 주제, 현재 내용, 소유 문서를 생성 결과에 남기기 위해서다. 결정 28의 "상태·번복 관계만"을 넓게 읽은 것이므로 사용자가 다르게 정하면 머리말과 생성기(`Tools/Agent/decisions.cjs`의 `REQUIRED`)를 고친다. 결정 사이의 관계는 나중 결정의 머리말에만 적고, "결정 7에서 보완" 같은 반대 방향은 생성기가 계산한다.

1. `node Tools/Agent/decisions.cjs next`로 다음 번호를 확인하고 파일을 만든다. 머리말에는 `number`, `date`, `title`, `topic`, `status`, `summary`, `owners`를 쓰고, 진행 상태가 있으면 `progress`를 쓴다. 본문은 `# 날짜 — 결정 N: 제목` 아래에 배경, 결정, 트레이드오프 세 절로 이유를 적는다. 기존 파일의 머리말 형식을 따른다.
2. 과거 결정의 본문은 고치지 않는다. 앞선 결정을 바꾸면 새 결정의 머리말에 관계(`refines` 구체화, `amends` 보완, `supersedes_part` 일부 대체, `supersedes` 대체, `lifts_hold` 보류 해제)와 대상 번호를 적고, 본문에 "결정 M을 무엇으로 대체한다"와 이유를 쓴다. 앞선 결정은 머리말의 `status`와 `summary`만 현재 상태에 맞게 고친다. `status`는 `유효`, `유효(부분)`, `번복`, `완료`, `미확인` 중 하나다.
3. `node Tools/Agent/decisions.cjs generate --write`로 `Docs/Decisions.md`를 다시 만들고 `node Tools/Agent/decisions.cjs check`가 통과하는지 확인한다. 검사는 머리말, 번호 순서, 관계 대상, 생성 결과의 일치, 저장소의 모든 `결정 N` 링크를 본다.
4. 확정 수치나 인터페이스가 생겼으면 사양서에 옮기고 결정 번호를 붙인다.
