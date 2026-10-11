# 저장과 실험장: 같은 세계를 복원하고 조작 결과를 확인하기

세계 형식 7의 이름 있는 구간과 참여자별 복원·취소는 [세계 저장 참여자](EnvironmentRestoreParticipants.md), 맵의 저장 영역 선택은 [환경 맵 설정](EnvironmentMapConfiguration.md)으로 이어진다. 기존 본문의 커밋·사본 기준은 유지한다.

환경 저장은 시계·생활·표면·지형을 같은 완료 상태로 묶고, 실험장은 그 계약을 직접 조작하게 한다. 이 장은 기존 생활 저장에서 세계 저장으로의 확장과 UI 요청이 서버의 실제 변경으로 이어지는 흐름을 설명한다.

## 비교 기준과 단계 0의 의미

범위는 자연환경 0·1단계와 3-7단계의 공통 저장·검증이다. 비교 기준은 `50e5838` → `64a7549` → `7307465` → `0408d94`, 작업 트리의 실험 조작이며 전체 해시·브랜치 관계·작업 트리 식별값은 [비교 근거](EnvironmentEvidence.md)에 있다. `WT-2026-10-10`은 문서 작성 중 읽은 미커밋 소스 사본을 뜻한다. 이후 수정된 HEAD와 동일하다고 가정하지 않는다.

단계 0은 기능 구현 전에 사용할 엔진, 프로젝트 경로, 테스트 포함 여부와 기존 실패를 확인하는 작업이다. `Tools/Validation/environment_preflight.py`의 파일 존재 검사는 진단이며 실제 빌드 성공과 다르다. 실행 가능한 엔진으로 프로젝트 파일 생성·Editor 빌드·기존 회귀를 확인한 결과가 [환경 계획](../EnvironmentPlan.md)에 따로 남아 있다. 이번 문서 작성에서는 설치본의 Build.version이 5.8.3임을 읽었고 게임은 실행하지 않았다.

`UWorldSubsystem`은 월드 수명, `UGameInstanceSubsystem`은 맵 이동을 넘는 게임 인스턴스 수명이다. Snapshot은 지속할 값을 묶은 자료이고 Generation은 서로 맞는 저장 파일들의 묶음이다. 실행 중 UObject 포인터를 파일의 영구 신원으로 보관하지 않는다.

## 생활 저장에서 세계 저장으로

예전 `UCCLAgentWorldSubsystem::Save`는 서버 여부를 확인하고 Actor 위치를 생활 기록에 반영한 뒤 생활만 직렬화했다.

출처: `50e5838`, [CCLAgentWorldSubsystem.cpp](../../Source/CCL/Agents/CCLAgentWorldSubsystem.cpp), `return Simulation.Save(Bytes);`, 100줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
return Simulation.Save(Bytes);
```

환경 연결 뒤 같은 진입점의 끝이 바뀌었다.

출처: `a817082`, [CCLAgentWorldSubsystem.cpp](../../Source/CCL/Agents/CCLAgentWorldSubsystem.cpp), `auto* WorldSimulation = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();`, 126-127줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
auto* WorldSimulation = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
return WorldSimulation && WorldSimulation->Save(Simulation, Bytes, Error);
```

이 발췌는 Save 내부의 연결 부분이다. 앞의 권위·실행 상태 검사와 Actor 위치 갱신을 없앤 것이 아니다. WorldSimulation이 시계·환경·생활의 같은 완료 상태를 캡처한다. 새 외부 호출을 한 줄 추가한 것으로 설명을 끝내지 않고 [CCLWorldSnapshot.cpp](../../Source/CCL/Environment/CCLWorldSnapshot.cpp)의 직렬화와 검증까지 따라가야 한다.

[서버 시간 연결 기록](../EnvironmentPlan.md#서버-시간과-통합-저장-연결-설계)은 세션 버전 6에서 기존 Agent 저장 필드에 통합 스냅샷을 넣는 이전 방식을 설명한다. 세계 스키마와 내부 표면 버전은 또 다른 번호다. 하나가 증가했다고 나머지도 같은 번호가 되지 않는다. 현재 작업 트리에는 후속 생태·플레이어 저장 확장이 있으므로 그 최신 번호를 7단계 당시 형식으로 소급하지 않는다.

## 여러 파일의 완료 세대

지형까지 저장하면 파일 하나의 쓰기 성공으로 끝나지 않는다. 세계 파일만 새로 쓰고 지형 파일은 이전 상태로 남으면 서로 유효한 파일이어도 결합할 수 없다. `FCCLWorldGenerationStore`는 별도 세대 디렉터리에 세계·지역 파일을 쓰고 CRC와 문맥을 검사한 뒤 완료 표식을 게시한다.

출처: `0408d94`, [CCLWorldGenerationStore.cpp](../../Source/CCL/Environment/CCLWorldGenerationStore.cpp), `if (!WriteFlushed(Directory / TEXT("complete.tmp")`, 115-120줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
if (!WriteFlushed(Directory / TEXT("complete.tmp"), Marker)
    || !IFileManager::Get().Move(*(Directory / TEXT("complete.bin")), *(Directory / TEXT("complete.tmp")), false, false, false, true))
{
    Error = TEXT("Cannot publish the completion marker; previous generation retained.");
    return false;
}
```

이 코드는 파일 쓰기가 끝난 뒤 실행한다. 읽는 쪽은 `complete.bin`과 모든 파일이 유효한 가장 높은 세대를 찾는다. 최신 세대가 불완전하거나 손상됐으면 이전 완료 세대를 검토한다. 모든 세대가 잘못됐으면 실행 중 상태를 유지하고 실패를 반환한다. CRC는 손상 검출이며 인증 수단이 아니다. 실제 전원 차단 뒤 디스크 내구성까지 증명한 기록도 아니다.

불변 세대를 보관하면 중단 복구를 설명하기 쉽다. 반면 디스크 사용량이 늘고 정리 정책이 필요하다. 완료 표식의 의미와 실제 내구성은 분리해 검증한다.

## 맵 전환과 World 종료

`SaveSessionForTravel`은 준비 중인 지형이 있으면 맵 전환용 저장을 거부한다. 모든 지역이 확정됐을 때 세계와 지형을 같은 묶음으로 GameInstance 수명의 세션 저장소에 넣는다. 두 환경 맵은 서로 다른 도메인으로 보관해 한 맵의 실험이 다른 맵의 저장을 덮지 않게 한다.

새 월드는 해당 도메인의 묶음을 읽고 처음부터 저장 지형으로 충돌을 준비한다. 임시 기본 지형을 먼저 게시하지 않는다. 일반 종료는 `OnWorldBeginTearDown`에서 준비 후보를 취소하고 확정 상태를 남긴다. 뒤늦은 Deinitialize가 세계 바이트만 덮어 같은 묶음의 지형과 어긋나지 않게 한다. 이 메모리 보존과 새 프로세스에서의 디스크 복원은 서로 다른 시험이다.

## 실험 UI에서 서버까지

`7307465`에서 두 실험장과 12개 구역 정의가 추가됐다. 후속 기능이 없는 시점에는 조작을 비활성화했고 각 단계에서 실제 계산·표현을 연결했다. 화면 등록·문맥·핸들은 [공용 UI](UIFramework.md)의 기존 계약을 사용한다. 화면이 실험 상태의 정본을 소유하지 않는다.

출처: `WT-2026-10-10`, [CCLExperimentPlayerController.cpp](../../Source/CCL/Environment/CCLExperimentPlayerController.cpp), `void ACCLExperimentPlayerController::ServerExperiment_Implementation`, 148-155줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
void ACCLExperimentPlayerController::ServerExperiment_Implementation(ECCLExperimentAction Action, FName CaseId,
    FGuid Generation, FGuid RunId, uint64 TerrainSerial)
{
    FString Message;
    auto* Director = ACCLExperimentDirector::Find(GetWorld());
    const bool bAccepted = Director && Director->Execute(this, Action, CaseId, Generation, RunId, Message, TerrainSerial);
    ClientExperimentResponse(Message.IsEmpty() ? (bAccepted ? TEXT("조작 요청을 처리했다.") : TEXT("조작 요청을 거부했다.")) : Message);
}
```

소유 PlayerController의 서버 RPC가 Director에 전달한다. Director는 서버 권한, 월드 도메인, 준비 상태, 세대·실행 ID와 조작 담당자를 검사한다. 순간이동 같은 관찰자 허용 요청과 상태를 바꾸는 담당자 요청은 구분한다. 지형 조작은 게시 순번도 검사하므로 오래된 화면이 새 지형에 낡은 명령을 적용하지 않는다. 응답은 클라이언트 화면 메시지로 돌아온다.

F7은 실험 화면 열기·닫기다. 일반 UI 뒤로 가기는 PIE에서 F6, 독립 게임에서는 Esc로 구분한다. 입력 소유자가 화면으로 바뀌면 Pawn의 게임 입력에만 묶은 열기 키가 닫기까지 전달되지 않을 수 있다. [UI 기반](UIFramework.md)의 후속 절과 [실험 메뉴 기록](../EnvironmentPlan.md#실험-메뉴-조작과-검증)이 그 연결을 설명한다.

## 단계 7까지 직접 따라갈 순서

다음은 기존 구현과 검사 기록에 근거한 재현 안내다. 이 문서를 쓰면서 새로 실행한 결과는 아니다.

1. 환경 맵에서 F7을 열고 실험 구역과 기대 결과를 읽는다. 시간 조작은 격리 Scenario의 제한을 확인한다.
2. 천체·차폐에서 시각·문을 바꿔 일조와 노출의 차이를 본다. 변경 후에도 같은 완료 시각과 세대인지 확인한다.
3. 구역 05에서 지형을 편집하고 충돌·경로·참가자 준비를 기다린다. 구역 04의 물과 함께 저장·복원한다.
4. 구역 03에서 메뉴를 닫고 눈을 걸어 압축 흔적을 남긴다. 저장 후 융해하고 다시 불러와 흔적·수지를 비교한다.
5. 구역 02에서 자연·강제 비·강제 눈을 바꾼다. 지붕 아래와 열린 셀의 물 유입을 비교한다.
6. 구역 10·11에서 점화·소화·문 환기를 시험한다. 세계 배율 0과 전체 일시 정지를 구분한다.

## 검증의 해석과 이해 점검

소스 존재, 자동 검사, 실제 서버 실행, 화면, 새 프로세스 저장, 패키지 실행은 서로 다른 근거다. NullRHI 실행으로 시각 품질을 판정하지 않는다. Editor의 `-server` 실행을 Server 타깃 패키지 검증으로 쓰지 않는다. 실행 결과의 소유 문서는 [환경 계획](../EnvironmentPlan.md), [지형](../EnvironmentTerrain.md), [물](../EnvironmentWater.md), [눈](../EnvironmentSnow.md), [날씨](../EnvironmentWeather.md), [불](../EnvironmentFire.md)이다.

**CRC가 맞는 파일 두 개면 함께 복원해도 될까?** 세계 ID·세대·완료 시각이 다를 수 있다. 각 파일의 유효성과 파일 사이의 일치 조건을 함께 검사해야 한다.

**화면의 성공 응답이면 모든 클라이언트 충돌도 준비됐을까?** 서버 요청 수락과 각 수신자의 준비는 별개다. 화면 응답의 의미를 실제 게시·수신 상태와 구분해야 한다.
