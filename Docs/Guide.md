# ccl8 안내

새 세션은 저장소 루트에서 시작한다. ccl8은 Orbis의 방향과 그래픽 스타일을 공유하는 단기 소울라이크 RPG다. 제작 범위와 역할은 [Direction](Direction.md), 세부 기획과 아트 초안은 문서 지도에서 확인한다. 멀티플레이 기본 흐름과 테스트 맵을 구현했으며 검증 상태는 [MultiplayerFoundation](MultiplayerFoundation.md)에 있다. 프로젝트 위치와 도구 경로는 [AgentContextTools](AgentContextTools.md)를 따른다. 기본 엔진 기준은 UE 5.8.1이며 실제 검증에 사용한 설치본은 작업 문서에 기록한다.

## 협업 규칙

1. AI는 기획, 아트 제안, 설계, 구현, 통합, 검증과 문서화를 주도한다. 사용자는 결과를 검토하고 수정 사항과 방향을 지시한다.
2. 합의된 제작 범위의 AI 파일 수정과 실행 경계는 Coding을 따른다. 작업 전 관련 기획·아트·사양 문서를 읽고, 필요한 초안을 작성한 뒤 구현한다.
3. 엔진 동작은 현재 엔진 소스에서 검증하고 `path:line`으로 근거를 남긴다. UE4와 UE5 차이를 구분하고 설계 대안에는 트레이드오프를 붙인다.
4. 확정된 결정은 DesignLog에 날짜와 이유를 추가하고 Decisions의 현재 상태를 갱신한다. 과거 결정을 소급 변경하지 않는다.
5. 기본은 동료 모드다. 훈련·힌트를 요청했을 때만 Mentoring 절차를 적용한다. 학습 평가는 실제 수행 근거를 따른다.

한국어 반말로 간결하게 답하고 코드·식별자는 영어로 유지한다. 사실, 추론, 제안과 미확인을 구분한다. 설계는 ccl8의 확정 방향에서 출발한다. Orbis의 모듈 구성·시스템 규모·수치 목표는 별도 판단 없이 적용하지 않는다.

## 문서 지도

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [Direction](Direction.md) | 제작 목표·확정 범위·역할 분담 |
| [GameDesign](GameDesign.md) | 플레이 흐름·전투·콘텐츠 기획 초안 |
| [MultiplayerFoundation](MultiplayerFoundation.md) | 스폰·이동·카메라·사망·재도전 구현과 실행·검증 기준 |
| [CombatFoundation](CombatFoundation.md) | 일반 몹·최소 전투의 확정 사양, 구현과 검증 상태 |
| [CampaignFoundation](CampaignFoundation.md) | 마을·일반 적·보스·승리 흐름의 구현과 검증 상태 |
| [Packaging](Packaging.md) | Windows 검토용 패키지의 생성·실행과 한계 |
| [SessionFoundation](SessionFoundation.md) | 시작·접속 메뉴, 저장 체크포인트·설정과 검증 |
| [ContentFoundation](ContentFoundation.md) | 일반 적·보스 패턴과 NPC·상점·퀘스트·튜토리얼 |
| [ProgressionFoundation](ProgressionFoundation.md) | 인벤토리·장비·성장·스킬 UI의 설계와 검증 |
| [UIFoundation](UIFoundation.md) | 장르 독립적인 UI 관리자·화면·문맥·레이어·연출 제어의 계약과 검증 |
| [EnvironmentPlan](EnvironmentPlan.md) | 천체·날씨·영구 지형·눈·물·불·차폐·생태계와 실험장 구현 계획, 단계별 현재 상태 |
| [영구 지형 통합과 검증](EnvironmentTerrain.md) | 단계 3의 다층 표면·완료 세대·경로·클라이언트 준비와 구역 05 조작·검증 |
| [AgentFoundation](AgentFoundation.md) | 개인의 삶·기억·목표·자원·판단과 Actor/Mass 수명의 구현 계약 |
| [생활 Agent·무기·지도 확인 안내](AgentReview-2026-10-08.md) | 직접 확인할 조작, 클래스 관계와 데이터 위치 |
| [장비·UI 확인 안내](UIReview-2026-10-08.md) | 2026-10-08 구현 요약, 직접 확인할 화면과 검증 범위 |
| [ArtDirection](ArtDirection.md) | 그래픽 스타일·시각 기준·아트 제작 초안 |
| [Coding](Coding.md) | 코드 설계·리뷰·빌드와 파일 수정 경계 |
| [WritingGuide](WritingGuide.md) | 문서 작성·검토·결정 기록 |
| [시스템 학습 문서 작성 규칙](SystemExplanationGuide.md) | 교사식 설명, Git 전후 코드 비교, 변경 이유와 검증의 작성 기준 |
| [인벤토리 UI 학습](Learning/InventoryUI.md) | Slate 문법부터 공용 화면 관리로의 변경 이유까지 설명하는 전체 예시 |
| [시스템 학습 문서 목록](Learning/README.md) | 전체 시스템 대응표·읽는 순서·고정한 비교 기준 |
| [이동·카메라·사망·재스폰 학습](Learning/MovementRespawn.md) | Controller·Character·GameMode와 소유자별 복구 |
| [GAS 전투 학습](Learning/GASCombat.md) | ASC·Attribute·Ability와 권위 판정 |
| [적 AI 학습](Learning/EnemyAI.md) | StateTree·탐지·패턴·전투 연결 |
| [캠페인 학습](Learning/Campaign.md) | 원정 단계·보스·승리·재접속 상태 |
| [마을 서비스 학습](Learning/VillageServices.md) | NPC 대화·상점·퀘스트·튜토리얼 |
| [아이템·인벤토리 학습](Learning/ItemInventory.md) | 정의·GUID·Fast Array·이동·복원 |
| [장비·부착 학습](Learning/Equipment.md) | 태그 슬롯·양손 점유·외형·효과 수명 |
| [성장·훈련 학습](Learning/Progression.md) | 포인트·지속 효과·재스폰 |
| [무기·투사체 학습](Learning/Weapons.md) | Action·발사·명중 규칙·소비 |
| [세션·저장 학습](Learning/SessionSave.md) | 메뉴·호스트·접속·체크포인트·검증 |
| [공용 UI 기반 학습](Learning/UIFramework.md) | Registry·Context·Root·Handle·입력 |
| [HUD·대화 학습](Learning/HUDDialogue.md) | MVVM 표시 값·선택 입력·수명 |
| [UI 연출 요청 학습](Learning/UIPresentation.md) | 숨김·입력 차단 합성·소유권·해제 |
| [지도 학습](Learning/Map.md) | 공개 조건·좌표 투영·미니맵·전체 지도 |
| [로컬 연출 학습](Learning/Cinematics.md) | 카메라·음향·조명·취소·복원 |
| [Agent 상태 학습](Learning/AgentState.md) | ID·Feature·버전·Lease·Snapshot |
| [Agent 판단·기억 학습](Learning/AgentDecision.md) | 관측·믿음·관계·욕구·선택 유지 |
| [생활·경제 학습](Learning/LifeEconomy.md) | 기회·거래·목표·실패 경험·저장 |
| [Actor·StateTree·Mass 학습](Learning/AgentExecution.md) | 실행 예약·축약 실행·표현 전환 |
| [콘텐츠 제작·검증 학습](Learning/ContentValidation.md) | 에디터 도구·에셋·자동 검사·패키징 |
| [환경·생태계 계획 학습](Learning/Environment.md) | 공통 시간·천체·날씨·지형·눈·물·불·차폐·생명 |
| [학습 문서 엔진 근거](Learning/EngineEvidence.md) | UE 5.8.3의 ASC·Fast Array·구조체·CommonUI·MVVM·Mass 소스 근거 |
| [VersionControl](VersionControl.md) | Git·LFS·커밋 주체와 동기화 |
| [AgentContextTools](AgentContextTools.md) | 도구 설치·설정·복구·진단 |
| [AgentToolingResearch](AgentToolingResearch.md) | AI 주도 제작용 도구 후보·근거·도입 제안 |
| [Setup](Setup.md) | 초기 구성 범위와 검증 결과 |
| [Decisions](Decisions.md) | 현재 유효한 제작·게임 설계 결정 |
| [DesignLog](DesignLog.md) | 게임 설계 결정의 이유와 번복 이력 |
| [Mentoring](Mentoring.md) | 훈련 요청과 학습 기록 절차 |
| [LearningLog](LearningLog.md) | 현재 학습 과제와 수행 증거 |
| [StudyRoadmap](StudyRoadmap.md) | 제작 일정 초안·완료 증거·다음 작업 |
| [DeliveryPlan](DeliveryPlan.md) | 단계별 구현 범위와 생성·빌드·검토·제출 절차 |
| [NarrativeConcept](NarrativeConcept.md) | 서사 작업 범위와 문서 지도 |

새 문서는 담당 지도에 등록한다. 일반 지식은 연결된 개인 볼트, ccl8 고유 결정은 이 저장소에 기록한다. 설계·구현·기술 조사·문제 해결에서는 [Obsidian 활용 절차](AgentContextTools.md#obsidian-활용-절차)에 따라 시작할 때 기존 노트를 참고하고 마무리할 때 검증한 재사용 지식을 반영한다.

## 도구 라우팅

| 작업 | 사용할 도구·스킬 |
|---|---|
| 프로젝트 코드 위치·심볼·호출 관계 | `graft` MCP 또는 CLI, 미생성·누락 시 실제 파일 검색 |
| 엔진 내부 동작·API·셰이더 | `graft_ue58`, 부족한 범위는 엔진 소스 직접 확인 |
| 코드 설계·인터페이스·모듈 경계 | `ccl8-design` |
| 여러 열린 설계 결정의 선택 | `grilling` |
| 문서 작성·결정·볼트 정리 | `ccl8-docs` |
| 에이전트 지침·스킬 편집 | `writing-for-agents` |
| 여러 문서의 관계·누락·영향 | `graphify`, `Tools/Graphify/graphify_ccl8.cmd` |
| 서사 조사·초안·검토·정본화 | `ccl8-narrative` |
| 학습 로드맵 검토 | `roadmap-review` |
| 볼트 문법 | `obsidian-markdown` |
| 코드 리뷰 | 사용 가능한 리뷰 도구, 수정 요청이 없으면 리뷰만 |

문서 그래프는 탐색 단서다. 원문을 확인하며 오래되거나 없다는 이유로 자동 재빌드하지 않는다. 다중 에이전트, 다른 채팅으로 메시지 보내기, 예약 작업은 사용자가 요청한 범위에서만 사용한다. 도구 호출 자체가 파일 편집이나 외부 전송 권한을 추가하지 않는다.

세션 시작 때 Git 상태와 현재 작업 문서를 확인한다. 동기화는 VersionControl에 따르고, 대화 이력만으로 완료·승인 상태를 추정하지 않는다.
