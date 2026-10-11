# 불과 열: 연소 결과를 물·눈·실내에 연결하기

2026-10-12 공용 모듈 분리 이후의 위치는 [Framework 파일 이동표](../FrameworkPlan.md#코드-위치의-전후-비교)를 따른다. 아래 발췌·이름·줄 번호·해시는 기록 당시 기준을 유지하며 소스 링크는 현재 대응 파일을 연다.

불 시스템은 연료를 소비하고 열·연기·증발량을 계산한 뒤 표면과 공간에 전달한다. Niagara의 불꽃은 이 상태를 표시한다. 이 장은 연소를 참·거짓 표시에서 수지가 있는 지속 상태로 이해하는 과정이다.

## 비교 기준과 최초 구현

범위는 자연환경 7단계의 연소·표면 연결이다. 비교 기준은 `a817082`의 물·눈 표면과 작업 트리의 연소 모델이며 전체 해시·브랜치 관계·작업 트리 식별값은 [비교 근거](EnvironmentEvidence.md)에 있다. `WT-2026-10-10`은 문서 작성 중 읽은 미커밋 소스 사본을 뜻한다. 이후 수정된 HEAD와 동일하다고 가정하지 않는다.

기준 커밋에는 연소 코어가 없다. 과거의 가상 `bOnFire` 구현을 만들어 전후 코드라고 제시하지 않는다. `a5c79b0`, `ab195b1`, `ac0deb5`, `9e3570f`는 각각 코어·표면·공기·실험장 검증을 기록한 문서 커밋이다. 실제 신규 소스는 작업 트리 식별값으로 구분한다.

[결정 19](../DesignLog.md)와 [불 사양](../EnvironmentFire.md)은 연료·수분·열·차폐·소화를 연결하도록 정했다. 연소는 kg, 열은 J, 열용량은 J/K를 사용한다. 열용량은 온도를 1K 올리는 데 필요한 에너지다. 여기서 Flux는 이번 단계에 다른 시스템으로 전달할 양이다. 재료 계수는 게임용 근사이며 실물의 정밀 물성값으로 해석하지 않는다.

## 물만 있던 표면에서 연료·열을 가진 표면으로

이전 표면은 강수·증발·융해 같은 Forcing을 소비했다. [물 순환](EnvironmentWater.md)의 발췌가 그 실제 코드다. 현재 `FCCLSurfaceGrid::Fire`는 선택적 연소 셀 배열을 추가한다. 비어 있으면 연소 계산을 건너뛴다. 활성화된 지역은 표면 셀과 연료 셀을 대응시킨다.

출처: `WT-2026-10-10`, [CCLCombustionModel.cpp](../../Source/KSIEnvironment/Private/KSICombustionModel.cpp), `const double Burned = FMath::Min`, 115-122줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
const double Burned = FMath::Min(S.FuelKg, M.BurnKgPerGameSecond * Dt
    * (1. + FMath::Min(Wind, 20.) * 0.05) * (1. - WetRatio / M.MaximumWaterFuelRatio));
S.FuelKg -= Burned;
S.BurnedKg += Burned;
const double Heat = Burned * M.EnergyJoulesPerKg;
S.ReleasedHeatJoules += Heat;
S.HeatJoules += Heat;
F.SmokeKg += Burned * M.SmokeKgPerFuelKg;
```

연소량은 남은 연료를 넘을 수 없다. 연료가 줄어든 만큼 누적 연소량을 늘리고, 소모량에 단위 연료당 에너지를 곱해 열을 얻는다. 설명용으로 0.01kg을 태워 kg당 1,000,000J를 얻는 재료라면 이번 열은 10,000J다. 실제 기본 계수와 별개의 계산 예시다.

발췌 전에는 수분 대비 연료 비율과 발화·소화 온도로 연소 여부를 정한다. 발화 온도와 계속 타는 동안의 소화 온도를 구분해 경계 부근에서 매번 켜졌다 꺼지는 반응을 줄인다. 결과는 검증한 지역 후보에서만 확정한다. 외부 열을 더하는 Ignite는 소모한 연료를 다시 채우지 않는다.

## 물 소유자에게 전달량을 돌려준다

연소 코어는 현재 사용할 수 있는 물의 kg를 받아 증발량을 반환한다. 실제 물 보유량은 표면이 소유한다. 같은 물을 연료 내부에도 지속 저장하면 표면과 연료가 각각 자기 물로 계산할 수 있다.

출처: `WT-2026-10-10`, [CCLSurfaceFire.cpp](../../Source/KSIEnvironment/Private/KSISurfaceFire.cpp), `const double Vapor = Flux.VaporizedWaterKg`, 117-124줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
const double Vapor = Flux.VaporizedWaterKg / WaterKgPerCubicMeter;
const double LiquidLoss = FMath::Min(Cell.WaterCubicMeters, Vapor);
const double SoilLoss = FMath::Min(Cell.SoilCubicMeters, FMath::Max(0., Vapor - LiquidLoss));
Cell.WaterCubicMeters -= LiquidLoss;
Cell.SoilCubicMeters -= SoilLoss;
G.Ledger.EvaporatedCubicMeters += LiquidLoss + SoilLoss;
Layer.VaporizedWaterKg += Flux.VaporizedWaterKg;
Layer.EmittedSmokeKg += Flux.SmokeKg;
```

코어가 반환한 kg를 표면의 m³로 환산하고 액체, 토양 순으로 차감한다. 같은 차감량을 기존 증발 장부에 넣는다. 남은 표면 전달 열은 얼음과 눈을 녹인다. 녹인 눈의 물 환산량은 액체로 옮기고 외형 체적은 녹은 비율만큼 줄인다. 실내라면 연기·수증기·남은 열을 [공기 모델](EnvironmentAir.md)에 전달한다.

인접 셀의 열 전달은 고정한 열 상태에서 계산해 한 번 적용한다. 받은 열을 같은 단계에서 다시 보내지 않는다. 높이 범위가 분리됐거나 벽이 막으면 직접 전달을 제한한다. 현재는 풍속과 노출률을 사용하며 풍향별 정밀 확산이나 복사열을 모두 푸는 모델은 아니다.

## 시간·권한·수명

불과 환기는 게임 초를 사용한다. 날씨의 강수는 세계 초를 사용한다. 배율 0에서 불이 계속 탈 수 있는 이유다. 새 입력을 받지 않는 일시 정지와 세계 배율 0을 같은 상태로 설명하면 안 된다.

출처: `WT-2026-10-10`, [CCLWorldSimulationSubsystem.cpp](../../Source/CCL/Environment/CCLWorldSimulationSubsystem.cpp), `bool UCCLWorldSimulationSubsystem::IgniteSurface`, 683-686줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
bool UCCLWorldSimulationSubsystem::IgniteSurface(FGuid RegionId, int32 CellIndex, double HeatJoules, FString& Error)
{
    return CheckAuthority(Error) && SurfaceSimulation.Ignite(RegionId, CellIndex, HeatJoules, Error);
}
```

World 실행기가 서버 권한을 확인하고 값 모델에 요청한다. 실험 버튼은 소유 PlayerController의 RPC로 서버에 요청하며 직접 연료 배열을 바꾸지 않는다. 실패하면 표면·날씨·불·공통 시계 후보를 폐기한다. 저장은 연료·열·전달 누계를 같은 표면 스냅샷에 포함하고 표현 Actor는 그 결과를 읽는다.

## 현재 코드의 후속 열 피해 연결

현재 작업 트리에는 단계 7 최초 통과 이후의 열 피해 연결도 있다. `c182af3`의 문서 기록과 [열 피해 검증](../EnvironmentFire.md#열-피해-구현검증)이 근거다. 생태계 8단계 전체를 설명하지 않지만, 불이 기존 GAS 전투와 만나는 경계는 여기서 짚는다.

`UCCLThermalExposureSubsystem::Prepare`는 이전 확정 표면과 Actor 위치에서 피해 후보를 만든다. 공통 진행 실패 시 Cancel하고, 성공 후 Commit이 세계 Epoch·StepId와 대상의 현재 Avatar를 다시 확인한다. 후보를 먼저 비운 뒤 즉시 체력 효과를 적용하므로 중복 Commit을 막는다. 이것은 물리 열량을 HP로 보존 변환하는 식이 아니라 별도 게임 피해 정책이다.

출처: `WT-2026-10-10`, [CCLThermalExposure.cpp](../../Source/CCL/Environment/CCLThermalExposure.cpp), `auto Work = MoveTemp(Pending);`, 222-224줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
auto Work = MoveTemp(Pending);
Cancel();
for (const auto& Item : Work)
```

GAS 체계는 [GAS 전투](GASCombat.md)를 먼저 읽는다. 현재 코드의 Agent 사망·부상 저장 세부는 후속 생애 시스템 영역이다. 불 계산의 완료와 모든 생태 개체의 열 사망을 같은 완료 항목으로 묶지 않는다.

## 검증·대안과 이해 점검

기존 단계 7 실행은 [불 사양](../EnvironmentFire.md)의 코어·표면·공기·상시 구역 결과를 구분해 읽는다. [CCLCombustionTests.cpp](../../Source/CCL/Environment/CCLCombustionTests.cpp)와 [CCLSurfaceFireTests.cpp](../../Source/CCL/Environment/CCLSurfaceFireTests.cpp)가 수지·실패 재시도의 근거다. 이번 문서 작성에서는 실행하지 않았다.

연소를 값 모델로 분리하면 화면 없는 서버와 저장 재개를 다루기 쉽다. 대신 연료·열·수분 전달의 단위와 상한을 모든 연결부에서 맞춰야 한다. Niagara 입자만으로 화재를 표현하는 대안은 시각 작업이 간단하지만 서버 소화·융해·저장 규칙을 별도로 만들어야 한다. GAS와 RPC는 UE4에도 있던 체계이며 여기서의 연소 원자성은 프로젝트 계약이다.

**물을 뿌려 껐으면 연소 누계도 줄여야 할까?** 이미 소비한 연료는 돌아오지 않는다. 소화는 이후 연소를 막는 입력이며 과거 수지를 바꾸지 않는다.

**생활 계산이 실패했는데 체력부터 깎아도 될까?** 다음 재시도에서 같은 구간 피해가 중복될 수 있다. 외부 GAS 효과는 공통 진행 성공 뒤의 별도 확정 경계에 연결한다.
