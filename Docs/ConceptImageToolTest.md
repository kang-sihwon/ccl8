# 컨셉 이미지 도구 비교 테스트

시나리오와 배경 설명으로 컨셉 시안과 3D 모델링용 기준 이미지를 만들 AI 도구를 고르기 위한 비교 테스트다. 같은 프롬프트를 후보 도구마다 넣고 채점표로 비교한 뒤, 단계별로 쓸 도구를 정한다. 이 문서는 비교 대상, 진행 조건, 테스트 프롬프트, 채점 기준을 소유한다. 시각 기준은 [ArtDirection](ArtDirection.md)이, 에이전트 도구 조사는 [AgentToolingResearch](AgentToolingResearch.md)가 소유한다.

2026-10-30까지 T1 영어·한국어와 T2 프롬프트의 1라운드 일부, T3부터 T7까지의 1라운드를 실행했고 도구는 아직 정하지 않았다. T0은 2026-10-31에 실행했고 T8은 아직 실행하지 않았다. 결과는 이 문서의 "결과" 절에 날짜를 붙여 추가하고, 결과 이미지는 `ConceptImageToolTest/` 폴더에 테스트 ID별로 둔다.

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
4. 2라운드는 T0 시안 세 장을 묶어 스타일 참조로 걸고 T8의 새 피사체를 뽑는다. 시안은 Midjourney로 만들고 다른 도구가 화풍만 이어받아 새 모델을 만드는 방향을 사용자가 2026-10-31에 제안해, T1을 참조로 T2와 T3를 다시 뽑던 계획을 바꿨다. Midjourney는 `--sref <이미지 URL>`을 쓴다. GPT Image 2.5, Nano Banana Pro, FLUX 2는 이미지를 첨부하고 아래 문장을 프롬프트 끝에 붙인다. 모델링 기준 이미지는 고른 조명이 필요하므로 참조의 조명은 따라오지 않게 했다.

   ```
   Match only the rendering style, color palette and material texture of the attached images. Do not copy their subjects, designs, compositions or lighting.
   ```

5. 프롬프트에 작가 이름과 Orbis 이름을 넣지 않는다.
6. 프롬프트를 고쳐야 하면 기존 프롬프트를 바꾸지 않고 새 ID(예: T1b)로 추가한다. 고친 프롬프트로 뽑은 결과는 이전 결과와 비교할 수 없기 때문이다.

## 테스트 프롬프트

### T1. 마을 분위기

영어 키워드형과 한국어 서술형을 모두 넣는다. 한국어 서술형은 시나리오 문장을 이해하는 정도를 보려고 키워드를 나열하지 않았다.

```
A small hillside village at the edge of an old forest, late afternoon. Timber-framed houses with moss on slate roofs, worn stone paths, faded linen on laundry lines, a stone well in the central square, tools leaning against walls. Soft natural sunlight, light atmospheric haze in the distance, calm lived-in mood. Wide establishing shot at eye level. Game environment concept art, physically based materials, large readable color masses. No text, no foreground characters. 16:9.
```

```
오래된 숲 가장자리의 언덕에 작은 마을이 있다. 사람들은 대대로 나무를 베고 천을 짜며 살아왔고, 집집마다 손때 묻은 연장이 벽에 기대 있다. 이끼 낀 지붕 너머로 늦은 오후의 햇살이 비스듬히 들고, 멀리 숲은 옅은 안개에 잠겨 있다. 광장 한가운데의 돌우물은 마을에서 가장 오래된 것이다.
이 장면을 게임 환경 컨셉아트로 그려 줘. 눈높이의 넓은 풍경 샷, 16:9, 사실적인 재질, 큰 색면 위주의 읽기 쉬운 구성, 글자와 전경 인물 없이.
```

### T2. 보스 공간

T1과 같은 세계의 다른 분위기를 요구한다.

```
The same world as a calm forest village, but deep inside: a vast ruined stone hall eroded by time, collapsed pillars, roots breaking through cracked floor tiles. A single shaft of cold light falls from a broken ceiling; everything else is in deep shadow. Cold, desaturated palette, limited light sources, oppressive sense of scale. A wide circular open floor clear of debris for combat, with clearly readable floor edges. Game environment concept art, physically based materials. No text, no characters. 16:9.
```

### T3. 주인공 디자인

```
Full-body character concept of a lone traveler-warrior for a dark fantasy action RPG. Practical layered gear: padded gambeson, worn leather straps and belts, a short cloak, simple steel pauldron on one shoulder, a one-handed sword and a small round shield. Neutral standing pose, entire body visible from head to feet, no cropping. Plain light gray background, even soft lighting, strong clear silhouette, readable materials (cloth, leather, steel). No motion blur, no text. 2:3.
```

### T4. 부분 수정

T3 결과 한 장을 첨부하고 두 프롬프트를 따로 넣는다. Midjourney는 수정 방식이 다르므로 에디터 기능으로 할 수 있는 범위만 비교한다.

```
Keep the character exactly the same: face, body proportions, colors, materials, pose, background and lighting. Change only one thing: replace the short cloak with a hooded mantle that reaches the knees.
```

```
Keep everything identical. Change only the weapon: replace the sword with a long-handled single-bladed axe held in the same hand.
```

### T5. 3면도

T3나 T4 결과를 첨부한다.

```
Character turnaround model sheet of the character in the attached image. Four views arranged left to right on one sheet: front, three-quarter front, left side, back. Same scale in every view, head and feet aligned on the same horizontal lines. A-pose with arms about 45 degrees from the body. Orthographic camera with no perspective distortion. Flat even lighting, no cast shadows, plain white background. Keep every design detail consistent between views. No text labels. 16:9.
```

### T6. 일반 몹 실루엣

세 역할은 GameDesign의 역할 예시인 빠른 근접 압박, 느리지만 강한 공격, 거리 유지와 견제를 따랐다.

```
Lineup of three enemy creature designs for a dark fantasy action game, standing side by side at the same scale, with a plain human silhouette next to them for size reference:
1) a lean, fast, low-crouching melee hunter with long forelimbs;
2) a huge, slow, armored brute with one massive stone-like club arm;
3) a thin, tall, robed ranged attacker that keeps its distance, holding a long staff.
Each must be instantly distinguishable by silhouette alone. Neutral standing poses, plain gray background, even lighting, no text. 16:9.
```

### T7. 소품

이미지→3D 단계의 입력으로도 쓴다. 첫 프롬프트는 여러 시점을 한 장에 넣는 시트이고, 둘째는 3/4 시점 한 장이다. 이미지→3D 도구가 깨끗한 배경의 3/4 시점 한 장을 잘 받는다는 것은 조사 출처의 일반적인 안내이고 ccl8에서 확인하지 않았다.

```
Game prop concept sheet: an old village water well with a wooden roof frame, hemp rope and a wooden bucket, weathered stone ring, patches of moss, iron fittings. Orthographic front view, side view and top view on one sheet, same scale. Plain white background, flat lighting, no shadows, realistic proportions, clearly readable forms. No text. 16:9.
```

```
The same village water well, single object, three-quarter view from slightly above, centered, entire object visible, plain white background, soft even lighting, no shadows on the ground, no other objects. 1:1.
```

### T0. 시안 기준

Midjourney로 게임의 느낌을 정하는 시안을 만든다. 후보 A의 세계에 맞춰 배경, 캐릭터, 몬스터를 하나씩 뽑고, 사용자가 그림 느낌을 기준으로 종류마다 한 장을 고른다. 고른 세 장은 묶어서 T8의 참조로 쓴다.

- 이 단계는 도구 비교가 아니므로 점수를 매기지 않는다. 진행 조건 2의 재시도 금지도 적용하지 않는다.
- 시안의 분위기를 보려고 T3, T6과 달리 배경과 조명을 단색으로 제한하지 않는다.
- 사용자가 YouArt 이미지 생성 화면에서 실행한다. 설정은 프롬프트의 화면비, HD 끔, Raw 모드 켬, 품질 1, Stylize 100이다. 워크플로 노드를 쓰지 않는 이유는 남은 일 6에 있다.

T0a는 배경인 마을이다.

```
A small high-mountain village of herders and ice cutters in a long winter, late afternoon. Low stone-and-timber houses with heavy snow on slate roofs, wool blankets drying on wooden racks, stacks of firewood running low, sheep pens, ice saws leaning against walls, a frozen river beside the village. Far up the valley, a colossal ancient stone dam gate closes off the gorge. Soft low sun, thin chimney smoke, cold air with distant haze, quiet worn lived-in mood. Wide establishing shot at eye level. Game environment concept art, physically based materials, large readable color masses. No text, no foreground characters. 16:9.
```

T0b는 캐릭터인 주인공이다.

```
Full-body character concept of a lone ice cutter from a starving mountain village, a reluctant warrior in a dark fantasy action RPG. Practical layered winter gear: thick undyed wool tunic, patched sheepskin coat, worn leather straps, fur-lined hood pushed back, iron spikes strapped to the boots, a heavy ice hatchet in one hand and a small wooden shield bound with iron. Standing pose, entire body visible from head to feet, no cropping. Muted snowy background, strong clear silhouette, readable materials (wool, fleece, leather, iron). No text. 2:3.
```

T0c는 몬스터인 옛 수문 경비병이다. 느리고 강한 공격 역할이다. T6에서 Midjourney가 갑옷을 빠뜨렸으므로 갑옷을 앞에 적었다.

```
Full-body creature concept of an ancient dam gate guard in a dark fantasy action game, long dead yet still standing watch. Heavy corroded iron plate armor covers the whole body, and thick blue-white ice has grown over the armor like a second shell. A huge, slow figure with one arm fused into a massive frozen stone mace, and a hollow helmet with frost inside. Standing pose, entire body visible from head to feet, a small human figure beside it for scale. Muted frozen riverbed background, strong clear silhouette, readable materials (corroded metal, ice, stone). No text. 2:3.
```

### T8. 화풍 이어받기

T0에서 고른 세 장의 화풍과 질감만 참조해 다른 피사체를 그린다. 이 테스트가 2라운드다. 참조의 내용을 따라오지 않으면서 같은 게임의 그림처럼 보이는 도구를 찾는 것이 목표다. T8a와 T8b는 모델링 기준 이미지이므로 조명과 배경을 고정한다.

피사체가 다른 세 장을 묶으면 공통으로 남는 화풍이 더 잘 전달될 것으로 추정한다. 반대로 한 장의 특징이 다른 그림으로 번질 수도 있어, T8b는 경비병 한 장만 참조한 실행과 비교한다. 묶음을 세 장으로 정한 이유는 네 도구 중 FLUX 2 Max의 참조 한도가 3장으로 가장 작기 때문이다. YouArt 노드 설명의 한도는 GPT Image 2.5 16장, Nano Banana Pro 10장, Midjourney 스타일 참조 4장이다.

| ID | 참조 | 새 피사체 | 크기 |
|---|---|---|---|
| T8a | 세 장 묶음 | 마을 안내인인 늙은 양치기 | 2:3 |
| T8b | 세 장 묶음 | 거리를 유지하는 수로지기 | 2:3 |
| T8b 대조 | T0c에서 고른 경비병 한 장 | T8b와 같음 | 2:3 |
| T8c | 세 장 묶음 | 수문 내부의 보스 공간 | 16:9 |

- GPT Image 2.5, Nano Banana Pro, FLUX 2가 주 비교 대상이다. Midjourney는 `--sref`로 실행하는 대조군이다.
- 크레딧은 T3, T6 기록으로 계산해 2:3 실행이 각각 126, T8c가 118이다. 네 실행을 합치면 496이다. GPT Image 2.5는 참조 이미지 수에 따라 크레딧이 달라진다고 노드 설명에 적혀 있으며, 늘어나는 양은 확인하지 않았다.
- T8b 대조는 참조가 한 장이므로 위의 문장을 단수형으로 바꿔 쓴다. "attached image"와 "its subject, design, composition or lighting"으로 쓴다.
- Midjourney 스타일 참조를 이미지 생성 화면에서 쓸 수 있는지는 확인하지 않았다. 여러 장 참조도 어느 도구에서든 실행해 보지 않았다.

T8a는 안내인이다.

```
Full-body character reference of an elderly shepherd who serves as the guide of a snowbound mountain village, for a dark fantasy action RPG. Long felt coat with a sheepskin collar, wool scarf, worn leather satchel, tall wooden crook, small iron lantern at the belt. Neutral standing pose, entire body visible from head to feet, no cropping. Plain light gray background, even soft lighting, strong clear silhouette, readable materials (felt, fleece, leather, wood, iron). No text. 2:3.
```

T8b는 수로지기다. 체형이 T0c와 반대라 참조의 형태가 따라오는지 보기 쉽다.

```
Full-body creature reference of an ancient canal keeper in a dark fantasy action game, a ranged enemy that keeps its distance. Very tall and thin figure in a long frayed robe stiff with frost, narrow hooded head, no armor, coils of rusted chain at the waist, holding a long harpoon of clear ice at its side. Neutral standing pose, entire body visible from head to feet, no cropping, with a plain human silhouette beside it for scale. Plain gray background, even lighting, strong clear silhouette, readable materials (cloth, ice, rusted iron). No text. 2:3.
```

T8c는 보스 공간이다. 조명은 마을과 달라도 재질과 그림체가 같은 세계로 보이는지 본다.

```
Inside a colossal ancient stone dam gate: a vast round chamber of eroded masonry, giant bronze gears and chains locked in thick ice, a frozen waterfall against the far wall. A single shaft of cold light falls from a narrow opening above; everything else is in deep shadow. Cold, desaturated palette, limited light sources, oppressive sense of scale. A wide circular open floor of flat ice clear of debris for combat, with clearly readable floor edges. Game environment concept art, physically based materials. No text, no characters. 16:9.
```

### 이미지→3D 단계

테스트하는 사람이 T7의 두 결과와 T5 시트를 Hunyuan3D, Tripo, Meshy에 각각 넣는다. 원본 실루엣과 맞는지, 보이지 않던 뒷면을 그럴듯하게 만드는지, 폴리곤 수와 텍스처를 쓸 수 있는지 비교한다. 결과 메시는 최종 에셋이 아니라 블록아웃이나 스컬프트 기반으로 보고, 리토폴로지 등 후처리가 필요하다고 가정한다. 후처리에 쓸 Blender는 설치를 확인한 기록이 없다([AgentToolingResearch](AgentToolingResearch.md#반영-상태)).

## 채점 기준

항목마다 1-5점을 준다. 비용·속도는 점수 대신 4장을 뽑는 데 든 크레딧과 시간을 적는다.

| 항목 | 보는 것 | 해당 테스트 |
|---|---|---|
| 지시 반영 | 프롬프트의 요소가 빠지거나 엉뚱한 요소가 추가되지 않았는가 | T0을 뺀 전부 |
| 서술 이해 | 한국어 서술문의 분위기와 디테일을 살렸는가 | T1 한국어 |
| 스타일 적합 | ArtDirection의 PBR 재질, 생활감, 큰 색면, 차가운 보스 공간 기준에 맞는가 | T1, T2, T3 |
| 톤 유지 | 피사체가 바뀌어도 참조와 같은 화풍, 색감, 질감인가 | T8 |
| 내용 번짐 | 참조의 피사체, 디자인, 구도가 따라오지 않았는가. 5점이 번짐 없음이다 | T8 |
| 편집 보존 | 지시한 한 가지만 바뀌었는가 | T4 |
| 시점 일관성 | 시점끼리 디테일이 맞고, 뒷면을 지어내지 않고, 축이 정렬됐는가 | T5, T7 |
| 모델링 활용도 | 형태가 모호하지 않고 Blender 배경 평면으로 바로 쓸 수 있는가 | T3, T5, T6, T7, T8a, T8b |
| 실루엣 구분 | 실루엣만 보고 세 몹을 구분할 수 있는가 | T6 |
| 비용·속도 | 4장에 든 크레딧과 시간 | 전부 |

## 결과

### 2026-10-29 T1 영어 1라운드 중간 기록

도구별 최고점 이미지 기준으로 Nano Banana Pro가 세 항목 모두 가장 높았다. 다만 4장을 채운 도구는 Midjourney뿐이고 나머지는 1장이라 진행 조건 2를 채우지 못했다. 그래서 순위는 확정하지 않는다. 이미지는 사용자가 생성했고, AI가 이미지를 보고 채점했다.

- 프롬프트는 T1 영어이고, 스타일 참조와 재시도 없이 생성했다. T1 한국어 결과는 다음 절에 있다.
- 비교 대상 표의 GPT Image 2 자리는 GPT Image 2.5로 테스트했다. YouArt는 화면에 모델 이름을 표시했다. ChatGPT의 이미지 모델이 2.5라는 것은 같은 날 웹 조사로 추정한 것이다.
- 모델링 활용도는 채점 기준 표에서 T1 대상이 아니다. 건물과 우물을 블록아웃 기준으로 쓸 수 있는지 보려고 참고로 채점했다.
- 비용과 시간은 기록하지 않았다.
- 원본이 WebP인 이미지는 LFS 규칙에 맞추려고 화질 손실 없이 PNG로 바꿔 저장했다.

| 도구 | 경로와 설정 | 크기 | 지시 반영 | 스타일 적합 | 모델링 활용도(참고) | 그림 느낌(사용자 순위) | 이미지 |
|---|---|---|---|---|---|---|---|
| Nano Banana Pro | YouArt, 1K, 16:9 | 1376×768 | 5 | 5 | 5 | 4 | [보기](ConceptImageToolTest/T1/t1-en-youart-nanobananapro.jpg) |
| Gemini 웹 | 모드 "자동", "높음". 실제 모델은 미확인(Nano Banana 2 추정), 워터마크 있음 | 1024×572 | 4 | 5 | 4 | 3 | [보기](ConceptImageToolTest/T1/t1-en-gemini-auto.png) |
| Midjourney v8.2 1번 | YouArt, HD 끔, Stylize 100, Chaos·Weird·Experimental 0, Raw 모드 켬 | 1456×816 | 4 | 4 | 4 | 1 | [보기](ConceptImageToolTest/T1/t1-en-youart-mj82-1.png) |
| Midjourney v8.2 2번 | 같음 | 1456×816 | 3 | 3 | 3 | 1 | [보기](ConceptImageToolTest/T1/t1-en-youart-mj82-2.png) |
| Midjourney v8.2 3번 | 같음 | 1456×816 | 3 | 3 | 3 | 1 | [보기](ConceptImageToolTest/T1/t1-en-youart-mj82-3.png) |
| Midjourney v8.2 4번 | 같음 | 1456×816 | 3 | 4 | 4 | 1 | [보기](ConceptImageToolTest/T1/t1-en-youart-mj82-4.png) |
| FLUX 2 | YouArt, max | 1280×720 | 4 | 4 | 4 | 2 | [보기](ConceptImageToolTest/T1/t1-en-youart-flux2.jpg) |
| GPT Image 2.5 | YouArt, landscape_16_9, 품질 high | 1088×608 | 4 | 3 | 4 | 5 | [보기](ConceptImageToolTest/T1/t1-en-youart-gptimage25.png) |
| GPT Image 2.5 | ChatGPT, 채팅 모델 GPT-6.1 Tera Max | 1672×941 | 4 | 3 | 3 | 5 | [보기](ConceptImageToolTest/T1/t1-en-chatgpt-gpt61-teramax.png) |
| GPT Image 2.5 | ChatGPT, 채팅 모델 GPT-6 Astra High | 1672×941 | 4 | 3 | 4 | 5 | [보기](ConceptImageToolTest/T1/t1-en-chatgpt-gpt6-astrahigh.png) |

Midjourney 4장의 평균은 지시 반영 3.25, 스타일 적합 3.5, 모델링 활용도 3.5다. 그림 느낌은 사용자가 도구 단위로 매긴 순위이고 1이 가장 좋다. 나머지 점수와 따로 매겼다.

도구별 관찰은 다음과 같다.

- **Nano Banana Pro**: 프롬프트 요소가 모두 들어갔고 시키지 않은 요소가 없다. 벽에 기댄 도구가 가장 명확하고, 돌계단으로 언덕의 높이차를 보였다. 재질과 건물 구조가 사실적이라 블록아웃 기준으로 쓰기 쉽다. 화각이 좁아 마을 전경보다 골목 장면에 가깝고, 우물이 중앙에서 오른쪽으로 치우쳤다.
- **Gemini 웹**: 추가 랜드마크 없이 ArtDirection의 차분한 노출과 재질에 가깝다. 우물이 중앙에 있지 않고 시점이 높다.
- **Midjourney v8.2**: 4장 모두 눈높이이고 추가 랜드마크가 없다. 4장 모두 언덕을 무시하고 평지 마을을 그렸고, 소품이 빽빽해 큰 색면이 약하다. 2번은 역광과 대비가 강해 분위기가 무겁다. 3번은 숲 안쪽 마을처럼 보이고 건물 층 구조가 뒤틀렸다. 4번은 위아래에 검은 띠가 생겨 쓸 수 있는 면적이 줄었다. 장별 편차가 크다.
- **FLUX 2**: 우물이 전경 중앙에 가장 크게 보이고 계단식 언덕을 그렸다. 같은 삽을 벽마다 반복해 세웠고, CG 렌더처럼 정돈돼 생활감이 약하며 집과 문의 비례가 어색하다.
- **GPT Image 2.5**: 세 결과 모두 시키지 않은 언덕 위 성, 강이나 호수, 먼 산을 넣었다. ChatGPT의 두 채팅 모델과 YouArt에서 같았으므로 이미지 모델 자체의 습관으로 추정한다. YouArt 결과는 언덕을 가장 잘 살렸지만 채도 높은 동화풍이라 ArtDirection과 거리가 있다.

같은 날 확인한 YouArt 설정은 다음과 같다.

- Midjourney의 HD는 화면에 "네이티브 2K"로 표시되지만 2944×1648로 출력됐다. 다른 모델의 1K와 화소 수를 맞추려고 1라운드는 HD를 껐다.
- Nano Banana Pro의 1K 16:9 출력은 1376×768이다.
- GPT Image 2.5의 landscape_16_9, high 출력은 1088×608이다.
- 같은 영어 프롬프트로 HD를 켠 Midjourney 실행도 한 번 있었다. 4장 모두 2944×1648이고 채점하지 않았다.

### 2026-10-29 T1 한국어 1라운드 중간 기록

서술 이해는 Nano Banana Pro, GPT Image 2.5, FLUX 2가 모두 5점이고 Midjourney가 가장 낮았다. 반면 한국어 프롬프트에서는 세 도구의 그림체가 영어 때와 달라져 스타일 적합이 흔들렸다. 영어 T1과 같은 이유로 순위는 확정하지 않는다. 이미지는 사용자가 YouArt에서 생성했고, AI가 사용자의 YouArt 실행 기록에서 원본을 열어 채점했다. 이미지 파일은 저장소에 저장하지 않았다.

- GPT Image 2.5는 화면비 설정이 square_hd여서 1:1로 나왔다. 진행 조건 1을 어겼으므로 다른 도구와 같은 조건의 결과가 아니다.
- 모델링 활용도와 비용·시간은 영어 T1과 같은 방식으로 다뤘다.

| 도구 | 경로와 설정 | 크기 | 지시 반영 | 서술 이해 | 스타일 적합 | 모델링 활용도(참고) | 그림 느낌(사용자 순위) |
|---|---|---|---|---|---|---|---|
| Nano Banana Pro | YouArt, 16:9, 1K | 1376×768 | 4 | 5 | 4 | 4 | 3 |
| GPT Image 2.5 | YouArt, square_hd, 품질 high | 1024×1024 | 4 | 5 | 3 | 4 | 4 |
| FLUX 2 | YouArt, max | 1280×720 | 3 | 5 | 2 | 4 | 2 |
| Midjourney v8.2 1번 | YouArt, 16:9, HD 끔, Raw 모드 켬, 품질 1 | 1456×816 | 3 | 3 | 2 | 3 | 1 |
| Midjourney v8.2 2번 | 같음 | 1456×816 | 3 | 3 | 4 | 3 | 1 |
| Midjourney v8.2 3번 | 같음 | 1456×816 | 4 | 4 | 5 | 4 | 1 |
| Midjourney v8.2 4번 | 같음 | 1456×816 | 3 | 4 | 4 | 3 | 1 |

Midjourney 4장의 평균은 지시 반영 3.25, 서술 이해 3.5, 스타일 적합 3.75, 모델링 활용도 3.25다. 그림 느낌은 영어 T1과 같이 사용자가 도구 단위로 매긴 순위다.

도구별 관찰은 다음과 같다.

- **Nano Banana Pro**: "나무를 베고 천을 짜며"를 벽에 기댄 도끼·톱과 베틀로 그렸다. 언덕으로 올라가는 길, 이끼 지붕, 비스듬한 빛, 숲의 안개가 있고 이끼 낀 돌우물로 오래됨을 보였다. 영어 때의 사실적인 사진풍이 아니라 붓 자국이 보이는 컨셉 페인팅이 됐다. 우물은 광장 중앙이 아니라 길 오른쪽에 있다.
- **GPT Image 2.5**: 장작더미와 벽의 연장, 걸어 둔 직물로 서술을 살렸고 지붕 달린 우물이 광장 중앙에 있다. 성과 호수는 없지만 먼 산과 안개 낀 골짜기를 넣었고, 영어 때처럼 채도 높은 황금빛 동화풍이다.
- **FLUX 2**: 물레와 베틀을 여러 개 그렸고, 원형 돌바닥 한가운데에 낡은 돌우물을 두어 "가장 오래된 것"을 가장 분명하게 보였다. 그림체가 카툰풍 일러스트로 바뀌어 "사실적인 재질" 지시를 어겼다.
- **Midjourney v8.2**: 4장 중 천을 짜는 흔적을 그린 장은 없다. 1번은 한낮의 밝은 하늘과 애니메이션풍 그림체이고 우물이 작게 왼쪽에 있다. 2번은 사실적이지만 우물이 화면 아래에서 거의 잘렸다. 3번은 숲 가장자리, 안개, 비스듬한 빛, 이끼 지붕, 광장의 돌우물이 모두 있어 한국어 결과 중 ArtDirection에 가장 가깝다. 4번은 언덕 아래 골짜기로 이어지는 넓은 풍경이지만 시점이 높고 마을 규모가 크며 우물이 작다.

영어 T1과 비교하면 다음과 같다.

- "비교 대상"의 추정, 곧 대화형 모델이 한국어 서술을 Midjourney보다 잘 이해한다는 추정에 맞는 결과다. FLUX 2도 서술을 잘 살렸다. 한 번의 실행이라 확인으로 보지 않는다.
- 한국어의 "사실적인 재질"은 영어의 "physically based materials"만큼 그림체를 고정하지 못했다. Nano Banana Pro는 페인팅으로, FLUX 2는 카툰으로 바뀌었다.

### 2026-10-29 T2 1라운드 중간 기록

스타일 적합은 Midjourney가, 지시 반영과 모델링 활용도는 FLUX 2가 가장 높았다. T1 영어에서는 Nano Banana Pro가 세 항목 모두 가장 높았으므로 점수 경향은 다르다. 사용자의 그림 느낌 순위는 T1 영어와 같이 Midjourney, FLUX 2 순이었다. 4장을 채운 도구는 Midjourney뿐이라 순위는 확정하지 않는다. 이미지는 사용자가 YouArt에서 생성했고, AI가 사용자의 YouArt 실행 기록에서 원본을 열어 채점했다. 이미지 파일은 저장소에 저장하지 않았다.

- 프롬프트는 T2이고, 스타일 참조와 재시도 없이 생성했다. T2는 영어 프롬프트만 있다.
- Midjourney의 첫 실행은 Raw 모드가 꺼진 채로 돌았다. T1과 조건을 맞추려고 Raw 모드를 켜고 다시 생성했다. 진행 조건을 바로잡은 실행이므로 재시도로 보지 않는다. 표의 Midjourney는 다시 생성한 결과다.
- 모델링 활용도는 채점 기준 표에서 T2 대상이 아니다. 기둥과 바닥을 블록아웃 기준으로 쓸 수 있는지 보려고 참고로 채점했다. 비용과 시간은 기록하지 않았다.

| 도구 | 경로와 설정 | 크기 | 지시 반영 | 스타일 적합 | 모델링 활용도(참고) | 그림 느낌(사용자 순위) |
|---|---|---|---|---|---|---|
| FLUX 2 | YouArt, max | 1280×720 | 5 | 4 | 5 | 2 |
| GPT Image 2.5 | YouArt, landscape_16_9, 품질 high | 1088×608 | 4 | 4 | 4 | 3 |
| Nano Banana Pro | YouArt, 16:9, 1K | 1376×768 | 4 | 3 | 4 | 4 |
| Midjourney v8.2 1번 | YouArt, 16:9, HD 끔, Raw 모드 켬, 품질 1 | 1456×816 | 3 | 5 | 3 | 1 |
| Midjourney v8.2 2번 | 같음 | 1456×816 | 4 | 5 | 4 | 1 |
| Midjourney v8.2 3번 | 같음 | 1456×816 | 3 | 4 | 3 | 1 |
| Midjourney v8.2 4번 | 같음 | 1456×816 | 4 | 5 | 3 | 1 |

Midjourney 4장의 평균은 지시 반영 3.5, 스타일 적합 4.75, 모델링 활용도 3.25다. Raw 모드를 끈 첫 실행도 참고로 채점했고, 장별 점수는 4/5/3, 4/5/4, 3/4/3, 3/5/3으로 평균이 같았다.

도구별 관찰은 다음과 같다.

- **FLUX 2**: 빛줄기가 하나이고, 잔해 없는 원형 바닥의 경계선이 네 도구 중 가장 또렷하다. 뿌리, 무너진 기둥, 아치가 사실적인 재질로 정돈돼 있다. 규모감이 약하고, 빛이 원형 바닥이 아니라 뒤쪽 잔해에 떨어진다.
- **GPT Image 2.5**: 어둡고 규모감이 크다. 원형 바닥은 비었지만 젖은 반사 때문에 경계가 흐리다. 천장 구멍 너머에 시키지 않은 탑을 넣어, T1에서 본 랜드마크 추가 습관이 T2에서도 나왔다.
- **Nano Banana Pro**: 프롬프트 요소는 모두 있다. 그러나 영어 T1의 사실적인 렌더가 아니라 일러스트풍이 됐고, 청록색이라 채도가 기준보다 높다. 아치 복도처럼 보여 압도감이 약하다.
- **Midjourney v8.2**: 4장 모두 차가운 색, 빛줄기 하나, 큰 규모감이 있어 보스 공간 기준에 가장 가깝다. 원형 바닥은 화면 아래에 작게 깔리고, 3번은 바닥이 통로처럼 좁다. 지붕 덮인 홀보다 동굴이나 협곡처럼 보인다. Raw 모드를 끈 첫 실행은 청록 기운이 강했고, 켠 실행은 회청색이라 "낮은 채도"에 더 가깝다. 원형 바닥이 작은 문제는 두 실행이 같았다.

"같은 세계"라는 문장은 네 도구 모두 거의 반영하지 않았다. 이끼와 뿌리만 T1과 겹치고, T1 마을의 목조 골조와 슬레이트 지붕 같은 건축 요소는 이어지지 않았다. 세계 연결은 T1 결과를 스타일 참조로 거는 2라운드에서 다시 본다.

### 2026-10-29 T3 1라운드 기록

캐릭터 기준 이미지에서는 GPT Image 2.5가 지시 반영과 모델링 활용도 모두 가장 높았고, 사용자 순위도 1위였다. T1과 T2에서 시키지 않은 요소를 넣던 습관은 캐릭터 단독 컷에서 나오지 않았다. T3는 네 도구 모두 4장을 채워 진행 조건 2를 처음으로 지켰다.

- AI가 YouArt MCP로 새 프로젝트 `ConceptImageToolTest T3`를 만들고, 프롬프트를 고치지 않고 넣어 한 번에 실행했다. 이미지 파일은 저장소에 저장하지 않았다.
- FLUX 2는 YouArt에 2:3 크기가 없어 가장 가까운 3:4(768×1024)로 생성했다. 진행 조건 1에서 벗어났다.
- GPT Image 2.5도 2:3 프리셋이 없다. 그래서 Nano Banana Pro 1K와 화소 수가 비슷한 custom 832×1248로 정했다. variant는 기본값 flare다.
- 크레딧은 Midjourney 4장 묶음 10, Nano Banana Pro 40, GPT Image 2.5 20, FLUX 2 56으로 모두 126이다. 실행 시간은 도구별로 22-75초였다.

| 도구 | 경로와 설정 | 크기 | 지시 반영 | 스타일 적합 | 모델링 활용도 | 그림 느낌(사용자 순위) |
|---|---|---|---|---|---|---|
| GPT Image 2.5 1번 | YouArt, custom, 품질 high, 4장 | 832×1248 | 5 | 4 | 5 | 1 |
| GPT Image 2.5 2번 | 같음 | 832×1248 | 5 | 4 | 5 | 1 |
| GPT Image 2.5 3번 | 같음 | 832×1248 | 5 | 4 | 5 | 1 |
| GPT Image 2.5 4번 | 같음 | 832×1248 | 5 | 4 | 5 | 1 |
| Midjourney v8.2 1번 | YouArt, 2:3, HD 끔, Raw 모드 켬, 품질 1, Stylize 100 | 896×1344 | 4 | 4 | 3 | 2 |
| Midjourney v8.2 2번 | 같음 | 896×1344 | 4 | 4 | 4 | 2 |
| Midjourney v8.2 3번 | 같음 | 896×1344 | 4 | 4 | 4 | 2 |
| Midjourney v8.2 4번 | 같음 | 896×1344 | 3 | 4 | 3 | 2 |
| FLUX 2 1번 | YouArt, max, 노드 4개 | 768×1024 | 4 | 4 | 4 | 3 |
| FLUX 2 2번 | 같음 | 768×1024 | 3 | 4 | 4 | 3 |
| FLUX 2 3번 | 같음 | 768×1024 | 5 | 4 | 5 | 3 |
| FLUX 2 4번 | 같음 | 768×1024 | 4 | 4 | 4 | 3 |
| Nano Banana Pro 1번 | YouArt, 2:3, 1K, 4장 | 848×1264 | 4 | 5 | 5 | 4 |
| Nano Banana Pro 2번 | 같음 | 848×1264 | 3 | 4 | 4 | 4 |
| Nano Banana Pro 3번 | 같음 | 848×1264 | 4 | 4 | 5 | 4 |
| Nano Banana Pro 4번 | 같음 | 848×1264 | 3 | 5 | 4 | 4 |

평균은 다음과 같다. 순서는 지시 반영, 스타일 적합, 모델링 활용도다. GPT Image 2.5는 5, 4, 5이고 Nano Banana Pro는 3.5, 4.5, 4.5다. FLUX 2는 4, 4, 4.25이고 Midjourney는 3.75, 4, 3.5다. 순서 번호는 YouArt 결과 목록의 순서다.

도구별 관찰은 다음과 같다.

- **GPT Image 2.5**: 퀼팅 갬비슨, 교차 가죽끈, 짧은 망토, 한쪽 견갑, 한손검, 원형 방패, 밝은 회색 배경이 모두 있고 추가 요소가 없다. 4장이 거의 같은 인물이라 장별 편차가 가장 작다. 얼굴이 미남 주인공 쪽이고 다른 도구보다 덜 낡았다.
- **Midjourney v8.2**: 낡고 겹겹이 입은 생활감이 가장 강하다. 망토가 긴 코트처럼 내려오고 레이어가 뭉쳐 구조가 덜 읽힌다. 바닥에 방향성 그림자가 있어 고른 조명 지시와 다르다.
- **FLUX 2**: 페인팅 컨셉아트풍이고 요소가 모두 있다. 2번은 오른쪽 아래에 서명 같은 표시를 넣어 글자 금지 지시를 어겼다. 1번은 시키지 않은 후드를 씌웠고 4번은 바닥 그림자가 있다.
- **Nano Banana Pro**: 재질이 가장 사실적이지만 게임 컨셉아트보다 실사 의상 사진에 가깝다. 4장 모두 시키지 않은 후드를 씌웠고, 2번과 4번은 방패를 등에 메어 정면에서 잘 보이지 않는다. 3번만 일러스트풍이라 T2에서 본 그림체 흔들림이 다시 나왔다.

### 2026-10-29 T4 1라운드 기록

부분 수정에서는 GPT Image 2.5가 두 프롬프트의 8장 모두에서 지시한 한 가지만 바꿨다. Nano Banana Pro가 근소한 차이로 뒤를 이었고, FLUX 2는 지시하지 않은 부분까지 바꿔 편집 보존이 가장 낮았다.

- 입력은 사용자가 고른 T3 GPT Image 2.5 1번이다. AI가 YouArt MCP로 T3 프로젝트에 편집 노드를 추가했다. 그리고 T3 GPT 노드의 첫 출력을 연결해 실행했다. 실행 기록에서 입력 파일이 T3 1번 이미지인 것을 확인했다. 이미지 파일은 저장소에 저장하지 않았다.
- Midjourney는 빠졌다. YouArt의 Midjourney v8.2 노드는 편집 입력을 받지 않고, T4 절이 Midjourney는 에디터 기능으로 할 수 있는 범위만 비교하라고 정하기 때문이다.
- Nano Banana Pro와 FLUX 2는 edit 모드로, GPT Image 2.5는 이미지 입력으로 편집했다. 크기와 품질은 T3와 같다. FLUX 2는 3:4로만 출력돼 원본 2:3과 구도가 조금씩 달라졌다.
- 크레딧은 프롬프트마다 Nano Banana Pro 40, GPT Image 2.5 20, FLUX 2 56이다. 두 프롬프트를 합쳐 232이고, 전체 실행은 약 62초였다.
- 점수는 지시 반영과 편집 보존이다. 편집 보존은 지시한 한 가지 외에 얼굴, 자세, 장비, 배경이 그대로인지 본다. 지시를 따랐는지가 결과로 분명하게 드러나는 테스트라 사용자의 그림 느낌 순위는 매기지 않는다.

| 프롬프트 | 도구 | 1번 | 2번 | 3번 | 4번 | 평균 |
|---|---|---|---|---|---|---|
| 망토를 무릎까지 오는 후드 망토로 | GPT Image 2.5 | 5/5 | 5/5 | 5/5 | 5/5 | 5 / 5 |
| | Nano Banana Pro | 5/5 | 4/5 | 4/5 | 4/3 | 4.25 / 4.5 |
| | FLUX 2 | 5/4 | 4/3 | 4/4 | 3/2 | 4 / 3.25 |
| 검을 긴 자루 외날 도끼로 | GPT Image 2.5 | 5/5 | 5/5 | 5/5 | 5/5 | 5 / 5 |
| | Nano Banana Pro | 5/5 | 5/5 | 4/5 | 4/5 | 4.5 / 5 |
| | FLUX 2 | 4/3 | 5/3 | 4/2 | 4/3 | 4.25 / 2.75 |

도구별 관찰은 다음과 같다.

- **GPT Image 2.5**: 망토 프롬프트는 후드를 쓴 망토를 무릎까지 내렸고, 도끼 프롬프트는 같은 손에 긴 자루 외날 도끼를 쥐게 했다. 8장 모두 얼굴, 자세, 방패, 견갑, 배경이 원본과 같다.
- **Nano Banana Pro**: 보존은 GPT Image 2.5와 비슷하다. 망토 프롬프트의 2-4번은 후드를 내린 상태라 후드 망토로 덜 읽힌다. 4번은 반대쪽 어깨에 시키지 않은 견갑을 하나 더 달았다. 도끼 프롬프트의 3번과 4번은 자루가 조금 짧다.
- **FLUX 2**: 바꾸라는 부분은 바꿨지만 방패 색과 테두리가 달라졌다. 도끼 프롬프트의 3번은 원래 검을 등에 멘 채로 남겼다. 망토 프롬프트의 4번은 망토가 바닥에 끌릴 만큼 길어지고 견갑이 사라졌다.

### 2026-10-29 T5 1라운드 기록

3면도에서도 GPT Image 2.5가 세 항목 모두 가장 높았다. 그래서 캐릭터 단계인 T3, T4, T5에서 모두 1위다. 다만 팔을 45도로 벌린 A 포즈와 무기 표시는 지키지 못해, 3D로 넘기기 전에 보강이 필요하다. Midjourney는 입력 캐릭터를 유지하지 못해 가장 낮았다.

- 입력은 T4와 같은 T3 GPT Image 2.5 1번이다. AI가 YouArt MCP로 T3 프로젝트에 노드를 추가하고 T3 GPT 노드의 첫 출력을 연결했다. 이미지 파일은 저장소에 저장하지 않았다.
- Nano Banana Pro와 FLUX 2는 edit 모드로, GPT Image 2.5는 이미지 입력으로 생성했다. Midjourney는 입력을 이미지 프롬프트 칸에 연결했다. 크기는 Nano Banana Pro 1376×768, GPT Image 2.5 1088×608, FLUX 2 1280×720, Midjourney 1456×816이다.
- Midjourney는 같은 설정으로 두 번 "input.rejected"로 거부됐고, 세 번째에 생성됐다. 사용자는 웹에서도 Midjourney가 실패한 뒤 다시 실행하면 되는 일이 잦았다고 했다. 그래서 설정 문제가 아닌 일시적 거부로 보고, 재시도로 세지 않는다. 첫 실행 때 같은 묶음의 다른 노드도 함께 실패했지만, 실패한 노드의 크레딧은 모두 돌려받았다.
- 크레딧은 Nano Banana Pro 40, GPT Image 2.5 16, FLUX 2 56, Midjourney 10으로 모두 122다. 시간은 Midjourney를 뺀 실행이 약 53초, Midjourney가 2분 30초였다.
- 점수는 지시 반영, 시점 일관성, 모델링 활용도다. T4와 같은 이유로 사용자의 그림 느낌 순위는 매기지 않는다.

| 도구 | 1번 | 2번 | 3번 | 4번 | 평균 |
|---|---|---|---|---|---|
| GPT Image 2.5 | 4/4/5 | 4/3/4 | 4/4/5 | 4/4/5 | 4 / 3.75 / 4.75 |
| Nano Banana Pro | 3/2/4 | 3/4/4 | 3/3/4 | 3/3/4 | 3 / 3 / 4 |
| FLUX 2 | 3/2/3 | 2/2/2 | 2/2/3 | 3/3/4 | 2.5 / 2.25 / 3 |
| Midjourney v8.2 | 2/2/2 | 2/2/2 | 2/1/2 | 2/4/3 | 2 / 2.25 / 2.25 |

도구별 관찰은 다음과 같다.

- **GPT Image 2.5**: 4장 모두 정면, 3/4, 옆, 뒤 순서다. 흰 배경에서 머리와 발 높이가 맞다. 옆모습이 실제 왼쪽이라 왼쪽 어깨의 견갑이 보이고, 뒤에서도 같은 쪽에 있다. 원본의 재질과 얼굴이 그대로다. 팔은 45도가 아니라 거의 내렸다. 1번과 4번은 무기를 모두 뺐고, 2번은 검이 옆모습에만 있다.
- **Nano Banana Pro**: 실사 렌더를 플랫한 셀 셰이딩 선화로 바꿨다. A 포즈는 네 도구 중 가장 잘 지켰다. 4장 모두 옆모습이 오른쪽이다. 1번, 3번, 4번은 그 오른쪽 옆모습에 견갑이 있어 좌우가 뒤집혔다. 무기는 장마다 있거나 없다.
- **FLUX 2**: 3/4 대신 옆모습을 두 번 그리거나 정면을 두 번 그렸다. 3번은 A-E 글자 라벨을 넣었다. 2번과 3번은 카툰풍이 되고 색이 바뀌었으며 견갑이 양쪽에 생겼다.
- **Midjourney v8.2**: 네 시점을 늘어놓는 시트 구성은 깔끔하다. 하지만 4장 모두 입력과 다른 인물이고 견갑, 방패, 녹색 망토가 없다. 3번은 시점마다 성별이 바뀌었고, 2번은 채색과 선화가 섞였다. 이미지 프롬프트가 편집 입력이 아니라 느슨한 참고로 쓰이는 것으로 추정한다.

### 2026-10-29 T4·T5 Midjourney 입력 추가 기록

사용자 요청으로 T3 Midjourney v8.2 1번을 입력으로 T4와 T5를 다시 실행했다. 입력을 바꿔도 도구 순위는 같았고, GPT Image 2.5가 편집과 3면도 모두 가장 높았다. GPT Image 2.5는 Midjourney의 회화풍 재질과 조명을 유지했다. 그래서 Midjourney로 디자인하고 GPT Image 2.5로 수정과 3면도를 만드는 조합도 쓸 수 있다고 본다.

- 입력과 설정 외의 조건은 GPT 입력 실행과 같다. 실행 기록에서 입력 파일이 T3 Midjourney 1번인 것을 확인했다. T4는 이번에도 Midjourney를 뺐다. 이미지 파일은 저장소에 저장하지 않았다.
- Midjourney가 실패하면 같은 실행의 다른 노드도 함께 실패했다. 그래서 Midjourney만 따로 실행했고, 이번에는 한 번에 생성됐다.
- 크레딧은 T4 232, T5 122로 모두 354다. 실행 시간은 Midjourney를 뺀 실행이 약 58초, Midjourney가 약 1분 36초였다.

| 테스트 | 도구 | 1번 | 2번 | 3번 | 4번 | 평균 | GPT 입력 평균 |
|---|---|---|---|---|---|---|---|
| T4 망토 (지시/보존) | GPT Image 2.5 | 5/5 | 5/5 | 5/5 | 5/5 | 5 / 5 | 5 / 5 |
| | Nano Banana Pro | 4/4 | 4/4 | 4/5 | 4/4 | 4 / 4.25 | 4.25 / 4.5 |
| | FLUX 2 | 4/3 | 4/3 | 3/2 | 4/3 | 3.75 / 2.75 | 4 / 3.25 |
| T4 도끼 (지시/보존) | GPT Image 2.5 | 5/5 | 5/5 | 5/5 | 5/5 | 5 / 5 | 5 / 5 |
| | Nano Banana Pro | 5/5 | 5/5 | 4/5 | 5/5 | 4.75 / 5 | 4.5 / 5 |
| | FLUX 2 | 5/4 | 3/3 | 3/3 | 5/4 | 4 / 3.5 | 4.25 / 2.75 |
| T5 (지시/일관성/모델링) | GPT Image 2.5 | 4/3/4 | 4/3/4 | 4/3/4 | 4/3/4 | 4 / 3 / 4 | 4 / 3.75 / 4.75 |
| | Nano Banana Pro | 3/2/3 | 4/4/4 | 3/3/4 | 3/3/4 | 3.25 / 3 / 3.75 | 3 / 3 / 4 |
| | FLUX 2 | 2/2/3 | 3/3/4 | 2/2/3 | 2/2/3 | 2.25 / 2.25 / 3.25 | 2.5 / 2.25 / 3 |
| | Midjourney v8.2 | 2/3/2 | 2/2/2 | 2/4/2 | 2/3/2 | 2 / 3 / 2 | 2 / 2.25 / 2.25 |

도구별 관찰은 다음과 같다.

- **GPT Image 2.5**: T4 8장 모두 지시한 부분만 바꿨다. T5는 네 시점 모두 검과 방패가 일관되게 들어가 GPT 입력 때보다 무기 표시가 낫다. 하지만 원본에 없던 견갑을 반대쪽 어깨에 하나 더 달았고 옆모습이 오른쪽이다.
- **Nano Banana Pro**: T4 보존은 좋다. 다만 망토 프롬프트의 망토가 무릎이 아니라 발목까지 내려오고, 1번, 2번, 4번은 망토가 견갑을 덮었다. 도끼 프롬프트는 도끼를 대각선으로 들게 바꿨다. T5는 이번에도 셀 셰이딩으로 바꿨다. 2번은 무기 위치까지 시점마다 맞았다.
- **FLUX 2**: T4 망토 프롬프트는 원본 색을 올리브나 베이지로 바꿨다. 도끼 프롬프트는 인물을 잘 유지했지만 2번은 양손으로 가로로 들었고 3번은 거대한 장식 도끼가 됐다. T5는 3/4 대신 옆모습을 두 번 그리거나 카툰풍으로 바꿨다.
- **Midjourney v8.2**: T5에서 이번에도 입력과 다른 인물을 그렸다.

### 2026-10-29 T6 1라운드 기록

몹 실루엣에서도 GPT Image 2.5가 세 항목 모두 가장 높았고, 사용자 순위도 1위였다. 실루엣 구분은 Midjourney도 5점이었다. 하지만 Midjourney는 8장 모두 갑옷 없는 바위 거인을 그렸고 형태가 어두운 덩어리로 뭉쳐, 모델링 활용도가 가장 낮았다.

- Nano Banana Pro, GPT Image 2.5, FLUX 2는 YouArt 워크플로 `ConceptImageToolTest T3`의 T6 노드로 생성했다. 같은 워크플로의 `T6 MJ v8.2` 노드에는 결과가 없다. 이 노드는 MCP에서 Raw 모드를 켠 채 6번 `service.generation_failed`로 실패했고, 줄바꿈을 공백으로 바꾼 한 줄 프롬프트도 실패했다. 같은 설정의 짧은 점검 프롬프트는 생성됐다. 그래서 사용자가 YouArt 이미지 생성 화면에서 같은 프롬프트로 따로 실행했다.
- Midjourney 실행은 두 번이다. 22:58 실행은 Raw 모드가 꺼져 있고 23:47 실행은 켜져 있다. T2와 같은 이유로 Raw 모드를 켠 실행을 표에 넣고, 끈 실행은 참고로 채점했다. 두 실행 모두 한 번 실패한 뒤 생성됐으므로 Raw 모드가 실패 원인이라고 볼 근거는 없다. MCP 실행이 반복해서 실패한 원인은 확인하지 못했다.
- AI가 2026-10-30에 Claude in Chrome으로 YouArt 원본을 열어 채점했다. 이미지 생성 화면의 Midjourney 프롬프트가 T6 원문과 같은 것을 확인했다. 이미지 파일은 저장소에 저장하지 않았다.
- 크레딧은 Nano Banana Pro 40, GPT Image 2.5 12, FLUX 2 56(노드 4개), Midjourney 실행마다 10이다. Midjourney를 뺀 실행의 합계 108은 실행 기록에서 확인했다. 시간은 Midjourney를 뺀 실행이 약 51초, Midjourney Raw 모드 켬이 1분 17초, 끔이 2분 22초였다.
- 점수는 지시 반영, 실루엣 구분, 모델링 활용도다. 지시 반영은 세 역할의 특징, 갑옷과 바위 곤봉 팔, 사람 실루엣, 고른 조명, 추가 개체가 없는지를 본다.

| 도구 | 경로와 설정 | 크기 | 1번 | 2번 | 3번 | 4번 | 평균 | 그림 느낌(사용자 순위) |
|---|---|---|---|---|---|---|---|---|
| GPT Image 2.5 | 워크플로, 4장 | 1088×608 | 5/5/4 | 5/5/4 | 5/5/4 | 5/5/4 | 5 / 5 / 4 | 1 |
| Nano Banana Pro | 워크플로, 4장 | 1376×768 | 5/5/4 | 4/5/4 | 4/5/4 | 4/4/2 | 4.25 / 4.75 / 3.5 | 4 |
| FLUX 2 | 워크플로, 노드 4개 | 1280×720 | 3/5/2 | 4/5/4 | 3/5/4 | 3/4/4 | 3.25 / 4.75 / 3.5 | 3 |
| Midjourney v8.2 | 이미지 생성, 16:9, HD 끔, Raw 모드 켬, 품질 1 | 1456×816 | 3/5/2 | 4/5/3 | 4/5/3 | 4/5/3 | 3.75 / 5 / 2.75 | 2 |

Raw 모드를 끈 Midjourney 실행은 장별 점수가 3/4/2, 4/5/3, 4/5/2, 4/5/3이고 평균은 3.75, 4.75, 2.5다. 순서 번호는 YouArt 결과 목록의 순서다.

도구별 관찰은 다음과 같다.

- **GPT Image 2.5**: 4장 모두 낮게 웅크린 사냥꾼, 한쪽 팔이 바위 덩어리인 갑옷 거한, 긴 지팡이를 든 키 큰 로브 원거리형, 회색 사람 실루엣이 있다. 재질이 사실적이고 4장이 거의 같은 디자인이라 장별 편차가 가장 작다. 해진 천과 깃털 가장자리가 많아 리토폴로지 부담은 크다.
- **Nano Banana Pro**: 세 역할을 모두 지켰고 사람 실루엣과의 크기 비교도 맞다. 2번은 바위 팔과 별도로 반대 손에 곤봉을 더 들었다. 3번만 채색이고 거한이 갑옷 없는 바위 골렘이다. 4번은 내부 디테일 없는 평면 회색 실루엣이라 구분은 되지만 모델링 기준으로는 쓰기 어렵다.
- **FLUX 2**: 1번은 검은 실루엣만 그려 디자인 디테일이 없다. 1번, 3번, 4번은 원거리형의 지팡이 끝이 창날이라 근접형으로도 읽힌다. 4번은 원거리형의 키가 사람 실루엣과 비슷해 "키 큰" 특징이 약하다. 2번은 세 역할과 지팡이가 모두 맞다.
- **Midjourney v8.2**: 실루엣 대비가 가장 극적이고 크기 차이가 커서 한눈에 구분된다. 그러나 8장 모두 거한에 갑옷이 없고, 역광과 바닥 반사로 "고른 조명" 지시를 어겼다. Raw 모드를 켠 1번과 끈 1번은 시키지 않은 생물이나 인물을 하나 더 넣었다. 거한이 사람의 약 세 배라 같은 축척 안에서도 다른 몹이 작게 보인다.

### 2026-10-30 T7 1라운드 기록

소품에서도 GPT Image 2.5가 두 프롬프트 모두 가장 높았고, 사용자 순위도 1위였다. 3/4 시점 4장은 지시를 모두 지켰고, 따로 생성했는데도 시트와 같은 지붕, 도르래, 손잡이 구성이 나왔다. 다만 시트의 평면도는 4번만 위에서 똑바로 내려다봤다. Midjourney는 두 프롬프트 모두 가장 낮았다.

- AI가 YouArt MCP로 `ConceptImageToolTest T3` 워크플로에 T7 노드 14개를 추가했다. 프롬프트마다 Midjourney, Nano Banana Pro, GPT Image 2.5 노드 하나와 FLUX 2 노드 4개다. 설정은 T6 노드와 같고, 3/4 시점은 1:1 크기(GPT Image 2.5는 square_hd, FLUX 2는 1024×1024)로 맞췄다. 이미지 파일은 저장소에 저장하지 않았다.
- 둘째 프롬프트의 "The same village water well"에 맞춰 첫 결과를 첨부하라는 규정은 없다. 그래서 두 프롬프트를 이미지 입력 없이 따로 생성했다.
- 워크플로의 Midjourney 두 노드는 함께 한 번, 시트 노드만 한 번 더 실행했다. 세 번 모두 약 2초 만에 `service.generation_failed`로 실패했고 크레딧은 돌려받았다. 남은 일 6의 원인과 같은지는 확인하지 않았다. 그래서 사용자가 이미지 생성 화면에서 두 프롬프트로 Midjourney를 따로 실행했다. AI가 Claude in Chrome으로 결과를 열어, 프롬프트가 T7 원문과 같고 설정이 HD 끔, Raw 모드 켬, 품질 1인 것을 확인했다.
- Midjourney 실패가 다른 노드를 함께 멈추지 않도록 나머지 12개 노드를 따로 실행했다. 12개 모두 한 번에 생성됐고 실행은 약 63초였다.
- 크레딧은 프롬프트마다 Nano Banana Pro 40, FLUX 2 56(노드 4개), Midjourney 10이고, GPT Image 2.5는 시트 12, 3/4 시점 16이다. 모두 240이고 잔액 차이와 같다. 실행 뒤 잔액은 3,000이다. Midjourney 실행 시간은 기록하지 않았다.
- 시트의 점수는 지시 반영, 시점 일관성, 모델링 활용도다. 3/4 시점은 한 장짜리라 지시 반영과 모델링 활용도만 매긴다.
- 사용자의 그림 느낌 순위는 GPT Image 2.5, Nano Banana Pro, FLUX 2, Midjourney 순이다. 두 프롬프트를 묶어 도구 단위로 매겼다.

| 프롬프트 | 도구 | 크기 | 1번 | 2번 | 3번 | 4번 | 평균 |
|---|---|---|---|---|---|---|---|
| 시트 (지시/일관성/모델링) | GPT Image 2.5 | 1088×608 | 4/3/4 | 4/3/4 | 4/3/4 | 5/4/5 | 4.25 / 3.25 / 4.25 |
| | Nano Banana Pro | 1376×768 | 4/3/4 | 3/2/3 | 4/3/4 | 3/2/3 | 3.5 / 2.5 / 3.5 |
| | FLUX 2 | 1280×720 | 3/2/3 | 2/2/2 | 3/2/3 | 3/2/3 | 2.75 / 2 / 2.75 |
| | Midjourney v8.2 | 1456×816 | 2/2/2 | 2/2/2 | 3/3/3 | 2/2/2 | 2.25 / 2.25 / 2.25 |
| 3/4 시점 (지시/모델링) | GPT Image 2.5 | 1024×1024 | 5/5 | 5/5 | 5/5 | 5/5 | 5 / 5 |
| | FLUX 2 | 1024×1024 | 4/4 | 4/4 | 5/5 | 3/4 | 4 / 4.25 |
| | Nano Banana Pro | 1024×1024 | 3/3 | 4/4 | 4/4 | 4/4 | 3.75 / 3.75 |
| | Midjourney v8.2 | 1024×1024 | 3/2 | 2/1 | 1/1 | 2/1 | 2 / 1.25 |

순서 번호는 YouArt 결과 목록의 순서이고, FLUX 2는 노드 순서다.

도구별 관찰은 다음과 같다.

- **GPT Image 2.5**: 시트 4장 모두 정면, 옆, 위 순서다. 사실적인 재질에 바닥 그림자가 없고 글자도 없다. 정면의 맞배지붕 경사면과 옆면의 삼각 박공, 오른쪽 손잡이가 시점끼리 맞는다. 옆면에서 두 기둥이 겹쳐 하나로 보이는 것도 맞다. 1-3번의 평면도는 지붕 절반을 걷어 낸 비스듬한 부감이라 정투영이 아니다. 4번만 지붕이 우물을 덮은 평면도인데, 용마루 없이 판자만 그렸다. 3/4 시점 4장은 흰 배경, 그림자 없음, 화면 중앙, 전체 노출을 모두 지켰다. 이끼는 시트보다 적다.
- **Nano Banana Pro**: 시트 4장이 모두 선화 일러스트이고, T5에서 본 그림체 변화가 다시 나왔다. 4장 모두 평면도에서 지붕을 빼고 골조만 그렸다. 2번은 지붕널 없이 골조만 있고, 정면의 돌 고리가 원근으로 그려졌으며, 평면도의 두레박이 틀 밖에 있다. 4번은 정면에 지붕과 손잡이 대신 도르래가 있어 옆면과 구조가 다르다. 3/4 시점은 실사 렌더로 바뀌었다. 4장 모두 밝은 회색 배경에 옅은 접지 그림자가 있다. 1번은 시키지 않은 수동 펌프를 달았다.
- **FLUX 2**: 시트 4장이 모두 카툰풍이다. 1번은 옆면 대신 정면을 한 번 더 그렸고, 2번은 세 시점 모두 원근이 들어갔으며 두레박이 둘이다. 4장 모두 평면도에 지붕이 없고, 1번과 3번은 우물 안을 돌로 채웠다. 4번은 평면도에 골조까지 없다. 바닥에 풀과 돌조각을 넣은 장도 있다. 3/4 시점은 사실적인 렌더지만 1번, 2번, 4번은 지붕이 없고, 1번과 4번은 두레박이 금속 양동이다. 4번은 오른쪽에 그림자가 분명하다. 3번만 지붕, 나무 두레박, 쇠 장식이 모두 있다.
- **Midjourney v8.2**: 시트는 재질과 낡은 느낌이 가장 강하지만, 4장 모두 원근이 들어간 시점이다. 1번과 2번은 지붕 없는 돌 고리만 따로 그려 평면도를 대신했고, 3번과 4번은 비슷한 각도의 우물을 세 번 그렸다. 4장 모두 화면 가장자리에 알아볼 수 없는 글자가 있고, 바닥에 이끼, 물웅덩이, 그림자가 깔렸다. 2번은 왼쪽 아래에 제목처럼 보이는 큰 글자까지 넣었다. 3/4 시점은 4장 모두 지붕과 두레박이 없다. 1번만 도르래와 손잡이가 있고 2-4번은 원통형 돌이나 무너진 구덩이에 가깝다. 2번은 땅바닥이 화면 밖으로 잘렸고, 3번은 그림자가 뚜렷하다.

이미지→3D 단계의 입력 후보는 GPT Image 2.5의 시트 4번과 3/4 시점이다. 두 결과를 따로 생성했는데도 디자인이 거의 같아 한 소품의 입력으로 함께 쓸 수 있다.

### 남은 일

1. T1과 T2 모두 도구마다 4장을 채우고 버전, 품질 설정, 크레딧을 기록한다.
2. GPT Image 2.5의 T1 한국어를 16:9로 다시 생성한다. 진행 조건을 어긴 실행을 바로잡는 것이므로 재시도로 보지 않는다.
3. GPT Image 2.5에만 시키지 않은 요소를 빼는 T1b를 만들어 실행한다. T1b 프롬프트는 아직 만들지 않았다.
4. 사용자가 T0 결과에서 종류마다 참조 이미지를 한 장씩 고른 뒤 T8을 실행한다.
5. GPT Image 2.5 3면도의 A 포즈, 무기 표시, 견갑 위치를 보강하는 T5b를 만들지 정한다. T5b 프롬프트는 아직 만들지 않았다.
6. 워크플로의 `T6 MJ v8.2` 노드에 결과가 없는 원인을 확인한다. T7 Midjourney 노드도 `service.generation_failed`로 실패했지만, 같은 프롬프트가 이미지 생성 화면에서는 생성됐다. 원인을 알 때까지 Midjourney는 이미지 생성 화면에서 따로 실행한다.
7. GPT Image 2.5의 T7 시트 4번과 3/4 시점 결과로 이미지→3D 단계를 진행한다. 2026-10-30 T7 실행 뒤 YouArt 잔액은 3,000이다.
