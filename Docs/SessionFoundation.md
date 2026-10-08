# 시작·접속·저장 흐름

단계 6은 시작 화면에서 혼자 시작, 방 생성, 주소로 접속, 저장 불러오기와 종료를 제공한다. 게임 중 Esc는 같은 메뉴를 열어 저장·설정·시작 화면 복귀를 제공한다. 전투 중에도 월드는 계속 실행되며 메뉴는 해당 플레이어의 입력만 막는다.

## 책임과 수명

| 구성 | 책임과 수명 |
|---|---|
| GameInstance | 접속 결과·보류한 불러오기와 LocalPlayer별 메뉴 핸들 |
| SessionMenuScreen·문맥 | 버튼·주소 입력·설정·성공과 실패 표시, 공용 UI 관리자의 월드 범위 화면 |
| SessionRecord와 Codec | 버전·무결성·수량·정의 ID 검증, Actor와 UI를 참조하지 않는 값 |
| Inventory·Loadout·Expedition | 검증된 개인 상태 적용, 서버 PlayerState 수명 |
| CampaignDirector | 처치 체크포인트와 남은 적 재구성, 서버 월드 수명 |

시작·일시정지 메뉴는 `UCCLSessionMenuScreen`에서 기존 Slate 본문을 표시한다. 포커스와 입력 모드는 CommonUI가 적용하며 GameInstance가 별도로 `SetInputMode`를 호출하지 않는다. 게임 중 뒤로 가기는 해당 플레이어의 메뉴를 닫고, 시작 화면에서는 메뉴를 유지한다. 메뉴를 여는 플레이어만 인벤토리·대화창을 닫는다. 맵 이동 전에는 모든 로컬 메뉴를 닫는다. 구현과 이전 검사 근거는 [UIFoundation](UIFoundation.md)을 따른다.

```mermaid
sequenceDiagram
    Player->>GameInstance: Load local slot
    GameInstance->>SessionCodec: Check size, checksum, version and definitions
    SessionCodec-->>GameInstance: Validated record or error
    GameInstance->>World: Open fresh campaign
    GameInstance->>PlayerState: Restore inventory, equipment, skills and quest
    GameInstance->>CampaignDirector: Restore defeated guard mask and victory
    GameInstance-->>Player: Result in menu
```

## 이번 저장 정책

단일 로컬 슬롯에 호스트의 인벤토리 GUID·정의 ID·수량, 장비, 학습 스킬, 포인트, 금화·퀘스트와 원정 체크포인트를 저장한다. 접속한 손님의 개인 상태는 이번 파일에 포함하지 않는다. 손님은 호스트의 파일 저장·불러오기를 요청할 수 없다. 최종 계정·협동 보상 소유권을 확정하는 정책은 아니다.

체크포인트는 처치한 경비병, 보스 격파 여부와 남은 마을 보급품이다. 호스트는 마을에서 완전한 체력으로 시작한다. Agent 스냅샷이 있는 저장은 생활 NPC와 전투 적의 부상을 복원한다. 살아 있던 전투 적은 체크포인트 위치에 재생성하며 공격 타이머는 다시 계산한다. Agent 스냅샷이 없는 이전 저장은 적의 기본 체력을 사용한다. 사망·행동 중에는 저장을 거부한다. 정확한 적 위치·공격 타이머를 직렬화하는 방식보다 재시작 상태의 일관성을 우선한 초안이다.

파일은 엔진의 플랫폼 저장 슬롯 API로 기록하되 UObject 클래스 경로를 역직렬화하지 않는다. 제한된 크기의 JSON 값 앞에 형식 식별자와 CRC를 붙인다. CRC는 우발적인 손상 감지용이며 변조 방지 장치가 아니다. 필수 필드·버전·알려진 정의 ID·수량·GUID 중복·장비 참조·스킬과 체크포인트 조합을 검증한 뒤 새 월드에 적용한다. 실패한 파일은 자동으로 덮어쓰지 않는다.

현재 저장 버전은 5다. 아이템 GUID별 `LoadedAmmo`를 기록하며 형식 1-4는 탄창을 비운 상태로 변환한다. 탄약 값은 해당 무기의 탄창 용량을 넘을 수 없다. 부위는 `Equipment.Slot.*` 문자열로 기록한다. 형식 3의 enum 이름 또는 숫자는 구체적인 슬롯 태그로 변환하고, 알 수 없는 태그·상위 태그·다른 범주의 태그는 거부한다. 형식 1-3 변환과 형식 4 왕복 자동 검사, 별도 프로세스 저장·복원을 통과했다. 검사 근거는 [ProgressionFoundation](ProgressionFoundation.md)의 태그 개편 검사 표를 따른다.

각 항목에 가방 슬롯을 기록하고 장착 항목은 `INDEX_NONE`으로 표시한다. 부위별 장착 목록은 GUID를 참조하며 양손 장비는 같은 GUID로 두 손을 모두 차지해야 한다. 형식 1은 배열 순서로 가방 칸을 만들고, 형식 1·2의 단일 장비 참조는 새 부위 목록으로 변환한다. 중복 부위·잘못된 장비 종류·불완전한 양손 점유·어느 부위에도 연결되지 않은 장착 항목은 거부한다. 구현 근거는 `Source/CCL/Session/CCLSessionRecord.cpp`다.

태그 변경 전 형식 3에서는 2026-10-07 별도 프로세스로 가방 칸·장착 상태·GUID·수량·효과를 저장·복원했고 양손 장비 직렬화와 불완전한 점유 거부도 통과했다. 근거는 `Saved/StageValidation/Equipment-Session.log`와 `Saved/Tests/UIVisual/1280x720-20261007-170902/game.log`다.

주소 입력은 호스트 이름 또는 IPv4와 선택적인 포트만 허용한다. URL 옵션·경로·공백·범위를 벗어난 포트는 거부한다. LAN 또는 직접 접속이 가능한 호스트 주소를 쓰며 매치메이킹·계정·NAT 중계는 이번 범위가 아니다. 접속·이동 실패는 엔진 이벤트를 받아 메뉴에 표시한다.

설정은 엔진 GameUserSettings의 화면 품질과 창 모드를 사용한다. 입력은 메뉴 버튼으로 적용하고 저장하며 다음 실행에도 유지한다. 원격 서버나 다른 플레이어의 설정을 바꾸지 않는다.

## 기존 구현 검증 결과

2026-10-07 UE 5.9에서 프로젝트 생성 BAT·CCLEditor 빌드·시작 맵 저장을 통과했다. 서로 다른 프로세스의 저장·불러오기에서 GUID·수량·장비·두 스킬·체력·금화·퀘스트와 부분 처치 상태를 확인했다. 이미 수집한 보급품은 복원하지 않았다. 메뉴 복귀 후 재불러오기, 남은 경비병 처치로 보스 진입, 승리 저장·복원 후 적이 없는 상태도 통과했다.

잘못된 주소, 손상된 실제 검증 슬롯, 모르는 버전·정의, 음수 수량, 중복 GUID와 없는 장비 참조를 거부했다. 화면 품질 변경과 설정 파일 재읽기로 저장을 확인했다. 별도 호스트·클라이언트가 메뉴의 접속 경로로 연결됐고 손님의 호스트 저장 요청은 거부됐다. 존재하지 않는 포트의 접속 실패도 메뉴에 표시됐다.

실제 Slate 첫 버튼에 Enter 입력을 보내 새 원정을 시작했다. 전체 버튼의 수동 마우스 조작과 화면 모드 전환은 미확인이다. 화면 검토에서 글자 크기·문구 잘림·키보드 포커스를 수정한 뒤 최종 렌더링과 저장 검사를 다시 통과했다. 시작·불러온 인벤토리·게임 중 메뉴 화면을 확인했다. 콘텐츠 Standalone, 성장 Dedicated, 전투 Listen 회귀도 통과했다. 패키징 실행은 단계 7에서 별도로 검사한다.

| 검사 | 로컬 근거 |
|---|---|
| 생성·빌드·시작 맵 | `Saved/StageValidation/Stage6-Generate.log`, `Stage6-Build.log`, `Stage6-Assets.log` |
| 최종 저장·불러오기·키보드·화면 | `Stage6-Rendered-Final.log`, `Saved/Tests/SessionSmoke/`의 최종 실행 원본 로그 |
| 방 생성·접속, 접속 실패 | `Stage6-Network.log`, `Stage6-Failure.log` |
| 콘텐츠·성장·전투 회귀 | `Stage6-Content.log`, `Stage6-Progression.log`, `Stage6-Combat.log` |
| 화면 | `Saved/Tests/SessionVisual/` |

표에서 파일명만 쓴 로그는 `Saved/StageValidation/` 기준이다. 테스트는 `CCL_Validation` 슬롯을 쓰며 일반 플레이의 `CCL_Checkpoint` 슬롯과 구분한다. 로컬 저장 파일은 Git에 넣지 않는다. 엔진 API 근거는 `<Engine>/Source/Runtime/Engine/Private/GameplayStatics.cpp:2215`의 `SaveDataToSlot`과 같은 파일 2316행의 `LoadDataFromSlot` 경로와 `Engine.h:2390`의 `OnNetworkFailure` 이벤트다.


Agent 생활 저장 통합은 결정 18과 [AgentFoundation](AgentFoundation.md)을 따른다. v5에는 선택적 `AgentSimulation` 스냅샷과 `AccountId`가 추가된다. 계정이 포함된 저장의 이전 `Coins` 필드는 0이며, 옛 저장에만 금화 이관 값으로 쓰인다. 생활 스냅샷은 Unreal 구조체 기반 로컬 저장이므로 JSON의 Definition 허용 목록과 별개로 Feature 타입·버전·교차 참조를 검증한다. 신뢰할 수 없는 외부 저장 파일을 안전하게 읽는 보안 경계로 사용하지 않는다.
