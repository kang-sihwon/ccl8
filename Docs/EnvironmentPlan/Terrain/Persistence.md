# 지형 저장과 맵 전환

완료 세대 저장, 복구, 맵 전환과 세션 보존을 바꿀 때 읽는다.

## 완료 세대와 복원

실험 저장 위치는 `Saved/EnvironmentGenerations/<슬롯>/g-<세대>-<GUID>/`다. 세계 파일, 지역별 파일과 CRC를 검증하고 파일을 닫은 다음 `complete.tmp`를 `complete.bin`으로 바꾼다. 복구는 완료 표식과 모든 파일이 유효한 가장 높은 세대를 선택한다. 세계 ID, 기반 버전, 세대와 두 완료 시각이 다른 지역 파일은 같은 묶음에 들어갈 수 없다.

최신 세대의 지역 파일이 손상되거나 누락됐거나 완료 표식이 없으면 이전 완료 세대를 찾는다. 모든 세대가 잘못됐으면 복구를 거부하고 실행 상태를 유지한다. 이 절차는 미완료 파일과 프로세스 중단을 다룬다. 실제 전원 차단 후 디스크 내구성까지 검증한 것은 아니다.

보호 한도는 지역 파일 32MiB, 한 묶음 64개 지역과 전체 256MiB다. 이전 디렉터리는 보존하며 자동 정리 정책은 아직 없다. 불변 세대는 실패 시 되돌리기 쉽지만 저장 공간이 계속 늘어난다. 현재 실험장의 복원은 한 지역을 대상으로 하며 여러 로드 지역의 동시 런타임 복원은 후속 확장 범위다.

## 맵 전환과 세션 복원

실험장의 맵 전환은 `SaveSessionForTravel`로 시계, Agent와 로드된 지형을 같은 게임 스레드에서 캡처한 뒤 시작한다. 모든 지역이 확정됐을 때만 후보 묶음을 `UCCLAgentSessionStore::TravelSnapshots`에 넣는다. 준비 중인 지역이 있으면 전환을 거부하며 작업 완료 후 재시도할 수 있다. 전환 요청이 접수된 뒤에는 추가 조작을 막는다.

`EnvironmentPlayground`와 `EnvironmentScenario`는 서로 다른 도메인에 저장한다. 새 월드는 해당 세계 스냅샷으로 시작하고 `InitializeTerrain`이 저장된 지형으로 첫 메시와 충돌을 준비한다. 지역 ID와 저장 문맥이 맞지 않으면 초기화를 거부한다. 평평한 기본 지형을 중간 상태로 게시하지 않는다. Actor와 Epoch는 새 월드에서 다시 만든다.

일반 월드 종료에서는 `OnWorldBeginTearDown`에서 진행 중 후보를 취소하고 확정된 상태를 보관한다. 명시적 전환에서 이미 캡처한 경우에는 그 묶음을 유지한다. 뒤늦은 `Deinitialize`가 세계 바이트만 덮어 지형과 세대를 섞지 않도록 막는다. 도메인을 초기화하면 그 도메인의 전환 묶음도 지운다.

이 경로는 프로세스 안의 세션 보존이다. 디스크 저장은 앞 절의 완료 세대 저장소가 담당한다. 맵별 마지막 묶음을 메모리에 유지하는 비용이 생기며, 현재 실험장의 초기 복원은 한 지역을 대상으로 한다. 공용 모듈 분리는 [FrameworkPlan](../../FrameworkPlan.md)이 소유한다.

엔진 근거는 `<Engine>/Source/Runtime/Engine/Private/World.cpp`의 `UWorld::BeginTearingDown`과 `UnrealEngine.cpp`의 맵 해제 순서다. 프로젝트 연결은 `Source/CCL/Agents/CCLAgentWorldSubsystem.cpp`와 `Source/CCL/Environment/CCLExperimentTerrain.cpp`에 있다.
