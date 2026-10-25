# ccl8 코딩 규칙

코드를 설계·작성·리뷰·검증할 때 따르는 규칙이다. 파일 수정과 실행의 경계, 설계 원칙, 사람이 판단할 배치 규칙, 작업 완료 절차를 소유한다. 기계로 판정할 수 있는 형식 규칙은 `Tools/Agent/cpp-rules.cjs`가 검사하고 이 문서에는 그 목록만 둔다.

## 파일 경계와 실행 범위

AI는 [Direction](Direction.md)의 합의된 제작 범위에서 `Source/`, `Config/`, `Build.cs`, `Target.cs`, `.uproject`와 에셋을 작성·수정하고 검증한다. 매 파일의 재승인은 받지 않는다. 근거는 [결정 2](DesignLog.md)다. 리뷰만 요청한 작업은 리뷰로 끝낸다.

구현 전에 관련 기획·아트·사양 문서를 읽고 목표, 변경 범위, 검증 기준을 해당 문서에 적는다. 통상적인 구현 선택은 AI가 판단하고 기록한다. 장르·핵심 경험·콘텐츠 범위의 변경, Orbis 원본 수정, 유료 구매, 외부 공개는 사용자의 별도 지시를 따른다.

기준 환경은 Windows와 UE 5.8.1 소스 빌드다. 실제 엔진 버전은 `<Engine>/Build/Build.version`에서 확인한다. 엔진 소스는 읽기 전용 참고 자료이고, 경로는 로컬 설정에 둔다([AgentContextTools](AgentContextTools.md)).

빌드 타깃은 `CCL`, `CCLEditor`, `CCLServer`다. 분리 모듈의 목록·의존·독립 검사용 최소 프로젝트는 [FrameworkPlan](FrameworkPlan.md)이 소유한다. 그 밖의 프로젝트 빌드 타깃은 실행하지 않는다. 엔진 전체 재빌드와 대형 인덱스 생성은 필요성과 비용을 먼저 사용자에게 제시한다.

## 장르에 독립적인 베이스 시스템 설계

HUD, Inventory, Item, Drop, Save/Load, 일반 전투 판정처럼 여러 장르가 쓰는 베이스 시스템은 특정 장르나 콘텐츠를 전제하지 않는다. 근거는 [결정 9](DesignLog.md)다. 공통 도메인의 책임과 데이터 흐름을 먼저 정하고, 장르 규칙과 콘텐츠는 데이터·정책·기능 조합으로 연결한다.

정의 데이터, 개별 런타임 상태, 기능 실행, 표현을 나눈다. 공통 기반은 구체적인 무기, 몬스터, 플레이어 클래스, 화면 구성을 참조하지 않고, 콘텐츠 계층이 공통 계약을 사용한다. 소유자와 수명, 서버 권위, 복제 범위, 저장 경계는 공통 계약에 적는다.

### 적용 예시

| 시스템 | 공통 기반의 책임 | 콘텐츠에서 조합·선택할 규칙 |
|---|---|---|
| HUD | 표시 데이터의 조회·변경 통지와 화면 요소 연결 | 체력바, 탄약 표시, 퀘스트 목록과 화면 배치 |
| Inventory | 아이템 보관·이동·조회와 변경 결과의 일관성 | 슬롯·무게·태그 제한, 장비 규칙과 소유자 종류 |
| Item | 정의 식별, Fragment 조합과 개별 상태 관리 | 장비·소비·탄약·재료 등 기능과 사용 효과 |
| Drop | 조건에 따른 보상 선택과 지급 요청 | 드롭 테이블, 확률, 소유권, 월드 생성 또는 직접 지급 |
| Save/Load | 저장 레코드의 식별·버전 관리·직렬화와 복원 연결 | 저장 대상, 저장 시점, 진행·성장·체크포인트 정책 |
| 일반 전투 판정 | 판정 문맥, 대상 검증, 적중 식별과 결과 전달 | 피해 공식, 적대 관계, 중복 적중 정책, 회피·가드·패링과 자원 비용 |

Item을 예로 들면 `ItemDefinition`이 필요한 `ItemFragment`를 조합한다. 표시, 중첩, 장비, 소비, 전투 기능은 필요한 아이템에만 붙인다. 아이템 종류마다 파생 클래스를 만들거나 종류 enum 분기로 기능을 정하는 구조는 기본값이 아니다.

공유 `ItemDefinition`에는 변하지 않는 정의를 두고, 수량·내구도·강화처럼 개별 아이템마다 바뀌는 값은 런타임 상태에 둔다. Fragment는 기능별 정의를 제공하고 실제 사용·장착·공격은 실행 기능이 처리한다. 저장 계약은 정의 식별자와 필요한 개별 상태만 담고 UI나 일시적인 Actor 참조를 담지 않는다. Fragment의 `UObject`·구조체 표현, 에셋 식별, 복제 방식은 사양서에서 비교해 정한다.

조합 원칙은 도메인에 맞게 적용한다. 모든 시스템에 Fragment라는 같은 자료구조를 강제하지 않는다. 공통 기반이 특정 장르의 스태미나, 패링, 장비 슬롯, 플레이어 HUD를 필수 구성으로 요구하면 위반이다.

### 설계와 검토 기준

설계자는 공통 책임, 콘텐츠 규칙, 확장 지점을 구분하고 의존 방향을 제시한다. 검토자는 현재 장르와 다른 사용 사례를 대입해 기반 수정 없이 데이터·정책·조합으로 연결되는지 본다. 예를 들어 Inventory는 상자에도, 일반 전투 판정은 스태미나가 없는 파괴 가능한 물체에도 적용할 수 있어야 한다.

범용 계약과 확장 지점은 베이스 설계 때 정하고 실행 기능은 지금 필요한 범위부터 구현한다. 가상의 기능을 위해 클래스 계층을 늘리지 않는다. 모듈·플러그인 분리는 의존성과 재사용 경계로 판단하며 범용성의 필수 조건이 아니다.

이 방식은 장르 전용 구현보다 계약, 연결 코드, 데이터 검증 비용이 크다. 대신 같은 기반을 다른 콘텐츠에 조합하고 기능별로 검증할 수 있다. 제작 기간이 짧다는 이유로 장르 규칙을 공통 기반에 고정하지 않는다.

## UE5 신규·현행 시스템 우선

적용 대상에 맞는 UE5의 신규·현행 시스템을 기본으로 쓴다. 근거는 [결정 10](DesignLog.md)이다. 대표 대상은 지오메트리의 Nanite, 월드 구성·스트리밍의 World Partition, 능력·효과·능력치의 Gameplay Ability System(GAS)과 AttributeSet이다. GAS처럼 UE4부터 있었고 UE5에서도 현행인 시스템도 포함한다.

기존 방식이나 자체 구현은 현행 시스템이 명확히 불합리하다는 근거가 있을 때만 택한다. 근거는 대상 플랫폼·하드웨어의 지원 제약, 필수 요구와의 충돌, 재현된 결함, 측정한 성능·메모리·제작 비용이다. 어떤 조건에서 왜 불합리한지, 대안과 트레이드오프를 함께 기록한다. 익숙하지 않음, 초기 설정 비용, 작은 프로젝트라는 이유, 검증하지 않은 성능 우려는 근거가 아니다.

채택과 설정은 사용 중인 엔진의 소스·지원 조건과 프로젝트 검증으로 확인한다. 기본 선택, 실제 활성화, 검증 완료를 구분해 기록한다.

## 코드 관례

- Epic C++ Coding Standard의 `A`, `U`, `F`, `E`, `I` 타입 접두사와 불리언 변수의 `b` 접두사를 쓴다.
- 헤더의 클래스·구조체 불리언 멤버는 `uint8 bDead = 0;`처럼 `uint8`로 선언하고 0 또는 1만 저장한다. 근거는 [결정 4](DesignLog.md)다. 함수 반환값·매개변수와 `.cpp` 지역 변수는 `bool`을 쓸 수 있고, 엔진 오버라이드와 외부 인터페이스의 시그니처는 원래 타입을 유지한다.
- UE의 문자열·컨테이너·오브젝트 참조 타입을 쓴다. 리플렉션과 에디터 노출은 필요한 범위에만 붙인다.
- 전방 선언과 직접 필요한 헤더만 include한다. 모듈 의존은 해당 `Build.cs`에 적는다.
- 책임, 수명, 상태 소유자, 네트워크 권위를 설계에서 설명한다. 멀티플레이 범위와 미정 요구는 [GameDesign](GameDesign.md)을 따른다.
- 폴더·모듈·성능 목표는 요구사항이 정해진 뒤 고른다.

## 기계 검사 규칙

`node Tools/Agent/cpp-rules.cjs check [--summary] [paths...]`가 아래 규칙을 보고한다. 소스는 고치지 않는다. 바꾼 줄이 `.clang-format`과 맞는지는 `node Tools/Agent/cpp-rules.cjs format [--base <rev>]`가 검사한다. 파일 전체에는 적용하지 않는다. 기존 파일은 namespace 들여쓰기와 이어진 줄 정렬이 섞여 있어 규칙과 무관한 차이가 수천 건 생기기 때문이다. 바꾼 파일에 새 위반을 남기지 않는다. 기존 위반의 정리는 별도 작업으로 한다.

| 규칙 | 내용 | 근거 |
|---|---|---|
| `pragma-once` | 헤더는 `#pragma once`로 시작한다 | 결정 6 |
| `coreminimal-first` | 헤더의 첫 include는 `CoreMinimal.h`다 | 결정 6 |
| `generated-include-last` | `.generated.h`는 마지막 include다 | 결정 6 |
| `own-header-first` | `.cpp`의 첫 include는 대응하는 클래스 헤더다 | 결정 6 |
| `bool-member` | 헤더의 `b` 불리언 멤버는 `uint8`이다 | 결정 4 |
| `editor-uproperty` | `UPROPERTY`는 `WITH_EDITOR`가 아니라 `WITH_EDITORONLY_DATA`로 감싼다 | UHT, 아래 설명 |
| `editor-only-data-nested` | `WITH_EDITORONLY_DATA`를 `WITH_EDITOR` 안에 중첩하지 않는다 | 결정 6 |
| `generated-body-blank` | `GENERATED_BODY()` 뒤에 선언이 이어지면 빈 줄 하나, 클래스가 끝나면 빈 줄 없음 | 결정 13 |
| `access-blank` | 접근 지정자 바로 뒤에 빈 줄을 두지 않는다 | 결정 13 |
| `blank-double` | 빈 줄을 두 줄 연속으로 두지 않는다 | 결정 7, 13 |
| `blank-after-open`, `blank-before-close` | 여는 중괄호 바로 뒤와 닫는 중괄호 바로 앞에 빈 줄을 두지 않는다 | 결정 13, 14 |
| `brace-own-line` | 제어문의 여는 중괄호는 자기 줄에 두고 한 줄 블록을 쓰지 않는다 | 결정 14 |
| `braced-body` | `if`, `else`, `for`, `while` 본문은 한 문장이어도 중괄호로 감싼다 | 결정 14 |
| `else-line` | `else`는 닫는 중괄호 다음 줄에서 시작한다 | 결정 14 |
| `cpp-one-line-function` | `.cpp`에 정의한 함수는 짧아도 본문을 여러 줄로 펼친다 | 결정 14 |

`editor-uproperty`의 근거: UE 5.8.2의 `<Engine>/Source/Programs/Shared/EpicGames.UHT/Parsers/UhtPropertyParser.cs:668`은 CCL 같은 런타임 모듈에서 `WITH_EDITOR`로 감싼 `UPROPERTY`를 오류로 처리한다.

## 헤더 배치

[결정 6](DesignLog.md)의 순서로 쓴다. 검사기가 보지 않는 영역 순서와 선언 묶음은 리뷰에서 확인한다. 내용이 없는 선언 종류, 접근 지정자, 전처리 블록은 생략한다.

1. include는 `CoreMinimal.h`, 부모 클래스 헤더, 그 밖의 필요한 헤더, `.generated.h` 순이다.
2. 파일 범위 선언은 매크로(`#define`), 델리게이트, `extern`, enum, struct, class 순이다. 정의와 전방 선언 중 필요한 형태를 쓴다.
3. `GENERATED_BODY()` 다음은 생성자와 부모 인터페이스 함수 영역이다. 오버라이드는 반환형이나 이름에 관계없이 여기 둔다.
4. 다음은 클래스 자체 함수 영역이다. 일반 함수 뒤에 Get·Set과 단순 상태 조회·검사 함수를 둔다. 작업을 수행하고 성공 여부를 `bool`로 돌려주는 함수는 일반 함수다.
5. 에디터 전용 영역에는 함수와 프로퍼티를 함께 두고 다시 나누지 않는다. 함수는 `WITH_EDITOR`, 리플렉션 데이터는 `WITH_EDITORONLY_DATA`로 감싼다. 에디터 전용 오버라이드는 부모 인터페이스 영역보다 이 영역에 둔다.
6. 마지막은 멤버 변수 영역이다. `UPROPERTY`가 없는 멤버 변수도 여기 둔다.

각 영역 안의 접근 지정자는 `public`, `protected`, `private` 순이다. 영역이 바뀌면 같은 접근 지정자를 다시 쓴다. 같은 영역·접근 수준의 static 함수는 연속해서 묶는다. 단순 반환·설정·검사 함수는 헤더에 인라인으로 정의하고, 추가 의존성이나 복잡한 처리가 필요하면 CPP에 정의하되 그룹 순서를 유지한다.

빈 줄은 [결정 7과 결정 13](DesignLog.md)을 따른다. 검사기 규칙 밖에서 리뷰할 항목은 다음과 같다.

- 역할이 다른 함수 영역과 멤버 변수 영역은 빈 줄 하나로 나누고 필요한 접근 지정자를 다시 쓴다. 영역 주석이 있으면 빈 줄은 그 주석 앞에 둔다.
- `UFUNCTION`, 함수 선언, 바로 앞의 설명 주석은 내부 빈 줄 없이 한 묶음이다. 묶음과 이웃 선언 사이에는 빈 줄 하나를 둔다.
- 멤버 변수는 설명 주석이나 `UPROPERTY`가 붙은 경우에만 한 묶음으로 나누고 묶음 사이에 빈 줄 하나를 둔다. 둘 다 없는 멤버 변수끼리는 빈 줄 없이 잇는다.

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CCLCharacter.generated.h"

class UCameraComponent;

UCLASS()
class CCL_API ACCLCharacter : public ACharacter
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	ACCLCharacter();

	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	// 내 클래스 함수
public:
	void Die();

	bool IsDead() const { return bDead != 0; }

private:
	// 복제된 사망 상태를 표현한다.
	UFUNCTION()
	void OnRep_Dead();

	// 에디터 전용
#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	TObjectPtr<UObject> EditorPreview;
#endif

	// 프로퍼티
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(ReplicatedUsing = OnRep_Dead)
	uint8 bDead = 0;
};
```

예시는 배치를 보여 주는 발췌다. 에디터 전용 선언을 모든 클래스에 넣으라는 뜻이 아니다.

## CPP 배치

대응하는 클래스 헤더를 첫 include로 두고, 나머지 의존 헤더 뒤에 파일 범위 델리게이트, static 선언, `extern`, 익명 namespace, 디버깅용 선언을 차례로 둔다. 익명 namespace에는 파일 내부 상수와 보조 함수를 둔다. 상수는 상수식으로 초기화할 수 있을 때만 `constexpr`, 그 밖에는 `const`다. `FName`과 `FString`에 `constexpr`를 일괄 적용하지 않는다.

함수 정의는 생성자, 부모 인터페이스 오버라이드, 클래스 자체 일반 함수, Get·Set과 단순 검사 함수, `WITH_EDITOR` 함수 순이고 각 그룹 안에서는 헤더 선언 순서를 따른다. RPC의 `_Implementation`은 원래 RPC 선언의 그룹을 따르고, 부모의 `_Implementation`을 오버라이드하면 부모 인터페이스 그룹이다. 헤더에서 인라인으로 정의한 함수는 CPP에 다시 정의하지 않는다. `bool` 반환형만으로 그룹을 정하지 않는다. `ShouldCreateSubsystem`은 부모 인터페이스 그룹이다.

```cpp
#include "CCLCharacter.h"

// 필요한 의존 헤더

// 델리게이트, static, extern 선언

namespace
{
	constexpr float DefaultWalkSpeed = 500.f;
	const FName BodyComponentName(TEXT("Body"));
}

// 디버깅용 선언

// 생성자
ACCLCharacter::ACCLCharacter()
{
}

// 부모 인터페이스 함수
void ACCLCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	Die();
}

// 내 클래스 함수
void ACCLCharacter::Die()
{
}

void ACCLCharacter::OnRep_Dead()
{
}

// Get·Set·단순 검사 중 헤더에서 정의하지 않은 함수

// 에디터 전용
#if WITH_EDITOR
void ACCLCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
```

## CPP 함수 본문

함수 본문은 제어 흐름과 처리 단계가 보이게 쓴다. 근거는 [결정 14](DesignLog.md)다. 중괄호와 줄바꿈은 검사기가 보고, 리뷰는 단계 구분을 본다.

- 한 줄에는 실행문 하나만 둔다. `for`의 초기화·조건·증감 절은 예외다.
- 독립된 검증·분기 블록 사이와 블록이 끝난 뒤 다음 처리 단계 사이에 빈 줄 하나를 둔다. 같은 작업을 하는 연속 선언·호출은 한 묶음으로 둔다.
- `if`와 연결된 `else if`·`else` 사이에는 빈 줄을 넣지 않는다.

```cpp
if (!TargetASC)
{
    return FCCLHitResolution(FGameplayTag(CCLTags::Outcome_Rejected));
}

if (bBroken)
{
    TargetASC->CancelAllAbilities();
    TargetASC->ApplyEffect(UCCLStaggerEffect::StaticClass(), 0.f, TargetFighter->GuardBreakDuration);
}

TargetFighter->NotifyHit(Result);
return Result;
```

## 형식 정리와 배치 변경

형식과 배치만 바꾸는 작업은 동작을 보존한다. 조건식, 호출·평가 순서, 반환값, 변수 수명, 함수 시그니처와 본문, 접근 권한, 리플렉션 지정자를 그대로 둔다. 멤버 변수의 선언 순서는 초기화 순서를 정하므로 형식 통일을 위해 바꾸지 않는다. 함수 분리, 이름 변경, 로직 수정은 형식 정리와 다른 커밋으로 나눈다.

변경 전후를 함수별로 비교하고 프로젝트 빌드로 UHT와 컴파일을 확인한다. 배치와 공백만 바뀐 작업에는 멀티프로세스 플레이 검증을 반복하지 않는다.

## 코드 작업 완료 검증

코드 작업을 마칠 때 최종 변경 상태에서 다음을 순서대로 실행한다. 근거는 [결정 12](DesignLog.md)다. 문서만 바꾼 작업에는 적용하지 않는다.

1. 사용 중인 엔진의 `GenerateProjectFiles.bat`을 `CCL.uproject` 대상으로 실행한다. 새 파일이 없어도 실행한다.
2. 생성에 성공하면 `CCLEditor Win64 Development`를 빌드한다. 다른 타깃·플랫폼이 작업 대상이면 그 구성도 빌드한다.
3. `node Tools/Agent/cpp-rules.cjs format`과 `node Tools/Agent/floor-guard.cjs`를 실행한다. 바닥선 검사의 종료 코드는 0 통과, 1 발견, 2 실행 불가다.
4. 실패하면 작업 범위의 원인을 고치고 그 단계부터 다시 실행한다. 해결하지 못한 오류가 남으면 코드 작성 상태와 검증 미완료를 나눠 보고한다.
5. 게임 동작을 바꿨으면 `ccl8-verify` 스킬로 실행 검증하고 증거 강도를 적는다.

완료 보고에는 프로젝트 파일 생성, 빌드, 두 검사의 결과를 각각 적는다. Live Coding, 코드 검사, 일부 모듈의 컴파일은 프로젝트 빌드 통과가 아니다.

## 근거·설계·리뷰

엔진 API와 실행 흐름은 `graft_ue58`로 찾고 현재 엔진 소스에서 확인한다. 그래프의 범위가 부족하면 소스를 직접 검색한다. 확인한 엔진 버전과 `path:line`을 남기고 UE4와 UE5의 차이를 구분한다.

설계는 `ccl8-design` 스킬의 산출물 순서를 따른다. 샘플 프로젝트는 관련성과 트레이드오프를 설명한 뒤 참고한다.

리뷰는 지적마다 위치, 원인, 영향, 검증 방법을 적고 결함, 설계 위험, 선택적 개선으로 나눈다. 수정 요청이 없으면 지적까지만 한다. 절차와 보고 형식은 `ccl8-review` 스킬, 버그 원인 찾기는 `ccl8-debug` 스킬이 맡는다. 성능 주장에는 빌드·장면·설정 조건과 실측값을 붙인다. 실행 전 예상과 실제 결과를 비교하고, 확인하지 못한 동작은 미확인으로 남긴다.
