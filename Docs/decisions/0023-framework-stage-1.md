---
number: 23
date: 2026-10-11
title: "Framework 단계 1 착수와 선택 사용 검증"
topic: "공용 시스템 분리"
status: "유효"
summary: "단계 1의 `CCLEditor`, `KSIUI`, `KSIMap`을 프로젝트 모듈로 분리하고 최소 프로젝트로 선택 사용을 검증한다"
owners: "[FrameworkPlan](../FrameworkPlan.md)"
lifts_hold: [20, 22]
---

# 2026-10-11 — 결정 23: Framework 단계 1 착수와 선택 사용 검증

### 배경

사용자가 ContentFoundation 보완 이후 FrameworkPlan 작업의 진행을 지시했다. 결정 22의 단계별 순서에 따라 에디터 도구, UI 코어와 지도를 첫 분리 대상으로 삼았다.

### 결정

결정 20·22의 구현 보류를 단계 1에 한해 해제한다. `CCLEditor`, `KSIUI`, `KSIMap`을 프로젝트 모듈로 분리하며 단계 2는 단계 1 결과를 사용자가 검토한 뒤 진행한다. 타입 이름과 네이티브 경로는 CoreRedirects로 연결한다. 게임 화면·콘텐츠와 서버 권위는 CCL에 남긴다.

KSIUI는 기존 화면·입력 수명 계약을 위해 CommonUI·UMG·Enhanced Input을 사용한다. MVVM은 기능별 표시 모델이 있는 CCL에 남긴다. KSIMap은 엔진 기본 모듈만 사용하며 KSIUI에 의존하지 않는다. 모듈을 별도 최소 프로젝트로 복사해 단독 선택 사용을 검증하고, 검증한 소스와 엔진 범위를 FrameworkPlan에 기록한다. 공용 저장소·submodule 배포 방식은 아직 정하지 않는다.

### 트레이드오프

독립 모듈은 CCL로 향하는 역방향 의존을 빌드 경계에서 제한한다. 타입 리다이렉트, DLL export와 모듈별 빌드·회귀 검증은 추가로 관리해야 한다. CommonUI를 유지하면 기존 입력·위젯 풀 동작을 활용하지만 UI 단독 프로젝트도 필요한 엔진 플러그인을 활성화해야 한다. 검사용 복사는 독립성을 확인하기 쉽지만 여러 실제 프로젝트를 계속 동기화하는 배포 수단으로 확정하지 않는다.
