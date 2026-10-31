# 물 순환: 높이 대신 보존할 양부터 정하기

2026-10-12 공용 모듈 분리 이후의 위치는 [Framework 파일 이동표](../FrameworkPlan/Stage4.md#코드-위치의-전후-비교)를 따른다. 아래 발췌·이름·줄 번호·해시는 기록 당시 기준을 유지하며 소스 링크는 현재 대응 파일을 연다.

표면 시스템은 물·얼음·토양 수분을 물 환산 체적으로 보관하고, 이동·상변화·외부 출입을 같은 장부로 검증한다. 이 장을 읽으면 수면 메시를 움직이는 일과 물의 양을 보존하는 계산이 어떻게 연결되는지 설명할 수 있다.

## 비교 기준과 처음 필요한 구현

범위는 자연환경 4단계이다. 비교 기준은 `92d1b38` → `0408d94`, 표현 보완 `a817082`이며 전체 해시·브랜치 관계·작업 트리 식별값은 [비교 근거](EnvironmentEvidence.md)에 있다. `WT-2026-10-10`은 문서 작성 중 읽은 미커밋 소스 사본을 뜻한다. 이후 수정된 HEAD와 동일하다고 가정하지 않는다.

`92d1b38`의 지형 실행기는 참여자 계약을 갖췄지만 현재의 표면 물 계산은 아직 없다. `0408d94`에 물·눈·지형 통합 코드가 함께 커밋됐다. 따라서 이 커밋 내부에서 물만 있던 중간 코드를 Git으로 복원할 수는 없다. 단계 4와 5의 실행 순서는 각각 [물 사양](../EnvironmentPlan/Water.md)과 [눈 사양](../EnvironmentPlan/Snow.md)의 기록으로 구분한다.

`FCCLSurfaceGrid`는 셀의 배열이다. 각 셀은 바닥·상한 높이와 액체·얼음·토양 수분을 가진다. 물 환산 체적은 얼음이나 눈을 녹였을 때의 물 부피다. 수지는 시작량과 유입·유출로 현재 총량을 설명할 수 있는지 보는 계산이다. `Ledger`는 그 누계를 저장하는 값이다.

## 강수를 셀의 체적으로 바꾸기

강수율이 초당 0.001m, 시간 간격이 10세계 초, 셀 면적이 4m²라고 가정하자. 내린 깊이는 `0.001 × 10 = 0.01m`다. 체적은 `0.01 × 4 = 0.04m³`다. 지붕 때문에 절반만 도달하면 `0.04 × 0.5 = 0.02m³`를 더한다. 설명용 숫자이며 현재 기후 기본값이 아니다.

출처: `a817082`, [CCLSurfaceSimulation.cpp](../../Source/KSIEnvironment/Private/KSISurfaceSimulation.cpp), `const double Rain =`, 87-89줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
const double Rain = F.RainMetersPerWorldSecond * WorldDt * Area * Cell.RainExposure;
Cell.WaterCubicMeters += Rain;
G.Ledger.RainCubicMeters += Rain;
```

같은 Rain을 셀 보유량과 외부 강수 누계에 한 번씩 기록한다. 두 번 물을 생성하는 것이 아니라 현재량과 원인을 따로 기록하는 것이다. 침투는 액체에서 토양으로 옮기므로 외부 유출에 더하지 않는다. 결빙도 물에서 얼음으로 옮기며 물 환산 합은 유지한다.

## 흐름에서 같은 물을 두 번 쓰지 않기

모든 경계의 유량 후보를 먼저 만든 뒤 출발 셀의 물과 도착 셀의 빈 공간으로 제한한다. 적용 중 받은 물을 바로 다음 경계의 출발 예산으로 쓰면 셀 순회 방향에 따라 한 단계에서 이동하는 거리가 달라진다.

출처: `a817082`, [CCLSurfaceSimulation.cpp](../../Source/KSIEnvironment/Private/KSISurfaceSimulation.cpp), `const double Requested = Transfer.Amount`, 189-194줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
const double Requested = Transfer.Amount * FMath::Min(SourceScales[Transfer.From], TargetScales[Transfer.To]);
// Remaining frozen budgets absorb final-ulp rounding without spending newly received water.
const double Amount = FMath::Min3(Requested, Remaining[Transfer.From], RoomLeft[Transfer.To]);
Remaining[Transfer.From] -= Amount;
RoomLeft[Transfer.To] -= Amount;
Added[Transfer.To] += Amount;
```

`Remaining`은 이번 단계 시작 때부터 쓸 수 있던 물, `RoomLeft`는 받을 수 있는 공간, `Added`는 이번에 받은 물이다. 새 물은 Added에 모으고 마지막에 합친다. 제한 비율을 먼저 계산해도 반올림이 남을 수 있으므로 실제 남은 양을 다시 상한으로 쓴다.

강수·증발·상변화는 세계 초를 사용하고 수평 흐름은 게임 초를 사용한다. 세계 배율을 높여도 수평 물 이동까지 같은 배율로 과도하게 빨라지는 것을 피하는 계약이다. 세계 구간을 여러 하위 계산으로 나눌 때 강수량도 해당 구간만큼 나눠야 한다.

## 지형 참여자가 최신 물을 읽는 이유

처음 지형 코어의 `CommitEdit`은 지형 값만 바꿨다. 물이 추가된 뒤에는 새 바닥이 기존 물을 밀어낼 공간도 검증해야 한다. 다음은 물 통합 커밋의 확정 직전 구현이다.

출처: `0408d94`, [CCLTerrainWaterParticipant.cpp](../../Source/CCL/Environment/CCLTerrainWaterParticipant.cpp), `bool FCCLTerrainWaterParticipant::ValidateCommit`, 75-86줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
bool FCCLTerrainWaterParticipant::ValidateCommit(const FCCLTerrainCandidate& Candidate, FString& Error) const
{
    const auto* Runtime = Owner.Get();
    if (!Runtime || Ticket != Candidate.GetTicket())
    {
        Error = TEXT("Terrain water owner or preparation ticket changed.");
        return false;
    }
    Prepared = Runtime->GetSurfaceSimulation();
    return Prepared.RebaseTerrain(RegionId, Beds, Candidate.GetSnapshot().Revision, Error);
}
```

Prepare는 새 바닥 높이를 준비한다. ValidateCommit은 최신 표면 전체를 복사해 `RebaseTerrain`으로 재배치한다. 성토로 넘친 물은 연결된 빈 공간이나 허용된 외부 경계로 옮긴다. 보존할 공간이 없으면 지형 편집도 거부한다. 성공 뒤 Commit은 준비한 물을 옮기고 Abort는 후보를 버린다.

## 상태·표현·복제와 비용

서버 World의 `FCCLSurfaceSimulation`이 정본이고 `ACCLSurfacePresentation`은 준비된 사본으로 메시를 만든다. 표현의 모서리 보간이 서버 체적을 되쓰지 않는다. `a817082`는 연속 표면·재질과 실험 조작을 보완했다. `UDynamicMeshComponent`는 메시 표시 수단이며 이 물 수지 알고리즘은 프로젝트 코드다.

클라이언트는 표면이 요구하는 지형 게시 순번까지 준비된 뒤 물을 표시한다. 오래된 바닥 위에 새 수면만 표시하지 않도록 하는 연결이다. 저장·복제의 CRC는 우발적인 손상 검사이며 내용 변경 감지에는 끝에 붙인 CRC 필드를 제외한 본문을 사용한다.

현재 수평 격자는 파도 운동량·동굴 내부 유체·정밀 유체 역학을 풀지 않는다. 전체 후보 복사와 스냅샷은 실패·재접속을 단순하게 만들지만 큰 지역·많은 접속자의 비용이 증가한다. 양 보존과 시각 품질은 별도로 평가한다.

## 확인과 이해 점검

기존 실행 기록은 [물 검증](../EnvironmentPlan/Water.md#실행-근거와-검토-결과), 코드 검사는 [CCLSurfaceSimulationTests.cpp](../../Source/CCL/Environment/CCLSurfaceSimulationTests.cpp)다. 이번 작업은 문서·소스 대조이고 새 실행 검증은 하지 않았다.

| 상황 | 확인할 결과 |
|---|---|
| 비·증발 없는 닫힌 지역 | 내부 흐름 후 총량 보존 |
| 액체가 얼거나 토양에 스밈 | 물 환산 총량 유지 |
| 지형 준비 동안 강수 발생 | 최신 물을 새 지형에 재배치 |
| 후속 생활 진행 실패 | 물·장부·시간 모두 원본 유지 |

**얼음이 녹았으니 유입 장부를 늘려야 할까?** 기존 얼음이 액체로 바뀐 내부 이동이다. 외부 유입으로 더하면 검증식이 실제 총량보다 커진다.

**셀 A가 B에서 받은 물을 즉시 C로 보내도 총량만 같으면 충분할까?** 총량은 맞아도 열거 순서가 이동 속도를 바꿀 수 있다. 보존과 순서 의존성은 각각 검사해야 한다.
