# 실내 환기: 여러 표면이 공유하는 공기를 한 번만 진행하기

공기 모델은 방의 열·수증기·연기를 보관하고 문으로 교환되는 양을 계산한다. 한 방에 표면 지역이 여러 개 있어도 환기 시간은 한 번만 진행해야 한다. 이 장은 상태 후보 복사에 더해 계산 순서와 공유 예산이 필요한 이유를 설명한다.

## 비교 기준과 이전의 차폐

범위는 자연환경 7단계의 공간 환경이다. 비교 기준은 `4458dea`의 개구부 입력과 `c8816e7`에 커밋된 공기 결합이며 전체 해시와 브랜치 관계는 [비교 근거](EnvironmentEvidence.md)에 있다. 이 장이 처음 쓰였을 때 인용한 미커밋 작업 트리(`WT-2026-10-10`)는 `c8816e7`에 커밋됐다. 현재 기준은 2026-10-31의 `main`인 `d4888b0`이다. 코드가 `Source/KSI*/` 모듈로 옮겨진 위치는 [Framework 파일 이동표](../FrameworkPlan/Stage4.md#코드-위치의-전후-비교)를 따른다.

이전 차폐 시스템에는 지붕·벽·문 개구와 양 끝 공간 ID가 있었지만 실내 공기의 열·수분·연기를 진행하는 코드는 없었다. [천체와 차폐](EnvironmentCelestial.md)는 광선 투과율을 계산하는 실제 이전 코드를 설명한다. `54123fb`, `ac0deb5`, `39005ee`는 환기 값 모델·월드 결합·기하 차폐를 기록한 문서 커밋이다.

`FKSIAirSpace`는 공간 ID, 체적, 위치 범위와 공기 보유량을 가진다. 농도는 양을 체적으로 나눈 값이다. `FKSISurfaceOpening`은 두 공간 또는 공간과 외부를 연결하는 개구다. 평균 공간 모델은 한 방 안의 정밀한 연기 분포를 계산하지 않는다.

## 문 면적을 교환 부피로 바꾸기

출처: `d4888b0`, [KSIAirSimulation.cpp](../../Source/KSIEnvironment/Private/KSIAirSimulation.cpp), `const double Speed =`. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
const double Speed = 0.05 + Wind * 0.1 + FMath::Sqrt(FMath::Abs(TA - TB)) * 0.01;
const double Volume = FMath::Min(Limit * 0.25, 4. * O.HalfExtentsMeters.X * O.HalfExtentsMeters.Y * O.OpenFraction * Speed * Dt);
```

HalfExtents는 사각형의 반너비·반높이다. 둘에 4를 곱해야 전체 면적이 된다. 설명용으로 반너비 0.5m, 반높이 1m, 열림 비율 0.5라면 면적은 `4 × 0.5 × 1 × 0.5 = 1m²`다. 교환 속도 0.1m/s로 0.25초 진행하면 교환 부피는 `1 × 0.1 × 0.25 = 0.025m³`다. 실제 코드의 공간 체적 상한도 함께 적용한다.

온도 차와 바람은 교환 속도의 게임용 근사식에 들어간다. 같은 부피가 양방향으로 교환되지만 양쪽 열·수증기·연기 농도가 다르므로 순이동량이 생긴다. 닫힌 문은 교환량 0이다. 단순히 매 프레임 연기를 일정 비율 지우는 방식과 달리 어디로 나갔는지를 기록할 수 있다.

## 여러 문이 같은 공기를 중복 소비하지 않기

문마다 계산한 교환량을 모으고 한 공간의 총 교환 예산으로 제한한다. 이전 농도에서 각 문 전달량을 구한 뒤 Incoming에 모아 동시에 반영한다. 한 단계에서 A방에서 받은 연기를 즉시 C방으로 다시 보내지 않는다. 문 배열의 순서가 바뀌어도 같은 계산 순서를 유지하도록 ID를 정렬한다.

공간 A가 가진 연기 1kg 중 0.1kg이 B로 이동했다면 두 실내 공간의 합은 1kg이다. 외부로 나간 경우에는 보유량 합이 줄고 외부 배출 누계가 늘어야 한다. 이 숫자는 보존 설명용 예시다. 외부 유입·배출, 외부 열원과 내부 이동을 같은 항목에 섞지 않는다.

## 표면을 다 계산한 뒤 환기를 한 번 실행한다

출처: `d4888b0`, [KSISurfaceSimulation.cpp](../../Source/KSIEnvironment/Private/KSISurfaceSimulation.cpp), `const FKSIAirSimulation PreviousAir = Air;`. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
const FKSIAirSimulation PreviousAir = Air;
FKSISurfaceThermalContext Context;
Context.PreviousAir = &PreviousAir;
Context.Shelter = Shelter;
Context.Observer = Observer;
```

공통 하위 단계가 시작되면 공기를 고정한다. 모든 표면 지역은 같은 `PreviousAir`를 읽고 연소·증발의 전달량을 `Context`에 모은다. 그 후 공간별 전달량을 더하고 환기를 한 번 실행한다.

출처: `d4888b0`, [KSISurfaceSimulation.cpp](../../Source/KSIEnvironment/Private/KSISurfaceSimulation.cpp), `if (!Air.Advance(GameDelta, EffectiveOpenings, Exteriors, Error))`. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
if (!Air.Advance(GameDelta, EffectiveOpenings, Exteriors, Error))
{
    return false;
}
```

지역마다 환기를 실행하면 지역이 두 개일 때 같은 방의 시간이 두 배 흐른다. 후보 전체를 복사했더라도 이 잘못된 계산은 성공할 수 있다. 원자적 확정과 올바른 시간 소비 횟수는 별도의 검사 대상이다.

공기가 차가운 연료를 데울 때도 같은 보유 열을 여러 셀이 전부 사용하면 안 된다. 현재 결합은 방의 접촉 셀 수로 열 예산을 나누고 전달량을 제한한다. 정밀 온도 분포를 포기하는 대신 작은 방에서 음수 열량이 생기는 문제를 막는 선택이다.

## 문 표시와 실제 통로의 차이

[기하 연결 기록](../EnvironmentPlan/Fire.md)은 열린 문 뒤의 벽·성토가 환기를 막는 경우를 다룬다. 현재 `FKSISurfaceScene::QueryOpeningFraction`은 개구와 정본 장면의 일치를 확인하고 열린 사각 영역에서 표본을 질의한다. 밀도 내부인지, 짧은 광선이 다른 벽·지형을 통과하는지 검사해 실제 유효 비율을 구한다.

창이 화면상 열렸다는 값만 믿으면 벽으로 막힌 공간에서도 연기가 빠질 수 있다. 반대로 표본 질의는 작은 틈·복잡한 기하를 정밀 해석하는 방법은 아니다. 질의 실패 시 환기만 건너뛰며 세계를 완료하지 않고 공통 후보를 거부한다.

## 수명·복원·검증

실내 상태는 표면 시뮬레이션의 값 상태로 저장·복제된다. 표현 Actor가 사라졌다고 공기 상태를 매번 초기화하지 않는다. 읽는 저장의 버전에 공간 모델이 없으면 임의의 방을 채우지 않는 이전 형식 의미를 보존한다. 세계 시간이 멈춰도 게임 시간이 진행하면 환기는 진행할 수 있다.

[CCLAirSimulationTests.cpp](../../Source/CCL/Environment/CCLAirSimulationTests.cpp)는 문 개방·두 방 보존·여러 문·잘못된 입력을 다룬다. [CCLSurfaceAirTests.cpp](../../Source/CCL/Environment/CCLSurfaceAirTests.cpp)는 여러 지역의 단일 환기와 열 예산·실패 원본 보존을 다룬다. [CCLVentilationGeometryTests.cpp](../../Source/CCL/Environment/CCLVentilationGeometryTests.cpp)는 확정 기하 연결을 검사한다. 기존 실행 결과는 [불·공간 환경](../EnvironmentPlan/Fire.md)에 있고 이번에는 재실행하지 않았다.

공간 평균 모델은 정밀 3D 유체보다 적은 상태로 열 축적·문 환기를 다룬다. 그 대신 산소 반응·정밀 압력·연기 중독·실내 각 지점의 농도 차이를 제공하지 않는다. UE5 Niagara는 표현 수단이고 이 평균 모델은 프로젝트 구현이다.

**새 표면 지역을 추가했더니 연기가 두 배 빨리 빠진다면 무엇부터 확인할까?** 동일한 GameDelta로 Air.Advance를 지역마다 실행하는지 확인한다. 공유 모델의 시간 진행은 지역 루프 뒤에서 한 번 수행해야 한다.

**모든 문이 각각 보유 공기의 절반만 옮기면 안전할까?** 문이 여러 개면 합이 보유량을 넘는다. 각 문 상한과 공간 전체 상한을 함께 적용해야 한다.
