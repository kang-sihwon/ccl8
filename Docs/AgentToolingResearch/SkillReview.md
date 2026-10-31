# 외부 스킬 후보 검토

외부 스킬 후보의 채택과 제외 근거를 찾을 때 읽는다. 스킬 묶음 세 개의 통합 판단은 [스킬 묶음 통합 검토](BundleReview.md)가 소유한다.

## 스킬 탐색 결과

사용자가 언급한 스킬 탐색에 해당하는 후보는 Vercel의 `find-skills`다. 공개 스킬을 찾아주는 안내 스킬이며 `npx skills find` CLI는 이 스킬을 설치하지 않아도 사용할 수 있다. Codex의 내장 `skill-installer`는 공식 목록 조회와 지정한 GitHub 경로 설치를 담당한다. [Vercel find-skills 원문](https://github.com/vercel-labs/skills/blob/main/skills/find-skills/SKILL.md), [공식 스킬 목록](https://github.com/openai/skills/tree/main/skills/.curated)

2026-10-05 조회는 내장 `skill-installer`의 공식 목록과 `skills@1.7.0`의 `unreal`, `game development`, `blender` 검색으로 했다. 공식 목록에서 UE 전용 스킬은 찾지 못했다. 후반 검색은 결과 출력 뒤 Windows libuv의 `UV_HANDLE_CLOSING` assertion으로 비정상 종료해 세 명령 모두 정상 종료했다고 보지 않는다. 관련 후보는 GitHub 원문을 직접 읽어 판단했고 설치 수를 품질 근거로 쓰지 않았다.

| 후보 | 판단 | 근거와 조건 |
|---|---|---|
| vercel-labs/skills의 find-skills | 선택적 도입 | 향후 스킬 탐색 반복에 편리하다. 설치만으로 UE 제작 기능이 생기는 것은 아니다 |
| Superpowers의 systematic-debugging, verification-before-completion | 기존 설치 상태 확인 후 선별 활용 | 원인 확인과 실행 증거 중심의 절차가 유용하다. 전체 워크플로를 다시 설치하면 ccl8의 계획·승인 규칙과 중복될 수 있다 |
| arjun988/blender-skills의 unreal-export | Blender 채택 후 조건부 도입 | 단위·충돌·소켓·LOD·UE 반입 검증을 다룬다. 선행 스킬과 참조 파일이 있으므로 단일 파일만 복사해서는 충분하지 않다 |
| 같은 묶음의 asset-optimization | 수정 검토 후 조건부 도입 | 모델 정리 체크리스트는 유용하지만 고정 LOD 비율·UV 수치·이름 규칙은 ccl8 에셋 종류에 맞춰 검토해야 한다 |
| Jeffallan/claude-skills의 game-developer | 지금은 보류 | Unity와 Unreal을 함께 다루고, 고정 프레임 목표와 Unity 규칙이 섞여 있다. 기존 ccl8 설계 지침보다 추가 이득이 작다고 판단한다 |
| sickn33/agentic-awesome-skills의 unreal-engine-cpp-pro | 원문 그대로 도입하지 않음 | AddToRoot 일반 우선 권고와 비동기 로딩 절의 LoadSynchronous 예시를 검토해야 한다. 현재 엔진 소스와 프로젝트 규칙을 우선한다 |
| openai/skills의 gh-fix-ci | GitHub Actions 도입 후 재검토 | CI 실패 로그 조사용이다. 현재 원격·CI가 없고 gh 실행 파일도 PATH에서 확인하지 못했다 |

후보의 원문: [Superpowers 디버깅](https://github.com/obra/superpowers/blob/main/skills/systematic-debugging/SKILL.md), [검증 절차](https://github.com/obra/superpowers/blob/main/skills/verification-before-completion/SKILL.md), [unreal-export](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/unreal-export/SKILL.md), [asset-optimization](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/asset-optimization/SKILL.md), [game-developer](https://github.com/Jeffallan/claude-skills/blob/main/skills/game-developer/SKILL.md), [unreal-engine-cpp-pro](https://github.com/sickn33/agentic-awesome-skills/blob/main/skills/unreal-engine-cpp-pro/SKILL.md), [gh-fix-ci](https://github.com/openai/skills/blob/main/skills/.curated/gh-fix-ci/SKILL.md).

## pstack 검토: 2026-10-25

pstack은 Lauren Tan이 만든 Cursor 플러그인 `cursor/plugins/pstack`의 Claude Code 이식본이다. 같은 이름의 커뮤니티 이식본이 여럿 있으며, 이번에는 MIT 라이선스로 상류를 추적하는 `ahmedbenaw/pstack`을 기준으로 했다. 원칙 스킬 23개, `poteto-mode` 플레이북, 조사·리뷰 스킬(`how`, `why`, `interrogate`, `architect`, `arena`, `swarm`), 검증 스킬 생성·유지 스킬, 문장 정리와 TypeScript 지침, 훅 템플릿을 포함한다.

`ahmedbenaw/pstack`의 GitHub 원문 9개를 읽었고 설치와 설정 변경은 하지 않았다. 대상은 `create-verification-skill`과 그 기능 지도 예시, `maintain-verification-skill`, `poteto-mode`, 원칙 6개(`prove-it-works`, `never-block-on-the-human`, `sequence-verifiable-units`, `guard-the-context-window`, `encode-lessons-in-structure`, `subtract-before-you-add`)다.

| 대상 | 판단 | 근거와 조건 |
|---|---|---|
| 묶음 전체 | 도입하지 않음 | 조사·설계·질문·문장·리뷰 스킬이 graft, `ccl8-design`, `grilling`, `ccl8-docs`, `code-review`와 겹친다. 운영 규칙의 소유자가 Guide와 pstack 둘로 나뉜다 |
| `poteto-mode` | 도입하지 않음 | 작업마다 플레이북 단계를 할 일 목록에 복사하고, 서브에이전트를 백그라운드와 역할별 모델로 운용하며, `gh` 기반으로 출시한다. Guide의 다중 에이전트 사용 범위, 원격·CI가 없는 현재 저장소와 맞지 않는다 |
| `create-verification-skill` | 구조만 참고 | 프로젝트 전용 검증 스킬에 실행·상태 확인·조작·증거·정리 절과 기능 지도를 두게 한다. 웹 앱 전용이 아니며 기존 실행 도구를 먼저 쓰게 한다. ccl8의 `Tools/Validation/run_*.ps1`을 묶는 `ccl8-verify` 초안에 적용했다 |
| `maintain-verification-skill` | 순서만 참고 | 기능별 읽기 전용 서브에이전트 병렬 분석과 PR 제출을 전제로 한다. 지도 정리, 소스 대조, 실제 실행, 문서 차이·도구 공백·제품 결함의 구분 순서는 `ccl8-verify`의 지도 갱신 절에 순차 절차로 옮겼다 |
| 원칙 6개 | 별도 도입하지 않음 | `prove-it-works`, `sequence-verifiable-units`, `subtract-before-you-add`는 Coding의 완료 검증·배치 변경 검증과 대부분 겹친다. `never-block-on-the-human`은 되돌릴 수 있는 작업을 확인 없이 진행하게 해 삭제·덮어쓰기 전에 사용자 확인을 받는 작업 규칙과 충돌한다. `guard-the-context-window`는 큰 출력을 서브에이전트로 넘기게 해 Guide의 사용 범위와 충돌한다. `encode-lessons-in-structure`의 반복 지시를 검사기로 바꾸는 방식은 후속 개선 후보로 남긴다 |

pstack의 증거 기준은 내부 setter나 테스트 전용 경로를 사용자 경로의 증거로 인정하지 않는다. 이 기준을 적용하면 ccl8 스모크의 증거 강도는 스크립트마다 다르다. `run_network_smoke.ps1`은 Enhanced Input Action에 입력을 주입하고, 전투·Campaign 스모크는 Ability 입력 태그를, UI·세션·실험 화면 스모크는 Slate 이벤트를 사용한다. `run_integration_smoke.ps1` 같은 나머지 스모크는 Controller·Subsystem 함수를 직접 호출한다. 스크립트별 분류는 `ccl8-verify`의 기능 지도에 둔다.

나머지 원칙 17개, 플레이북 본문, MCP 서버와 다른 이식본은 읽지 않았다. 원문: [저장소](https://github.com/ahmedbenaw/pstack), [create-verification-skill](https://github.com/ahmedbenaw/pstack/blob/HEAD/skills/create-verification-skill/SKILL.md), [maintain-verification-skill](https://github.com/ahmedbenaw/pstack/blob/HEAD/skills/maintain-verification-skill/SKILL.md), [poteto-mode](https://github.com/ahmedbenaw/pstack/blob/HEAD/skills/poteto-mode/SKILL.md), [prove-it-works](https://github.com/ahmedbenaw/pstack/blob/HEAD/skills/principle-prove-it-works/SKILL.md), [never-block-on-the-human](https://github.com/ahmedbenaw/pstack/blob/HEAD/skills/principle-never-block-on-the-human/SKILL.md).
