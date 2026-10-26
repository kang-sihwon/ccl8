---
number: 15
date: 2026-10-07
title: "구조체 Fragment와 양손 장비 체계"
topic: "아이템·장비"
status: "유효"
summary: "공통 정보는 ItemDefinition, 선택 기능은 FInstancedStruct로 구성한다. 양손 장비와 장비 창은 ProgressionFoundation의 계약을 따른다"
owners: "[ProgressionFoundation](../ProgressionFoundation.md)"
---

# 2026-10-07 — 결정 15: 구조체 Fragment와 양손 장비 체계

### 배경

사용자는 공통 표시 정보와 중첩 수를 ItemDefinition이 소유하고, 선택 기능은 구조체 Fragment로 구성하는 방향을 제시했다. 바닥에 놓을 StaticMesh는 Visual에 두고 SkeletalVisual이 상속하며, 무기 속성은 무기 Fragment 내부에 유지하도록 요청했다. 양손 장비와 손별 입력, 캐릭터 모델 양옆에 장비 슬롯을 배치하는 인벤토리 화면도 요청했다.

### 결정

기능 정의는 FInstancedStruct로 보관한다. 장착 부위와 한손·양손 점유, 기존 장비의 가방 반환, 손별 액션과 장비 창은 [ProgressionFoundation](../ProgressionFoundation.md)의 계약을 따른다. 아이템 소유·가방 보관은 Inventory, 장착 부위와 교체 규칙은 Loadout이 담당한다. 공유 정의와 개별 GUID 상태를 분리한다. 이 결정은 설계 방향의 확정이며 구현·에셋 변환·실행 검증 상태는 사양서에서 별도로 기록한다.

### 트레이드오프

값 기반 Fragment는 선택 기능을 하나의 배열에서 편집할 수 있지만 이전 UObject 에셋의 변환과 파생 구조체 조회가 필요하다. 양손 장비는 항목 하나와 슬롯 두 개를 연결하므로 반환 공간과 중복 효과를 함께 검증해야 한다. 캐릭터 캡처는 실제 장비 표현을 공유하지만 화면을 열고 있는 동안 추가 렌더링 비용이 든다.
