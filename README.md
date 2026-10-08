# ccl8

Orbis의 방향과 그래픽 스타일을 공유하는 소규모 소울라이크 RPG다. AI가 제작을 주도하고 사용자는 검토와 방향 지시를 맡는다. 확정 범위와 기간은 [제작 방향](Docs/Direction.md), 플레이 초안은 [게임 기획](Docs/GameDesign.md), 시각 기준은 [아트 방향](Docs/ArtDirection.md)에 있다.

개발 환경과 기획 초안, `CCL.uproject`의 기본 프로젝트 구조를 구성했다. [멀티플레이 기본 흐름](Docs/MultiplayerFoundation.md)에 스폰, 이동·카메라와 개별 재스폰의 구현·검증 상태와 조작법을 정리했다. 저장소 루트에는 `CCL.uproject`가 있다.

다음 환경 구현 작업은 [자연 환경과 실험장 계획](Docs/EnvironmentPlan.md)을 따른다. 노트북의 시작점, 단계별 통과 기준과 미검증 범위를 함께 정리했다.

## 폴더

| 폴더 | 용도 |
|---|---|
| `Config/` | UE 프로젝트 설정 |
| `Content/` | 게임 에셋과 바이너리 원본, Git LFS 관리 |
| `Docs/` | 규칙, 사양, 결정, 학습·작업 현황 |
| `Source/` | CCL 모듈의 UE C++ 코드 |
| `Tools/` | Graft·Graphify와 에이전트 환경 복구 도구 |

엔진 소스는 별도로 준비한다. `CCL.uproject`를 사용하며 프로젝트 플러그인과 배포용 `Build/` 원본은 필요할 때 추가한다. 생성 파일과 분석 캐시는 Git에서 제외한다.

## 시작

1. [개발 안내](Docs/Guide.md)를 읽는다.
2. 새 PC에서는 [도구 설치·복구](Docs/AgentContextTools.md)를 따라 로컬 경로와 런타임을 준비한다.
3. [버전 관리](Docs/VersionControl.md)에 따라 Git LFS를 사용하고 연결된 원격과 작업 트리 상태를 확인한다.

도구는 기존 개인 프로젝트에서 이전했지만, 게임 코드·에셋·기존 기획 원문·과거 결정 이력은 가져오지 않았다. 공유할 방향은 ccl8의 독립 문서로 정리한다. 이전 범위와 검증 결과는 [초기 구성 기록](Docs/Setup.md)에 남긴다.
