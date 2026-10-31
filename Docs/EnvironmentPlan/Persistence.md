# 환경 저장과 네트워크

세계 저장 참여 계약, 맵 도메인, 저장과 스트리밍, 복제의 원칙이다.

## 세계 저장 참여와 맵 도메인

2026-10-11에 세계 저장을 형식 7로 바꿨다. 세계 코덱은 시계·환경·생활 같은 코어와 이름 있는 구간의 봉투만 소유한다. 플레이어(`Players`, 참가 순서 0)와 보관함(`Inventories`, 참가 순서 1)은 `IKSIWorldParticipant`로 자기 구간을 수집·준비·확정·취소한다. 구간 검증기는 `FKSIWorldSectionRegistry`에 등록하며, 앞 구간이 청구한 아이템 GUID를 뒤 구간이 다시 청구하면 거부한다. 형식 6 이하의 플레이어·보관함 덩어리는 읽을 때 같은 이름의 구간으로 옮긴다. 원격 플레이어의 자동 신원 연결과 복원 시 접속자 유지는 [원격 플레이어 신원 연결과 복원 시 접속자 유지](../SessionFoundation/PlayerRestore/Identity.md#원격-플레이어-신원-연결과-복원-시-접속자-유지)가 소유한다.

현재 저장 참여 계약은 `KSICore`의 `IKSIWorldParticipant`와 `FKSIWorldSectionRegistry`에 있다. 표면·생활 검증 자료는 CCL의 선택 문맥으로 연결하고, 저장 형식 7과 참가 순서는 유지한다. 분리 결정과 검증은 [FrameworkPlan](../FrameworkPlan/Stage2.md#단계-2-구현-설계와-검증-기준)이 소유한다.

저장 참여 계약은 저장의 Capture/Prepare/Commit/Publish/Cancel만 맡는다. 환경 시간이 필요한 구현만 CCL의 `ICCLWorldAdvanceParticipant`를 함께 구현하고 `RegisterAdvanceParticipant`로 따로 등록한다. 2026-10-11 설계 후속 수정에서 나눈 계약이다. 지금은 플레이어와 보관함의 저장 서브시스템이 두 인터페이스를 모두 구현한다. 각 목록은 같은 기존 우선순위로 정렬하며 모든 Commit 뒤에 Publish한다. 저장 구간 검증 문맥에 있는 Surface·Life 참조는 그대로 유지한다. 검증 근거는 [ProgressionFoundation의 2026-10-11 기록](../ProgressionFoundation/Verification.md#2026-10-11-설계-후속-수정)이다.

맵의 저장 도메인은 맵 이름이 아니라 `ACCLWorldEnvironmentConfig::Domain`으로 정한다. 설정 Actor가 없는 맵은 Campaign 도메인을 사용한다. 같은 설정 Actor가 생활 인구 Seed, 캐릭터의 눈 후처리 AnimBP, 실험 주민 클래스를 제공한다. 이전 구현은 맵 이름에 `Environment`가 있는지로 후처리를 켜고, 두 실험 맵을 뺀 모든 맵을 Campaign으로 취급했다. 날씨·화재 Niagara 시스템의 저작 함수는 런타임 표현 Actor에서 에디터 전용 `UCCLEnvironmentFXLibrary`로 옮겼다. Niagara Internal 헤더 경로는 Editor 빌드에만 추가하므로 게임·서버 타깃은 그 헤더를 사용하지 않는다. 에디터 저작은 여전히 비공개 속성 `FixedBounds`를 이름으로 찾으므로 엔진을 갱신할 때 다시 확인해야 한다.

## 저장·스트리밍·네트워크

세계 ID·기반 월드 버전·스키마·확정 시각·세대를 공통 헤더에 둔다. 지형·표면·개체군·Agent를 같은 확정 세대로 저장하고 완료 표식을 마지막에 기록한다. 중단 시 이전 완료 세대를 유지한다. 기존 UCCLAgentSessionStore와 새 저장 경로가 같은 삶을 각각 복원하지 않도록 하나로 연결한다.

v1-v5 세션은 기존 변환 후 Agent 시각을 새 공통 시각의 시작점으로 사용한다. 환경이 없는 저장은 정의된 초기 환경으로 시작한다. 기반 월드 버전이 다른 저장은 명시한 변환 없이 적용하지 않는다. 큰 청크는 별도 저장 단위로 분리하고 크기 제한·손상·부분 누락을 검사한다.

지역을 내릴 때 물질 총량과 세부 지형·눈길을 보존한다. 먼 지역은 저해상도 모델로 진행하며 누적 기후 입력과 배율 이력으로 상태를 맞춘다. 현재 날씨 하나로 과거 전체를 대신하지 않는다. World Partition과 실제 맵 구성을 확인한 뒤 로딩 어댑터를 선택한다.

서버는 관련 지역의 스냅샷과 리비전별 변경분을 복제한다. 누락·지연 접속·재접속은 스냅샷으로 회복한다. 늦은 클라이언트 하나 때문에 서버 전체를 멈추지 않는다. 해당 클라이언트는 충돌과 필수 표면이 준비된 지역부터 참여한다.
