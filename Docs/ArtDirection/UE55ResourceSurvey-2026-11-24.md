# UE5_5 석조 건축 부재

`Medieval_MWP`의 메시 48개를 UE5_2와 같은 `/Game/ExternalPackage/BuildingParts` 아래에 정리했다. 이름이 `SM_MMW_000`부터 `SM_MMW_047`까지 번호로만 구분되어, 원본 패키지에 저장된 썸네일에서 개구부의 위치와 모양을 보고 용도를 나눴다.

| 찾는 부재 | `BuildingParts` 아래 폴더 | 수량 | 형태 예시 |
|---|---|---:|---|
| 문틀·통로 | `DoorsAndGates` | 13 | `002`는 바닥까지 열린 둥근 아치, `045`는 뾰족한 아치다. |
| 창이 있는 벽 | `WindowsAndShutters` | 29 | `011`은 세 칸 창, `035`는 창살이 있는 좁은 창이다. |
| 벽판·홈이 있는 벽 | `Walls` | 6 | `003`은 아치형 홈, `005`는 직사각형 홈, `042`는 석조 벽판이다. |

![석조 부재 48개의 원본 썸네일과 에셋 번호](UE55ResourceSurvey-2026-11-24/mesh-sheet.jpg)

공용 재질 1개는 `Materials/Medieval_MWP`, 텍스처 7개는 `Textures/Medieval_MWP`, 전시 맵은 `Demo/Medieval_MWP/Maps/Showcase`에 있다. 세 경로의 루트는 모두 `/Game/ExternalPackage`다. [57개 패키지의 이동 전후 경로표](UE55ResourceSurvey-2026-11-24/ExternalPackage-paths.tsv)

UE 5.5.4에서 이동 전 원본 사본과 적용 후 실제 프로젝트의 패키지 57개를 모두 로드했다. 프로젝트 내부의 하드 참조 101개가 새 경로에서 일치했고, 소프트 참조와 리디렉터는 각각 0개였다. 쇼케이스를 열어 액터 60개의 이름·클래스·트랜스폼과 할당된 정적 메시가 이동 전과 같은지도 대조했다. 원본 팩은 `UE5_5/Saved/BeforeExternalPackage-20261124/Content/Medieval_MWP`에 보관했다.
