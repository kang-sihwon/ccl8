# 컨셉 이미지 도구 비교 테스트

시나리오와 배경 설명으로 컨셉 시안과 3D 모델링용 기준 이미지를 만들 AI 도구를 고르기 위한 비교 테스트다. 같은 프롬프트를 후보 도구마다 넣고 채점표로 비교한 뒤, 단계별로 쓸 도구를 정한다. 이 문서는 비교 대상, 진행 조건, 테스트 프롬프트, 채점 기준을 소유한다. 시각 기준은 [ArtDirection](ArtDirection.md)이, 에이전트 도구 조사는 [AgentToolingResearch](AgentToolingResearch.md)가 소유한다.

2026-10-29까지 T1 영어·한국어 프롬프트의 1라운드 일부를 실행했고 도구는 아직 정하지 않았다. 결과는 이 문서의 "결과" 절에 날짜를 붙여 추가하고, 결과 이미지는 `ConceptImageToolTest/` 폴더에 테스트 ID별로 둔다.

## 피사체의 지위

프롬프트의 마을, 주인공, 몹, 소품은 도구를 비교하기 위한 임시값이다. 2026-10-28 기준 [세계관 정본](Narrative/World.md)과 [시나리오 정본](Narrative/Scenario.md)에는 확정한 설정이 없다. 그래서 피사체는 ArtDirection의 시각 기준 제안과 [GameDesign](GameDesign.md)의 일반 몹 역할 예시에만 맞췄다. 이 프롬프트나 결과 이미지를 세계 설정, 인물 디자인, 몹 외형의 근거로 쓰지 않는다.

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
4. 2라운드는 1라운드에서 점수가 가장 높은 T1 이미지를 스타일 참조로 걸고 T2와 T3를 다시 뽑는다. 지역과 피사체가 바뀌어도 같은 톤이 유지되는지 보기 위해서다. Midjourney는 `--sref <이미지 URL>`을 쓴다. GPT Image 2와 Nano Banana Pro는 이미지를 첨부하고 아래 문장을 프롬프트 끝에 붙인다.

   ```
   Match only the rendering style, palette and lighting quality of the attached image, not its content.
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

### 이미지→3D 단계

테스트하는 사람이 T7의 두 결과와 T5 시트를 Hunyuan3D, Tripo, Meshy에 각각 넣는다. 원본 실루엣과 맞는지, 보이지 않던 뒷면을 그럴듯하게 만드는지, 폴리곤 수와 텍스처를 쓸 수 있는지 비교한다. 결과 메시는 최종 에셋이 아니라 블록아웃이나 스컬프트 기반으로 보고, 리토폴로지 등 후처리가 필요하다고 가정한다. 후처리에 쓸 Blender는 설치를 확인한 기록이 없다([AgentToolingResearch](AgentToolingResearch.md#반영-상태)).

## 채점 기준

항목마다 1-5점을 준다. 비용·속도는 점수 대신 4장을 뽑는 데 든 크레딧과 시간을 적는다.

| 항목 | 보는 것 | 해당 테스트 |
|---|---|---|
| 지시 반영 | 프롬프트의 요소가 빠지거나 엉뚱한 요소가 추가되지 않았는가 | 전부 |
| 서술 이해 | 한국어 서술문의 분위기와 디테일을 살렸는가 | T1 한국어 |
| 스타일 적합 | ArtDirection의 PBR 재질, 생활감, 큰 색면, 차가운 보스 공간 기준에 맞는가 | T1, T2, T3 |
| 톤 유지 | 2라운드에서 지역과 피사체가 바뀌어도 같은 톤인가 | 2라운드 |
| 편집 보존 | 지시한 한 가지만 바뀌었는가 | T4 |
| 시점 일관성 | 시점끼리 디테일이 맞고, 뒷면을 지어내지 않고, 축이 정렬됐는가 | T5, T7 |
| 모델링 활용도 | 형태가 모호하지 않고 Blender 배경 평면으로 바로 쓸 수 있는가 | T3, T5, T6, T7 |
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

### 남은 일

1. 도구마다 4장을 채우고 버전, 품질 설정, 크레딧을 기록한다.
2. GPT Image 2.5의 T1 한국어를 16:9로 다시 생성한다. 진행 조건을 어긴 실행을 바로잡는 것이므로 재시도로 보지 않는다.
3. GPT Image 2.5에만 시키지 않은 요소를 빼는 T1b를 만들어 실행한다. T1b 프롬프트는 아직 만들지 않았다.
4. 2라운드 스타일 참조는 4장을 채운 뒤 정한다. 현재 후보는 영어 T1의 Nano Banana Pro 결과다.
