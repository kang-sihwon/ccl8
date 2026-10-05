# ccl8 Codex 작업 규칙

작업을 시작할 때 [Docs/Guide.md](Docs/Guide.md)를 읽고, 문서 지도가 가리키는 해당 작업의 규칙과 현황을 확인한다. 프로젝트 스킬은 `.agents/skills/`에 있고 Claude Code의 공통 스킬과 같은 내용으로 유지한다.

코드 위치·심볼·호출 관계는 프로젝트 `graft`, UE 엔진 내부 동작은 `graft_ue58` MCP로 먼저 확인한다. 그래프가 없거나 결과가 불완전하면 `rg`와 실제 소스로 확인한다. UE 프로젝트는 `CCL.uproject`이고 기본 모듈은 `Source/CCL/`에 있다. 프로젝트 그래프는 사용자가 요청한 시점에 생성한다.

게임 코드·설정 수정 권한과 문서의 Git 처리 규칙은 각각 `Docs/Coding.md`, `Docs/VersionControl.md`가 소유한다.
