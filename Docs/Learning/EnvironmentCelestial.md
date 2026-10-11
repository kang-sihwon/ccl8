# 천체와 차폐: 같은 세계 시각에서 지역의 환경 입력 구하기

2026-10-12 공용 모듈 분리 이후의 위치는 [Framework 파일 이동표](../FrameworkPlan.md#코드-위치의-전후-비교)를 따른다. 아래 발췌·이름·줄 번호·해시는 기록 당시 기준을 유지하며 소스 링크는 현재 대응 파일을 연다.

천체 시스템은 확정 시각에서 하늘을 계산하고, 표면 질의는 그 빛·강수·바람이 특정 위치에 도달하는지를 계산한다. 태양 Actor를 회전시키는 표현과 게임 규칙이 소비하는 일조를 분리하는 것이 학습 목표다.

## 비교 기준과 필요한 개념

범위는 자연환경 2단계이다. 비교 기준은 `7307465` → `4458dea` → `5f62e1e`, 이후 `0408d94`이며 전체 해시·브랜치 관계·작업 트리 식별값은 [비교 근거](EnvironmentEvidence.md)에 있다. `WT-2026-10-10`은 문서 작성 중 읽은 미커밋 소스 사본을 뜻한다. 이후 수정된 HEAD와 동일하다고 가정하지 않는다.

`7307465`에는 실험장과 공통 시간이 있지만 천체·차폐 계산은 아직 없다. 최초 계산은 `4458dea`, 실험장 조명·저장·복제 연결은 `5f62e1e`에서 도입됐다. 가상의 이전 천체 코드를 만들지 않는다. 당시의 고정 조명은 [단계 1 완료 기록](../EnvironmentPlan.md#실험장-구현과-단계-1-통과)에 남아 있다.

궤도 정의는 반장축·이심률·주기처럼 계산에 필요한 입력이다. 관측 결과는 특정 시각·위도·경도에서 본 방향과 일조다. `FVector3d`는 세 성분의 double 벡터이고 `FGuid`는 천체·공간을 포인터 수명과 독립적으로 식별하는 값이다. 우주 위치는 km, 표면 질의는 m, Actor 배치는 cm를 사용한다.

## 이전 고정 조명에서 시간 기반 관측으로

고정 조명은 실험 메뉴를 읽기에는 충분했다. 그러나 낮·밤과 날씨를 연결하려면 저장을 복원한 같은 시각에서 같은 태양 방향이 나와야 한다. [결정 19](../DesignLog.md)와 [천체·표면 질의 설계](../EnvironmentPlan.md#천체표면-질의의-구현-설계)가 이 요구의 근거다.

출처: `4458dea`, [CCLCelestialSystem.cpp](../../Source/KSICelestial/Private/KSICelestialSystem.cpp), `const double E = EccentricAnomaly`, 205-209줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
    const double E = EccentricAnomaly(Phase(Elapsed, Body.OrbitalPeriodSeconds, Body.MeanAnomalyDegrees), Body.Eccentricity);
    const FVector3d Orbit(Body.SemiMajorAxisKm * (FMath::Cos(E) - Body.Eccentricity),
        Body.SemiMajorAxisKm * FMath::Sqrt(1. - Body.Eccentricity * Body.Eccentricity) * FMath::Sin(E), 0.);
    State.PositionKm = Candidate[ParentIndices[Index]].PositionKm + Orient(Orbit, Body);
}
```

부모 천체가 먼저 계산된 배열에서 부모 위치를 읽는다. `EccentricAnomaly`가 시각과 궤도 입력을 타원 위 각도로 바꾸고, `Orient`가 궤도면 방향을 반영한다. 위성도 같은 방법으로 부모 행성의 위치에 상대 위치를 더한다. 매 프레임 이전 위치에 속도를 누적하는 적분 방식과 달리 지정 시각을 다시 평가할 수 있다.

예를 들어 이심률 0인 원 궤도에서 각도가 0이면 괄호 속 좌표는 `(반장축, 0, 0)`이다. 90도이면 `(0, 반장축, 0)`이다. 이것은 식을 이해하기 위한 예시이며 실제 콘텐츠의 궤도 크기를 정하는 값은 아니다. 현재 모델은 입력한 궤도를 평가하며 천체끼리의 다체 중력을 풀지 않는다.

`Observe`는 평가 결과에 관찰자의 위도·경도와 자전 방향을 적용한다. 실험장의 `ACCLWorldEnvironmentState`가 확정 관측을 전달하고 `ACCLWorldEnvironmentPresentation`이 태양·하늘을 표시한다. 조명을 움직였다는 사실만으로 서버의 일조가 변경되지 않는다.

## 다층 표면과 세 가지 차폐

동굴 바닥과 지붕이 같은 수평 좌표에 있을 수 있다. 단일 높이값으로는 둘을 구별하기 어렵다. `ICCLSurfaceProvider`는 광선에 걸리는 여러 표면과 각 표면의 재질·투과율을 반환하는 프로젝트 인터페이스다. 처음에는 유한 평면 패치를 사용했고, 영구 지형 단계에서는 확정 밀도 지형 공급자를 같은 경계에 연결했다.

출처: `4458dea`, [CCLSurfaceQuery.cpp](../../Source/KSICore/Private/KSISurfaceQuery.cpp), `const FVector3d Directions[]`, 254-259줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
const FVector3d Directions[] = { Query.ToSun, Query.ToPrecipitationSource, Query.ToWindSource };
const FVector3d Offsets[] = { FVector3d::ZeroVector, FVector3d(Query.ProbeRadiusMeters, 0., 0.),
    FVector3d(-Query.ProbeRadiusMeters, 0., 0.), FVector3d(0., Query.ProbeRadiusMeters, 0.), FVector3d(0., -Query.ProbeRadiusMeters, 0.) };
const int32 ProbeCount = Query.ProbeRadiusMeters > 0. ? 5 : 1;
double Channels[3] = { 0., 0., 0. };
for (int32 Channel = 0; Channel < 3; ++Channel)
```

일사는 태양 쪽, 강수는 강수원 쪽, 바람은 바람원 쪽을 각각 검사한다. 지붕이 비를 막아도 열린 옆문으로 바람이 들어올 수 있기 때문이다. 각 광선이 통과한 표면의 해당 채널 투과율을 곱한다. 설명용으로 투과율 0.5인 표면 두 개를 통과하면 `1 × 0.5 × 0.5 = 0.25`가 남는다.

질의에는 `Revision`과 `Epoch`를 함께 넣는다. Revision은 내용 변경 번호, Epoch는 실행 세대다. 저장 복원으로 예전 Revision이 다시 등장해도 Epoch가 다르면 예전 질의 결과를 그대로 사용하지 않는다. 질의 도중 공급자 상태가 바뀌면 계산을 거부한다. 단순 `bIndoor` 하나보다 상태가 많아지지만 지붕·문·다층 지형의 서로 다른 영향을 표현할 수 있다.

## 개구부와 지형 공급자의 연결

문은 열린 비율만 가진 표시물이 아니라 표면의 실제 개구 영역이다. 반 열림은 열린 사각 영역에만 적용한다. 공간 A와 B를 연결하는 ID는 뒤의 [실내 환기](EnvironmentAir.md)가 사용한다. 문이 열렸어도 바깥에 다른 벽이 막고 있으면 실효 환기 면적은 별도로 판단해야 한다.

`FCCLSurfaceScene`에 지형 공급자를 등록하면 같은 질의 경계를 기존 평면과 확정 지형이 구현한다. [영구 지형](EnvironmentTerrain.md)에서 후보 메시가 아닌 게시된 상태만 읽는 이유를 이어서 다룬다. 표면 공급자 교체와 공통 시계는 프로젝트 설계다. UE5의 double 좌표 타입을 사용한다는 사실과 궤도·차폐 계산 자체를 엔진이 제공한다는 주장은 구분한다.

## 실패·검증과 비용

정의 로딩은 부모 관계·범위를 검사하고 유효한 후보만 적용한다. 잘못된 궤도나 오래된 표면 세대를 거부했을 때 이전 관측·장면을 유지해야 한다. 광선마다 여러 표면을 읽는 비용은 단일 높이 질의보다 크며, 현재 모델은 정밀 반그림자·대기 굴절·복잡한 유체 바람을 계산하지 않는다.

검사 코드는 [CCLCelestialTests.cpp](../../Source/CCL/Environment/CCLCelestialTests.cpp), [CCLSurfaceQueryTests.cpp](../../Source/CCL/Environment/CCLSurfaceQueryTests.cpp)다. 기존 실행과 화면 검토는 [단계 2 완료 기록](../EnvironmentPlan.md#단계-2-완료-결과)에 있다. 이번 작업은 소스·기록 대조이며 천문 관측 정확도나 게임 화면을 새로 측정하지 않았다.

| 상황 | 기대 결과 |
|---|---|
| 같은 정의·세계 시각 재평가 | 같은 위치·일조 |
| 부모 행성의 공전과 위성 평가 | 움직인 부모 위치를 기준으로 위성 계산 |
| 지붕 아래와 열린 문 옆 | 일사·강수·바람 차폐를 각각 판정 |
| 복원 전 Epoch로 질의 | 오래된 결과 거부 |

**문을 열면 비 노출률과 환기량을 같은 숫자로 써도 될까?** 비는 특정 위치로 오는 광선의 차폐이고 환기는 공간 사이 교환 면적이다. 같은 문 기하를 참고하지만 입력과 결과의 단위가 다르다.

**하늘의 행성 크기를 보기 좋게 키우면 서버 궤도 반지름도 바꿔야 할까?** 관측 UI의 표시 축척은 표현 값이다. 물리 입력을 바꾸면 일조·주기 의미까지 달라지므로 별개로 유지한다.
