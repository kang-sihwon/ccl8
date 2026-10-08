# 지도: 같은 공개 자료를 미니맵과 전체 지도에 표시하기

미니맵과 전체 지도는 `UCCLMapSubsystem`이 모은 지형·마커를 공유하고 표시 범위만 다르게 사용한다. 이 장은 월드 위치를 화면 좌표로 옮기는 과정과 관찰자에게 공개할 정보의 경계를 설명한다.

## 1. 비교 기준

| 구분 | 전체 커밋 |
|---|---|
| 이전 기준 | `ed9a763db54edd93c7fe802b887ee96b553bb0df` |
| 도입·변경 기준 | `0931fbfb5c96c4da24acb9d2ed44243fc0e6171f` |
| 현재 기준 | `50e583859c832d6cb325e5e76beb8356e8c13959` |

조사 시작 때 확인한 `main`과 로컬 원격 추적 `origin/main`의 `50e5838`을 게임 코드 기준으로 고정한 학습 기록이다. 원격 서버를 새로 조회한 결과는 아니며 이후 추가된 커밋과 구분한다. 작업 트리의 설정·빌드 파일 변경은 이 장의 코드 기준에서 제외했다. 아래 발췌의 경로·줄 번호는 표시한 커밋 기준이고 상대 링크는 현재 체크아웃을 연다.

이전 기준에는 `Source/CCL/Map/CCLMapSystem.cpp`와 `UI/CCLMapScreen.cpp`가 없었다. `0931fbf`의 최초 도입을 현재 기준과 대조한다. 요구·기존 확인은 결정 18과 [AgentReview](../AgentReview-2026-10-08.md)에 있다.

## 2. 해결할 문제

전체 지도를 열었다는 이유로 먼 적의 위치나 주민의 숨은 목적까지 나타나면 정보 공개 규칙이 깨진다. 미니맵에서 보이지 않는 적이 전체 지도에만 보이는 불일치도 피해야 한다.

현재는 공개 지형과 관찰자별 마커를 나눈다. 다만 표시 필터는 클라이언트에 이미 전달된 Actor 정보를 서버 수준에서 숨기는 보안 장치는 아니다. 네트워크 관련성·복제 공개 범위는 별도 계약이다.

## 3. 좌표, 캐시와 그리기

`FBox2D`는 XY 최소·최대값으로 영역을 표현한다. 월드 위치에서 최소값을 빼고 폭으로 나누면 상대 위치를 얻는다. 화면 Y 방향에 맞춰 세로 좌표는 뒤집는다.

`TActorIterator`는 해당 World의 Actor를 순회한다. 매번 지형을 다시 계산하지 않도록 World별로 캐시하고, 움직이는 마커는 갱신마다 수집한다.

`NativePaint`는 UMG 화면이 Slate DrawElement를 추가하는 경로다. 여기서는 Box와 Text를 직접 그린다. UMG와 Slate의 연결은 UE4에도 있었으며 현재 변경은 프로젝트 지도 기능의 도입이다.

## 4. 도입 당시의 공개 조건

이전 지도 구현이 없으므로 최초 공개 판정 코드를 읽는다.

출처: `0931fbf`, [CCLMapSystem.cpp](../../Source/CCL/Map/CCLMapSystem.cpp):9의 `UCCLMapMarkerComponent::IsRevealedTo`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
bool UCCLMapMarkerComponent::IsRevealedTo(const APlayerController* Observer) const
{
    if (!Observer || !Observer->GetPawn() || !IsValid(GetOwner()) || GetOwner()->IsHidden())
    {
        return false;
    }
    if (GetOwner() == Observer->GetPawn())
    {
        return true;
    }
    return RevealDistance > 0 && FVector::DistSquared(Observer->GetPawn()->GetActorLocation(), GetOwner()->GetActorLocation()) <= FMath::Square(RevealDistance) &&
        (!bRequireLineOfSight || Observer->LineOfSightTo(GetOwner()));
}
```

관찰자·Pawn·Owner의 유효성을 확인하고 숨겨진 Owner는 제외한다. 자기 Pawn은 표시한다. 다른 대상은 거리와 선택적 시야 검사를 통과해야 한다. 모르는 Agent의 내부 기억을 여기서 직접 조회하지 않는다.

## 5. 공개 자료를 공유한 이유

결정 18의 지도 요구를 구현한 구조다. 코드에서 분석하면, 두 화면이 같은 수집 결과를 쓰면 공개 조건의 중복을 줄일 수 있다. 확대 화면을 별도 탐지 시스템으로 만들 필요도 없다.

대신 같은 자료를 쓰는 것과 동일한 화면 범위로 그리는 것은 다르다. 미니맵은 플레이어 근처, 전체 지도는 지역 경계를 사용한다.

## 6. 현재의 투영과 화면 범위

출처: `50e5838`, [CCLMapSystem.cpp](../../Source/CCL/Map/CCLMapSystem.cpp):96의 `UCCLMapSubsystem::Project`. 실제 코드 전체 함수이며 들여쓰기를 공백으로 바꿨다.

```cpp
FVector2D UCCLMapSubsystem::Project(FVector World, const FBox2D& InBounds, FVector2D Size)
{
    const FVector2D Extent = InBounds.GetSize();
    return FVector2D((World.X - InBounds.Min.X) / FMath::Max(1., Extent.X) * Size.X,
        (1 - (World.Y - InBounds.Min.Y) / FMath::Max(1., Extent.Y)) * Size.Y);
}
```

첫 줄은 영역 크기를 구한다. X는 왼쪽 기준 비율, Y는 뒤집힌 비율에 표시 크기를 곱한다. 분모는 최소 1로 제한한다. 이 함수 자체는 영역 밖 좌표를 가장자리에 붙이지 않으며 화면의 클리핑이 표시를 제한한다.

화면에서는 Context의 전체 지도 여부로 크기·원점·Bounds를 정한다.

출처: `50e5838`, [CCLMapScreen.cpp](../../Source/CCL/UI/CCLMapScreen.cpp):43의 `UCCLMapScreen::NativePaint`. 실제 코드 함수 일부이며 들여쓰기를 공백으로 바꿨다.

```cpp
    const FVector2D Screen = Geometry.GetLocalSize();
    const float Scale = FMath::Min(Screen.X / 1280, Screen.Y / 720);
    const FVector2D Size = (Context->bFullMap ? FVector2D(880, 480) : FVector2D(270, 220)) * Scale;
    const FVector2D Origin = Context->bFullMap ? (Screen - Size) / 2 : FVector2D(Screen.X - Size.X - 24 * Scale, 68 * Scale);
    FBox2D Bounds = Map->GetBounds();
    if (!Context->bFullMap)
    {
        const FVector2D Center(PC->GetPawn()->GetActorLocation());
        Bounds = FBox2D(Center - FVector2D(1400, 1140), Center + FVector2D(1400, 1140));
    }
```

이후 같은 Terrain과 Markers를 Project로 변환해 그린다. 전체 지도 이름표는 가까이 겹치는 위치를 일부 생략한다. 정교한 지도 레이아웃 엔진은 아니다.

## 7. 자료 수집 안쪽

Refresh는 World가 바뀌면 충돌이 켜진 StaticMeshActor의 Bounds에서 공개 지형 사각형을 만든다. 크기 조건에 맞지 않는 메시를 제외하고 큰 면부터 정렬한다. 유효한 RegionDefinition이 있으면 작성한 경계·지형 자료로 교체한다.

매 갱신에서는 MarkerComponent가 있고 IsRevealedTo를 통과한 Actor의 위치·방향·종류·표시 이름을 수집한다. 지역 정의를 바꾸는 SetRegion은 World 캐시를 무효화한다.

현재 방식은 지형의 실제 삼각형, 층별 실내 구조, 영구 탐험 안개를 생성하는 기능이 아니다. 같은 World에서 동적 지형이 변했을 때는 캐시 갱신 정책을 추가 검토해야 한다.

## 8. 로컬 플레이어와 입력

MapSubsystem은 LocalPlayer 수명이며 화면은 자기 OwningPlayer로 Refresh한다. 서로 다른 플레이어의 관찰 조건을 전역 배열 하나로 섞지 않도록 한 구조다.

`UCCLMapScreen::OnManagedTick`은 0.2초 누적 기준으로 Refresh한다. 전체 지도는 공용 UI 화면으로 열리고 HUD 숨김 요청을 가진다. 화면 열기와 Back 입력은 [UI 기반](UIFramework.md)을 따른다.

## 9. 닫기와 무효 관찰자

관찰자나 Pawn이 없으면 마커를 비운다. 월드가 바뀌면 지형 캐시를 다시 만든다. 이전 월드의 Actor 포인터를 지도 영구 데이터로 저장하지 않는다.

전체 지도 Context가 해제되면 자신이 발급받은 HUD 숨김 Handle만 Release한다. 연출이 동시에 HUD를 숨겼다면 전체 지도를 닫았다는 이유로 그 요청까지 없애지 않는다.

## 10. 대안과 비용

| 방식 | 이점 | 비용 |
|---|---|---|
| SceneCapture 지형 영상 | 월드 외형을 직접 표시 | 렌더링 비용·숨길 대상 제어 |
| 작성한 지도 텍스처·자료 | 아트 품질·범위 제어 | 제작·지형 변경 동기화 |
| 현재 Bounds 기반 도형 | 빠른 구현·공유 자료 검사 | 단순한 모양, 동적 변경 갱신 필요 |

현재 구현은 검토용 지도의 기반이다. 최종 지도 아트와 다층 던전 탐색 정책까지 확정하지 않는다.

## 11. 확인 방법

| 상황 | 기대 결과 |
|---|---|
| 적이 공개 거리·시야 밖에 있음 | 두 지도에서 같은 공개 정책 |
| 전체 지도 열기·닫기 | 미니맵 등 HUD 숨김 요청 해제 |
| 다른 연출 중 지도 닫기 | 연출의 숨김 요청 유지 |
| Pawn 사라짐·월드 교체 | 오래된 마커 제거·캐시 재생성 |
| 지역 모서리·영역 밖 위치 | 좌표 방향과 클리핑이 일관됨 |

기존 통합 확인은 [AgentReview](../AgentReview-2026-10-08.md)에 있다. 이번에는 코드로 좌표·공개·해제 경로를 확인했으며 화면이나 네트워크를 새로 실행하지 않았다.

## 12. 이해 확인

**전체 지도가 더 넓으니 더 먼 적을 자동 공개해도 될까?**

표시 범위와 탐지 조건은 별개다. 현재는 같은 공개 자료를 넓은 범위에 그리므로 탐지 정책도 공유한다.

**지도에서 안 그린 적은 클라이언트가 그 위치를 모른다고 보장할까?**

아니다. Actor가 복제로 이미 전달됐다면 지도 필터와 관계없이 클라이언트에 정보가 있다. 서버의 정보 공개 정책은 따로 검증해야 한다.
