# 시스템 학습 문서

ccl8의 시스템을 실제 플레이 상황과 Git의 이전·현재 코드로 읽는 학습 문서다. [인벤토리 UI](InventoryUI.md)를 기준 예시로 삼고 [시스템 학습 문서 작성 규칙](../SystemExplanationGuide.md)을 따른다. 각 장은 필요한 UE 개념, 책임을 바꾼 이유, 내부 호출, 권한·수명, 실패·종료, 대안, 확인 방법과 해설이 있는 질문을 담는다.

## 읽는 순서

게임 실행 흐름은 이동·재스폰 → GAS 전투 → 적 AI → 캠페인 → 마을 서비스 순으로 읽는다. 플레이어의 지속 상태는 아이템·인벤토리 → 장비 → 성장 → 무기 → 세션·저장 순서가 적합하다.

화면 구현은 공용 UI 기반 → 인벤토리 UI 예시 → HUD·대화 → UI 연출 요청 → 지도 → 로컬 연출 순서로 연결한다. 삶의 시뮬레이션은 Agent 상태 → Agent 판단·기억 → 생활·경제 → Actor·StateTree·Mass 순서로 읽는다.

콘텐츠 제작·검증은 앞선 기능이 에셋과 실행 결과로 이어지는 경로다. 환경·생태계는 현재 코드와 구현 계획을 나누어 읽는다. 엔진 API의 확인 위치는 [엔진 근거](EngineEvidence.md)에 모았다.

## 전체 목록

| 문서 | 학습 범위 |
|---|---|
| [이동·카메라·사망·재스폰](MovementRespawn.md) | Controller·Character·GameMode와 소유자별 복구 |
| [GAS 전투](GASCombat.md) | ASC·Attribute·Ability와 권위 판정 |
| [적 AI](EnemyAI.md) | StateTree·탐지·패턴·전투 연결 |
| [캠페인](Campaign.md) | 원정 단계·보스·승리·재접속 상태 |
| [마을 서비스](VillageServices.md) | NPC 대화·상점·퀘스트·튜토리얼 |
| [아이템·인벤토리](ItemInventory.md) | 정의·GUID·Fast Array·이동·복원 |
| [장비·부착](Equipment.md) | 태그 슬롯·양손 점유·외형·효과 수명 |
| [성장·훈련](Progression.md) | 포인트·지속 효과·재스폰 |
| [무기·투사체](Weapons.md) | Action·발사·명중 규칙·소비 |
| [세션·저장](SessionSave.md) | 메뉴·호스트·접속·체크포인트·검증 |
| [공용 UI 기반](UIFramework.md) | Registry·Context·Root·Handle·입력 |
| [인벤토리 UI](InventoryUI.md) | 사용자가 승인한 전후 비교 예시와 Slate 문법 |
| [HUD·대화](HUDDialogue.md) | MVVM 표시 값·선택 입력·수명 |
| [UI 연출 요청](UIPresentation.md) | 숨김·입력 차단 합성·소유권·해제 |
| [지도](Map.md) | 공개 조건·좌표 투영·미니맵·전체 지도 |
| [로컬 연출](Cinematics.md) | 카메라·음향·조명·취소·복원 |
| [Agent 상태](AgentState.md) | ID·Feature·버전·Lease·Snapshot |
| [Agent 판단·기억](AgentDecision.md) | 관측·믿음·관계·욕구·선택 유지 |
| [생활·경제](LifeEconomy.md) | 기회·거래·목표·실패 경험·저장 |
| [Actor·StateTree·Mass](AgentExecution.md) | 실행 예약·축약 실행·표현 전환 |
| [콘텐츠 제작·검증](ContentValidation.md) | 에디터 도구·에셋·자동 검사·패키징 |
| [환경·생태계 계획](Environment.md) | 공통 시간·천체·날씨·지형·눈·물·불·차폐·생명 |
| [UE 5.8 엔진 근거](EngineEvidence.md) | 여러 장에서 사용하는 엔진 API의 실제 소스 위치 |

## 코드 범위와 문서 대응

‘다른 시스템 전부’의 범위는 아래와 같이 고정한 게임 코드 전체와 연결된 콘텐츠·검증 경로다. 파일 하나마다 같은 설명을 반복하는 대신 하나의 동작을 시작부터 종료까지 따라갈 수 있게 묶었다.

| 코드 영역 | 담당 문서 |
|---|---|
| `CCLCharacter`·`CCLPlayerController`·`CCLGameModeBase` | 이동·재스폰, GAS 전투, 세션·저장 |
| `CCLPlayerState`·`CCLHUD` | GAS 전투, 성장, HUD·대화 |
| `AbilitySystem/` | GAS 전투, 성장, 장비 |
| `Combat/` | GAS 전투, 적 AI, 무기·투사체 |
| `Actions/` | 무기·투사체 |
| `Campaign/` | 캠페인, 마을 서비스, Actor·StateTree·Mass |
| `Items/` | 아이템·인벤토리, 장비, 성장, 무기 |
| `Session/` | 세션·저장 |
| `UI/Core/` | 공용 UI 기반, UI 연출 요청 |
| `UI/`의 인벤토리·세션·HUD·지도 화면 | 각 화면 문서, 세션·저장, 지도 |
| `Agents/`의 Store·Feature·Snapshot | Agent 상태, Agent 판단·기억 |
| `Agents/`의 Decision·Goal·Economy·LifeSimulation·Account | Agent 판단·기억, 생활·경제, 마을 서비스 |
| `Agents/`의 Component·AIController·WorldSubsystem·MassBridge | Agent 상태, Actor·StateTree·Mass |
| `Map/`·`Presentation/` | 지도, 로컬 연출 |
| `Editor/`·`CCL.cpp` | 콘텐츠 제작·검증, 장비 |
| `Tests/`·`Tools/Validation/` | 각 장의 확인 방법, 콘텐츠 제작·검증 |
| 자연 환경과 생태계 계획 | 환경·생태계 계획 |

`CCL.Build.cs`·Target·프로젝트 설정은 기능 모듈·플러그인·검사 포함 조건의 근거이며, 미커밋 설정 변경을 이번 학습 기준에 합치지 않았다. Graft·Graphify·개인 도구 설정의 운영은 [AgentContextTools](../AgentContextTools.md)가 소유한다. 이 목록은 개발 도구 자체의 신규 매뉴얼을 만드는 범위가 아니다.

## 비교 기준과 현재 상태의 의미

새 시스템 장의 게임 코드 기준은 `50e583859c832d6cb325e5e76beb8356e8c13959`다. 조사 시작 때 로컬 `main`과 로컬 원격 추적 `origin/main`이 같았고 별도 기능 브랜치는 없었다. 원격 서버의 최신 상태를 다시 조회한 결과는 아니다. 따라서 브랜치를 이동하지 않고 의미 있는 도입·변경 커밋의 실제 코드를 비교했다.

문서 작성 중 별도 환경 작업의 `aab86619b813db2af3d72365cae10fc6a15b0bdc`가 추가됐다. 환경 장은 그 문서의 준비·미검증 상태만 보충하고 게임 코드 비교 기준은 유지한다. 다른 작업의 미커밋 코드·설정은 이 문서 묶음의 구현 완료로 계산하지 않는다.

기존 InventoryUI 예시는 자체 비교 커밋을 유지한다. 각 장의 ‘현재’는 그 장이 지정한 커밋이며 이후 HEAD와 항상 같다는 뜻이 아니다. 코드 발췌의 줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다. 실제 코드 일부는 주변 검사·후반 정리를 본문에 설명한다.

## 사실·이유·검증을 읽는 방법

변경 사실은 코드와 Git 이력, 당시 요구는 Foundation·Decisions·DesignLog를 근거로 삼는다. 코드로 분석한 장단점은 당시 의사결정 기록과 구분한다. 최초 구현 전에는 코드가 없었다는 사실을 밝히며 가상의 이전 코드를 만들지 않는다.

구현 상태와 확정 사양의 원본은 [Guide](../Guide.md)가 가리키는 Foundation 문서다. 이 학습 묶음은 새 게임 규칙을 확정하거나 사용자의 학습 성취를 기록하지 않는다.

이번 작업은 문서·코드·설치 엔진 소스의 대조다. UE 5.8.3 소스 위치를 확인했지만 빌드·게임 실행·네트워크·패키징을 새로 수행하지 않았다. 과거 문서의 UE 5.9.0 등 다른 환경의 검증 결과는 당시 기록으로만 인용한다. 환경 기능의 준비 상태와 미검증 범위는 EnvironmentPlan을 따른다.
