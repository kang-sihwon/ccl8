# 1라운드 프롬프트

T1부터 T7과 이미지→3D 단계의 프롬프트 원문이다. 결과와 대조할 때 읽는다.

## T1. 마을 분위기

영어 키워드형과 한국어 서술형을 모두 넣는다. 한국어 서술형은 시나리오 문장을 이해하는 정도를 보려고 키워드를 나열하지 않았다.

```
A small hillside village at the edge of an old forest, late afternoon. Timber-framed houses with moss on slate roofs, worn stone paths, faded linen on laundry lines, a stone well in the central square, tools leaning against walls. Soft natural sunlight, light atmospheric haze in the distance, calm lived-in mood. Wide establishing shot at eye level. Game environment concept art, physically based materials, large readable color masses. No text, no foreground characters. 16:9.
```

```
오래된 숲 가장자리의 언덕에 작은 마을이 있다. 사람들은 대대로 나무를 베고 천을 짜며 살아왔고, 집집마다 손때 묻은 연장이 벽에 기대 있다. 이끼 낀 지붕 너머로 늦은 오후의 햇살이 비스듬히 들고, 멀리 숲은 옅은 안개에 잠겨 있다. 광장 한가운데의 돌우물은 마을에서 가장 오래된 것이다.
이 장면을 게임 환경 컨셉아트로 그려 줘. 눈높이의 넓은 풍경 샷, 16:9, 사실적인 재질, 큰 색면 위주의 읽기 쉬운 구성, 글자와 전경 인물 없이.
```

## T2. 보스 공간

T1과 같은 세계의 다른 분위기를 요구한다.

```
The same world as a calm forest village, but deep inside: a vast ruined stone hall eroded by time, collapsed pillars, roots breaking through cracked floor tiles. A single shaft of cold light falls from a broken ceiling; everything else is in deep shadow. Cold, desaturated palette, limited light sources, oppressive sense of scale. A wide circular open floor clear of debris for combat, with clearly readable floor edges. Game environment concept art, physically based materials. No text, no characters. 16:9.
```

## T3. 주인공 디자인

```
Full-body character concept of a lone traveler-warrior for a dark fantasy action RPG. Practical layered gear: padded gambeson, worn leather straps and belts, a short cloak, simple steel pauldron on one shoulder, a one-handed sword and a small round shield. Neutral standing pose, entire body visible from head to feet, no cropping. Plain light gray background, even soft lighting, strong clear silhouette, readable materials (cloth, leather, steel). No motion blur, no text. 2:3.
```

## T4. 부분 수정

T3 결과 한 장을 첨부하고 두 프롬프트를 따로 넣는다. Midjourney는 수정 방식이 다르므로 에디터 기능으로 할 수 있는 범위만 비교한다.

```
Keep the character exactly the same: face, body proportions, colors, materials, pose, background and lighting. Change only one thing: replace the short cloak with a hooded mantle that reaches the knees.
```

```
Keep everything identical. Change only the weapon: replace the sword with a long-handled single-bladed axe held in the same hand.
```

## T5. 3면도

T3나 T4 결과를 첨부한다.

```
Character turnaround model sheet of the character in the attached image. Four views arranged left to right on one sheet: front, three-quarter front, left side, back. Same scale in every view, head and feet aligned on the same horizontal lines. A-pose with arms about 45 degrees from the body. Orthographic camera with no perspective distortion. Flat even lighting, no cast shadows, plain white background. Keep every design detail consistent between views. No text labels. 16:9.
```

## T6. 일반 몹 실루엣

세 역할은 GameDesign의 역할 예시인 빠른 근접 압박, 느리지만 강한 공격, 거리 유지와 견제를 따랐다.

```
Lineup of three enemy creature designs for a dark fantasy action game, standing side by side at the same scale, with a plain human silhouette next to them for size reference:
1) a lean, fast, low-crouching melee hunter with long forelimbs;
2) a huge, slow, armored brute with one massive stone-like club arm;
3) a thin, tall, robed ranged attacker that keeps its distance, holding a long staff.
Each must be instantly distinguishable by silhouette alone. Neutral standing poses, plain gray background, even lighting, no text. 16:9.
```

## T7. 소품

이미지→3D 단계의 입력으로도 쓴다. 첫 프롬프트는 여러 시점을 한 장에 넣는 시트이고, 둘째는 3/4 시점 한 장이다. 이미지→3D 도구가 깨끗한 배경의 3/4 시점 한 장을 잘 받는다는 것은 조사 출처의 일반적인 안내이고 ccl8에서 확인하지 않았다.

```
Game prop concept sheet: an old village water well with a wooden roof frame, hemp rope and a wooden bucket, weathered stone ring, patches of moss, iron fittings. Orthographic front view, side view and top view on one sheet, same scale. Plain white background, flat lighting, no shadows, realistic proportions, clearly readable forms. No text. 16:9.
```

```
The same village water well, single object, three-quarter view from slightly above, centered, entire object visible, plain white background, soft even lighting, no shadows on the ground, no other objects. 1:1.
```

## 이미지→3D 단계

테스트하는 사람이 T7의 두 결과와 T5 시트를 Hunyuan3D, Tripo, Meshy에 각각 넣는다. 원본 실루엣과 맞는지, 보이지 않던 뒷면을 그럴듯하게 만드는지, 폴리곤 수와 텍스처를 쓸 수 있는지 비교한다. 결과 메시는 최종 에셋이 아니라 블록아웃이나 스컬프트 기반으로 보고, 리토폴로지 등 후처리가 필요하다고 가정한다. 후처리에 쓸 Blender는 설치를 확인한 기록이 없다([AgentToolingResearch](../../AgentToolingResearch.md#반영-상태)).
