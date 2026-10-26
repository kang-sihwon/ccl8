# ccl8 문서 지도

작업에 필요한 문서를 여기서 찾는다. 한 주제는 한 문서가 소유하고, 새 문서는 해당 묶음에 한 줄로 등록한다. 서사 문서의 소유는 [NarrativeConcept](NarrativeConcept.md)가 정한다. 진입점과 협업 규칙은 저장소 루트의 [AGENTS.md](../AGENTS.md)가 소유한다.

## 방향과 기획

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [Direction](Direction.md) | 제작 목표·확정 범위·역할 분담 |
| [GameDesign](GameDesign.md) | 플레이 흐름·전투·콘텐츠 기획 초안 |
| [ArtDirection](ArtDirection.md) | 그래픽 스타일·시각 기준·아트 제작 초안 |
| [NarrativeConcept](NarrativeConcept.md) | 서사 작업 범위와 문서 지도 |

## 시스템 사양과 검증 상태

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [MultiplayerFoundation](MultiplayerFoundation.md) | 스폰·이동·카메라·사망·재도전 구현과 실행·검증 기준 |
| [CombatFoundation](CombatFoundation.md) | 일반 몹·최소 전투의 확정 사양, 구현과 검증 상태 |
| [CampaignFoundation](CampaignFoundation.md) | 마을·일반 적·보스·승리 흐름의 구현과 검증 상태 |
| [SessionFoundation](SessionFoundation.md) | 시작·접속 메뉴, 저장 체크포인트·설정과 검증 |
| [ContentFoundation](ContentFoundation.md) | 일반 적·보스 패턴과 NPC·상점·퀘스트·튜토리얼 |
| [ProgressionFoundation](ProgressionFoundation.md) | 인벤토리·장비·성장·스킬 UI의 설계와 검증 |
| [UIFoundation](UIFoundation.md) | 장르 독립적인 UI 관리자·화면·문맥·레이어·연출 제어의 계약과 검증 |
| [AgentFoundation](AgentFoundation.md) | 개인의 삶·기억·목표·자원·판단과 Actor/Mass 수명의 구현 계약 |
| [Packaging](Packaging.md) | Windows 검토용 패키지의 생성·실행과 한계 |

## 자연 환경

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [EnvironmentPlan](EnvironmentPlan.md) | 천체·날씨·영구 지형·눈·물·불·차폐·생태계와 실험장 구현 계획, 단계별 현재 상태 |
| [기능별 환경 실험장](EnvironmentLaboratories.md) | 전용 레벨·단일 시험과 표본 수 선택, 표시·시간 조작·리소스 제출 범위 |
| [자연 날씨 계산과 검증](EnvironmentWeather.md) | 지역 기후·천체 일조·대기 수분과 물·눈 연결, 구역 02 조작·검증 |
| [영구 지형 통합과 검증](EnvironmentTerrain.md) | 단계 3의 다층 표면·완료 세대·경로·클라이언트 준비와 구역 05 조작·검증 |
| [물 순환 설계와 검증](EnvironmentWater.md) | 보존량·경계 유량·물과 지형의 확정·저장·실험장 구현 |
| [깊은 눈과 캐릭터 이동](EnvironmentSnow.md) | 단계 5의 적설·압축·흔적·CMC 예측·발 IK와 구역 03 조작·검증 |
| [불·열·공간 환경](EnvironmentFire.md) | 단계 7의 연소 코어·수지 검증과 표면·환기 통합 계약 |
| [생태계·생애](EnvironmentEcology.md) | 단계 8의 개체군·개체 전환·식생·부패·노화 구현과 검증 |
| [집계 생물의 영양 상태](EnvironmentNutrition.md) | 단계 8의 집계 에너지·먹이·기아, 예약 상태 이전·저장과 규모 검증 |

## 코드 구조

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [FrameworkPlan](FrameworkPlan.md) | 공용 시스템 후보·Framework 명명·독립성 검증과 분리 보류·재검토 기준 |
| [CodeStructurePlan](CodeStructurePlan.md) | 2026-10-11 전체 설계 검토의 결함·설계 위험과 세계 소유권·저장·진입점 구조의 개선 순서 |

## 날짜 붙은 QA·검토 보고서

작성 시점의 기록이다. 현재 상태는 위 사양 문서에서 확인한다.

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [환경 실험장 PIE QA](EnvironmentPIEQA-2026-10-12.md) | 시간·천체 배율 조작의 MCP 실행 검사, 최초 메뉴 입력 정책 누락과 종료 오류 분석 |
| [종합 QA 수정과 검증](GameplayQAFixes-2026-10-12.md) | 최초 메뉴 입력·종료 수명·방패 배치·실험 안내·NPC 선택 수정과 검증 |
| [환경·전투·멀티플레이·Campaign 종합 QA](FullGameplayMCPQA-2026-10-12.md) | 두 환경 맵 1-11번, 전투·2인 네트워크·원정 완료·저장 복원의 직접 PIE 검사와 결함·미확인 범위 |
| [생활 Agent·무기·지도 확인 안내](AgentReview-2026-10-08.md) | 직접 확인할 조작, 클래스 관계와 데이터 위치 |
| [장비·UI 확인 안내](UIReview-2026-10-08.md) | 2026-10-08 구현 요약, 직접 확인할 화면과 검증 범위 |
| [코드 설계 후속 수정](CodeDesignFixes-2026-10-11.md) | 장비 일관성·카탈로그 식별·기능과 시간 진행 분리·표면 공유 발행본 검증 |

## 결정

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [Decisions](Decisions.md) | 현재 유효한 제작·게임 설계 결정의 표. 결정 파일의 머리말에서 생성한다 |
| [결정 파일](decisions/) | 결정 1건당 파일 하나. 배경·결정·트레이드오프와 머리말의 상태·관계 |

## 계획과 진행

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [진행 중인 작업](Work/README.md) | 여러 세션에 걸친 작업의 재개 지점과 작업별 지도. 결정 28의 에이전트 작업 체계 재구성이 첫 작업 |
| [StudyRoadmap](StudyRoadmap.md) | 제작 일정 초안·완료 증거·다음 작업 |
| [DeliveryPlan](DeliveryPlan.md) | 단계별 구현 범위와 생성·빌드·검토·제출 절차 |
| [Setup](Setup.md) | 초기 구성 범위와 검증 결과 |

## 작업 규칙

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [Coding](Coding.md) | 코드 설계·리뷰·빌드와 파일 수정 경계 |
| [WritingGuide](WritingGuide.md) | 문서 작성·검토·결정 기록 |
| [시스템 학습 문서 작성 규칙](SystemExplanationGuide.md) | 교사식 설명, Git 전후 코드 비교, 변경 이유와 검증의 작성 기준 |
| [VersionControl](VersionControl.md) | Git·LFS·커밋 주체와 동기화 |
| [Mentoring](Mentoring.md) | 훈련 요청과 학습 기록 절차 |

## 도구

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [AgentContextTools](AgentContextTools.md) | 도구 설치·설정·복구·진단, 검사 도구와 확인 훅의 명령 |
| [AgentToolingResearch](AgentToolingResearch.md) | AI 주도 제작용 도구 후보·근거·도입 제안, 외부 스킬 묶음 세 개의 통합 검토 |

## 학습

| 문서 | 소유 내용과 읽는 시점 |
|---|---|
| [LearningLog](LearningLog.md) | 현재 학습 과제와 수행 증거 |
| [시스템 학습 문서 목록](Learning/README.md) | 전체 시스템 대응표·읽는 순서·고정한 비교 기준 |
| [인벤토리 UI 학습](Learning/InventoryUI.md) | Slate 문법부터 공용 화면 관리로의 변경 이유까지 설명하는 전체 예시 |
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
| [자연환경 시스템 학습](Learning/Environment.md) | 단계 0-7의 메인 문서·하위 장·기존 게임 연결 |
| [공통 시간 학습](Learning/EnvironmentTime.md) | 독립 Tick에서 생활·환경의 후보 확정으로 변경 |
| [천체·차폐 학습](Learning/EnvironmentCelestial.md) | 공통 시각의 관측·다층 표면·세 채널 차폐 |
| [영구 지형 학습](Learning/EnvironmentTerrain.md) | 밀도 값·비동기 충돌·경로·클라이언트 준비 |
| [물 순환 학습](Learning/EnvironmentWater.md) | 물 환산 체적·유량 예산·최신 지형 재배치 |
| [깊은 눈·이동 학습](Learning/EnvironmentSnow.md) | 압축·질량·접촉 중복·CMC 예측·IK |
| [자연 날씨 학습](Learning/EnvironmentWeather.md) | 수동 입력에서 기후·지속 대기 상태로 확장 |
| [불·열 학습](Learning/EnvironmentFire.md) | 연료·열·수분 수지와 후속 GAS 피해 |
| [실내 환기 학습](Learning/EnvironmentAir.md) | 여러 표면의 공통 공기·교환 예산·기하 차폐 |
| [환경 저장·실험장 학습](Learning/EnvironmentPersistence.md) | 완료 세대·맵 전환·서버 요청·검증 구분 |
| [환경 학습 비교 근거](Learning/EnvironmentEvidence.md) | 브랜치·전체 커밋·작업 트리 해시·기존 보고서 |
| [시간 분할 정밀도 학습](Learning/EnvironmentTimeAccuracy.md) | 두 시계의 끝점·분할 예산·취소와 저장 재개 |
| [세계 저장 참여자 학습](Learning/EnvironmentRestoreParticipants.md) | 형식 7 구간·중첩 확정·역순 취소·알림 |
| [환경 맵 설정 학습](Learning/EnvironmentMapConfiguration.md) | 저장 도메인과 눈 후처리의 데이터 선택 |
| [환경 후속 비교 근거](Learning/EnvironmentFollowupEvidence.md) | 후속 커밋·브랜치·기존 자동화·패키지 검증 한계 |
| [환경 착수 전 학습 기록](Learning/EnvironmentPlanningHistory.md) | 구현 이전의 고정 기준 해설 보존 |
| [학습 문서 엔진 근거](Learning/EngineEvidence.md) | UE 5.8.3의 ASC·Fast Array·구조체·CommonUI·MVVM·Mass 소스 근거 |
