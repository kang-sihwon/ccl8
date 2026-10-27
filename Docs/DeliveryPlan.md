# 단계별 구현과 검증 계획

이 문서는 제작 단계 하나를 제출하는 절차를 소유한다. 로드맵의 각 단계는 구현, 프로젝트 파일 생성, 빌드, 실행 검사와 코드 검토를 통과한 뒤 제출한다. 2026-10-07 사용자는 단계별 오류 수정과 commit·push 후 다음 단계로 진행하도록 지시했다.

제작 범위는 [Direction](Direction.md), 단계의 범위와 완료 기준은 [StudyRoadmap](StudyRoadmap.md)을 따른다. 단계별 결과와 검증 근거는 각 단계의 사양서가, 2026-10-11 UE 5.8.3 재검증은 [QA 기록](QA/FullReviewReverification-2026-10-11.md)이 소유한다. 자연 환경과 실험장의 다음 작업은 [EnvironmentPlan](EnvironmentPlan.md)을 따른다. 현재 기기의 엔진 버전은 실제 Build.version에서 다시 확인한다.

## 제출 절차

1. AI는 현재 단계의 사양과 변경 범위를 기록한다.
2. AI는 구현 후 `GenerateProjectFiles.bat`을 프로젝트 대상으로 실행한다. 명령은 [Coding](Coding.md)이 소유한다.
3. AI는 `CCLEditor Win64 Development`와 해당 단계의 추가 타깃을 빌드한다.
4. AI는 변경된 기능과 기존 기능의 회귀를 실행으로 확인한다. 실행 순서와 증거 기록은 `ccl8-verify` 스킬을 따른다.
5. AI는 변경 코드를 다시 검토하고 결함을 수정한다. 수정한 최종 코드에서 생성·빌드와 관련 실행 검사를 반복한다.
6. AI는 단계별 검증 근거와 미확인을 문서에 반영하고 의도한 파일만 commit·push한다. push 대상은 [VersionControl](VersionControl.md)을 따른다.

실패한 검사, 생략한 검사와 시각 검증이 남은 항목은 통과로 기록하지 않는다. 기기별 경로, 로그, 빌드 산출물과 개인 저장 파일은 제출하지 않는다. 엔진 전체 재빌드가 필요하면 작업량과 이유를 먼저 확인한다.
