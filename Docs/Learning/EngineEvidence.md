# 학습 문서의 UE 5.8 엔진 근거

학습 문서가 사용하는 엔진 계약을 실제 설치 소스에서 확인한 위치다. 프로젝트의 비교 커밋과 엔진 소스 버전은 별개다. 확인한 `Engine/Build/Build.version`은 UE 5.8.3, Changelist `58210709`, CompatibleChangelist `55116800`이다. 파일이 존재한다는 사실은 엔진 설치 완료·프로젝트 빌드 성공을 뜻하지 않는다.

아래 `<Engine>`은 설치 엔진의 `Engine/` 디렉터리다. 줄 번호는 이 설치 소스 기준이다. 다른 버전에서는 심볼로 다시 찾는다. 엔진 Graft로 먼저 조회했고 충분한 본문이 나오지 않은 범위는 실제 소스로 보완했다.

## ASC의 Owner와 Avatar

근거: `<Engine>/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp:161`, `UAbilitySystemComponent::InitAbilityActorInfo`.

함수 앞부분은 ActorInfo를 초기화하고 Owner·Avatar를 갱신한다. 아래는 실제 코드 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
void UAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
    check(AbilityActorInfo.IsValid());
    bool WasAbilityActorNull = (AbilityActorInfo->AvatarActor == nullptr);
    bool AvatarChanged = (InAvatarActor != AbilityActorInfo->AvatarActor);

    AbilityActorInfo->InitFromActor(InOwnerActor, InAvatarActor, this);

    SetOwnerActor(InOwnerActor);
```

Owner와 Avatar를 따로 받기 때문에 PlayerState의 ASC를 새 Pawn에 연결할 수 있다. 이 코드가 프로젝트의 Ability 재부여·Attribute 복원·입력 바인딩을 자동 수행한다는 뜻은 아니다. [GAS 전투](GASCombat.md), [성장](Progression.md)의 프로젝트 경로를 함께 읽는다. Owner·Avatar 구분은 UE4의 GAS에도 있던 개념이다.

## Fast Array의 변경 표시

근거: `<Engine>/Source/Runtime/Net/Core/Classes/Net/Serialization/FastArraySerializer.h:441`, `FFastArraySerializer::MarkItemDirty`와 `:457`의 `MarkArrayDirty`.

실제 함수는 새 항목의 ReplicationID를 부여하고 항목의 ReplicationKey를 증가시킨 뒤 배열도 Dirty 처리한다.

```cpp
void MarkItemDirty(FFastArraySerializerItem & Item)
{
    if (Item.ReplicationID == INDEX_NONE)
    {
        Item.ReplicationID = ++IDCounter;
        if (IDCounter == INDEX_NONE)
        {
            IDCounter++;
        }
    }

    Item.ReplicationKey++;
    MarkArrayDirty();
}
```

MarkArrayDirty는 ItemMap을 비우고 배열 키·캐시를 갱신한다. 같은 헤더 `:213`은 ReplicationID가 양쪽에서 맞춰져도 배열 인덱스는 같지 않을 수 있음을 설명한다. [아이템·인벤토리](ItemInventory.md)의 Item GUID는 이 엔진 복제 ID와 다른 게임 식별자다.

이 계약은 Fast Array 직렬화의 근거다. 프로젝트가 Iris를 사용한다고 판정하거나 Fast Array를 UE5 전용 신기능으로 설명하는 근거로 사용하지 않는다.

## FInstancedStruct의 타입과 값

근거: `<Engine>/Source/Runtime/CoreUObject/Public/StructUtils/InstancedStruct.h:87`의 InitializeAs, `:169`의 GetScriptStruct, `:175`의 GetMemory, `:181`의 Reset, `:192`의 GetPtr.

FInstancedStruct는 ScriptStruct와 구조체 메모리를 함께 다루는 값 컨테이너다. GetPtr은 맞는 타입의 포인터를 얻는 접근 경로이며 잘못된 타입을 무조건 같은 구조체로 해석하는 C++ 캐스팅과 구분한다.

[장비](Equipment.md)와 [Agent 상태](AgentState.md)는 이 컨테이너 위에 프로젝트의 타입·태그·버전 검증을 추가한다. 컨테이너가 있다는 이유로 Feature 의존성·마이그레이션·저장 무결성이 자동 보장되지 않는다. 여기서는 UE 5.8.3의 API를 기준으로 설명하며 UE4 전 버전에 동일한 API가 있다고 가정하지 않는다.

## CommonUI의 화면 풀

근거: `<Engine>/Plugins/Runtime/CommonUI/Source/CommonUI/Public/Widgets/CommonActivatableWidgetContainer.h:49`의 초기화 콜백을 받는 AddWidget, `<Engine>/Plugins/Runtime/CommonUI/Source/CommonUI/Private/Widgets/CommonActivatableWidgetContainer.cpp:191`의 AddWidgetInternal, `:236`의 ReleaseWidget.

AddWidgetInternal은 `GeneratedWidgetsPool.GetOrCreateInstance`로 인스턴스를 얻고 초기화 콜백을 호출한다. ReleaseWidget은 대응하는 Widget을 풀에 반환한다. 매번 새로운 UObject를 생성한다고 가정하면 이전 Context·Delegate·포커스가 남는 문제를 놓칠 수 있다.

[UI 기반](UIFramework.md)의 BindContext·ReleaseContext가 이 재사용 경계에서 프로젝트 상태를 정리한다. [인벤토리 UI](InventoryUI.md)의 Slate 본문이 CommonUI 전환 후에도 남아 있다는 설명과 모순되지 않는다. 화면 관리와 내용 렌더링은 다른 층이다.

## MVVM 값 변경 알림

근거: `<Engine>/Plugins/Runtime/ModelViewViewModel/Source/ModelViewViewModel/Public/MVVMViewModelBase.h:20`의 `UE_MVVM_SET_PROPERTY_VALUE`, `:54`의 AddFieldValueChangedDelegate, `:55`의 RemoveFieldValueChangedDelegate.

매크로는 SetPropertyValue에 멤버·새 값·FieldId를 전달한다. 값 변경 알림과 Delegate 해제 API가 존재한다. [HUD·대화](HUDDialogue.md)의 ViewModel은 ASC 상태를 화면용 값으로 옮긴다. 화면을 숨기는 일, 게임 상태 복제, Delegate의 소유권은 값 알림 하나로 해결되지 않는다.

이 문서의 MVVM 플러그인 API는 확인한 UE5 기준이다. UE4의 UMG 바인딩과 동일한 구현으로 설명하지 않는다.

## LocalPlayer 수명

근거: `<Engine>/Source/Runtime/Engine/Public/Subsystems/LocalPlayerSubsystem.h:12`의 클래스 계약, `:23`의 GetLocalPlayer, `:35`의 PlayerControllerChanged.

LocalPlayerSubsystem은 LocalPlayer와 수명을 공유하고 GetOuter에서 LocalPlayer를 얻는다. Controller 변경 콜백도 별도로 둔다. 따라서 [UI 기반](UIFramework.md), [지도](Map.md), [로컬 연출](Cinematics.md)은 한 Pawn에 영구적으로 묶인 서비스로 읽으면 안 된다.

이 선언만으로 모든 프로젝트의 World 전환 처리가 안전하다고 보장할 수 없다. 프로젝트의 Controller·World 변경 감지와 정리 코드를 확인해야 한다.

## Mass Entity의 생성과 반환

근거: `<Engine>/Source/Runtime/MassEntity/Public/MassEntityManager.h:284`의 CreateEntity, `:354`의 DestroyEntity, `:768`의 GetFragmentDataChecked.

Manager는 Archetype으로 Entity를 만들고 Fragment를 조회하며 Entity를 파괴한다. [Agent 실행](AgentExecution.md)의 Lease 획득·Commit·Release는 이 API 밖의 프로젝트 계약이다. Entity 삭제가 Agent 기록의 저장과 쓰기 실행권 해제를 대신하지 않는다.

현재 소스와 소규모 연결 검사의 존재는 대규모 성능 측정 결과가 아니다. 프로세서 구성·개체 수·프레임 예산의 검증은 별도로 남는다.

## 확인 범위

이번 확인은 설치 파일의 버전·심볼·본문과 프로젝트 코드 발췌의 대조다. 엔진 빌드, 에디터 실행, 패키지 실행은 하지 않았다. 각 Foundation에 기록된 이전 엔진 환경의 성공을 현재 UE 5.8.3에서 새로 재현한 결과로 사용하지 않는다.
