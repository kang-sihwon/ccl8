# 자연 날씨: 수동 강수 입력을 지속 대기 상태로 확장하기

자연 날씨는 천체의 일조와 이전 기온·수분을 읽어 다음 강수 입력을 만든다. 기존 물·눈 계산을 유지하면서 입력의 생산자가 어떻게 달라졌는지, 날씨도 실패 시 되돌려야 하는 이유를 배운다.

## 비교 기준과 변경 이유

범위는 자연환경 6단계이다. 비교 기준은 `a817082`의 수동 표면 입력과 작업 트리의 자연 날씨이며 전체 해시·브랜치 관계·작업 트리 식별값은 [비교 근거](EnvironmentEvidence.md)에 있다. `WT-2026-10-10`은 문서 작성 중 읽은 미커밋 소스 사본을 뜻한다. 이후 수정된 HEAD와 동일하다고 가정하지 않는다.

`5a640bf`와 `a5c79b0`는 날씨 구현·표현의 검증을 기록한 문서 커밋이다. 그 해시에 날씨 신규 소스까지 들어 있다고 가정하지 않는다. 비교한 소스는 미커밋 `WT-2026-10-10`이며 파일별 해시는 [비교 근거](EnvironmentEvidence.md)에 남긴다.

처음 물 실험은 비·기온을 지정해 보존을 확인하면 충분했다. [결정 19](../DesignLog.md)는 일조·계절·날씨의 연결을 요구했고 [날씨 계산 계약](../EnvironmentWeather.md#계산-계약)은 지역 기후와 지속 대기 상태를 기존 표면에 붙이기로 했다. 당시 기록된 이유와 여기서 계산한 예시를 구분한다.

`FCCLClimate`는 기준 기온·수분 공급·반응 시간 같은 정의다. `FCCLWeatherState`는 현재 기온·수증기·구름 같은 변하는 값이다. `Forcing`은 물·눈 계산에 넘기는 외부 입력이다. 정의, 상태, 소비자 입력을 구분하면 강제 실험과 자연 진행을 같은 표면 모델에 연결할 수 있다.

## 이전 수동 입력과 모드 전환

출처: `a817082`, [CCLSurfaceSimulation.cpp](../../Source/CCL/Environment/CCLSurfaceSimulation.cpp), `bool FCCLSurfaceSimulation::ChangeForcing`, 299-311줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
bool FCCLSurfaceSimulation::ChangeForcing(FGuid RegionId, const FCCLSurfaceForcing& Forcing, FString& Error)
{
    auto* Region = Regions.Find(RegionId);
    if (!Region || !ValidForcing(Forcing) || Region->Revision == MAX_uint64)
    {
        return Fail(Error, TEXT("Invalid surface forcing or exhausted revision."));
    }

    Region->Forcing = Forcing;
    ++Region->Revision;
    return true;
}
```

이전 함수는 검증한 Forcing을 대입했다. 값이 바뀌기 전까지 같은 강수율·기온을 사용했다. 현재 함수는 직접 강수 입력을 바꾸면 자연 모드를 끈다.

출처: `WT-2026-10-10`, [CCLSurfaceSimulation.cpp](../../Source/CCL/Environment/CCLSurfaceSimulation.cpp), `Region->Climate.bEnabled = 0;`, 358-361줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
    Region->Climate.bEnabled = 0;
    ++Region->Revision;
    return true;
}
```

자연 모드가 계속 켜져 있으면 다음 시간 진행이 수동 비 입력을 자연 강수율로 덮어쓸 수 있다. `ChangeClimate`로 자연 진행을 다시 켜면 보관한 날씨 상태에서 이어간다. 이 전환은 임의의 두 계산기가 같은 입력을 번갈아 쓰지 않도록 소유자를 정하는 처리다.

## 기온·수분을 계산하는 순서

현재 값 모델의 일부를 보자. 발췌 앞에서는 입력 범위와 시간 상한을 검사하고 이전 상태를 `Next`로 복사한다.

출처: `WT-2026-10-10`, [CCLWeatherModel.cpp](../../Source/CCL/Environment/CCLWeatherModel.cpp), `const double Target =`, 33-41줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
const double Target = C.BaseTemperatureCelsius + C.SolarWarmingCelsius * FMath::Min(2., Next.IrradianceWattsPerM2 / 1000.);
Next.TemperatureCelsius = Target + (State.TemperatureCelsius - Target) * FMath::Exp(-Delta / C.ThermalResponseSeconds);
Next.VaporMeters += C.MoistureSupplyMetersPerSecond * (0.5 + Front) * Delta;
const double Capacity = 0.005 * FMath::Exp(FMath::Clamp(Next.TemperatureCelsius * 0.06, -10., 10.));
const double Condensed = FMath::Max(0., Next.VaporMeters - Capacity) * (1. - FMath::Exp(-Delta / C.CondensationSeconds));
Next.VaporMeters -= Condensed;
Next.CloudMeters += Condensed;
const double Precipitation = Next.CloudMeters * (1. - FMath::Exp(-Delta / C.CondensationSeconds));
Next.CloudMeters -= Precipitation;
```

먼저 일조가 목표 기온 Target을 만든다. 현재 기온은 목표값으로 즉시 바뀌지 않고 반응 시간에 따라 가까워진다. 설명용으로 현재 10°C, 목표 20°C, 진행 간격과 반응 시간이 같다면 `exp(-1)`은 약 0.368이다. 다음 온도는 `20 + (10 - 20) × 0.368 = 16.32°C`다.

그다음 외부 수분 공급을 더하고 기온별 용량을 넘는 수증기를 구름으로 옮긴다. 구름 일부가 강수가 되며 기온으로 비·눈 비율을 나눈다. 표면에 적용할 때는 셀 면적·시간·차폐율로 체적을 구한다. 대기 모델은 외부 수분 공급을 허용하는 열린 계다. 표면 증발을 모두 대기로 되돌리는 닫힌 물 순환이나 실제 기상 예보 모델은 아니다.

## 천체와 표면을 한 구간으로 묶기

`FCCLSurfaceSimulation::AdvanceSlice`는 구간 중간 시각의 일조를 구하고 날씨를 진행한다. 성공한 날씨의 기온·비·눈·증발률을 Forcing에 넣은 뒤 표면 계산을 실행한다. 날씨의 한 계산 구간은 최대 60세계 초다. 큰 요청은 내부에서 구간을 나누며 실행 예산 초과 시 원본과 대기 시간을 보존한다.

출처: `WT-2026-10-10`, [CCLSurfaceSimulation.cpp](../../Source/CCL/Environment/CCLSurfaceSimulation.cpp), `G.Forcing.TemperatureCelsius = G.Weather`, 567-570줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
G.Forcing.TemperatureCelsius = G.Weather.TemperatureCelsius;
G.Forcing.RainMetersPerWorldSecond = G.Weather.RainMetersPerSecond;
G.Forcing.SnowMetersPerWorldSecond = G.Weather.SnowMetersPerSecond;
G.Forcing.EvaporationMetersPerWorldSecond = G.Weather.EvaporationMetersPerSecond;
```

기후·날씨가 표면 격자에 있으므로 [공통 시간](EnvironmentTime.md)의 CandidateSurface에 함께 복사된다. 표면 계산 후 생활이 실패해도 원본 기온·수증기·구름은 그대로다. 물만 되돌리고 대기의 수증기는 이미 감소한 상태로 두면 재시도 강수가 달라진다.

동적 차폐 지역은 같은 후보에서 지붕·문·확정 지형의 노출률도 갱신한다. 필요한 문맥이 없거나 질의가 실패하면 일부 셀만 게시하지 않는다. 강제 비·눈도 차폐를 우회하지 않는다.

## 표현·수명·비용

서버가 지속 상태를 소유하고 클라이언트는 복제된 확정값을 읽는다. `ACCLWeatherPresentation`의 Niagara는 입자를 표시하고 구름 표현은 관측 위치 주변 지역 상태를 사용한다. Niagara가 물을 생성하거나 강수량을 결정하지 않는다. 에셋 생성은 에디터 경로이고 매 게임 시작 시 새 효과를 컴파일하는 구조가 아니다.

지역마다 기후 상태를 두면 기존 저장·후보 확정을 재사용하기 쉽다. 반면 여러 지역이 같은 광역 기상을 공유할 때 계산이 중복될 수 있다. 현재는 광역 이류·실제 예보 정확도·전 영역 고해상도 계산을 제공하지 않는다. 자연 날씨 연결은 프로젝트 확장이며 UE4와 UE5의 자동 API 전환이 아니다.

## 확인과 이해 점검

기존 실행 결과는 [날씨 검증](../EnvironmentWeather.md#차폐와-표현의-추가-검증)에 있다. 최초 계산, 차폐·표현 연결, 눈 효과 생성 이력 보완은 각각 다른 검증 범위다. [CCLWeatherTests.cpp](../../Source/CCL/Environment/CCLWeatherTests.cpp)에서 저장 재개·큰 요청 분할·생활 실패·동적 차폐를 읽을 수 있다. 이번에는 소스와 기록을 대조했으며 실행하지 않았다.

| 상황 | 기대 결과 |
|---|---|
| 강제 비 선택 후 다음 진행 | 자연 날씨가 입력을 덮어쓰지 않음 |
| 자연 진행 재선택 | 기존 대기 상태에서 계산 재개 |
| 천창을 열고 같은 강수 진행 | 열린 셀의 유입 증가 |
| 생활 실패 후 재시도 | 기온·수분·표면의 중복 진행 없음 |

**구름이 늘었으니 바로 더 많은 비가 내려야 할까?** 강수량은 구름 보유량과 해당 시간 간격의 반응식으로 계산한다. 눈에 보이는 구름 밝기만으로 강수량을 결정하지 않는다.

**같은 Seed면 시간 분할이 달라도 바이트가 같을까?** 같은 입력·같은 분할의 재현성과 임의의 다른 적분 간격에서의 동일성은 다르다. 후자는 현재 모델 전체의 보장이 아니다.
