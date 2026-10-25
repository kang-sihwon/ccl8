---
name: ccl8-help
description: ccl8에서 직접 불러 쓰는 스킬과 명령의 목록이 필요할 때 쓴다.
disable-model-invocation: true
---

# ccl8 직접 호출 스킬 안내

아래 스킬은 에이전트가 스스로 부르지 않는다. 필요할 때 이름을 입력한다(Claude Code는 `/이름`, Codex는 `$이름`). 사용자가 무엇을 하려는지 말했으면 맞는 스킬 하나를 추천하고 이유를 한 줄로 붙인다.

| 스킬 | 언제 |
|---|---|
| `ccl8-work` | 여러 세션에 걸칠 작업을 시작할 때, 조각을 끝내거나 작업을 닫을 때 |
| `ccl8-handoff` | 작업을 중간에 멈출 때, 다른 세션이 이어받을 때 |
| `ccl8-decision-log` | 긴 작업 중에 내린 결정을 근거와 함께 한 줄씩 남길 때 |
| `ccl8-retro` | 같은 실수가 반복됐을 때, 작업 하나를 끝내고 돌아볼 때 |
| `ccl8-teach` | 시스템이나 개념을 단계적으로 배우고 싶을 때 |
| `ccl8-benchmark` | 성능 수치를 보고하거나 비교하기 전에 |
| `ccl8-blast-radius` | 위험한 변경을 머지하기 전에 안전하다는 근거를 실행으로 확인할 때 |
| `roadmap-review` | `Docs/StudyRoadmap.md` 같은 로드맵 초안을 검토할 때 |
| `graphify` | 문서 그래프를 직접 만들거나 질의할 때 |
| `/trace` (Claude Code 명령) | UE 5.8 심볼·CVar를 엔진 소스에서 추적해 볼트에 적을 때 |

에이전트가 스스로 찾아 쓰는 스킬은 `graft`, `ccl8-design`, `ccl8-verify`, `ccl8-debug`, `ccl8-review`, `ccl8-docs`, `ccl8-writing`, `ccl8-narrative`, `grilling`, `writing-for-agents`다. 이 스킬들도 이름으로 직접 부를 수 있다.
