# 깊은 눈과 이동: 질량·깊이·예측 입력을 나누기

2026-10-12 공용 모듈 분리 이후의 위치는 [Framework 파일 이동표](../FrameworkPlan/Stage4.md#코드-위치의-전후-비교)를 따른다. 아래 발췌·이름·줄 번호·해시는 기록 당시 기준을 유지하며 소스 링크는 현재 대응 파일을 연다.

후속 변경에서 눈 후처리를 Character의 맵 이름 조건에서 컴포넌트의 설정 참조로 옮겼다. 실제 전후 코드는 [환경 맵 설정](EnvironmentMapConfiguration.md)에 있으며 아래의 기존 예측·질량 설명은 당시 기준을 유지한다.

깊은 눈은 물의 양을 보존하면서 눌리고, 캐릭터는 그 깊이에 따라 저항과 발 배치를 바꾼다. 이 장에서는 일반 CMC를 확장한 이유와 클라이언트 재실행에서 과거 눈 입력을 유지하는 방법을 배운다.

## 비교 기준과 필요한 개념

범위는 자연환경 5단계이다. 비교 기준은 `50e5838` → `0408d94`, 표현 보완 `a817082`이며 전체 해시·브랜치 관계·작업 트리 식별값은 [비교 근거](EnvironmentEvidence.md)에 있다. `WT-2026-10-10`은 문서 작성 중 읽은 미커밋 소스 사본을 뜻한다. 이후 수정된 HEAD와 동일하다고 가정하지 않는다.

CMC는 `UCharacterMovementComponent`의 약칭이다. SavedMove는 클라이언트가 서버 보정 뒤 이동을 다시 실행하기 위해 보관하는 입력이다. IK는 목표 위치에 맞게 관절을 배치하는 계산이며 여기서는 두 발의 목표를 보정한다. 서버의 눈 체적, 로컬의 임시 눌림, 애니메이션 목표는 서로 다른 상태다.

[결정 19](../decisions/0019-natural-environment-and-labs.md)는 쌓인 눈을 통과하면 길이 남고 융해·물 순환으로 이어지도록 요구했다. 표면을 색칠하는 효과만으로는 이동 저항과 영구 흔적을 설명할 수 없다. [눈 검토 기록](../EnvironmentPlan/Snow.md)은 자기 발밑 압축으로 저항이 약해지는 문제와 강제 보정 경로의 수정을 기록한다.

## 일반 이동에서 눈 입력을 소비하는 이동으로

이전 Character 생성자는 기본 이동 컴포넌트를 사용했다.

출처: `50e5838`, [CCLCharacter.cpp](../../Source/CCL/CCLCharacter.cpp), `ACCLCharacter::ACCLCharacter()`, 25-26줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
ACCLCharacter::ACCLCharacter()
{
```

변경 후에는 기본 서브오브젝트의 클래스를 지정한다.

출처: `0408d94`, [CCLCharacter.cpp](../../Source/CCL/CCLCharacter.cpp), `ACCLCharacter::ACCLCharacter(`, 27-29줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
ACCLCharacter::ACCLCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UCCLSnowMovementComponent>(ACharacter::CharacterMovementComponentName))
{
```

`FObjectInitializer`는 UObject 생성 과정의 초기화 문맥이다. `SetDefaultSubobjectClass`는 ACharacter가 만들 이동 컴포넌트 자리에 파생 클래스를 지정한다. 이동 컴포넌트를 하나 더 만들어 두 개를 동시에 구동하는 방식이 아니다. CMC의 기존 이동·네트워크 흐름을 유지한 채 눈 관련 입력을 확장한다. 이 확장 방식과 CMC는 UE4 계열에도 있었으며 프로젝트 변경을 UE5 전용 발명으로 설명하지 않는다.

## 물 환산량과 외형 체적

`SnowCubicMeters`는 녹였을 때의 물 부피, `SnowVolumeCubicMeters`는 보이는 눈의 부피다. 깊이는 외형 체적을 셀 면적으로 나눈다. 예를 들어 면적 1m²에 외형 0.6m³면 깊이는 0.6m다. 물 환산량이 0.06m³이면 밀도 비율은 `0.06 / 0.6 = 0.1`이다. 압축으로 외형이 줄어도 물 환산량은 줄이지 않는다.

접촉 계산은 일부 눈을 이웃으로 옮기고 남은 눈을 압축한다. 이웃 공간이 부족하면 남은 양을 원래 셀에 보관한다. 융해할 때는 물 환산량을 액체로 옮긴다. 물과 눈을 함께 보존하는 장부는 [물 순환](EnvironmentWater.md)에서 설명했다.

출처: `0408d94`, [CCLSnowSimulation.cpp](../../Source/KSIEnvironment/Private/KSISnowSimulation.cpp), `if (Sequence <= SnowContacts.FindRef(SourceId))`, 57-60줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
if (Sequence <= SnowContacts.FindRef(SourceId))
{
    return true;
}
```

`SourceId`는 접촉 출처, `Sequence`는 그 출처의 진행 순번이다. 이미 처리한 순번이면 같은 발자국을 다시 압축하지 않는다. 검사 실패 때 순번을 먼저 소비하지 않고 격자 검증 성공 후 눈과 영수증을 함께 확정한다. 저장 복원에도 영수증을 포함해야 재전송된 접촉이 중복 반영되지 않는다.

## 저항과 예측 재실행

출처: `0408d94`, [CCLSnowMovementComponent.cpp](../../Source/CCL/Environment/CCLSnowMovementComponent.cpp), `float UCCLSnowMovementComponent::GetMaxSpeed`, 168-172줄. 실제 코드의 연속 발췌이며 들여쓰기만 정리했다.

```cpp
float UCCLSnowMovementComponent::GetMaxSpeed() const
{
    const float Base = Super::GetMaxSpeed();
    return IsMovingOnGround() ? Base * FMath::Clamp(1.f / (1.f + 3.f * SnowDepthMeters), 0.25f, 1.f) : Base;
}
```

깊이 0.6m라면 제한 전 비율은 `1 / (1 + 3 × 0.6) = 1 / 2.8`, 약 0.357이다. 예시 기본 속도 600cm/s라면 약 214cm/s가 된다. 실제 기본 속도를 600으로 확정하는 예가 아니라 식을 계산하는 연습이다. 캡슐은 지지 바닥을 걷고 부드러운 눈 표면에는 별도 지지 충돌을 만들지 않는다.

앞쪽 캡슐 주변의 눈을 읽어 이미 압축한 발밑 때문에 저항이 즉시 사라지는 것을 막는다. `FSnowSavedMove::SetMoveFor`는 표면 게시 순번·접촉 순번·깊이를 저장하고 `PrepMoveFor`는 보정 후 같은 입력을 복원한다. 네트워크 패킷에는 두 순번만 추가한다. 서버는 자기 전송 이력에서 깊이를 찾으므로 클라이언트가 보낸 임의의 깊이를 신뢰하지 않는다.

이력이 만료되거나 현재보다 과도하게 얕은 과거 상태를 요구하면 서버 현재 상태로 계산하고 보정한다. UE 5.8.3의 `ForceClientAdjustment()`는 보정 시각만 초기화한다. 실제 강제 보정에는 `bForceClientUpdate`도 설정한다. 근거는 [엔진 근거](EnvironmentEvidence.md#엔진-확인)와 [눈 사양](../EnvironmentPlan/Snow.md#cmc-예측과-발-배치)에 있다.

## 발 배치·실패·종료

`UCCLSnowAnimInstance`는 원래 포즈 뒤에서 두 다리 IK를 적용한다. 게임 스레드의 PreUpdate가 표면에서 목표를 복사하고 평가 경로는 그 복사값과 포즈를 사용한다. 다른 스레드에서 가변 World 상태를 직접 찾아 읽는 것으로 설명하면 안 된다. 지지 발과 다음 발 위치의 눈 높이를 함께 고려하지만 전신 균형·피로를 완성한 보행 시스템은 아니다.

권위 접촉은 서버 CMC가 실제 이동 경로를 나눠 적용한다. 순간이동은 출발점부터 연속 발자국을 긋지 않는다. 클라이언트 임시 눌림은 응답이나 만료로 정리한다. 초기화·복원 시 이동 위치와 표면 세대를 맞춘다. 전체 표면 이력을 접속자별로 보관하는 방식은 재실행 근거가 명확한 대신 메모리·전송 비용이 생긴다.

## 확인과 이해 점검

기존 [눈 실행 검증](../EnvironmentPlan/Snow.md#실행-근거와-검토-결과)은 흔적 저장·맵 왕복·지연과 손실 조건에서의 실제 이동 보정을 구분해 기록한다. 소스는 [CCLSnowSimulationTests.cpp](../../Source/CCL/Environment/CCLSnowSimulationTests.cpp)와 [CCLSnowSmokeSubsystem.cpp](../../Source/CCL/Tests/CCLSnowSmokeSubsystem.cpp)다. 이번에는 재실행하지 않았다.

| 상황 | 기대 결과 |
|---|---|
| 같은 SourceId·Sequence 재요청 | 추가 압축 없음 |
| 눈을 압축한 뒤 융해 | 물 환산량 보존 |
| 서버 보정 후 SavedMove 재실행 | 저장했던 입력 복원 |
| 접촉 위치가 유효 표면 밖임 | 상태·순번 보존 |

**눈 깊이를 줄인 만큼 물 장부에서도 빼야 할까?** 압축은 외형 부피를 줄인다. 물 환산량까지 빼면 밟을 때마다 물질이 사라진다.

**클라이언트가 현재 깊이만 다시 읽으면 될까?** 보정으로 과거 이동을 재실행할 때 환경은 이미 바뀌었을 수 있다. 이동이 사용했던 표면 식별값과 입력을 함께 보관해야 같은 동작을 재현할 수 있다.
