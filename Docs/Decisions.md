# 현재 설계 결정

초기 개발 환경의 확정 사항은 [Setup](Setup.md), 게임 제작 방향은 아래 소유 문서에서 확인한다.

| 주제 | 현재 결정 | 상태 | 근거 | 소유 문서 |
|---|---|---|---|---|
| 장르·기간·범위 | Orbis 방향과 그래픽 스타일을 공유하는 소울라이크 RPG를 3-4개월 안에 완성한다. 마을 하나, 보스 하나, 일반 몹 몇 종류, NPC 몇 명과 간단한 시나리오를 포함한다 | 유효 | 결정 1 | [Direction](Direction.md) |
| 협업 | AI가 제작을 주도하고 사용자는 검토·수정·방향 지시를 맡는다 | 유효 | 결정 2 | [Direction](Direction.md), [Coding](Coding.md) |
| 멀티플레이·작업 순서 | 멀티플레이 기본 흐름부터 전투·성장·콘텐츠를 구현한다. 시작 화면은 후반에 만든다. 초기 실행 환경은 결정 5로 구체화했다 | 유효 | 결정 3, 결정 5 | [GameDesign](GameDesign.md), [StudyRoadmap](StudyRoadmap.md) |
| 불리언 타입 | 헤더의 불리언 멤버 변수만 `uint8`을 사용한다. 함수 반환값·매개변수와 `.cpp` 지역 변수는 `bool`을 유지한다 | 유효 | 결정 4 | [Coding](Coding.md) |
| 실행·재도전 | 평소에는 Standalone, 서버 검증은 Dedicated Server와 Listen Server에서 수행한다. 사망한 플레이어만 개별 재스폰한다 | 유효 | 결정 5 | [MultiplayerFoundation](MultiplayerFoundation.md) |

위 결정으로 확정한 범위 밖의 [GameDesign](GameDesign.md), [ArtDirection](ArtDirection.md) 세부 제안과 [StudyRoadmap](StudyRoadmap.md)의 월별 배분은 후보로 유지한다. 결정 이력은 [DesignLog](DesignLog.md)에 기록한다.
