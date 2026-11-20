---
number: 70
date: 2026-11-19
title: "아이템 데이터의 변환 없는 재생성"
topic: "아이템·장비"
status: "유효"
progress: "구현 전"
summary: "결정 68·69 구현 때 기존 에셋과 저장을 변환하지 않는다. 정의는 생성 라이브러리로 다시 만들고, 저장 형식 번호를 올려 이전 저장을 거절한다"
owners: "[장비](../ProgressionFoundation/Equipment.md), [체크포인트](../SessionFoundation/Checkpoint.md)"
supersedes_part: [16]
---

# 2026-11-19 — 결정 70: 아이템 데이터의 변환 없는 재생성

### 배경

현재 `UCCLItemDefinition::PostLoad`가 이전 UObject Fragment를 구조체로 바꾸고, `FragmentSchemaVersion`과 숨겨진 이전 필드가 남아 있다. [결정 16](0016-tagged-slots-and-attachments.md)은 기존 구조체 에셋과 저장 형식의 호환 경로를 유지하도록 했다. 아이템 정의는 대부분 `CCLProgressionAssetLibrary`가 만든 시험 데이터이고 출시된 저장이 없다. 사용자는 변환 없이 새로 만드는 안을 골랐다.

### 결정

[결정 68](0068-item-fragment-hierarchy.md)·[결정 69](0069-hand-input-and-ability-sources.md)를 구현할 때 다음을 따른다. 결정 16의 호환 경로 유지는 이 범위에서 대체한다.

- 아이템 정의 에셋은 생성 라이브러리로 다시 만든다. `PostLoad` 변환, 이전 Fragment 클래스, 숨겨진 이전 필드를 지운다.
- 세션 저장 형식 번호를 올리고 이전 번호의 저장은 거절한다. 이전 저장을 읽어 변환하지 않는다.

### 트레이드오프

변환 코드와 이전 필드가 사라져 새 구조만 읽으면 된다. 대신 기존 로컬 저장과 손으로 고친 정의 에셋은 쓸 수 없게 된다. 생성 라이브러리 밖에서 만든 에셋이 있으면 다시 만들기 전에 목록을 확인해야 한다.
