# 공용 UI 검증

공용 UI를 고친 뒤 무엇을 검사할지, 왜 이 구조를 골랐는지 볼 때 읽는다. 실행 순서와 증거 기록은 `ccl8-verify` 스킬을 따른다. 당시 실행 결과는 [UI 이전 기록](Migration.md)이 소유한다.

## 검증 방법

- `CCL.UI.RegistryContracts`는 레이어·확장 지점·중복 태그·문맥 검증과 재연결 시 이전 콜백 해제를 검사한다. `CCL.UI.ViewModelLifetime`은 이전 ASC의 알림 해제, 새 ASC 연결, 같은 원본 재연결 때 중복 알림 방지를 검사한다.
- `Tools/Validation/run_ui_foundation.ps1`은 실제 게임 프로세스에서 동시 패널, 스택·큐, 풀 재사용, 미로드 클래스의 비동기 로딩 취소, 등록 소유자 제거, 로컬 플레이어 분리, 뒤로 가기 입력, 맵 이동을 검사한다. 필요한 Soft Class 에셋은 `Tools/Validation/create_ui_fixture.py`가 만든다. `UCCLUICloseOnHideProbe`는 초기 표시 정책의 콜백이 화면을 닫는 경로를 재현한다.
- 표시 제어 검사는 중첩 요청의 양방향 해제·중복 해제, 숨긴 상태에서 기본 표시값 변경·새 화면 열기, 소유자 소멸·맵 이동·플레이어 분리, 페이드 도중 역전, 입력과 포커스 복구, 캡처 정지, 숨김 중 사망을 포함한다. 같은 화면 등록 계약에 RPG HUD, RTS 동시 도구 패널, 대화 전용 표시 정책을 대입한다.
- 통합 검사는 Dedicated·Listen 호스트·지연/손실 클라이언트, 저장 파일의 별도 프로세스 복원, 접속·실패 메뉴를 대상으로 한다. 공용 수명 검사에는 RTS의 자원·선택·미니맵 동시 패널과 대화 중심 게임의 본문·선택지 스택을 별도 레지스트리로 구성한다. 이 예시는 게임 규칙이나 완성된 화면 에셋을 추가하지 않고 등록 데이터·입력 정책의 교체 가능성을 본다. 월드 일시정지 중 페이드와 숨긴 화면의 복구도 실제 프레임에서 본다.
- HUD·대화 검사는 능력치 변경 통지, 재스폰 후 ASC 재연결, 이전 구독 해제, 대사 교체·거리 이탈·사망·인벤토리 전환, 720p·1080p 배치, 입력 회귀를 포함한다. 실제 GAS 체력 변경이 FieldNotify를 거쳐 표시 문자열까지 동기적으로 갱신되는 경로도 게임 검사에 들어 있다.


실행 로그에는 엔진 실험 플러그인 ToolsetRegistry의 `PythonTestRunner` 초기화 오류가 따로 나타난다. 판정은 공용 UI 검사 실패, 입력 라우터 오류, assert, fatal과 종료 코드로 하며, 이 플러그인 초기화 문제를 UI의 무오류 검증으로 취급하지 않는다.

## 직접 확인

`CCL.uproject`에서 Campaign 맵을 실행해 아래 화면을 본다. 조작은 2026-10-08 통합 당시 기준이다.

1. 왼쪽 위 체력·기력과 아래쪽 안내 글자의 크기를 확인한다.
2. `I`로 인벤토리를 연다. 중앙 캐릭터와 양옆 장비 슬롯, 오른쪽 가방을 확인한다.
3. 아이템을 빈 칸과 다른 아이템 칸으로 드래그한다. 한손 장비 양쪽을 장착한 뒤 양손 장비로 교체해 반환된 아이템을 확인한다.
4. 인벤토리를 닫고 마을 안내인 근처에서 `T`를 누른다. 중앙 아래 대화 상자의 이름과 본문을 확인한다.
5. `Esc`로 메뉴를 열었다 닫는다. 캐릭터 조작이 복구되는지 확인한다.


연출 요청은 기반 API로 제공하며 일반 플레이에 컷신 단축키를 추가하지 않았다.

## 엔진 근거와 대안

아래 파일과 이름은 UE 5.8.3에서 다시 확인했다.

- `<Engine>/Source/Runtime/Engine/Public/Subsystems/LocalPlayerSubsystem.h`의 `PlayerControllerChanged`가 컨트롤러 교체 알림을 제공한다.
- `<Engine>/Plugins/Runtime/CommonUI/Source/CommonUI/Public/CommonActivatableWidget.h`는 활성화와 생성·파괴 수명이 다르다고 설명한다.
- 같은 폴더의 `Widgets/CommonActivatableWidgetContainer.h`는 풀에서 얻은 위젯의 활성화 전 초기화를 지원한다.

공식 [CommonUI 입력 가이드](https://dev.epicgames.com/documentation/en-us/unreal-engine/commonui-input-technical-guide-for-unreal-engine)는 활성 위젯 트리와 `CommonGameViewportClient`를 통한 입력 라우팅을 설명한다. 이 경로를 쓰면 자체 입력 스택의 유지 비용이 줄지만 기존 Slate 화면과 메뉴의 입력 처리를 함께 옮겨야 한다. 자체 관리자만으로 모든 입력을 적용하는 대안은 CommonUI와 포커스 정책이 충돌할 수 있어 채택하지 않았다.

공식 [UMG Viewmodel 문서](https://dev.epicgames.com/documentation/en-us/unreal-engine/umg-viewmodel-for-unreal-engine)는 변경 통지 기반 바인딩을 제공하며 Beta로 표시한다. 공용 문맥은 특정 표시 모델 기반 클래스에 묶지 않는다. 기능별 모델에서 FieldNotify 사용을 검증하고, 기존 네이티브 델리게이트를 연결하는 경우에도 갱신은 변경 통지를 기준으로 한다. 위 문서는 2026-10-07 확인했으며 실제 API와 실행 여부는 로컬 엔진 빌드로 검증한다.
