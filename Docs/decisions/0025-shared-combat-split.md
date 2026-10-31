---
number: 25
date: 2026-10-12
title: "공용 전투와 게임 정책의 분리"
topic: "공용 시스템 분리"
status: "유효"
summary: "KSICombat은 GAS를 필수로 사용한다. 피해·팀·스태미나·가드·패링과 무기·탄약은 CCL에 둔다"
owners: "[FrameworkPlan](../FrameworkPlan.md)"
lifts_hold: [24]
---

# 2026-10-12 — 결정 25: 공용 전투와 게임 정책의 분리

### 배경

사용자가 Framework 단계 2 검증·제출 이후 단계 3 구현과 검증·문서 정리·push를 지시했다. 적중 문맥에는 팀이, ASC에는 사망 태그가, 공통 효과 부여에는 게임 Magnitude 태그가 들어 있었다. 전투·능력·행동 코드를 그대로 옮기면 공용 모듈이 CCL 정책을 요구하므로 이 경계를 먼저 분리했다.

### 결정

결정 24의 단계 3 보류를 해제한다. `KSICombat`은 공격 식별·대상·거리·차폐·중복 검사, 적중 정책과 수신 계약, GAS 입력·부여·효과 적용, Source별 행동 수명을 소유한다. GameplayAbilities·GameplayTags·GameplayTasks를 필수 엔진 의존으로 둔다. CCL이나 다른 KSI 모듈은 요구하지 않는다.

피해·팀·스태미나·가드·패링·AttributeSet과 무기·탄약은 CCL이 소유한다. 발사 당시 팀은 게임 구조체를 FInstancedStruct 값으로 전달한다. UCCLCombatDefinition·UCCLAbilitySystemComponent·UCCLAbilitySet은 게임 파생형으로 유지하고 기존 에셋 경로를 보존한다. 옮긴 타입은 KSI 이름과 CoreRedirects를 사용한다. Source 행동 목록은 별도 복제하지 않으며 입력·능력 예측과 효과 복제는 GAS가 처리한다.

원본 에셋 로드, CCL 회귀와 패키지 실행에 더해, KSICombat만 복사한 별도 프로젝트에서 함정·표적·행동 수명을 검증했다. 검사 범위와 근거는 [FrameworkPlan](../FrameworkPlan/Stage3.md#단계-3-구현과-검증)이 소유한다. 단계 4와 공용 저장소 배포는 착수하지 않는다.

### 트레이드오프

GAS를 유지하면 기존 예측·태그·효과 수명을 재사용하고 게임 동작을 보존하기 쉽다. 대신 GAS가 없는 프로젝트에 KSICombat을 그대로 적용할 수 없다. 범용 효과 인터페이스를 따로 만들고 GAS 연결 모듈을 추가하는 대안은 공개 계약과 검증 조합이 늘어나므로 이번 분리에서는 선택하지 않았다.

게임 파생형과 값 문맥은 공용 모듈의 역방향 의존을 끊지만 게임이 공급할 정책과 자료를 별도로 검증해야 한다. ActionSet의 네이티브 클래스 이름이 달라져 이를 문자열 PrimaryAssetId로 저장하는 외부 사용자는 추가 변환이 필요하다. CCL의 기존 저장은 해당 식별자를 기록하지 않으며 저장 형식을 유지했다.
