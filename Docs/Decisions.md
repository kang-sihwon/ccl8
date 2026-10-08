# 현재 설계 결정

초기 개발 환경의 확정 사항은 [Setup](Setup.md), 게임 제작 방향은 아래 소유 문서에서 확인한다.

| 주제 | 현재 결정 | 상태 | 근거 | 소유 문서 |
|---|---|---|---|---|
| 장르·기간·범위 | Orbis 방향과 그래픽 스타일을 공유하는 소울라이크 RPG를 3-4개월 안에 완성한다. 마을 하나, 보스 하나, 일반 몹 몇 종류, NPC 몇 명과 간단한 시나리오를 포함한다 | 유효 | 결정 1 | [Direction](Direction.md) |
| 협업 | AI가 제작을 주도하고 사용자는 검토·수정·방향 지시를 맡는다 | 유효 | 결정 2 | [Direction](Direction.md), [Coding](Coding.md) |
| 멀티플레이·작업 순서 | 멀티플레이 기본 흐름부터 전투·성장·콘텐츠를 구현한다. 시작 화면은 후반에 만든다. 초기 실행 환경은 결정 5로 구체화했다 | 유효 | 결정 3, 결정 5 | [GameDesign](GameDesign.md), [StudyRoadmap](StudyRoadmap.md) |
| 불리언 타입 | 헤더의 불리언 멤버 변수만 `uint8`을 사용한다. 함수 반환값·매개변수와 `.cpp` 지역 변수는 `bool`을 유지한다 | 유효 | 결정 4 | [Coding](Coding.md) |
| 실행·재도전 | 평소에는 Standalone, 서버 검증은 Dedicated Server와 Listen Server에서 수행한다. 사망한 플레이어만 개별 재스폰한다 | 유효 | 결정 5 | [MultiplayerFoundation](MultiplayerFoundation.md) |
| 베이스 시스템 설계 | 장르에 독립적인 공통 계약과 콘텐츠 규칙을 분리하고 데이터·정책·기능 조합으로 구성한다. Item은 ItemDefinition과 Fragment 조합을 따른다 | 유효 | 결정 9 | [Coding](Coding.md) |
| 공용 UI | LocalPlayer별 관리자와 UMG·CommonUI를 사용한다. 화면 등록·문맥·핸들·수명과 중첩 표시 요청을 공용화하고 장르별 데이터는 기능 계층이 연결한다 | 유효, 구현 중 | 결정 17 | [UIFoundation](UIFoundation.md) |
| Agent의 삶 | 성격 6개·가치관 3개에서 시작하고 목표·경험·관계·실제 자원의 상호작용으로 개인의 삶을 누적한다. 선택적 Feature와 공통 실행 계약을 유지한다 | 유효, 구현 중 | 결정 18 | [AgentFoundation](AgentFoundation.md) |
| 아이템·장비 | 공통 정보는 ItemDefinition, 선택 기능은 FInstancedStruct로 구성한다. 슬롯과 부착 지점은 태그로 지정하고 양손 여부는 Weapon이 소유한다. 캐릭터별 AttachmentProfile과 장비 창은 ProgressionFoundation을 따른다 | 유효, 태그 개편 실행 검증 완료 | 결정 15, 결정 16 | [ProgressionFoundation](ProgressionFoundation.md) |
| 엔진 시스템 선택 | 명확히 불합리한 근거가 없으면 UE5 신규·현행 시스템을 기본 선택으로 사용한다. Nanite, World Partition, GAS와 Attribute·AttributeSet을 포함한다 | 유효 | 결정 10 | [Coding](Coding.md) |
| 방어 행동 | 최소 전투에 회피, 가드와 패링을 모두 포함한다. ASC 수명·조작·방어 결과와 초기 수치는 결정 11을 따른다 | 유효 | 결정 8, 결정 11 | [CombatFoundation](CombatFoundation.md) |
| 최소 전투 구현 | 플레이어 ASC는 PlayerState 소유, 마네킹·맨손으로 시작한다. 가드 붕괴 공격은 막고 경직하며 공격·가드·패링 중 카메라 수평 방향을 따른다 | 유효 | 결정 11 | [CombatFoundation](CombatFoundation.md) |
| 코드 작업 완료 검증 | 최종 코드 변경 후 GenerateProjectFiles.bat 실행에 성공하면 프로젝트를 빌드하고 두 결과를 각각 확인한다 | 유효 | 결정 12 | [Coding](Coding.md) |
| C++ 배치 | 헤더·CPP를 역할별 영역으로 정리한다. 헤더 영역은 빈 줄 하나로 구분하고 접근 지정자 다음 선언 묶음은 붙인다. CPP 제어문은 중괄호로 펼치고 독립된 처리 단계 사이를 빈 줄로 구분한다. 세부 배치는 Coding을 따른다 | 유효(부분 보완) | 결정 6, 결정 7, 결정 13, 결정 14 | [Coding](Coding.md) |

위 결정으로 확정한 범위 밖의 [GameDesign](GameDesign.md), [ArtDirection](ArtDirection.md) 세부 제안과 [StudyRoadmap](StudyRoadmap.md)의 월별 배분은 후보로 유지한다. 결정 이력은 [DesignLog](DesignLog.md)에 기록한다.
