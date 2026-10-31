# 컨셉 이미지 도구 비교 테스트

시나리오와 배경 설명으로 컨셉 시안과 3D 모델링용 기준 이미지를 만들 AI 도구를 고르기 위한 비교 테스트다. 같은 프롬프트를 후보 도구마다 넣고 채점표로 비교한 뒤, 단계별로 쓸 도구를 정한다. 이 문서는 비교 대상, 진행 조건, 테스트 프롬프트, 채점 기준을 소유한다. 시각 기준은 [ArtDirection](ArtDirection.md)이, 에이전트 도구 조사는 [AgentToolingResearch](AgentToolingResearch.md)가 소유한다.

2026-10-30까지 T1 영어·한국어와 T2 프롬프트의 1라운드 일부, T3부터 T7까지의 1라운드를 실행했고 도구는 아직 정하지 않았다. T0과 T8부터 T12까지는 2026-10-31에 실행했다. 결과는 이 문서의 "결과" 절에 날짜를 붙여 추가하고, 결과 이미지는 `ConceptImageToolTest/` 폴더에 테스트 ID별로 둔다.

## 하위 문서

| 문서 | 읽는 때 |
|---|---|
| [테스트 프롬프트](ConceptImageToolTest/Prompts.md) | 시험에 쓴 프롬프트 원문이 필요할 때 |
| [시험 결과](ConceptImageToolTest/Results.md) | 도구별 결과, 채점, 사용자 순위를 확인할 때 |

## 피사체의 지위

프롬프트의 마을, 주인공, 몹, 소품은 도구를 비교하기 위한 임시값이다. 2026-10-28 기준 [세계관 정본](Narrative/World.md)과 [시나리오 정본](Narrative/Scenario.md)에는 확정한 설정이 없다. 그래서 피사체는 ArtDirection의 시각 기준 제안과 [GameDesign](GameDesign.md)의 일반 몹 역할 예시에만 맞췄다. 이 프롬프트나 결과 이미지를 세계 설정, 인물 디자인, 몹 외형의 근거로 쓰지 않는다.

T0과 T8은 게임에 맞는 세계로 시험하려고 [T0 세계 후보 비교](Narrative/Drafts/2026-10-31-t0-world-candidates.md)의 후보 A, 닫힌 수문을 기준으로 삼았다. 사용자가 2026-10-31에 시안 기준으로 고른 후보이고 정본 승인은 아니다. 그래서 T0과 T8의 피사체도 임시값이다.

## 비교 대상

2026-10-28 웹 조사로 단계마다 후보를 골랐다. 출처 대부분이 업체나 제휴 블로그의 비교 글이고 일부는 자기 제품을 순위에 넣었다. 그래서 아래의 강점은 출처의 주장이며 ccl8에서 확인한 사실이 아니다.

| 단계 | 후보 | 출처가 주장하는 강점 |
|---|---|---|
| 분위기·발상 | Midjourney V8 | 컨셉아트·키아트 품질, `--sref` 스타일 코드 재사용 |
| 디자인 고정·부분 수정 | GPT Image 2 | 수정할 때 원래 형태와 실루엣 보존 |
| 참조 여러 장·3면도 | Nano Banana Pro | 참조 이미지 최대 14장(캐릭터 5장, 스타일 3장), 분위기 표현 |
| 오픈 가중치·로컬 제어 | FLUX.2 | 오픈 가중치, 자체 실행과 미세 조정, 프롬프트 반영 |
| 전용 모델 학습(선택) | Scenario | 이미지 5장 이상으로 LoRA 학습, 턴어라운드 기능 |
| 이미지→3D | Hunyuan3D, Tripo, Meshy | Hunyuan3D 2.1 오픈 가중치와 3.1 Pro의 8시점 입력, Tripo 자동 리깅, Meshy 로우폴리·PBR |

한국어 서술형 문장은 대화형 모델(GPT Image 2, Nano Banana Pro)이 Midjourney보다 잘 이해할 것이라고 추정했다. 이 추정은 T1의 한국어 프롬프트로 확인한다.

조사 출처는 다음과 같다.

- [Ludo, Best AI Image Generators for Game Art](https://ludo.ai/compare/best-ai-image-generators-for-game-art)
- [Tech Insider, GPT Image 2 vs Nano Banana Pro](https://tech-insider.org/gpt-image-2-vs-nano-banana-pro/)
- [laozhang, Nano Banana Pro vs GPT Image 2.5](https://blog.laozhang.ai/en/posts/nano-banana-pro-vs-gpt-image-2-5)
- [Midjourney, Character Reference](https://docs.midjourney.com/docs/character-reference)
- [Scenario, Generate Character Turnarounds](https://help.scenario.com/articles/1419523552-generate-character-turnarounds)
- [Apatero, AI Character Turnaround Sheet Guide 2026](https://apatero.com/blog/ai-character-turnaround-sheet-generation-guide-2026)
- [3D AI Studio, Best Image to 3D Tools 2026](https://www.3daistudio.com/3d-generator-ai-comparison-alternatives-guide/best-image-to-3d-tools-2026)
- [Inker, Meshy vs Tripo vs Hunyuan vs Rodin](https://inker.si/blog/best-ai-3d-model-generators-2026)
- [ComfyUI, FLUX workflows](https://comfy.org/workflows/model/flux/)

## 이용 조건과 출시 공개

테스트 결과를 제작에 쓰기 전에 도구별 이용 조건을 다시 확인한다. 아래는 2026-10-28에 읽은 2차 자료의 내용이고 각 서비스의 약관 원문은 확인하지 않았다.

- Midjourney는 연 매출 100만 달러가 넘는 회사에 Pro 이상 플랜을 요구하고, 비공개 생성(Stealth Mode)은 Pro와 Mega에서만 된다고 한다.
- Recraft와 Leonardo는 무료 플랜 결과물이 공개되고, Scenario 무료 플랜 결과물은 개인·평가용이라고 한다. 위 두 항목의 출처는 [Ludo의 비교 글](https://ludo.ai/compare/best-ai-image-generators-for-game-art)이다.
- Steam은 2026-01-16 개정으로 출시물에 들어가지 않는 발상용 컨셉아트를 공개 대상에서 뺐다고 한다. AI로 만든 메시나 텍스처를 게임에 넣으면 공개 대상이다. Steamworks 원문은 확인하지 않았다. [Legal Moves, Steam AI Policy](https://legalmoveslawfirm.com/steam-ai-policy/)

## 진행 조건

1. 테스트하는 사람이 각 도구에 프롬프트를 고치지 않고 그대로 넣는다. 화면비는 프롬프트에 적힌 값으로 맞춘다.
2. 프롬프트당 4장을 뽑고, 재시도 없이 첫 묶음으로 채점한다. 도구별 버전과 품질 설정을 함께 기록한다.
3. 1라운드는 스타일 참조 없이 진행한다.
4. 2라운드는 T0 시안을 종류마다 세 장씩 묶어 스타일 참조로 걸고 T8의 새 피사체를 뽑는다. 시안은 Midjourney로 만들고 다른 도구가 화풍만 이어받아 새 모델을 만드는 방향을 사용자가 2026-10-31에 제안해, T1을 참조로 T2와 T3를 다시 뽑던 계획을 바꿨다. Midjourney는 `--sref <이미지 URL>`을 쓴다. GPT Image 2.5, Nano Banana Pro, FLUX 2는 이미지를 첨부하고 아래 문장을 프롬프트 끝에 붙인다. 모델링 기준 이미지는 고른 조명이 필요하므로 참조의 조명은 따라오지 않게 했다.

   ```
   Match only the rendering style, color palette and material texture of the attached images. Do not copy their subjects, designs, compositions or lighting.
   ```

5. 프롬프트에 작가 이름과 Orbis 이름을 넣지 않는다.
6. 프롬프트를 고쳐야 하면 기존 프롬프트를 바꾸지 않고 새 ID(예: T1b)로 추가한다. 고친 프롬프트로 뽑은 결과는 이전 결과와 비교할 수 없기 때문이다.

## 채점 기준

항목마다 1-5점을 준다. 비용·속도는 점수 대신 4장을 뽑는 데 든 크레딧과 시간을 적는다.

| 항목 | 보는 것 | 해당 테스트 |
|---|---|---|
| 지시 반영 | 프롬프트의 요소가 빠지거나 엉뚱한 요소가 추가되지 않았는가 | T0을 뺀 전부 |
| 서술 이해 | 한국어 서술문의 분위기와 디테일을 살렸는가 | T1 한국어 |
| 스타일 적합 | ArtDirection의 PBR 재질, 생활감, 큰 색면, 차가운 보스 공간 기준에 맞는가 | T1, T2, T3 |
| 톤 유지 | 피사체가 바뀌어도 참조와 같은 화풍, 색감, 질감인가 | T8 |
| 내용 번짐 | 참조의 피사체, 디자인, 구도가 따라오지 않았는가. 5점이 번짐 없음이다 | T8 |
| 입력 일치 | 뽑은 오브젝트의 디자인, 재질, 색이 입력과 같은가 | T12 |
| 편집 보존 | 지시한 한 가지만 바뀌었는가. T11은 입력의 방이 유지되는가 | T4, T9, T11 |
| 시점 일관성 | 시점끼리 디테일이 맞고, 뒷면을 지어내지 않고, 축이 정렬됐는가 | T5, T7, T10 |
| 모델링 활용도 | 형태가 모호하지 않고 Blender 배경 평면으로 바로 쓸 수 있는가 | T3, T5, T6, T7, T8a, T8b, T10, T12 |
| 실루엣 구분 | 실루엣만 보고 세 몹을 구분할 수 있는가 | T6 |
| 비용·속도 | 4장에 든 크레딧과 시간 | 전부 |
