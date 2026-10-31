# 결함

지적의 분류와 기준 커밋은 [코드 구조 개선 계획](../CodeStructurePlan.md)을 따른다. 줄 번호는 기준 커밋 `55afa51`의 값이다.

## D1. 언로드 대체 저장의 참여자 구간 누락

`UCCLAgentWorldSubsystem::Deinitialize`는 맵 이동 저장이 실패한 경우 세계를 다시 저장해 세션 저장소에 넣는다 (`Source/CCL/Agents/CCLAgentWorldSubsystem.cpp:95-119`). 이때 인벤토리와 플레이어 저장 참여자가 이미 등록을 해제했으면, 그 구간이 빠진 세계가 정상 저장으로 남는다. 코드와 엔진 근거는 확인했고 실행 재현은 하지 않았다.

발생 경로는 다음과 같다.

1. 월드 해체가 시작되면 `OnWorldBeginTearDown`이 맵 이동 체크포인트를 저장한다 (`CCLAgentWorldSubsystem.cpp:223-241`). 지형 지역이 준비되지 않았으면 이 저장은 실패한다 (`CCLAgentWorldSubsystem.cpp:164-171`).
2. 참여자인 `UCCLInventoryPersistenceSubsystem`과 `UCCLPlayerPersistenceSubsystem`은 자기 `Deinitialize`에서 등록을 해제한다 (`Source/CCL/Items/CCLInventoryPersistenceSubsystem.cpp:92-99`, `Source/CCL/Session/CCLPlayerPersistenceSubsystem.cpp:64-71`).
3. 참여자가 Agent 서브시스템보다 먼저 해제되면 Agent의 대체 저장은 남은 참여자만 수집한다. `UCCLWorldSimulationSubsystem::Save`와 `CaptureParticipants`는 등록된 참여자를 순회할 뿐 누락을 검사하지 않는다 (`Source/CCL/Environment/CCLWorldSimulationSubsystem.cpp:323-357`, `:638-656`).
4. 스냅샷 검증은 존재하는 구간만 검사한다 (`Source/CCL/Environment/CCLWorldSnapshot.cpp:453-459`). 복원은 없는 구간을 빈 구간으로 처리한다 (`CCLWorldSimulationSubsystem.cpp:668`).

해제 순서는 엔진이 보장하지 않는다. UE 5.8.3은 서브시스템을 `Initialize` 호출 전에 맵에 넣는다 (`<Engine>/Source/Runtime/Engine/Private/Subsystems/SubsystemCollection.cpp:524-526`). 그래서 `InitializeDependency`로 끌려온 서브시스템은 끌어온 쪽보다 뒤에 들어간다. 해제는 이 맵의 순서대로 진행하며 의존 역순을 따르지 않는다 (`SubsystemCollection.cpp:349-358`). 인벤토리 저장 서브시스템이 먼저 생성되면 해제도 Agent보다 먼저 일어난다. 반면 모든 월드 서브시스템의 `PreDeinitialize`는 해제 전에 호출된다 (`<Engine>/Source/Runtime/Engine/Private/World.cpp:6474-6486`).

영향은 진행 손실이다. 보관함과 플레이어 프로필 구간이 빠진 세계가 세션 저장소에 들어가고, 다음 로드에서 그 상태가 비어 있는 채로 복원될 수 있다.

수정 방향은 두 가지를 함께 적용하는 것이다.

- 대체 저장을 `PreDeinitialize`로 옮기거나 없앤다. R1을 진행하면 이 책임은 `UCCLWorldSimulationSubsystem::PreDeinitialize`로 이동한다.
- 캡처할 때 등록된 구간은 비어 있어도 항상 기록한다. 현재 형식의 스냅샷에서는 등록된 구간의 누락을 거부하고, 누락 허용은 이전 형식의 변환에만 둔다.

검증은 자동 검사로 한다. 참여자 등록을 먼저 해제한 뒤 Agent의 대체 저장 경로를 실행하고, 결과 스냅샷에 인벤토리·플레이어 구간이 있는지 또는 저장이 거부되는지 확인한다. 수정 전 코드에서 이 검사가 실패하는지도 먼저 확인한다.

## D2. 열 노출 포인터의 역참조

`UCCLWorldSimulationSubsystem::AdvancePending`은 `UCCLThermalExposureSubsystem` 포인터를 얻은 뒤 (`CCLWorldSimulationSubsystem.cpp:245`) 준비 람다 안에서만 null을 검사한다 (`:250`). 실패 경로의 `Cancel` (`:268`)과 성공 경로의 `Commit` (`:290`)은 검사 없이 역참조한다. 두 서브시스템의 생성 조건이 같고 `Initialize`에서 의존 초기화를 하므로 (`:35`) 현재는 null이 되지 않는다. 생성 조건이 달라지면 크래시가 난다. R1에서 열 노출을 참여자로 옮기면 이 코드는 사라진다.
