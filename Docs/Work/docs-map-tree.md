# 문서 정리와 map+tree 구조

에이전트가 불필요한 내용을 적게 읽도록 문서를 하위부터 정리하고, 내용 경계로 나뉜 부모와 하위 군을 폴더로 묶는 작업이다. 근거는 [결정 30](../decisions/0030-document-cleanup-and-map-tree.md)이고, 원본은 태그 `docs-tree-v0`에 있다.

## 목적지

- `node Tools/Agent/docs.cjs check`와 `node Tools/Agent/decisions.cjs check`가 통과한다.
- 아래 진행의 문서가 모두 정리됐고, 문서마다 `preserve.cjs` 결과가 커밋 본문이나 이 지도에 있다.
- 나뉜 문서의 부모가 하위를 "언제 읽는가"와 함께 가리킨다.
- `Docs/README.md`와 `AGENTS.md`가 지도만 남기고, 같은 집계로 줄어든 읽기량을 확인했다.

## 진행

- [x] 1. 문서 검사기 (없음). `Tools/Agent/docs.cjs`와 시험을 추가한다. 완료 조건은 `docs.cjs check`가 통과하는 것이다.
  - 결과: 문서 149개, 오류 0건. 처음 발견한 깨진 링크 15건은 모두 [학습 문서의 알려진 깨진 링크](../Learning/README.md#알려진-깨진-링크)라 코드의 예외 목록에 넣었고, 고쳐지면 검사기가 목록 정리를 요구한다. `Tools/Agent/Tests` 전체 시험 38개 통과.
- [ ] 2. `EnvironmentWater` 정리 (1). 정리안 승인 뒤 적용한다. 완료 조건은 `preserve.cjs` 대조 통과다.
- [ ] 3. 환경 하위 나머지 (2). `Snow`, `Weather`, `Terrain`, `Nutrition`, `Fire`, `Laboratories`, `Ecology` 순으로 문서 하나씩 정리한다. 완료 조건은 문서마다 2번과 같다.
- [ ] 4. `EnvironmentPlan` 정리 (3). 하위와 겹치는 절을 지도로 바꾼다. 완료 조건은 2번과 같다.
- [ ] 5. 나머지 잎 (4). `*Foundation`, `Packaging` 등을 정리한다. 완료 조건은 2번과 같다.
- [ ] 6. `CodeStructurePlan`, `FrameworkPlan` 정리 (5). 완료 조건은 2번과 같다.
- [ ] 7. 규칙 문서 정리 (6). `WritingGuide`, `Coding`, `AgentContextTools` 등을 정리한다. 완료 조건은 2번과 같다.
- [ ] 8. `Docs/README.md`, `AGENTS.md` 지도화 (7). 완료 조건은 목적지의 네 번째 조건이다.

## 정한 것

- 크기 상한을 두지 않고 내용 경계로만 나눈다.
- 불필요한 내용과 중복을 먼저 정리하고, 그 결과를 기준으로 나눈다.
- 폴더는 미리 정하지 않는다. 나뉜 부모와 하위 군이 폴더가 된다.
- 하위 문서부터 정리한다.
- 정리안을 문서마다 사용자가 승인한다.

## 아직 모르는 것

- 부모 문서를 원래 경로에 둘지 폴더 안 `README.md`로 옮길지. 처음 문서를 나누기 전에 정한다. 제안은 원래 경로 유지다. 부모를 가리키는 링크가 깨지지 않는다.
- 실행 기록의 결론을 어디까지 사양에 남길지. `EnvironmentWater` 정리안에서 기준을 정한다.
- 5번과 7번 안의 문서 순서. 4번이 끝난 뒤 정한다.

## 범위 밖

- `Docs/Learning/`. [결정 28](../decisions/0028-agent-workflow-restructure.md)대로 시스템별 재작성에서 다룬다.
- 결정 본문, QA 보고서, 서사 정본의 확정 영역. 깨진 링크의 대상만 고친다.
- `.claude/settings.json`, `.mcp.json`, `.codex/config.toml`의 위치. 도구가 정한 경로다.
- 전역 설정과 프로젝트 설정의 graft 훅 중복. 전역 설정은 사용자가 정한다.
