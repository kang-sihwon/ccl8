# C++ 규칙 위반 정리

[Coding의 기계 검사 규칙](../Coding.md#기계-검사-규칙)이 기존 위반으로 기록한 183건을 동작을 바꾸지 않고 없앤다. 정리 방법은 [Coding의 형식 정리와 배치 변경](../Coding.md#형식-정리와-배치-변경), 완료 검증은 [코드 작업 완료 검증](../Coding.md#코드-작업-완료-검증)을 따른다. 규칙의 근거는 [결정 4](../decisions/0004-header-bool-members.md), [결정 6](../decisions/0006-cpp-declaration-layout.md), [결정 7](../decisions/0007-member-blank-lines.md), [결정 13](../decisions/0013-header-region-blank-lines.md), [결정 14](../decisions/0014-cpp-control-flow-readability.md)이다.

## 목적지

- `node Tools/Agent/cpp-rules.cjs check --summary`가 `0 finding(s)`를 출력한다.
- 조각마다 바꾼 파일을 공백을 지우고 비교했을 때 바뀐 문자가 아래 "정한 것"이 허용한 것뿐이다.
- 마지막 변경 상태에서 프로젝트 파일 생성과 `CCLEditor Win64 Development` 빌드가 통과한다.
- `node Tools/Agent/cpp-rules.cjs format`과 `node Tools/Agent/floor-guard.cjs`가 통과한다.
- [Coding](../Coding.md#기계-검사-규칙)의 기존 위반 기준선 문장이 현재 상태와 맞는다.

## 진행

2026-10-28 기준 분포는 규칙별로 한 줄 블록 110, 연속 빈 줄 27, `GENERATED_BODY()` 뒤 빈 줄 26, 닫는 중괄호 앞 빈 줄 15, 중괄호 없는 본문 4, 헤더 `bool` 멤버 1이다. 모듈별로는 `Source/CCL/Environment/`에 126건이 몰려 있다.

- [x] 1. KSI 모듈과 CCLEditor (없음). `Source/KSI*/`와 `Source/CCLEditor/`의 25건을 고친다. `KSIEcologyBehavior.cpp:295-296`의 중괄호 없는 본문 2건이 들어 있다. 완료 조건은 두 경로의 `cpp-rules.cjs check` 0건, 공백 제거 비교 통과, 빌드 통과다.
  - 결과: 11개 파일에서 25건을 고쳐 두 경로의 검사가 0건이고 전체는 158건이다. 공백 제거 비교에서 10개 파일은 같고, `KSIEcologyBehavior.cpp`는 X·Y·Z 이웃 반복문에 중괄호 2쌍만 늘었다. 안쪽 `continue`는 그대로 Z 반복문을 건너뛴다. `cpp-rules.cjs format`, `floor-guard.cjs` 0건. UE 5.8.3에서 GenerateProjectFiles와 `CCLEditor Win64 Development` 빌드가 성공했다(로그 `Saved/Tests/CppRuleCleanup-20261028/`, 로컬).
- [x] 2. CCL의 Environment 밖 (없음). `Source/CCL/`에서 `Environment/`를 뺀 32건(Tests, Agents, Session, Items, Campaign, 모듈 루트)을 고친다. 완료 조건은 1과 같다.
  - 결과: 22개 파일에서 32건을 고쳐 `Environment/` 밖의 검사가 0건이고 전체는 126건이다. 22개 파일 모두 공백 제거 비교가 같다. `CCLGameInstance.cpp:310, 382`처럼 여러 줄 조건 다음 줄의 `{ ...; return false; }`도 문장별로 펼쳤다. 4칸 공백으로 들여쓴 `CCLItemState.h`는 바꾼 줄이 `.clang-format`의 탭 들여쓰기에 걸려 15줄 전체를 탭으로 바꿨다. `cpp-rules.cjs format`, `floor-guard.cjs` 0건. GenerateProjectFiles와 `CCLEditor Win64 Development` 빌드 성공.
- [x] 3. Environment의 물 실험과 실험 화면 (없음). `CCLExperimentWater.cpp`, `CCLExperimentScreen.*`, `CCLExperimentLaboratoryScreen.cpp`의 형식 위반 49건을 고친다. 헤더 `bool` 멤버는 조각 5가 맡는다. 완료 조건은 1과 같다.
  - 결과: 3개 `.cpp`에서 49건을 고쳐 네 파일의 검사는 `bool` 멤버 1건만 남았고 전체는 77건이다. 세 파일 모두 공백 제거 비교가 같다. `CCLExperimentWater.cpp:173`의 중첩 한 줄 블록도 두 단계로 펼쳤다. 람다 안 줄의 형식 검사 실패는 `.clang-format` 수정("정한 것")으로 풀었다. `cpp-rules.cjs format`, `floor-guard.cjs` 0건, 도구 시험 30개 통과. GenerateProjectFiles와 `CCLEditor Win64 Development` 빌드 성공.
- [x] 4. Environment의 나머지 (없음). `Source/CCL/Environment/`의 남은 76건을 고친다. `CCLLaboratoryTests.cpp:65-66`의 중괄호 없는 본문 2건이 들어 있다. 완료 조건은 1과 같다.
  - 결과: 23개 파일에서 76건을 고쳐 전체 검사는 조각 5의 `bool` 멤버 1건만 남았다. 22개 파일은 공백 제거 비교가 같고, `CCLLaboratoryTests.cpp`는 Spacing·Scale·RainRate 반복문에 중괄호 2쌍만 늘었다. 같은 파일의 여러 줄 조건은 앞머리 `||`를 줄 끝으로 옮겨 형식 검사를 통과시켰다. 토큰 순서는 같다. 한 줄 블록을 펼치자 `floor-guard.cjs`가 단언문 삭제 14건을 잘못 보고해, 지운 줄이 같은 파일의 추가 줄을 공백 없이 이은 문자열에 들어 있으면 옮긴 것으로 보게 고치고, 같은 파일에서 나뉜 경우와 다른 파일로 간 경우를 기존 시험에 더했다. 도구 시험 30개 통과. `cpp-rules.cjs format`, `floor-guard.cjs` 0건. GenerateProjectFiles와 `CCLEditor Win64 Development` 빌드 성공.
- [x] 5. 헤더 bool 멤버 (3). `CCLExperimentScreen.h:89`의 `bool bDisplayedObservation`을 결정 4대로 `uint8`로 바꾼다. 형식 정리와 다른 커밋으로 남긴다. 이 멤버는 `CCLExperimentScreen.cpp:167`에서 `bool` 지역 변수와 비교하고 `:172`에서 그 값을 대입받으므로, `uint8`이 0과 1만 담는 한 비교 결과는 같다. 완료 조건은 그 파일의 `cpp-rules.cjs check` 0건, 이 멤버를 쓰는 곳의 대입·비교가 그대로인지 확인한 기록, 빌드 통과다.
  - 결과: 멤버를 `uint8 bDisplayedObservation = 0;`으로 바꿔 전체 검사가 0건이다. 첫 빌드는 `:167`의 `uint8 == bool` 비교가 MSVC C4805 오류로 실패해, 비교식을 `static_cast<bool>(bDisplayedObservation) == bObservation`으로 고쳤다. `:172`의 대입은 그대로이고 멤버에는 0과 1만 들어가므로 비교 결과는 같다. 실행 검증은 하지 않았다. 증거는 코드 대조와 빌드다. `cpp-rules.cjs format`, `floor-guard.cjs` 0건. GenerateProjectFiles와 `CCLEditor Win64 Development` 빌드 성공.
- [ ] 6. 닫기 (1-5). 전체 검사 0건과 목적지의 나머지 조건을 확인하고, Coding의 기준선 문장을 고친다. "아직 모르는 것"의 완료 검사 질문을 정한 뒤 지도를 닫는다.

## 정한 것

- 형식 정리만 한다. 조건식, 호출 순서, 이름, 함수 분리, 멤버 선언 순서는 바꾸지 않는다. 근거는 [Coding의 형식 정리와 배치 변경](../Coding.md#형식-정리와-배치-변경)이다.
- 파일 전체에 clang-format을 돌리지 않는다. 규칙과 무관한 차이가 수천 건 생기기 때문이다([Coding](../Coding.md#기계-검사-규칙)).
- 동작 보존은 공백 제거 비교로 확인한다. 바꾼 파일마다 `git show HEAD:<파일>`과 작업 트리 파일에서 공백 문자를 모두 지운 뒤 같은지 본다. 한 줄 블록, 빈 줄, `GENERATED_BODY()` 뒤 빈 줄은 공백만 바뀌므로 완전히 같아야 한다. 중괄호 없는 본문은 `{`와 `}`만 늘어야 한다. 헤더 `bool` 멤버는 조각 5에서 따로 확인한다.
- 조각마다 커밋하고 빌드를 확인한다. 배치와 공백만 바꾼 조각은 멀티프로세스 플레이 검증을 반복하지 않는다([Coding](../Coding.md#형식-정리와-배치-변경)).
- 2026-10-28 `.clang-format`의 `BeforeLambdaBody`를 `true`로, `UseTab`을 `ForContinuationAndIndentation`으로 바꿨다. 조각 3에서 Slate 체인의 람다 안 줄을 펼치자 `cpp-rules.cjs format`이 그 줄을 탭과 공백이 섞인 들여쓰기와 같은 줄 람다 중괄호로 고치라고 보고했다. 저장소의 `.cpp`에서 람다 여는 중괄호를 다음 줄에 둔 곳은 258곳, 같은 줄에 둔 곳은 9곳이다. 바꾼 설정으로 저장소 전체를 clang-format에 돌렸을 때 바뀔 줄은 13,700줄에서 11,634줄로 줄었다. 바꾼 설정에서 조각 1-3의 바뀐 줄은 모두 형식 검사를 통과한다.
- 여러 줄 조건의 연산자 위치는 기존 코드에 두 방식이 섞여 있다(줄 앞 `||`·`&&` 715줄, 줄 끝 799줄). `ColumnLimit: 0`인 지금 설정은 줄 끝 방식만 줄바꿈을 유지하므로, 바꾼 조건문만 줄 끝 방식으로 옮긴다. 설정은 바꾸지 않는다.

## 아직 모르는 것

- 위반이 0건이 된 뒤 전체 `cpp-rules.cjs check`를 [AGENTS.md](../../AGENTS.md)의 "끝내기 전"이나 Coding의 완료 검증에 넣을지. 지금은 바꾼 줄의 형식만 검사한다. 조각 6 전에 사용자와 정한다.
- 빌드할 때 에디터가 열려 있으면 Live Coding이 빌드를 막는다. 조각마다 빌드 전에 에디터가 닫혀 있는지 확인해야 한다.

## 범위 밖

- 이름 변경, 함수 분리, 로직 수정. 필요하면 별도 작업으로 한다.
- 기계 검사 규칙의 추가나 변경. `.clang-format`은 위 "정한 것"처럼 기존 코드에 맞추는 범위에서만 고친다.
- `Docs/Learning/` 재작성. [진행 중인 작업](README.md)의 다음 작업 후보로 남는다.
