# 스킬 선택과 사용 안내

작업을 요청하면 에이전트가 사용할 스킬과 역할을 제안하고 사용자 확인 뒤에 진행한다. 선택 안내에는 관련 스킬도 함께 보여 준다. 사용자가 스킬 이름을 기억하거나 먼저 지정할 필요는 없다. 이 절차는 CCL 저장소의 신규·기존 스킬과 해당 작업에 필요한 세션 스킬에 적용한다. 근거는 [결정 38](../decisions/0038-skill-selection-and-creation-suite.md)이다.

## 작업 시작 시 확인

1. 요청과 현재 작업 기록·스킬 설명을 읽고 필요한 산출물과 스킬 후보를 고른다. 선택에 필요한 읽기 전용 확인은 먼저 할 수 있다.
2. **이번에 사용할 스킬**의 이름·역할, **만들거나 확인할 결과**, **관련 선택 스킬**의 용도를 짧게 보여 준다. 관련 목록은 요청과 실제로 연결되는 항목을 고르며 전체 목록은 이 문서로 연결한다.
3. 사용자의 선택을 아직 받지 않은 조합이면 “이 조합으로 진행해도 될까?”라고 묻고 답을 기다린다. 질문 도구가 있으면 사용하고, 없으면 최종 응답으로 질문한다. 무응답이나 시간 경과를 동의로 취급하지 않는다. 확인 전에는 본 작업의 생성·수정·실행을 시작하지 않는다.
4. 사용자가 스킬을 직접 지정했으면 그 선택은 이미 동의한 것으로 다룬다. 사용 스킬과 관련 후보를 알리고 요청한 범위를 진행한다. 새로 추가할 전문 스킬이나 범위가 있다면 그 부분을 확인한다.
5. 승인한 조합과 범위로 작업을 이어가는 동안에는 다시 묻지 않는다. “계속해줘”는 같은 범위의 진행으로 다룬다. 새로운 과제, 산출물 확대, 승인하지 않은 전문 스킬의 추가·교체가 필요할 때 다시 제안한다. 승인 기록이 없으면 이전 동의를 추정하지 않는다.

제안 단계에서 문서화·검토·실행 검증에 필요한 보조 스킬까지 함께 포함한다. 관련 선택 스킬은 소개한 것만으로 실행하지 않는다. 범위 밖 스킬이 불필요해지면 이유를 알리고 제외할 수 있다. 스킬을 사용할 필요가 없는 단순 질문에는 “별도 스킬 없이 답할게”라고 알리고 답한다.

스킬 선택 동의는 삭제·외부 전송·유료 작업·서사 정본 승인 등의 권한을 대신하지 않는다. 권한은 원래 요청과 해당 프로젝트 규칙을 따른다. 이 확인은 사용자가 2026-11-04에 직접 요청한 작업 방식이다.

### 안내 예시

> 이번 작업에는 `creative-fiction-writer`로 사건 흐름을, `game-designer-assistant`로 몬스터 조우와 보스전의 판단을 구성하고, `ccl8-narrative`로 후보를 기록할게. 결과는 로덴 구간의 사건·전투·동료 변화·성장·다음 지역의 계기를 연결한 플롯 초안이야.
>
> 관련 스킬은 `character-creator`(동료 동기), `mythology-creator`(괴물 기원), `map-maker`(이동 공간)야. 이번 조합에는 포함하지 않았어. 이 조합으로 진행해도 될까?

같은 결과를 여러 스킬이 만들 수 있으면 주 담당 하나를 고르고 나머지는 다른 관점이 필요한 경우에만 제안한다. 내부 검색·파일 읽기마다 새 승인을 요청하지 않는다.

## 직접 사용하는 방법

일반 문장으로 작업을 요청해도 된다. 직접 고르려면 Codex에서는 `$스킬이름`, Claude Code에서는 `/스킬이름`을 붙인다. 예: `$game-designer-assistant 모든 무기 선택을 유지하면서 보스전의 관찰·대응·성장 흐름을 짜줘.`

스킬은 자동 발견 가능한 상태로 유지한다. 선택 확인은 협업 지침으로 수행하며 `allow_implicit_invocation: false`로 목록에서 숨기는 방식이 아니다. 현재 세션 목록에 새 스킬이 아직 표시되지 않으면 아래 정본 링크를 직접 읽어 사용할 수 있다. 새 세션에서의 자동 발견 여부와 실제 과제 수행 품질은 별도로 확인한다.

정본은 `.agents/skills/`, Claude용 사본은 `.claude/skills/`다. `node Tools/Agent/skills.cjs sync --write`로 맞추고 `node Tools/Agent/skills.cjs check`로 검사한다. 이 스킬 묶음은 프로젝트 범위이며 다른 저장소의 전역 지침을 바꾸지 않는다.

## 게임 제작·조사 스킬 15개

기존 Worldbuilding Studio·Map Maker와 신규 13개를 함께 안내한다. C++ Coding Assistant는 사용자 요청에 따라 포함하지 않았다. 공개 설명의 조사 근거는 [Jenova 전체 조사](../AgentToolingResearch/2026-11-04-jenova-resources.md)에 있다.

| 분야·스킬 | 맡길 일과 요청 예 | 함께 제안할 관련 스킬 |
|---|---|---|
| [Game Designer Assistant](../../.agents/skills/game-designer-assistant/SKILL.md) | 전투·성장·조우·보상·경제. “무기를 고정하지 않고 지역의 보스전과 성장 흐름을 짜줘” | Creative Fiction Writer, Map Maker, ccl8-design, ccl8-verify |
| [Creative Fiction Writer](../../.agents/skills/creative-fiction-writer/SKILL.md) | 사건·장면·대사·복선. “벨레스에서 로덴으로 이어지는 전투 중심 플롯을 구체화해줘” | Character Creator, Game Designer Assistant, ccl8-narrative |
| [Worldbuilding Studio](../../.agents/skills/worldbuilding-studio/SKILL.md) | 세계 규칙·도시·세력·문화·생활의 인과관계. “세계수가 물류와 권력에 미치는 영향을 정리해줘” | Map Maker, Religion Creator, Conlang Creator |
| [Map Maker](../../.agents/skills/map-maker/SKILL.md) | 세계·지역·정착지·던전 지도와 부분 수정. “상위 지도를 유지하면서 로덴 평면도를 만들어줘” | Worldbuilding Studio, Game Designer Assistant |
| [Character Creator](../../.agents/skills/character-creator/SKILL.md) | 주인공·동료·적대자의 동기·결점·관계·말투. “동료의 욕망과 두려움이 충돌하는 선택을 만들어줘” | Creative Fiction Writer, Concept Art Creator |
| [Mythology Creator](../../.agents/skills/mythology-creator/SKILL.md) | 창세·신계·전승·괴물 기원. “지역의 환경과 역사에서 보스의 기원과 금기를 도출해줘” | Religion Creator, Game Designer Assistant, Concept Art Creator |
| [Religion Creator](../../.agents/skills/religion-creator/SKILL.md) | 가상 종교의 교리·의례·교단·분파. “같은 세계수 신앙이 도시마다 갈라지는 이유와 생활 의례를 짜줘” | Mythology Creator, Worldbuilding Studio, Character Creator |
| [Conlang Creator](../../.agents/skills/conlang-creator/SKILL.md) | 가상 언어·문자·어원·명명 규칙. “기존 이름을 유지하면서 북부 지명의 공통 어근을 만들어줘” | Worldbuilding Studio, Map Maker, Graphic Designer |
| [Roleplay Game Master](../../.agents/skills/roleplay-game-master/SKILL.md) | 사용자 선택에 따른 NPC 반응·분기 모의 진행. “내가 주인공을 맡아 협상 장면을 시험할게” | Character Creator, Creative Fiction Writer, Game Designer Assistant |
| [Concept Art Creator](../../.agents/skills/concept-art-creator/SKILL.md) | 캐릭터·괴물·환경·장비의 형태·재질·기능 시안. “보스의 이동 방식이 읽히는 실루엣 시안을 만들어줘” | Character Creator, Mythology Creator, 이미지 제작 스킬 |
| [Graphic Designer](../../.agents/skills/graphic-designer/SKILL.md) | 로고·문장·아이콘·홍보물·시각 체계. “세력 문장을 작은 아이콘에서도 구별되게 만들어줘” | Worldbuilding Studio, UI/UX Reviewer, 이미지 제작 스킬 |
| [Storyboard Artist](../../.agents/skills/storyboard-artist/SKILL.md) | 컷신·보스 등장·추격의 숏·카메라·연속성. “보스 등장부터 조작권 반환까지 보드로 짜줘” | Creative Fiction Writer, Concept Art Creator, Game Designer Assistant |
| [UI/UX Reviewer](../../.agents/skills/ui-ux-reviewer/SKILL.md) | HUD·메뉴·인벤토리의 정보·상태·입력 흐름 검토. “패드로 장비를 바꾸는 화면 흐름을 검토해줘” | Graphic Designer, ccl8-design, ccl8-verify |
| [Music Composition Assistant](../../.agents/skills/music-composition-assistant/SKILL.md) | 지역·인물·보스 모티프·화성·편성·악보. “탐험 테마를 보스전에서 변주하는 구조를 짜줘” | Worldbuilding Studio, Storyboard Artist, Game Designer Assistant |
| [Deep Research](../../.agents/skills/deep-research/SKILL.md) | 역사·건축·무기·생태·기술의 다중 출처 조사. “하천 도시의 물류·방어 사례를 출처와 함께 비교해줘” | Worldbuilding Studio, Game Designer Assistant, ccl8-docs |

### 역할이 겹칠 때의 선택

| 요청의 중심 | 주 담당 | 나머지 역할 |
|---|---|---|
| 지역 사건과 주인공의 선택을 미리 설계 | Creative Fiction Writer | Game Designer Assistant가 전투 판단을, Character Creator가 동기를 보완 |
| 사용자가 선택하며 장면을 진행 | Roleplay Game Master | 모의 결과를 플롯 후보로 남길 때 Creative Fiction Writer와 ccl8-narrative 제안 |
| 도시·세력의 성립과 변화 | Worldbuilding Studio | 지도 배치 자체는 Map Maker |
| 신·괴물의 기원과 전승 | Mythology Creator | 교리·성직·분파·의례 제도는 Religion Creator |
| 캐릭터·환경의 형태와 재료 | Concept Art Creator | 로고·아이콘·홍보물의 시각 구성은 Graphic Designer |
| 화면을 읽고 사용하는 데 생긴 문제 | UI/UX Reviewer | 이미지·레이아웃 제작은 Graphic Designer, 실제 동작은 ccl8-verify |
| 전투 규칙·성장·보상의 기획 | Game Designer Assistant | UE 책임·수명·인터페이스는 ccl8-design |

스킬은 작업 방법을 제공한다. 이미지·악보·3D·게임 동작을 요청하면 현재 세션에서 실제 도구를 확인하고 그 도구의 스킬도 선택 안내에 포함한다. 콘셉트 그림은 완성 메시가 아니며, 악보는 최종 음원이 아니고, 역할극은 실제 전투 테스트가 아니다. 사용할 도구가 없으면 가능한 범위와 미실행 항목을 알린다.

## 기존 프로젝트 스킬과 함께 사용

| 작업 | 함께 사용할 기존 스킬 | 관계 |
|---|---|---|
| 서사 초안·기록·검토·정본 | [ccl8-narrative](../../.agents/skills/ccl8-narrative/SKILL.md) | 새 창작 스킬은 창작 방법을, 이 스킬은 기록·검토·승인 절차를 맡음 |
| UE 코드 설계 | [ccl8-design](../../.agents/skills/ccl8-design/SKILL.md) | Game Designer Assistant의 규칙을 구현 구조로 옮길 때 제안 |
| 버그 원인 추적 | [ccl8-debug](../../.agents/skills/ccl8-debug/SKILL.md) | 재현·가설·수정 확인 |
| 코드 검토 | [ccl8-review](../../.agents/skills/ccl8-review/SKILL.md) | 규칙·사양에 맞는지 검토 |
| 실제 게임 실행 확인 | [ccl8-verify](../../.agents/skills/ccl8-verify/SKILL.md) | 화면·코드만으로 확인할 수 없는 동작 검증 |
| 일반 문서 저장·정리 | [ccl8-docs](../../.agents/skills/ccl8-docs/SKILL.md), [ccl8-writing](../../.agents/skills/ccl8-writing/SKILL.md) | 문서 소유·등록과 문장 검토 |
| 코드 위치·호출 관계 | [graft](../../.agents/skills/graft/SKILL.md) | 해당 코드 작업의 읽기 도구 |
| 스킬·에이전트 지침 작성 | [writing-for-agents](../../.agents/skills/writing-for-agents/SKILL.md) | 선택 조건과 실행 지침을 명료하게 작성 |

그 밖의 직접 호출 스킬은 [ccl8-help](../../.agents/skills/ccl8-help/SKILL.md)에서 찾는다. 외부 제작 플러그인이나 세션 전용 스킬은 실제 사용 가능 여부를 확인한 뒤 관련 후보로 소개한다.
