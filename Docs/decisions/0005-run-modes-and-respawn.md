---
number: 5
date: 2026-10-06
title: "실행 환경과 개별 재스폰"
topic: "실행·재도전"
status: "유효"
summary: "평소에는 Standalone, 서버 검증은 Dedicated Server와 Listen Server에서 수행한다. 사망한 플레이어만 개별 재스폰한다"
owners: "[MultiplayerFoundation](../MultiplayerFoundation.md)"
refines: [3]
---

# 2026-10-06 — 결정 5: 실행 환경과 개별 재스폰

### 배경

사용자는 평소 작업은 Standalone에서 수행하고 서버 테스트는 Dedicated Server와 Listen Server에서 진행한다고 지정했다. 사망 처리는 사망한 플레이어만 개별 재스폰하는 안으로 구현하도록 지시했다.

### 결정

기본 흐름은 Standalone, Dedicated Server와 Listen Server에서 동작하도록 구현한다. 사망한 플레이어는 재도전 입력으로 시작 지점에 새 캐릭터를 생성하며 다른 플레이어의 진행은 유지한다. 결정 3의 초기 실행 계획을 이 방식으로 구체화한다. 최종 접속 인원과 협동·대전 규칙은 아직 미정이다.

사망과 캐릭터 생성은 서버가 결정한다. 서버가 없는 Standalone에서도 같은 권위 경로를 사용한다. 상세 설계와 검증 상태는 [MultiplayerFoundation](../MultiplayerFoundation.md)이 소유한다.

### 트레이드오프

Standalone은 평소 반복 작업을 단순하게 하지만 복제와 원격 입력 문제를 드러내지 못한다. 두 서버 형태에서 별도 검증이 필요하다. 개별 재스폰은 생존자의 조작을 유지하지만 파티 전체 재시작이나 동료 부활 규칙은 포함하지 않는다.
