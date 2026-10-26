---
number: 16
date: 2026-10-07
title: "장비 슬롯과 부착 위치의 태그 설정"
topic: "아이템·장비"
status: "유효"
progress: "태그 개편 실행 검증 완료"
summary: "슬롯과 부착 지점은 태그로 지정하고 양손 여부는 Weapon이 소유한다. 캐릭터별 AttachmentProfile과 장비 창은 ProgressionFoundation을 따른다"
owners: "[ProgressionFoundation](../ProgressionFoundation.md)"
refines: [15]
---

# 2026-10-07 — 결정 16: 장비 슬롯과 부착 위치의 태그 설정

### 배경

사용자는 방어구도 사용하는 Equip에 양손 여부가 있는 점을 지적하고, 해당 속성을 Weapon에 두도록 요청했다. 공통 부착 위치를 태그로 지정해 소켓 이름 입력 실수를 줄이는 방향을 제시한 뒤 적용을 승인했다.

### 결정

결정 15의 구조체 Fragment 구성을 유지한다. Equip은 허용·기본 슬롯을, Weapon은 한손·양손 사용 방식을 Gameplay Tag로 정의한다. Visual은 슬롯별 부착 지점 태그와 오프셋을 보관한다. 캐릭터의 AttachmentProfile이 지점 태그를 대상 메시의 실제 소켓으로 해석한다. 프로필 편집에는 메시에서 가져온 소켓 선택 목록, 아이템 미리보기와 데이터 검증을 제공한다.

양손 무기는 두 손 슬롯을 점유하되 오른손에 메시 하나를 표시한다. 보조 손 IK와 새 수납 동작은 포함하지 않는다. 기존 구조체 에셋과 저장 형식의 호환 경로를 유지한다. 계약과 검증 상태의 정본은 [ProgressionFoundation](../ProgressionFoundation.md), 저장 형식은 [SessionFoundation](../SessionFoundation.md)이다.

### 트레이드오프

아이템이 메시의 소켓 이름을 직접 알 필요가 없어지고 캐릭터별 매핑을 한곳에서 편집할 수 있다. 대신 프로필과 실제 메시의 호환 여부, 빠진 매핑과 중복 태그를 검증해야 한다. 기존 슬롯 enum과 숨겨진 이전 필드는 호환 데이터를 읽는 동안 남는다.
