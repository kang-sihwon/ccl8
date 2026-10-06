# ccl8 코딩 규칙

## 파일 경계와 환경

AI는 [Direction](Direction.md)의 합의된 제작 범위에서 `Source/`, `Config/`, `Build.cs`, `Target.cs`, `.uproject`와 에셋을 작성·수정하고 필요한 검증을 수행한다. 근거는 [결정 2](DesignLog.md)다. 사용자의 직접 구현이나 매 파일의 재승인을 선행 조건으로 삼지 않는다. 리뷰만 요청한 작업은 리뷰로 끝낸다.

구현 전에 관련 기획·아트·사양을 확인하고 해당 작업의 목표, 변경 범위와 검증 기준을 문서로 정리한다. 통상적인 구현 선택은 AI가 판단해 기록한다. 장르·핵심 경험·콘텐츠 범위를 변경하거나 Orbis 원본을 수정하는 권한으로 확대하지 않는다. 유료 구매와 외부 공개는 별도 지시를 따른다.

Windows와 UE 5.8.1 소스 빌드를 기준으로 한다. 실제 엔진 버전은 `Engine/Build/Build.version`에서 확인한다. 엔진 소스는 별도 설치의 읽기 전용 참고 자료다. 경로는 로컬 에이전트 설정에 두고 저장소에 기기별 절대 경로를 넣지 않는다.

프로젝트는 `CCL.uproject`, 모듈은 `Source/CCL/`, 빌드 타깃은 `CCL`과 `CCLEditor`다. 빌드 명령과 성공 여부는 실제 빌드를 검증한 뒤 기록한다. 다른 프로젝트의 빌드 타깃을 실행하지 않는다. 프로젝트 빌드와 에디터 실행은 합의된 구현을 검증하는 데 필요한 범위에서 수행한다. 엔진 전체 재빌드와 대형 인덱스 생성은 필요성과 비용을 먼저 제시한다.

## 코드 관례

- Epic C++ Coding Standard를 기준으로 `A`, `U`, `F`, `E`, `I` 타입 접두사와 불리언 변수의 `b` 접두사를 쓴다.
- 헤더의 클래스·구조체 불리언 멤버 변수만 `bool` 대신 `uint8`을 사용한다. 예를 들어 `uint8 bDead = 0;`으로 선언하고 불리언 값은 0 또는 1로 저장한다. 근거는 [결정 4](DesignLog.md)다.
- 함수 반환값·매개변수의 불리언 타입과 `.cpp`의 지역 불리언 변수는 `bool`을 사용할 수 있다. 엔진 오버라이드와 외부 인터페이스의 함수 시그니처는 원래 타입을 유지한다.
- UE의 문자열·컨테이너·오브젝트 참조 타입을 우선한다. 리플렉션과 에디터 노출은 필요한 범위에만 적용한다.
- 전방 선언과 직접 필요한 헤더를 사용해 의존성을 줄인다. 모듈 의존은 해당 `Build.cs`에 명시한다.
- 책임, 수명, 상태 소유자와 네트워크 권위를 설계에서 설명한다. 멀티플레이 범위와 미정 요구사항은 [GameDesign](GameDesign.md)을 따른다.
- 폴더·모듈·성능 목표는 요구사항이 정해진 뒤 선택한다. 기존 게임의 구조를 기본 요구로 삼지 않는다.

## 헤더 배치

[결정 6](DesignLog.md)에 따라 다음 순서로 작성한다. 내용이 없는 선언 종류, 접근 지정자와 전처리 블록은 생략한다. 접근 권한과 멤버 변수의 초기화 의존성을 배치 변경으로 바꾸지 않는다.

1. `#pragma once` 다음에 include를 둔다. `CoreMinimal.h`, 부모 클래스 헤더, 그 밖의 필요한 헤더 순으로 쓰고 `.generated.h`는 마지막 include로 둔다.
2. 파일 범위 선언은 매크로 정의(`#define`), 델리게이트, `extern`, enum, struct, class 순으로 둔다. enum·struct·class의 정의 또는 전방 선언 중 필요한 형태를 사용한다.
3. 클래스의 `GENERATED_BODY()` 다음에는 생성자와 부모 인터페이스 함수 영역을 둔다. 오버라이드는 반환형이나 이름에 관계없이 이 영역에 둔다.
4. 다음에는 클래스 자체 함수 영역을 둔다. 일반 함수 뒤에 Get·Set과 단순 상태 조회·검사 함수를 둔다. 반환형이 `bool`이어도 작업을 수행하고 성공 여부를 반환하는 함수는 일반 함수로 분류한다.
5. 에디터 전용 영역에는 함수와 프로퍼티를 함께 둔다. 이 영역 안에서는 함수·프로퍼티 영역을 다시 나누지 않는다. 함수는 `WITH_EDITOR`, 리플렉션 데이터는 `WITH_EDITORONLY_DATA`로 감싼다.
6. 마지막에는 멤버 변수 영역을 둔다. `UPROPERTY`가 없는 멤버 변수도 이 영역에 둔다.

각 영역은 필요한 접근 지정자를 `public`, `protected`, `private` 순으로 배치한다. 영역이 바뀌면 같은 접근 지정자를 다시 사용할 수 있다. 같은 영역·접근 수준의 static 함수는 연속해서 묶는다.

단순 반환·설정·간단한 검사 함수는 헤더에서 인라인으로 정의한다. 구현에 추가 의존성이나 복잡한 처리가 필요하면 CPP에 정의하고 해당 함수 그룹의 순서를 유지한다.

`UFUNCTION`과 함수 선언, 바로 앞의 설명 주석은 한 묶음으로 취급한다. 묶음 앞뒤에 빈 줄 하나를 두되 접근 지정자나 영역 경계의 기존 빈 줄과 중복하지 않는다. 멤버 변수도 설명 주석·`UPROPERTY`·선언을 한 묶음으로 취급하고 변수 사이에 빈 줄 하나를 둔다.

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

이 예시는 배치를 설명하는 발췌이며 에디터 전용 선언을 모든 클래스에 추가하라는 뜻은 아니다. 에디터 전용 오버라이드는 부모 인터페이스 영역보다 에디터 전용 영역 배치를 우선한다. 에디터 데이터의 `WITH_EDITORONLY_DATA` 블록은 `WITH_EDITOR` 안에 중첩하지 않는다. UE 5.8.2의 `<Engine>/Source/Programs/Shared/EpicGames.UHT/Parsers/UhtPropertyParser.cs:668`은 CCL 같은 런타임 모듈에서 `WITH_EDITOR`로 감싼 `UPROPERTY`를 오류로 처리한다.

## CPP 배치

대응하는 클래스 헤더를 첫 include로 둔다. 나머지 의존 헤더 뒤에는 파일 범위 델리게이트, static 선언, `extern`, 익명 namespace와 디버깅용 선언을 차례로 둔다. namespace에는 파일 내부 상수와 보조 함수를 둔다. 상수는 상수식으로 초기화할 수 있을 때만 `constexpr`를 사용하고, 그 밖에는 `const`를 사용한다. `FName`과 `FString`에 `constexpr`를 일괄 적용하지 않는다.

함수 정의는 생성자, 부모 인터페이스 오버라이드, 클래스 자체 일반 함수, Get·Set과 단순 상태 조회·검사 함수, `WITH_EDITOR` 함수 순으로 둔다. 각 그룹 안에서는 헤더 선언 순서를 따른다. RPC의 `_Implementation`은 원래 RPC 선언에 대응하는 클래스 자체 함수로 분류한다. 부모의 `_Implementation`을 오버라이드하면 부모 인터페이스 그룹에 둔다.

헤더에서 인라인으로 정의한 함수는 CPP에 다시 정의하지 않는다. `bool` 반환형만으로 그룹을 결정하지 않는다. 예를 들어 `ShouldCreateSubsystem`은 부모 인터페이스 그룹에 두고, 작업의 성공 여부를 반환하는 클래스 자체 함수는 일반 함수 그룹에 둔다.

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

배치 예시의 상수와 빈 함수 본문은 기능 구현을 대체하지 않는다. 실제 파일에는 사용하는 선언과 함수만 둔다.

## 배치 변경 검증

기존 코드를 정리할 때는 함수의 시그니처·본문, 접근 권한과 리플렉션 지정자를 보존한다. 멤버 변수의 선언 순서는 초기화 순서에 영향을 주므로 형식 통일만을 위해 바꾸지 않는다. 변경 전후의 함수별 내용을 비교하고 프로젝트 빌드로 UHT와 컴파일을 확인한다. 배치와 공백만 바뀐 작업에는 멀티프로세스 플레이 검증을 반복하지 않는다.

## 근거·설계·리뷰

엔진 API와 실행 흐름은 `graft_ue58`로 찾고 현재 소스에서 확인한다. 그래프가 없거나 범위가 부족하면 해당 소스를 검색한다. 확인한 버전과 `path:line`을 제시하고 UE4와 UE5의 차이를 구분한다.

설계는 `ccl8-design`을 사용한다. 목표·제약, 책임·수명, 클래스 다이어그램, 인터페이스, 코드 예시, 검증 방법을 다룬다. 학습 설명은 검토에 필요한 부분과 사용자 요청 범위에 맞춘다. 샘플 프로젝트는 관련성과 트레이드오프를 설명한 뒤 참고한다.

리뷰는 결함, 설계 위험과 선택적 개선을 구분하고 각 지적의 위치·원인·영향·검증 방법을 제시한다. 성능 주장은 빌드·장면·설정 조건과 실측을 붙인다. 실행 전 예상과 실제 결과를 비교하고 확인하지 못한 동작은 미확인으로 남긴다.
