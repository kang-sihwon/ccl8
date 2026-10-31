# C++ 배치 규칙

상위 문서는 [Coding](../Coding.md)이다.

## 기계 검사 규칙

`node Tools/Agent/cpp-rules.cjs check [--summary] [paths...]`가 아래 규칙을 보고한다. 소스는 고치지 않는다. 바꾼 줄이 `.clang-format`과 맞는지는 `node Tools/Agent/cpp-rules.cjs format [--base <rev>]`가 검사한다. 파일 전체에는 적용하지 않는다. 기존 파일은 namespace 들여쓰기와 이어진 줄 정렬이 섞여 있어 규칙과 무관한 차이가 수천 건 생기기 때문이다. 바꾼 파일에 새 위반을 남기지 않는다. 2026-10-25에 기록한 기존 위반 183건(60개 파일)은 2026-10-28에 동작을 바꾸지 않고 모두 정리했다. 그 뒤 저장소 전체 검사는 0건이다.

`.clang-format`은 람다 본문의 여는 중괄호를 다음 줄에 두고(`BeforeLambdaBody: true`), 이어진 줄도 탭으로 들여쓴다(`UseTab: ForContinuationAndIndentation`). 2026-10-28 기준 저장소의 `.cpp`는 람다 여는 중괄호를 다음 줄에 둔 곳이 258곳, 같은 줄에 둔 곳이 9곳이었다. 여러 줄 조건의 연산자는 기존 코드에 줄 앞 방식과 줄 끝 방식이 섞여 있다. `ColumnLimit: 0`인 이 설정은 줄 끝 방식의 줄바꿈만 유지하므로, 고치는 조건문은 `||`와 `&&`를 줄 끝에 둔다.

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

[결정 6](../decisions/0006-cpp-declaration-layout.md)의 순서로 쓴다. 검사기가 보지 않는 영역 순서와 선언 묶음은 리뷰에서 확인한다. 내용이 없는 선언 종류, 접근 지정자, 전처리 블록은 생략한다.

1. include는 `CoreMinimal.h`, 부모 클래스 헤더, 그 밖의 필요한 헤더, `.generated.h` 순이다.
2. 파일 범위 선언은 매크로(`#define`), 델리게이트, `extern`, enum, struct, class 순이다. 정의와 전방 선언 중 필요한 형태를 쓴다.
3. `GENERATED_BODY()` 다음은 생성자와 부모 인터페이스 함수 영역이다. 오버라이드는 반환형이나 이름에 관계없이 여기 둔다.
4. 다음은 클래스 자체 함수 영역이다. 일반 함수 뒤에 Get·Set과 단순 상태 조회·검사 함수를 둔다. 작업을 수행하고 성공 여부를 `bool`로 돌려주는 함수는 일반 함수다.
5. 에디터 전용 영역에는 함수와 프로퍼티를 함께 두고 다시 나누지 않는다. 함수는 `WITH_EDITOR`, 리플렉션 데이터는 `WITH_EDITORONLY_DATA`로 감싼다. 에디터 전용 오버라이드는 부모 인터페이스 영역보다 이 영역에 둔다.
6. 마지막은 멤버 변수 영역이다. `UPROPERTY`가 없는 멤버 변수도 여기 둔다.

각 영역 안의 접근 지정자는 `public`, `protected`, `private` 순이다. 영역이 바뀌면 같은 접근 지정자를 다시 쓴다. 같은 영역·접근 수준의 static 함수는 연속해서 묶는다. 단순 반환·설정·검사 함수는 헤더에 인라인으로 정의하고, 추가 의존성이나 복잡한 처리가 필요하면 CPP에 정의하되 그룹 순서를 유지한다.

빈 줄은 [결정 7](../decisions/0007-member-blank-lines.md)과 [결정 13](../decisions/0013-header-region-blank-lines.md)을 따른다. 검사기 규칙 밖에서 리뷰할 항목은 다음과 같다.

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

함수 본문은 제어 흐름과 처리 단계가 보이게 쓴다. 근거는 [결정 14](../decisions/0014-cpp-control-flow-readability.md)다. 중괄호와 줄바꿈은 검사기가 보고, 리뷰는 단계 구분을 본다.

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
