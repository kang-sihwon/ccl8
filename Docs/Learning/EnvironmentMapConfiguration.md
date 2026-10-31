# 환경 맵 설정: 이름과 에셋 경로에서 역할을 분리하기

맵의 저장 도메인과 캐릭터 눈 후처리는 `ACCLWorldEnvironmentConfig`의 데이터로 선택한다. 이 장에서는 같은 맵의 이름을 바꾸거나 눈 실험을 다른 맵으로 옮길 때 코드의 문자열 조건까지 고쳐야 했던 구조를 비교한다.

## 비교 기준과 개념

이전 `c8816e7`과 변경 `b888b13`을 비교하며 작성 기준은 `d7782f1`이다. 현재 기준은 2026-10-31의 `main`인 `d4888b0`이고, `b888b13` 발췌 세 개는 현재 코드에서 같은 줄을 확인했다. c8816e7 발췌는 이전 코드다. 전체 해시는 [후속 비교 근거](EnvironmentFollowupEvidence.md)에 있다. 변경 사실과 이유의 기록은 `b888b13` 커밋 본문과 [환경 계획](../EnvironmentPlan.md#다음-작업의-시작점)이다.

도메인은 Campaign·Playground·Scenario 중 어떤 영구 세계 영역을 사용하는지 정하는 분류다. `ACCLWorldEnvironmentConfig`는 월드에 배치하는 `AInfo` 파생 Actor다. `UPROPERTY(EditAnywhere, BlueprintReadWrite)`는 해당 값을 에디터와 Blueprint에서 다룰 수 있게 노출한다. 노출했다고 자동 저장·복제가 되는 것은 아니다.

`TSoftClassPtr<UAnimInstance>`는 애니메이션 클래스에 대한 지연 로딩 참조다. `LoadSynchronous` 호출 시 실제 클래스를 읽으므로 필요할 때 고를 수 있지만 그 호출은 동기 로딩 비용을 발생시킬 수 있다. 다음 비교는 프로젝트의 책임 배치 변경이다. 이 타입과 맵 설정 방식 자체를 UE5만의 신규 기능으로 설명하지 않는다.

## 저장 도메인을 고르는 코드

출처: `c8816e7`, [CCLWorldSimulationSubsystem.cpp](../../Source/CCL/Environment/CCLWorldSimulationSubsystem.cpp). 연속 발췌이며 들여쓰기만 정리했다.

```cpp
ECCLWorldDomain UCCLWorldSimulationSubsystem::DomainForWorld(const UWorld* World)
{
    const FString Name = UGameplayStatics::GetCurrentLevelName(World, true);
    if (Name == TEXT("EnvironmentPlayground"))
    {
        return ECCLWorldDomain::Playground;
    }

    return Name == TEXT("EnvironmentScenario") ? ECCLWorldDomain::Scenario : ECCLWorldDomain::Campaign;
}
```

이전 코드는 두 실험 맵의 정확한 이름만 특별 취급했다. 설명용으로 Scenario 맵을 복제해 새 이름으로 저장하면 마지막 분기에 따라 Campaign으로 분류될 수 있다. 이는 코드로 도출한 예이며 실제 데이터 오염 사고가 있었다는 기록은 아니다.

출처: `b888b13`, [CCLWorldSimulationSubsystem.cpp](../../Source/CCL/Environment/CCLWorldSimulationSubsystem.cpp). 연속 발췌이며 들여쓰기만 정리했다.

```cpp
ECCLWorldDomain UCCLWorldSimulationSubsystem::DomainForWorld(const UWorld* World)
{
    static_assert(static_cast<uint8>(ECCLWorldContentDomain::Campaign) == static_cast<uint8>(ECCLWorldDomain::Campaign));
    static_assert(static_cast<uint8>(ECCLWorldContentDomain::Playground) == static_cast<uint8>(ECCLWorldDomain::Playground));
    static_assert(static_cast<uint8>(ECCLWorldContentDomain::Scenario) == static_cast<uint8>(ECCLWorldDomain::Scenario));

    // Maps declare their store in data. A map without environment config uses the campaign store.
    const auto* Config = ACCLWorldEnvironmentConfig::Find(World);
    return Config ? static_cast<ECCLWorldDomain>(Config->Domain) : ECCLWorldDomain::Campaign;
}
```

현재 함수는 Config를 찾아 Domain을 읽는다. 두 enum의 값이 같아야 캐스팅할 수 있으므로 `static_assert`로 대응을 검사한다. 설정 Actor가 없으면 Campaign으로 돌아간다는 기본값도 유지된다. 맵을 복제할 때 Actor의 Domain을 함께 검토해야 하며, 설정이 누락된 실험 맵을 자동 오류로 거부하는 정책은 아니다.

## 눈 후처리를 적용하는 책임

기존에는 공통 Character의 BeginPlay에서 맵 이름에 `Environment`가 들어 있는지 검사했다.

출처: `c8816e7`, [CCLCharacter.cpp](../../Source/CCL/CCLCharacter.cpp). 연속 발췌이며 들여쓰기만 정리했다.

```cpp
void ACCLCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (GetWorld()->GetMapName().Contains(TEXT("Environment")))
    {
        if (auto* SnowClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Environment/Animation/ABP_SnowPostProcess.ABP_SnowPostProcess_C")))
        {
            GetMesh()->SetOverridePostProcessAnimBP(SnowClass);
        }
    }
}
```

문자열 조건에 맞으면 하드코딩한 애니메이션 Blueprint 경로를 로딩했다. 같은 이름 규칙을 따르는 모든 환경 맵이 같은 후처리를 받는 구조다. 눈 이동을 쓰는 다른 캐릭터나 다른 후처리 에셋을 선택할 책임도 Character 쪽에 남는다.

수정 뒤에는 눈 이동 컴포넌트의 BeginPlay가 설정을 읽는다.

출처: `b888b13`, [CCLSnowMovementComponent.cpp](../../Source/CCL/Environment/CCLSnowMovementComponent.cpp). 연속 발췌이며 들여쓰기만 정리했다.

```cpp
if (const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld()); Config && CharacterOwner && CharacterOwner->GetMesh())
{
    if (auto* PostProcess = Config->CharacterPostProcessAnim.LoadSynchronous())
    {
        CharacterOwner->GetMesh()->SetOverridePostProcessAnimBP(PostProcess);
    }
}
```

Config·캐릭터·메시가 있어야 로딩을 시도하고, 클래스가 로딩돼야 후처리를 설정한다. 실제 눈 접촉과 예측 입력은 [깊은 눈과 이동](EnvironmentSnow.md)의 경로를 그대로 따른다. 후처리 애니메이션은 발 표현을 연결하는 선택이며, 이 코드가 눈 질량이나 서버 이동 판정을 소유하지 않는다.

## 설정·로딩·종료에서 확인할 것

`ACCLWorldEnvironmentConfig::Find`는 월드의 해당 Actor를 순회해 첫 번째를 반환한다. 복수 설정 Actor의 우선순위나 병합을 구현한 함수는 아니다. 맵별로 어느 Actor가 도메인과 후처리를 소유하는지 명확히 해야 한다. 환경 초기화의 `BuildInputs`는 천체 정의·표면·개구부·관측 ID를 후보로 검사한 뒤 출력에 옮긴다.

후처리 클래스가 비어 있거나 로딩되지 않으면 위 분기는 적용을 건너뛴다. 이 짧은 코드에는 이전 후처리를 명시적으로 해제하는 처리가 없다. BeginPlay 시 선택 경로이므로 런타임 중 설정 Actor 값을 바꾸기만 하면 기존 캐릭터의 후처리도 즉시 재설정된다고 가정하면 안 된다. 캐릭터 재생성 시에는 새 컴포넌트가 다시 설정을 읽는다.

같은 검토 변경에서 날씨·불 Niagara 에셋을 만드는 코드는 에디터 전용 `Source/CCL/Editor/`의 `CCLEnvironmentFXLibrary.cpp`로 이동했다. 이 파일은 이후 `Source/CCLEditor/`로 옮겨져 지금은 [CCLEnvironmentFXLibrary.cpp](../../Source/CCLEditor/Private/CCLEnvironmentFXLibrary.cpp)다. 게임 중 만들어 둔 효과를 재생하는 일과 편집기에서 에셋을 저작하는 일의 수명을 나눈 것이다. 상세 빌드 조건은 [CCL.Build.cs](../../Source/CCL/CCL.Build.cs), 실제 패키지 검증 범위는 [Packaging](../Packaging.md)이 소유한다.

## 대안·확인과 이해 점검

맵 이름 분기는 맵 두 개만 있을 때 설정 누락이 적고 읽기 쉽다. 데이터 설정은 이름 변경과 기능 선택을 분리하는 대신 배치·에셋 참조·쿠킹을 검사해야 한다. 소프트 참조를 썼다는 사실만으로 비동기 로딩이나 패키지 에셋 포함이 보장되지는 않는다.

코드 검토에서 확인할 상황은 맵 이름 변경 뒤 같은 Domain 유지, Config 누락 시 Campaign 기본값, 빈 후처리 참조의 적용 생략, 눈 컴포넌트가 있는 새 캐릭터의 BeginPlay다. 이는 코드에서 도출한 확인 항목이며 이번 문서 작성에서 직접 실행한 결과가 아니다. 기존 Editor 재검증과 최신 변경 이후의 패키지 미검증은 [후속 비교 근거](EnvironmentFollowupEvidence.md)에 분리했다.

**이름을 바꿔도 동작하니 설정을 검사하지 않아도 될까?** 이름 의존성이 줄었을 뿐이다. 복제한 맵의 Domain이 잘못돼 있거나 Config가 없으면 의도와 다른 저장 영역을 선택할 수 있다.

**후처리 클래스가 비어 있으면 눈 이동도 멈출까?** 이 분기는 메시의 후처리 적용만 건너뛴다. 눈 이동 판정과 표면 시뮬레이션의 활성 여부는 각 코드 경로에서 따로 확인해야 한다.
