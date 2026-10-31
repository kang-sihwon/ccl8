# 테스트 프롬프트

컨셉 이미지 도구 비교에 쓴 프롬프트 원문을 소유한다. 상위 문서는 [컨셉 이미지 도구 비교 테스트](../ConceptImageToolTest.md)다.

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

Midjourney로 게임의 느낌을 정하는 시안을 만든다. 후보 A의 세계에 맞춰 배경, 캐릭터, 몬스터를 하나씩 뽑고, 사용자가 그림 느낌을 기준으로 종류마다 세 장을 고른다. 고른 세 장 묶음은 T8에서 같은 종류의 새 피사체를 그리는 참조로 쓴다.

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

T0에서 종류마다 고른 세 장의 화풍과 질감만 참조해 같은 종류의 다른 피사체를 그린다. 이 테스트가 2라운드다. 참조의 내용을 따라오지 않으면서 같은 게임의 그림처럼 보이는 도구를 찾는 것이 목표다. T8a와 T8b는 모델링 기준 이미지이므로 조명과 배경을 고정한다.

사용자가 종류마다 세 장을 골라 같은 종류의 세 장을 묶어 참조한다. 같은 종류라 참조의 내용이 따라오는지 잘 드러난다. T8b는 한 장만 참조한 실행과 비교한다. 묶음을 세 장으로 정한 이유는 네 도구 중 FLUX 2 Max의 참조 한도가 3장으로 가장 작기 때문이다. YouArt 노드 설명의 한도는 GPT Image 2.5 16장, Nano Banana Pro 10장, Midjourney 스타일 참조 4장이다.

| ID | 참조 | 새 피사체 | 크기 |
|---|---|---|---|
| T8a | T0b 1, 2, 3번 | 마을 안내인인 늙은 양치기 | 2:3 |
| T8b | T0c 2, 3, 4번 | 거리를 유지하는 수로지기 | 2:3 |
| T8b 대조 | T0c 3번 | T8b와 같음 | 2:3 |
| T8c | T0a 1, 2, 4번 | 수문 내부의 보스 공간 | 16:9 |

- GPT Image 2.5, Nano Banana Pro, FLUX 2가 주 비교 대상이다. Midjourney는 `--sref`로 실행하는 대조군이다.
- T8b 대조는 참조가 한 장이므로 위의 문장을 단수형으로 바꿔 쓴다. "attached image"와 "its subject, design, composition or lighting"으로 쓴다.
- Midjourney 스타일 참조는 워크플로 노드의 `sref` 입력으로 실행한다. 이미지 생성 화면에서 쓸 수 있는지는 확인하지 않았다.
- YouArt 노드 하나에 여러 장을 넣을 때는 입력을 `edge-in`, `edge-in-2`, `edge-in-3` 순서로 연결한다.

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

### T9. 시안 부분 수정

T4의 조건을 T8 결과에 적용한다. 입력마다 두 프롬프트를 따로 넣는다. 입력은 T8a Midjourney 2번, T8b Midjourney 2번, T8c GPT Image 2.5 1번이다. T4와 같이 편집 입력이 없는 Midjourney는 뺐다.

T9a는 양치기다.

```
Keep the character exactly the same: face, body proportions, colors, materials, pose, background and lighting. Change only one thing: replace the sheepskin collar with a dark knitted wool hood resting on the shoulders.
```

```
Keep everything identical. Change only the staff: replace the wooden crook with a long walking spear with an iron tip, held in the same hand.
```

T9b는 수로지기다.

```
Keep the creature exactly the same: proportions, colors, materials, pose, background and lighting. Change only one thing: replace the rusted chain at the waist with a wide cracked leather belt.
```

```
Keep everything identical. Change only the weapon: replace the ice harpoon with a long-handled sickle of rusted iron held in the same hand.
```

T9c는 보스 공간이다.

```
Keep the scene exactly the same: camera, composition, architecture, colors, materials and lighting. Change only one thing: replace the frozen waterfall with a huge closed bronze gate.
```

```
Keep everything identical. Change only one thing: add one large cracked bronze gear leaning against the left wall, and keep the circular floor clear.
```

### T10. 시안 3면도

T9의 양치기와 수로지기 입력에 T5 프롬프트를 고치지 않고 넣는다. T10a가 양치기, T10b가 수로지기다.

### T11. 공간 시점 변경

T7은 소품 시트이므로, 보스 공간에서는 기존 이미지를 유지한 채 카메라를 돌릴 수 있는지 본다. 입력은 T9c와 같다. T11a는 여러 시점의 시트이고, T11b는 시점을 옮긴 한 장이다.

```
Environment layout sheet of the room in the attached image. Three views arranged left to right on one sheet: the same view as the attached image, the room seen from the left side, and the room seen from directly above. Keep the same room, architecture, materials, colors and lighting in every view. No text labels. 16:9.
```

```
Show the exact same room from a different camera angle: move the camera around the room so it looks from the right edge of the circular floor toward the left wall. Keep the same architecture, gears, chains, frozen waterfall, ice floor, materials, colors and lighting. Do not add new objects or characters. 16:9.
```

### T12. 오브젝트 추출

배경 시안에서 오브젝트를 하나씩 뽑아 단독 에셋 이미지로 만들 수 있는지 본다. 조건은 T7의 3/4 시점을 따른다. T12a부터 T12d까지의 입력은 T0a 1, 2, 4번이다. 마을 그림에 톱니가 없어 T12e의 입력은 T8c Midjourney 1번이다.

T12a는 집이다.

```
Extract one timber-and-stone village house with its snow-covered slate roof from the attached images as a single standalone game asset. Keep the same design, materials, colors and weathering as in the images. Single object only, three-quarter view from slightly above, centered, entire object visible, plain white background, soft even lighting, no shadows on the ground, no other objects, no text. 1:1.
```

T12b는 돌이다.

```
Extract one large snow-dusted boulder from the attached images as a single standalone game asset. Keep the same shape language, materials, colors and weathering as in the images. Single object only, three-quarter view from slightly above, centered, entire object visible, plain white background, soft even lighting, no shadows on the ground, no other objects, no text. 1:1.
```

T12c는 지형이다.

```
Extract one square patch of the snowy riverside ground from the attached images as a standalone terrain block, like a small diorama. Keep the same snow, frozen ground, stones, materials and colors as in the images. Single terrain block only, three-quarter view from slightly above, centered, entire block visible, plain white background, soft even lighting, no shadows on the ground, no buildings, no other objects, no text. 1:1.
```

T12d는 성벽이다.

```
Extract one section of the colossal ancient stone dam wall from the attached images as a single standalone game asset. Keep the same masonry, materials, colors and weathering as in the images. Single object only, three-quarter view from slightly above, centered, entire object visible, plain white background, soft even lighting, no shadows on the ground, no other objects, no text. 1:1.
```

T12e는 톱니다.

```
Extract the large rusted gear from the attached image as a single standalone game asset. Keep the same design, materials, colors and weathering as in the image. Single object only, three-quarter view from slightly above, centered, entire object visible, plain white background, soft even lighting, no shadows on the ground, no other objects, no text. 1:1.
```

### 이미지→3D 단계

테스트하는 사람이 T7의 두 결과와 T5 시트를 Hunyuan3D, Tripo, Meshy에 각각 넣는다. 원본 실루엣과 맞는지, 보이지 않던 뒷면을 그럴듯하게 만드는지, 폴리곤 수와 텍스처를 쓸 수 있는지 비교한다. 결과 메시는 최종 에셋이 아니라 블록아웃이나 스컬프트 기반으로 보고, 리토폴로지 등 후처리가 필요하다고 가정한다. 후처리에 쓸 Blender는 설치를 확인한 기록이 없다([AgentToolingResearch](../AgentToolingResearch.md#반영-상태)).
