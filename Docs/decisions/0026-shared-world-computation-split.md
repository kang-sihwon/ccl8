---
number: 26
date: 2026-10-12
title: "공용 세계 계산과 게임 확정 경계의 분리"
topic: "공용 시스템 분리"
status: "유효"
progress: "단계 4 검증 완료"
summary: "프로젝트 안의 C++ 모듈로 단계 1-4의 추출·독립 실행·패키지 검증을 완료했다. 세계 후보 확정·권위·복제·삶 모델과 기존 저장 변환은 CCL에 둔다. KSIAgent는 Mass·StateTree를 요구하지 않는다"
owners: "[FrameworkPlan](../FrameworkPlan.md)"
lifts_hold: [25]
---

# 2026-10-12 — 결정 26: 공용 세계 계산과 게임 확정 경계의 분리

### 배경

사용자가 Framework 단계 4 구현 후 검증·문서 정리·push를 지시했다. 시간·지형·환경 계산과 Agent 지속 상태, 저장 파일 게시가 CCL 안에 있었다. 세계 조정과 삶 모델까지 함께 옮기면 독립 사용에 게임 정책이 따라오므로 재사용 계산과 게임의 확정 책임을 나눴다.

### 결정

결정 25의 단계 4 보류를 해제한다. KSIWorldTime·KSICelestial·KSITerrain·KSIAgent·KSIEnvironment·KSISave를 프로젝트 안의 Runtime 모듈로 분리한다. 지형과 환경의 공통 표면 질의는 KSICore에 둔다. KSIEnvironment는 KSICore의 시간 구간과 KSICelestial을 사용하며 시계 구현에는 의존하지 않는다. KSIAgent는 상태·Feature·작성권 수명을 제공하고 Mass·StateTree·GAS를 요구하지 않는다.

CCLWorldAdvance의 세계 후보 확정, TerrainRegion의 권위·게시·복제, Agent의 생활·경제·성격·생물과 Actor/Mass 연결은 게임이 소유한다. UCCLAgentSnapshot 클래스 경로와 기존 세계·지형 저장 형식은 유지한다. KSISave는 완료된 불변 파일 묶음을 게시·복구하며 게임 codec과 의미 검증은 동기 콜백으로 받는다. 공용 모듈에서 CCL 콘텐츠나 게임 클래스를 참조하지 않는다.

6개 최소 프로젝트의 독립 빌드·실행, 기존 에셋·저장 복원, CCL 회귀와 Game·실제 Server 패키지 검증을 완료했다. 구체 범위와 최초 실패·보완 근거는 [Framework 단계 4](../FrameworkPlan.md#단계-4-구현과-검증), 패키지 실행은 [Packaging](../Packaging.md#세계-모듈-분리-후-패키지)이 소유한다. 공용 저장소의 배포·동기화와 성능 목표 충족은 이번 결정에 포함하지 않는다.

### 트레이드오프

게임 어댑터와 의미 검증 콜백을 유지하면 기존 저장 형식과 확정 경계를 보존하면서 공용 모듈의 역방향 의존을 끊을 수 있다. 대신 모듈·export·직접 의존과 호스트 검증의 유지 비용이 늘어난다. 세계 조정자를 통째로 공용화하는 대안은 게임별 원자적 변경·복제 정책까지 공통 API로 만들어야 하므로 선택하지 않았다. 환경 계산을 더 작은 모듈로 나누는 대안도 보존 계약과 하위 단계의 상호작용을 여러 공개 경계로 나누는 비용이 있어 이번 범위에서는 보류한다.
