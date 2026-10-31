# 영구 지형: 땅의 값과 충돌을 함께 게시하기

2026-10-12 공용 모듈 분리 이후의 위치는 [Framework 파일 이동표](../FrameworkPlan/Stage4.md#코드-위치의-전후-비교)를 따른다. 아래 발췌·이름·줄 번호·해시는 기록 당시 기준을 유지하며 소스 링크는 현재 대응 파일을 연다.

지형 편집의 성공은 밀도값을 바꾸는 순간보다 늦게 결정된다. 바뀐 메시와 충돌을 준비하고 권한·점유·연결된 물까지 확인해야 플레이어가 새 지형을 사용할 수 있다. 이 장은 데이터 확정에서 실제 월드의 게시로 책임이 확장되는 과정을 설명한다.

## 비교 기준과 문제

범위는 자연환경 3단계이다. 비교 기준은 `a28772c` → `92d1b38` → `0408d94`이며 전체 해시·브랜치 관계·작업 트리 식별값은 [비교 근거](EnvironmentEvidence.md)에 있다. `WT-2026-10-10`은 문서 작성 중 읽은 미커밋 소스 사본을 뜻한다. 이후 수정된 HEAD와 동일하다고 가정하지 않는다.

[결정 19](../decisions/0019-natural-environment-and-labs.md)는 굴착·성토를 저장하고 보호 구역을 유지하도록 했다. [지형 실행기 설계](../EnvironmentPlan/Stages0to3.md#후보-충돌과-서버-확정-실행기-설계)는 기존 충돌을 유지한 채 후보를 준비하도록 정했다. 지형에서 생긴 구덩이가 화면에는 보이는데 캐릭터는 옛 바닥 위에 서 있으면 게임 규칙과 표현이 어긋난다.

`FCCLTerrainStore`는 밀도·재질의 확정 값을 소유한다. 밀도장의 경계에서 메시를 추출하므로 단일 높이맵보다 여러 표면을 다룰 데이터 기반이 생긴다. 청크는 지역을 나눠 저장·추출하는 단위다. Candidate는 편집 후보, Revision은 내용 번호, Epoch는 실행 세대, PublicationSerial은 현재 실행에서 게시한 순번이다. 저장 Revision이 같아도 게시 순번까지 같다는 뜻은 아니다.

## 첫 구현의 데이터 확정

출처: `a28772c`, [CCLTerrainStore.cpp](../../Source/KSITerrain/Private/KSITerrainStore.cpp), `bool FCCLTerrainStore::CommitEdit`, 474-491줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
bool FCCLTerrainStore::CommitEdit(const FCCLTerrainCandidate& Candidate, const FCCLTerrainAuthority& Authority, FString& Error)
{
    Error.Reset();
    FBox Affected;
    if (!Candidate.IsValid() || Candidate.Before != Committed || Candidate.Request.Epoch != Epoch
        || Candidate.PolicyRevision != Authority.PolicyRevision)
    {
        Error = TEXT("Terrain candidate or authority changed while its dependent data was prepared.");
        return false;
    }

    if (!ValidateRequest(Candidate.Request, Authority, Affected, Error))
    {
        return false;
    }

    Committed = Candidate.After;
    return true;
```

이전 값 코어는 후보가 출발한 `Before`, 요청 세대, 권한 정책이 여전히 맞는지 다시 확인한 뒤 `Committed`를 바꿨다. 편집 중 보호 정책이 바뀌면 Prepare 때의 허가만으로 확정하지 않는다. 이 함수의 성공은 값 저장소의 성공이다. 해당 커밋에는 CPU 메시 추출도 있지만 실제 월드 충돌과 내비게이션 통합은 뒤의 작업이다.

## 월드에 게시하는 실행기

`92d1b38`는 비동기 준비와 게시를 `ACCLTerrainRegion`으로 연결했다. 다음은 현재 커밋 계열에서 이어지는 게시 직전의 검사다.

출처: `0408d94`, [CCLTerrainRegion.cpp](../../Source/CCL/Environment/CCLTerrainRegion.cpp), `if (!Participant->ValidateCommit(PendingCandidate, Error))`, 407-411줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
if (!Participant->ValidateCommit(PendingCandidate, Error))
{
    FailPending(Error);
    return;
}
```

참여자는 지형 변경에 함께 맞춰야 하는 시스템이다. 후보 준비가 끝나도 마지막 `ValidateCommit`에서 거부될 수 있다. 물의 예를 보자. 충돌 준비 동안 비가 내리면 준비 시작 때 복사한 물은 이미 오래된 값이다. [물 순환](EnvironmentWater.md)의 참여자는 게시 직전에 최신 물을 읽어 새 바닥에 배치한다.

실제 실행은 `RequestEdit`에서 권한을 검사하고 후보를 만든 뒤 `StartPreparation`으로 청크를 준비한다. 완료를 모은 실행기가 `PublishPrepared`에서 티켓·권한·점유와 참여자를 재검사한다. 물리 쓰기 경계 안에서 새 충돌을 활성화하고 값·참여자 상태를 확정한다. 일부 충돌 활성화가 실패하면 준비한 충돌을 폐기하며 기존 충돌을 유지한다. 이 경로를 코드에서 확인할 때는 [CCLTerrainRegion.cpp](../../Source/CCL/Environment/CCLTerrainRegion.cpp)의 `PublishPrepared` 전체를 함께 읽는다.

## 충돌 준비와 경로 준비의 차이

지형 충돌이 바뀐 뒤 NavMesh가 아직 이전 길을 가리킬 수 있다. NavMesh는 AI가 걸을 수 있는 경로 자료다. 생활 Agent와 적 AI는 경로 준비 여부를 확인한다. 지역이 준비되지 않았다면 이동을 멈추고 기다린다. 시작점·목적지만 검사해서는 중간 경로가 갱신 중인 지역을 지나는 경우를 놓친다.

출처: `0408d94`, [CCLAgentAIController.cpp](../../Source/CCL/Agents/CCLAgentAIController.cpp), `if (!FCCLTerrainNavigation::IsRouteReady`, 87-91줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
if (!FCCLTerrainNavigation::IsRouteReady(GetWorld(), GetPawn()->GetActorLocation(), Destination))
{
    StopMovement();
    return EStateTreeRunStatus::Running;
}
```

여기서 `Running`은 이동 성공이 아니라 다음 갱신에서도 판단을 이어간다는 StateTree 상태다. 경로 대기가 계속되면 주변 시간 제한과 실패 처리도 함께 읽어야 한다. 이 변화는 [기존 Agent 실행](AgentExecution.md)의 이동 계약을 보완하며 판단 커널 자체를 대체하지 않는다.

## 저장·클라이언트·종료

서버가 새 충돌을 게시했어도 클라이언트가 같은 지형을 준비했는지는 별개다. 복제기는 전체 또는 차이 바이트를 청크로 보내고 수신자의 충돌 준비 응답을 기다린다. 준비되지 않은 접속자의 이동을 보류하며 다른 접속자까지 동시에 멈추지는 않는다. 같은 게시 순번의 재전송은 내용 일치와 실제 준비 상태를 확인한 뒤 응답한다.

저장은 세계 파일과 지역 파일을 한 완료 세대로 묶는다. 복원은 저장된 Revision을 되살릴 수 있지만 실행 세대는 새로 구분한다. 종료 때 준비 중 후보를 취소하고 마지막 확정 상태를 보존한다. 자세한 저장 절차는 [저장과 실험장](EnvironmentPersistence.md), 현행 제한과 실행 결과는 [영구 지형 사양](../EnvironmentTerrain.md)에 있다.

높이맵은 표면·저장 처리가 단순하지만 터널 기반 요구에 제약이 있다. 밀도 청크는 표현 범위를 넓히는 대신 추출·충돌·네트워크·저장 비용을 늘린다. 현재 표면 물은 지표 한 층을 대상으로 하므로 지형의 다층 질의 지원을 동굴 유체 완성으로 확대 해석하지 않는다. UE5 DynamicMesh·Chaos를 활용하지만 후보·게시·준비 응답은 프로젝트가 만든 계약이다.

## 확인과 이해 점검

이번 작성에서는 소스·Git 이력과 [지형 검증 기록](../EnvironmentTerrain.md)을 대조했다. [CCLTerrainTests.cpp](../../Source/CCL/Environment/CCLTerrainTests.cpp), [CCLTerrainMeshTests.cpp](../../Source/CCL/Environment/CCLTerrainMeshTests.cpp), [네트워크 검사](../../Source/CCL/Tests/CCLTerrainNetworkSmokeSubsystem.cpp)가 확인 지점이다. 빌드·게임 실행은 새로 하지 않았다.

| 상황 | 기대 결과 |
|---|---|
| 충돌 준비 도중 다른 편집·권한 변경 | 낡은 후보 거부 |
| 새 충돌 위치에 캐릭터가 들어옴 | 점유 검사로 잘못된 게시 방지 |
| 저장 Revision이 과거 번호로 돌아옴 | 새 실행 세대·게시 상태로 재연결 |
| 클라이언트 준비 ACK 지연 | 해당 접속자 준비를 재확인 |

**메시가 만들어졌으면 편집 완료로 표시해도 될까?** 메시·충돌·경로·클라이언트 준비는 서로 다른 완료 조건이다. 어떤 소비자가 사용할 수 있는지를 구분해 표시해야 한다.

**Prepare 때 물을 복사하면 더 편하지 않을까?** 비동기 준비 동안 바뀐 물을 덮어쓸 수 있다. 지형 모양을 먼저 준비하고 최신 물은 확정 직전에 맞추는 이유다.
