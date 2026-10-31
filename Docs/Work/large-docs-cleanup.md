# 큰 문서 내용 정리

하위 문서로 나눈 뒤에도 큰 문서를 크기 순으로 내용부터 정리하고, 정리된 결과에서 항목 경계가 보이면 나누는 작업이다. 근거는 [결정 33](../decisions/0033-large-document-cleanup-first.md)이다.

## 목적지

- 아래 진행의 문서마다 내용을 직접 읽고 현재 코드와 대조했다.
- 나뉜 문서의 부모가 하위를 "언제 읽는가"와 함께 가리킨다.
- 문서마다 `preserve.cjs` 대조 결과가 커밋 본문에 있다.
- `node Tools/Agent/docs.cjs check`와 `node Tools/Agent/decisions.cjs check`가 통과한다.
- "아직 모르는 것"을 사용자에게 물었고 답을 반영했다.

## 진행

크기는 2026-10-31 측정값이다.

- [x] 1. `EnvironmentPlan/Stages0to3` 62.6KB (없음). 완료 조건은 정리, 대조, 커밋이다. 이후 조각도 같다.
  - 결과: 31.8KB로 줄었고 `TimeAndLabs`, `Celestial`, `TerrainCore`로 나눴다. 가장 큰 하위는 12.2KB다. 시간, 지형, 환경 입력의 한도는 소스와 맞췄고, 천체 수, 표면 수, 진단 위치, 활성 청크 한도는 찾지 못해 당시 값으로 남겼다.
- [x] 2. `EnvironmentPlan/Ecology/Food` 56.4KB (없음).
  - 결과: 29.6KB로 줄었고 `State`, `Containers`, `Storage`, `Pickup`으로 나눴다. 가장 큰 하위는 11KB다. 묶음, 컨테이너, 거리 한도는 소스와 맞췄다.
- [x] 3. `ConceptImageToolTest/Results` 48.6KB (없음).
  - 결과: 전체 40.2KB로 줄었고 시험군별 5개 문서와 요약으로 나눴다. 요약은 4.2KB, 가장 큰 하위는 12.3KB다. 점수표와 관찰은 측정값이라 원문 그대로 옮겼다.
- [x] 4. `EnvironmentPlan` 31.2KB (없음).
  - 결과: 부모는 8.6KB로 줄었고 `Design`, `Persistence`, `Criteria`로 나눴다. 소유자 표의 없는 후보 이름을 실제 코드 이름으로 고쳤고, 컴파일되지 않는 인터페이스 초안을 뺐다.
- [x] 5. `SessionFoundation/PlayerRestore` 30.5KB (없음).
  - 결과: 21KB로 줄었고 `Profile`, `Loadout`, `Identity`로 나눴다. 가장 큰 하위는 8.7KB다. 뒤 절에서 뒤집힌 옛 복원 거부 계약은 현재 계약으로 합쳤고, 프로젝트 소스의 줄 번호는 함수 이름으로 바꿨다.
- [x] 6. `EnvironmentPlan/Ecology/Behavior` 26.3KB (없음).
  - 결과: 15.8KB로 줄었고 `Model`, `Space`, `Zone06`으로 나눴다. 가장 큰 하위는 6.1KB다. 설계와 검증 절을 기능별로 합쳤다.
- [x] 7. `CodeStructurePlan` 26.2KB (없음).
  - 결과: 부모는 7.2KB로 줄었고 `Defects`, `Risks`로 나눴다. 지적 본문은 기준 커밋의 기록이라 원문 그대로 옮겼다. D1, D2, R1이 지금도 남아 있는 것을 확인했다.
- [x] 8. `FrameworkPlan` 24.8KB (없음).
  - 결과: 부모는 6KB로 줄었고 `Modules`, `Naming`, `Process`로 나눴다. 본문은 원문 그대로 옮겼다.
- [x] 9. `CombatFoundation` 23.0KB (없음).
  - 결과: 전체 20.7KB, 부모 2.5KB로 줄었고 `Contracts`, `Actions`, `Verification`으로 나눴다. 규범 내용이 대부분이라 날짜별 실행 기록만 남은 사실 목록으로 줄였다.
- [x] 10. `EnvironmentPlan/Ecology` 22.6KB (없음).
  - 결과: 부모는 6.1KB로 줄었고 `Core`, `Representation`으로 나눴다. 끝난 다음 작업 문장과 소스 사본을 뺐다.
- [x] 11. `Packaging/Records` 22.1KB (없음).
  - 결과: 13KB로 줄었다. 패키지에서 얻은 규칙을 앞으로 모으고 빈 경로 표와 반복 한계 문장을 뺐다. 내용 경계가 없어 나누지 않았다.

## 정한 것

- 내용을 먼저 정리하고 그 결과로 나눈다. 크기 상한은 두지 않는다.
- 기록은 시간순 진행 서술을 빼고 시스템별 계약, 근거, 한도, 검증 결과로 다시 묶는다. 인계 목록, 끝난 다음 작업, 알림 관리, 당시 작업 트리 상태, 재실행 명령은 뺀다.

## 아직 모르는 것

없음.

## 범위 밖

- 20KB 미만 문서. 이 목록이 끝난 뒤 다시 측정해 정한다.
- `Docs/Learning/`, 결정 본문, QA 보고서, 서사 정본의 확정 영역.
