# 아이템 축 재설계

## 현재 상태

2026-11-19에 사용자가 아이템·장비 코드의 설계와 코드 스타일을 새로 만든다는 가정으로 다시 판단해 달라고 요청했다. 범위는 Items와 이를 쓰는 Equipment·Combat·KSIInventory다. 검토 과정은 [아이템 축 재설계 검토](../ProgressionFoundation/ItemRedesign-2026-11-19.md)에 있다.

- 사용자와 정할 것 6개를 정해 [결정 68](../decisions/0068-item-fragment-hierarchy.md)-[72](../decisions/0072-auto-usage.md)로 기록했다. 결정 16은 유효(부분)로 바뀌었다.
- 목표 구조는 [장비의 재설계 계약](../ProgressionFoundation/Equipment.md#재설계-계약)에 적었다. 입력 변경은 [전투 행동](../CombatFoundation/Actions.md#입력과-방향), 저장 형식은 [체크포인트](../SessionFoundation/Checkpoint.md#형식별-변환), 주석 언어와 `auto`는 [Coding](../Coding.md#코드-관례)과 AGENTS.md에 반영했다.
- 협업 방식: 구현은 AI가 하고, 방향은 사용자와 논의해 정한다.
- 2026-11-20에 구현 단계 1을 마쳤다. 결과는 아래 "단계 1 결과"에 있다.

## 이어갈 때

단계마다 프로젝트 파일 생성, 빌드, 자동화 테스트, 스모크를 돌리고 이 지도에 결과를 적는다. 단계 1은 끝났고 다음은 단계 2다.

1. (완료) Fragment 루트 검증 가상 함수와 편집 객체 `FKSIInventoryEdit`
2. 리다이렉트 시험 표 형식 정리 커밋, 루트 이름 `FKSIItemFragment`, Fragment 계층, 정의, 생성 라이브러리로 정의 재생성, 저장 형식 번호 올리기
3. Equipment 컴포넌트, 출처 등록부, 무브셋, 손 입력 태그, 적의 패턴별 스펙
4. Fragment별 상태 배열(`States`)과 `FindState`·`FindOrAddState`, Ranged·Ammo·Usable·Perishable 상태
5. Training 분리와 결과 코드
6. 형식 정리 커밋(들여쓰기, 영역 구분)

구현이 끝나면 Equipment의 현재 구현 절을 재설계 계약으로 바꾸고 이전 내용을 지운다.

## 단계 1 결과

- `FKSIInventoryEdit`(`Source/KSIInventory/Public/KSIInventoryEdit.h`)를 만들었다. 인벤토리의 Add·Remove·MoveToSlot·Restore·TransferTo·스냅샷 복원, 음식·탄약 서비스, Loadout의 장착·해제·사용·훈련, Loadout 복원 배치, 두 영속성 서브시스템, 상점 구매가 모두 이 객체로 항목을 바꾼다. `BeginTransaction`·`EndTransaction`·`RestoreValidated`·외부 `bMutationLocked` 조작은 없어졌다. `IsTransactionOpen`은 `IsEditOpen`이 됐다.
- friend 6개 중 항목을 고치던 4개를 지웠다. 저장 식별자와 음식 환경을 정하는 두 영속성 서브시스템은 남겼다.
- `Commit`은 바뀐 항목만 복제 표시하고 기존 항목의 복제 ID를 유지한다. 예전 `RestoreValidated`는 바뀔 때마다 전 항목을 지우고 다시 만들었다.
- 동작 차이: 단일 명령(Add 등)의 알림도 편집이 닫힐 때 잠긴 상태로 나간다. `Use`는 소모를 검증하고 효과를 적용한 뒤 확정한다(예전에는 소모 뒤 효과, 실패 시 되돌림). 상점 구매도 거래가 성립할 때만 확정한다.
- 공용 루트 `FKSIItemFragmentData`에 `Validate`, `GetInstanceStateType` 가상 함수와 `FKSIItemValidation`을 더했고, 정의 검증이 Fragment의 `Validate`를 부른다.
- 미룬 것: 상태 배열(`States`)과 `FindState`·`FindOrAddState` 분리는 단계 4로 옮겼다. `FCCLItemState`를 쪼갤 때 상태 접근 약 80곳을 어차피 다시 쓰기 때문이다. 루트 이름 `FKSIItemFragment`로의 변경은 단계 2로 옮겼다. `CCLFrameworkMigrationTests.cpp`의 리다이렉트 표가 공백 들여쓰기라서, 한 줄만 바꿔도 `format` 검사가 표 전체의 재정렬을 요구하고, `clang-format off`는 floor-guard가 막는다. 단계 2 전에 이 표의 형식만 정리하는 커밋이 필요하다.
- 검증(UE 5.8.3, 2026-11-20): 프로젝트 파일 생성과 `CCLEditor Win64 Development` 빌드 성공. `cpp-rules check` 0건, `format` 통과, `floor-guard` 0건. 자동화 `CCL` 172개 중 171개 통과(`Saved/Tests/Automation/20261120-113706-738/`). 실패 1개 `CCL.Framework.ExistingAssets`는 git이 무시하는 로컬 `Content/Fab/` 팩(PN_Fern, ParagonGreystone, ParagonSevarog)의 UE4 블루프린트가 `ResetOrientationAndPosition`을 찾지 못하는 오류이며 아이템 에셋 오류는 없다. 스모크 PASS: 인벤토리 복제, 투사체, Loadout 복원, 음식, 음식 줍기 S/L/D, 음식 환경 S/L/D, 음식 구역 S/D, 원정 진행(`-Progression`) S/L/D, 세션, 세계 저장 S, Agent 세계. 증거 강도는 원정 `Ability`, 세션·음식 환경·음식 구역 `Slate`, 나머지 `internal`이다. 입력 매핑은 바꾸지 않았다.

## 아직 모르는 것

- 장비 교체마다 능력을 부여·회수할 때의 복제 비용과, 원격 클라이언트의 교체 직후 입력 예측이 맞는지 측정하지 않았다.
- 회복의 중심(충전식·소모품·음식), 채집 루프, 양손 잡기 전환, 무기 스킬과 그 출처는 정하지 않았다.
- 생성 라이브러리 밖에서 만든 아이템 정의 에셋이 있는지 확인하지 않았다. 단계 2 전에 목록을 본다.
- 편집 객체가 알림 동안 인벤토리를 잠그므로, 알림을 받아 같은 인벤토리를 바로 고치는 리스너는 거절된다. 현재 코드와 시험에는 그런 리스너가 없었지만 UI나 새 시스템을 붙일 때 주의한다.
- 음식 구역의 Listen 모드는 돌리지 않았다.
