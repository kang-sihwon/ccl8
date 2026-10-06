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
| [ContentFoundation](ContentFoundation.md) | 일반 적·보스 패턴과 NPC·상점·퀘스트·튜토리얼 |
| [ProgressionFoundation](ProgressionFoundation.md) | 인벤토리·장비·성장·스킬 UI의 설계와 검증 |
| [ArtDirection](ArtDirection.md) | 그래픽 스타일·시각 기준·아트 제작 초안 |
| [Coding](Coding.md) | 코드 설계·리뷰·빌드와 파일 수정 경계 |
| [WritingGuide](WritingGuide.md) | 문서 작성·검토·결정 기록 |
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

새 문서는 담당 지도에 등록한다. 일반 지식은 연결된 개인 볼트, ccl8 고유 결정은 이 저장소에 기록한다. 볼트가 연결되지 않았으면 외부 경로를 추측하지 않고 저장 없이 답한다.

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
