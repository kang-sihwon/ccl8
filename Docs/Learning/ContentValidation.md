# 콘텐츠 제작·검증: 에셋 생성부터 패키지 실행까지 연결하기

C++ 클래스가 컴파일돼도 필요한 DataAsset·맵·StateTree가 없으면 플레이 흐름은 완성되지 않는다. 이 장은 에디터 생성 코드, Python 배치, 자동 검사와 패키징이 각각 무엇을 증명하는지 설명한다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `4238990815cc932a6f38c55f947be78065389651` |
| 도입·변경 기준 | `3b369682333bf541e91ee5078b03be38a87e4d75` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

전투 에셋 생성 기반에서 성장·장비·생활 Agent 제작 도구로 확장된 경로를 읽는다. 대상은 `Source/CCL/Editor/`, `Source/CCL/Tests/`, `Tools/Validation/`다. 실제 실행·제출 기준은 [Coding](../Coding.md), [DeliveryPlan](../DeliveryPlan.md), [Packaging](../Packaging.md)이 소유한다.

## 2. 해결할 문제

무기 클래스가 있어도 공격 정의·AbilitySet·부착 자료가 연결되지 않으면 의도한 무기로 작동하지 않는다. 에디터에서는 찾던 에셋이 패키지에 없을 수도 있다.

따라서 코드 존재, 에셋 생성, 컴파일, 자동 검사, 실제 화면과 패키지 실행을 서로 다른 증거로 남긴다. 한 단계 성공으로 다른 단계까지 통과 처리하지 않는다.

## 3. 에디터 전용 코드와 패키지

`WITH_EDITOR`는 에디터 코드 포함 여부를 가르는 컴파일 조건이다. 런타임 모듈 안에 함수 선언이 있어도 내부의 에디터 생성 코드는 게임 빌드에서 제외할 수 있다.

`UPackage`는 저장되는 에셋의 패키지다. `NewObject`로 객체를 만들고 AssetRegistry에 등록한 뒤 `UPackage::SavePackage`로 디스크에 저장하는 단계는 서로 다르다. `MarkPackageDirty`만 호출하면 저장 완료가 아니다.

Commandlet은 에디터의 배치 작업 실행 경로다. 테스트에서 사용하는 `-NullRHI`는 렌더링 없이 실행하기 위한 옵션이므로 화면 품질의 증거가 될 수 없다. 패키징의 Cook은 에셋을 대상 플랫폼 실행에 맞게 준비하는 단계다. 이 구분은 UE4에도 존재했으며 현재 스크립트와 API 사용은 이 프로젝트의 UE5 기준이다.

## 4. 이전의 전투 월드 구성

다음은 전투 검사 맵에서 내비게이션 경계를 준비하는 초기 코드다.

출처: `4238990`, [CCLCombatAssetLibrary.cpp](../../Source/CCL/Editor/CCLCombatAssetLibrary.cpp):177의 `UCCLCombatAssetLibrary::ConfigureCombatWorld`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLCombatAssetLibrary::ConfigureCombatWorld()
{
#if WITH_EDITOR
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;

    if (!World || !World->GetWorldPartition())
    {
        return false;
    }

    World->GetWorldPartition()->SetEnableStreaming(false);
    auto* Bounds = World->SpawnActor<ANavMeshBoundsVolume>();
    auto* Builder = NewObject<UCubeBuilder>(Bounds);
    Builder->X = 4000.f;
    Builder->Y = 4000.f;
    Builder->Z = 600.f;

    if (!Builder->Build(World, Bounds))
    {
        return false;
    }
```

현재 편집 중인 World와 WorldPartition을 확인한 뒤 Streaming을 끄고 이름으로 기존 Navigation Bounds를 찾는다. 뒤에서는 없을 경우 Volume을 만들고 Brush 크기·위치·Bounds를 설정한 뒤 Navigation 갱신과 Build를 요청한다.

이 함수는 일반 실행 중 월드를 구성하는 게임 규칙이 아니다. 에디터 제작 경로이며 기존 Level의 의도와 맞는지 확인하고 사용해야 한다.

## 5. 콘텐츠 생성 경로를 확장한 이유

성장·장비·무기·생활 Agent가 추가되면서 정의 에셋 간 연결을 반복해서 만들 필요가 생겼다. 설계 요구는 각 Foundation 문서와 결정 8·9·16·18을 따른다.

코드에서 분석하면, 이름과 연결을 생성 코드에 모으면 검사용 콘텐츠를 재현하기 쉽다. 반대로 이미 편집한 에셋을 다시 기본값으로 덮을 위험이 있으므로 ‘생성 함수’라는 이름만으로 재실행 안전성을 판단하지 않는다.

## 6. 현재 에셋 생성의 재실행 정책

장비 생성은 기존 패키지가 있으면 해당 항목을 건너뛴다.

출처: `50e5838`, [CCLProgressionAssetLibrary.cpp](../../Source/CCL/Editor/CCLProgressionAssetLibrary.cpp):162의 `UCCLProgressionAssetLibrary::CreateEquipmentAssets`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLProgressionAssetLibrary::CreateEquipmentAssets()
{
#if WITH_EDITOR
    const TCHAR* Names[] = {TEXT("DA_TrainingSword"), TEXT("DA_TrainingShield"), TEXT("DA_TrainingStaff"),    TEXT("DA_TrainingArmor"),
                            TEXT("DA_TrainingBoots"), TEXT("DA_TrainingCloak"),  TEXT("DA_TrainingNecklace"), TEXT("DA_TrainingRing")};
    const TCHAR* Labels[] = {TEXT("Training Sword"), TEXT("Training Shield"), TEXT("Two-hand Staff"),    TEXT("Training Armor"),
                             TEXT("Training Boots"), TEXT("Training Cloak"),  TEXT("Training Necklace"), TEXT("Training Ring")};
    const FGameplayTag Slots[] = {CCLItemTags::Slot_RightHand, CCLItemTags::Slot_RightHand, CCLItemTags::Slot_RightHand,
                                  CCLItemTags::Slot_Armor,     CCLItemTags::Slot_Boots,     CCLItemTags::Slot_Cloak,
                                  CCLItemTags::Slot_Necklace,  CCLItemTags::Slot_RingOne};
    for (int32 Index = 0; Index < 8; ++Index)
    {
        if (FPackageName::DoesPackageExist(FString(TEXT("/Game/Progression/")) + Names[Index]))
        {
            continue;
        }

        auto* Item = ProgressionAsset<UCCLItemDefinition>(Names[Index]);
        if (!Item)
        {
            return false;
        }

```

그러나 모든 생성 함수가 같은 정책은 아니다. `ProgressionAsset`는 기존 에셋을 Load할 수 있고 `CreateProgressionAssets`는 반환된 에셋의 일부 값을 다시 설정한다. `create_playground.py`는 기존 맵이 있으면 오류로 중단한다. 대상별 보호 조건을 먼저 읽어야 한다.

저장 도우미는 변경을 표시하고 패키지를 실제 저장한다.

출처: `50e5838`, [CCLProgressionAssetLibrary.cpp](../../Source/CCL/Editor/CCLProgressionAssetLibrary.cpp):46의 `SaveProgression`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool SaveProgression(UObject* Object)
{
    if (!Object)
    {
        return false;
    }

    Object->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(
        Object->GetOutermost(), Object,
        *FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
```

앞선 에셋 저장이 성공한 뒤 다음 저장이 실패할 수도 있다. 여러 패키지 저장을 하나의 원자적 거래로 보장하는 함수는 아니다.

## 7. 제작 도구와 검사의 연결

`CCLCombatAssetLibrary`는 전투 정의와 검사 월드를, `CCLProgressionAssetLibrary`는 성장·장비·부착·투사체·Agent 자료와 마이그레이션을 다룬다. Python 스크립트는 에디터의 Level·Actor·Asset API와 이 함수를 이용해 콘텐츠를 배치한다.

Details Customization은 에셋 편집을 돕는다. `CCL.cpp`의 모듈 Startup은 Agent 성향과 AttachmentProfile 편집기를 등록하고 Shutdown은 등록을 해제한다. Commandlet에서는 편집 UI 등록을 건너뛴다. 이 UI는 런타임 검증을 대신하지 않는다.

검사는 두 층이다. Automation은 값·규칙·실패 계약을, Smoke Subsystem은 실제 월드·복제·입력·UI의 연결을 확인한다. 테스트 소스가 존재해도 현재 Target에 포함됐는지와 실행 필터가 실제 테스트를 찾았는지는 별도로 확인한다.

## 8. 실행 환경과 권한

도구는 `.local/agent-paths.json`에서 기기별 엔진 위치를 읽는다. 공유 문서·스크립트에 특정 PC의 경로를 고정하지 않는다.

네트워크 검사는 Standalone, Listen, Dedicated를 구분한다. 현재 테스트마다 생성 조건·서버 종류가 다르므로 Listen 통과를 Dedicated 통과로 쓰지 않는다. 에셋 제작과 게임 실행은 문서 작성만으로 새로 수행된 작업이 아니다.

## 9. 실패 판정과 정리

`run_automation.ps1`는 프로세스 종료 코드 외에 결과 JSON의 실패·미실행·진행 중 개수와 실제 성공 개수를 검사한다. 테스트가 하나도 실행되지 않은 상태를 성공으로 처리하지 않는다. 제한 시간이 지나면 오류를 내고 finally에서 자신이 시작한 프로세스를 정리한다.

`package_windows.ps1`는 UAT BuildCookRun으로 빌드·Cook·Stage·Archive를 요청하고 0이 아닌 종료 코드를 실패로 처리한다. `-SkipBuild`는 빌드를 건너뛰므로 오래된 실행 파일 사용 가능성을 따로 확인해야 한다.

`-SkipZenStore`는 이전 쿠킹 오류에 대응한 선택 옵션이다. 과거 한 환경에서의 성공이 모든 엔진·에셋 조합의 필요 조건은 아니다. 상세 이력은 Packaging을 따른다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| 에디터 수동 제작 | 시각적으로 빠르게 조정 | 반복 제작·재현·검증 누락 가능성 |
| 코드·Python 생성 | 연결과 검사 콘텐츠 재현 | 재실행 보호·API 변화·부분 실패 관리 |
| 에셋 중심 제작 + 자동 검증 | 편집 자유와 계약 검사 병행 | 검증기·기준 데이터 유지 비용 |

현재 도구는 검토용 콘텐츠 제작을 돕는다. 모든 아티스트 편집을 생성 코드로 대체하는 결정은 아니다.

## 11. 확인 방법

| 증거 | 증명하는 범위 | 별도 확인할 것 |
|---|---|---|
| 소스·diff 검토 | 구현과 계약의 존재 | 컴파일·실행 |
| Editor 빌드 성공 | 해당 Target 컴파일 | 에셋 로드·게임 흐름 |
| Automation 성공 | 실행된 테스트의 조건 | 실제 화면·전체 통합 |
| 렌더링 Smoke 성공 | 지정 화면·조작 연결 | 수동 조작·장시간 품질 |
| UAT 성공 | 해당 패키지 생성 | 패키지 실제 실행 |
| 패키지 실행 성공 | 그 기기·설정의 흐름 | 다른 PC·Shipping·외부 네트워크 |

기존 실행 결과는 각 Foundation과 Packaging에 기록돼 있다. 특히 UE 5.9.0의 과거 패키지 성공을 이번 기기의 UE 5.8.3 호환 검증으로 바꾸지 않는다. 이번 문서 작업에서는 생성·빌드·게임 테스트·패키징을 실행하지 않았다.

## 12. 이해 확인

**생성 스크립트를 여러 번 실행해도 안전하다고 언제 말할 수 있을까?**

각 대상의 기존 파일 검사, 덮어쓰기 정책, 부분 저장 실패를 확인했을 때다. 함수 이름이나 한 맵의 보호 조건으로 전체 도구를 일반화할 수 없다.

**프로세스가 종료 코드 0이면 자동 검사가 끝났을까?**

테스트 검색·성공 개수·미실행·진행 중 상태까지 봐야 한다. 잘못된 필터로 아무 검사도 실행하지 않은 경우를 구분해야 한다.
