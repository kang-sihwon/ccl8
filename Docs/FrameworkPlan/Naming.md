# 공용 모듈 이름 규칙

공용 모듈과 타입의 이름을 정하거나 옮긴 타입의 이름을 바꿀 때 읽는다.

공용 모듈과 타입은 공통 접두사 `KSI`를 쓴다. `KSI`는 사용자 이름의 약자이며 결정 22에서 정했다. 게임 고유 모듈과 타입은 지금처럼 `CCL`을 유지한다.

| 대상 | 형식 | 예 |
|---|---|---|
| 공용 모듈 | `KSI{System}` | `KSICore`, `KSIUI`, `KSIMap` |
| 공용 타입 | UE 타입 접두사 + `KSI` + 이름 | `UKSIScreen`, `FKSIUIViewHandle`, `IKSIWorldParticipant` |
| export 매크로 | 모듈 이름 대문자 + `_API` | `KSICORE_API`, `KSIUI_API` |
| 게임 모듈 | 기존 이름 유지 | `CCL`, `CCLEditor` |

`Framework`는 모듈 이름에 붙이지 않는다. 결정 20의 `{Prefix}{System}Framework` 형식은 이름이 길다. 또한 UE 5.8.3에는 서버에서 UMG를 제어하는 실험 플러그인 `UIFramework`가 있어 `KSIUIFramework` 같은 이름이 그 플러그인과 혼동된다 (`<Engine>/Plugins/Experimental/UIFramework/UIFramework.uplugin`). `KSI Framework`는 문서에서 공용 시스템 전체를 부르는 이름으로만 쓴다.

접두사 자리에는 일반 단어를 쓰지 않는다. 접두사는 엔진·외부 플러그인과 이름이 겹치지 않게 하고 공용 코드의 출처를 드러내야 한다. `GameFramework`는 엔진 헤더 폴더(`#include "GameFramework/Character.h"`)와 ModularGameplay의 `UGameFrameworkComponent`와 섞인다. `GameModule`은 프로젝트의 주 게임 모듈을 뜻하는 엔진 용어이고, `Public`은 모듈 폴더와 `Build.cs`의 용어다. `Game`과 `Common`으로 시작하는 엔진 모듈도 많다. 예를 들어 `Common`을 접두사로 쓰면 `UCCLUIActionRouter`가 CommonUI의 `UCommonUIActionRouterBase`와 거의 같은 `UCommonUIActionRouter`가 된다. 접두사 뒤의 시스템 부분에는 일반 단어를 써도 된다.

2026-10-11 UE 5.8.3 소스에서 `KSI`의 이름 충돌을 확인했다. 엔진의 모듈 규칙 파일 이름 2,549개와 플러그인 이름 895개 중 `KSI`로 시작하는 이름은 없었다. 엔진의 C++·C# 소스에도 `KSI`로 시작하는 식별자가 없었고, 착수 전 CCL 저장소에도 쓰는 곳이 없었다.

모듈 이름은 첫 분리 뒤 바꾸지 않는다. 네이티브 타입의 경로 `/Script/<Module>`을 모듈 이름이 정하기 때문이다 (`<Engine>/Source/Runtime/CoreUObject/Private/Misc/PackageName.cpp:2324`). 모듈 이름을 바꾸면 그 모듈의 모든 타입에 리다이렉트가 다시 필요하다.

공용 모듈로 옮기지 않은 `CCL` 모듈, 클래스와 파일은 지금 이름을 유지한다. 타입을 공용 모듈로 옮길 때는 같은 작업에서 `KSI` 이름으로 바꾼다. 경로와 이름이 함께 바뀌어도 타입마다 CoreRedirects 한 줄로 처리할 수 있다. `WorldParticipant` 같은 개별 타입 이름을 다듬을지는 추출할 때 정한다.
