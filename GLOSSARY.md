# ccl8 용어

여러 문서와 코드에 걸쳐 쓰는 시스템·제작 용어다. 뜻은 짧게 적고 세부는 소유 문서가 맡는다. 서사 용어와 서사 상태는 [서사 용어](Docs/Narrative/Glossary.md)가 소유한다. 새 용어는 두 문서 이상에서 같은 뜻으로 쓰일 때 추가한다.

## 작업과 검증

| 용어 | 뜻 | 소유 |
|---|---|---|
| 결정 N | `Docs/decisions/NNNN-*.md` 파일 하나로 기록한 확정 결정. 현재 상태는 Decisions에서 본다 | [결정 파일](Docs/decisions/), [Decisions](Docs/Decisions.md) |
| 작업 지도 | 여러 세션에 걸친 작업 하나의 목적지, 진행, 정한 것, 아직 모르는 것, 범위 밖을 적은 파일 | [진행 중인 작업](Docs/Work/README.md) |
| 조각 | 작업 지도의 진행 단위. 먼저 끝나야 하는 조각과 완료 조건을 함께 적는다 | [진행 중인 작업](Docs/Work/README.md) |
| 정본 | 한 주제를 소유해 다른 사본이 따라야 하는 문서나 폴더. 스킬 정본은 `.agents/skills/`다 | [AGENTS.md](AGENTS.md) |
| 스모크 | `Tools/Validation/`의 기능별 실행 검사 스크립트 | [ccl8-verify](.agents/skills/ccl8-verify/SKILL.md) |
| 증거 강도 | 실행 검증이 게임 동작에 들어간 입력 경로의 분류. `EI`, `Ability`, `Slate`, `internal`, `MCP`로 적는다 | [ccl8-verify](.agents/skills/ccl8-verify/SKILL.md) |
| 바닥선 검사 | diff에서 새 억제 주석, 비활성화·삭제된 테스트와 검사, 늘어난 시간 제한, 미구현 표시를 찾는 검사 | `Tools/Agent/floor-guard.cjs` |
| 실험장 | `/Game/Maps/Tests`의 전용 레벨에서 시험 하나와 표본 수를 골라 정지한 세계에서 시작하는 환경 검증 레벨 | [기능별 환경 실험장](Docs/EnvironmentPlan/Laboratories.md) |

## 코드 구조

| 용어 | 뜻 | 소유 |
|---|---|---|
| KSI, CCL | 공용 모듈은 `KSI{System}`, 공용 타입은 `KSI` 접두사, 게임 코드는 `CCL` 접두사를 쓴다. 결정 22 | [FrameworkPlan](Docs/FrameworkPlan.md) |
| ItemDefinition, Fragment | 아이템의 공통 정보는 ItemDefinition, 선택 기능은 `FInstancedStruct` Fragment로 구성한다. 결정 9, 결정 15 | [ProgressionFoundation](Docs/ProgressionFoundation.md) |
| AttachmentProfile | 캐릭터별로 장비 부착 지점 태그를 실제 메시 소켓으로 해석하는 데이터. 결정 16 | [ProgressionFoundation](Docs/ProgressionFoundation.md) |
| Feature | Agent가 선택적으로 가지는 타입·버전·의존성·Migration 단위의 상태. 결정 18 | [AgentFoundation](Docs/AgentFoundation.md) |
| 후보 확정 | `CCLWorldAdvance`와 참가자가 같은 후보 구간을 계산하고 모두 성공할 때만 반영하는 세계 진행 방식. 실패하면 원본을 유지한다 | [EnvironmentPlan](Docs/EnvironmentPlan.md) |
| 완료 세대 | 세계·지역 저장 파일을 함께 검증한 뒤 게시한 저장 묶음. 최신 세대가 손상되면 이전 완료 세대로 복구한다 | [영구 지형 통합과 검증](Docs/EnvironmentPlan/Terrain.md) |
| 세계 프로필 키 | 서버가 첫 접속에서 발급한 비밀 키의 해시. LAN·직접 접속의 재접속 구분이며 인증이 아니다. 결정 21 | [SessionFoundation](Docs/SessionFoundation.md) |
