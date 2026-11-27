# 세브린 인물 시안 지시문

[세브린 인물 시안](README.md)을 만든 지시문 전체다. 같은 방식으로 다른 주민을 만들 때 이 문서를 틀로 쓴다.

## 실행 방법

- Codex CLI 0.172.0의 내장 이미지 도구 `image_gen.imagegen`을 쓴다. 에이전트 모델은 `gpt-6.2-astra`다.
- `codex exec --disable memories --skip-git-repo-check -s workspace-write -C <출력 폴더> -`로 실행하고, 아래 감싸는 지시와 블록 지시문을 표준 입력으로 넘긴다.
- 메모리를 끄는 이유: 켜 두면 개인 메모리의 "ccl8 인물 얼굴을 매력적으로" 선호가 들어가 얼굴이 비슷한 미남형으로 몰린다.
- 지시문을 그대로 넘기라고 지시하는 이유: 지시하지 않으면 에이전트가 지시문을 다시 써서 표현이 바뀐다.
- 실행 뒤 `~/.codex/sessions/`의 세션 기록에서 실제로 넘어간 `prompt`와 참조 경로를 대조한다.

감싸는 지시는 다음과 같다. `<참조 경로>`와 `<파일명>`만 바꾼다.

```text
Call your built-in image generation tool image_gen.imagegen exactly once (not a script, not an API call).
Pass the text between <<<PROMPT and PROMPT>>> as the `prompt` argument VERBATIM: do not rewrite, shorten, translate or add to it.
Pass referenced_image_paths = [<참조 경로>]. Those images show the same character; use them only for his identity (face, build, height, hair, costume). Do not copy their background, composition, rendering style, texture or grain.
Save the resulting image in the current working directory as <파일명>, overwriting if it exists. Do not create or modify any other files.
Reply only with the saved path and the image resolution.

<<<PROMPT
(아래 블록)
PROMPT>>>
```

## 순서

| 번호 | 파일 | 참조 | 내용 |
|---|---|---|---|
| R1 | r01-face.png | 이전 시안 P4(삭제됨) | 얼굴 기준, 단색 배경 |
| S1 | s01-body-ref.png | R1 | 전신 기준, 단색 배경 |
| S2 | s02-room-fullbody.png | R1, S1 | 깨끗한 방의 전신 스냅 |
| S3 | s03-ring-habit.png | R1 | 고리를 살피는 상반신 |

R1은 사용자가 고른 이전 시안의 외모를 단색 배경으로 옮긴 것이다. 원본 시안은 지웠으므로 이후에는 R1을 세브린 얼굴의 기준으로 쓴다.

## 지시문

### r01-face
```text
Clean identity reference photo of the same man as in the reference image. Keep his face, age, long grey hair tied back, short grey beard and stubble, eyebrows and smile lines exactly as they are.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. He wears the same ash-grey wool coat over a dark waistcoat and linen shirt.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### s01-body-ref
```text
Clean full-body identity reference photo of the same man as in the reference image. Keep his face, age (56), long grey hair tied low at the nape, short trimmed grey beard, and eyebrows whose tips turn white exactly as in the reference.
Body: 178 cm tall and 69 kg, lean and long-limbed. Gently sloping shoulders, long arms and long legs, slim torso. Realistic adult proportions: about 7.7 heads tall, the waist sitting slightly above the midpoint of his height, legs clearly longer than the torso. Hands with a noticeably fleshy pad at the base of the thumb.
Pose: standing naturally, front view, weight on both feet, arms relaxed at his sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to boots.
Costume: long ash-grey coat of smooth, softly worn wool reaching below the knees, open at the front, with one neatly stitched patch on the coat at ONE knee only; dark buttoned waistcoat; light linen shirt; dark trousers; a plain leather belt with a brass buckle; worn brown leather lace-up boots.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Fabric reads as soft wool at natural scale with gentle tonal gradients. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The man in the reference images: the first image fixes his face, hair and beard; the second image fixes his height, body proportions and costume. He is 178 cm tall and 69 kg, lean and long-limbed, with gently sloping shoulders, long arms and long legs; about 7.7 heads tall with the waist slightly above the midpoint of his height.
He stands in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: long ash-grey coat of smooth, softly worn wool reaching below the knees with one neatly stitched patch on the coat at ONE knee only, dark buttoned waistcoat, light linen shirt, dark trousers, leather belt with a brass buckle, worn brown lace-up boots.
He stands relaxed near the window, whole body visible from head to boots, one hand in his coat pocket, the other holding a mended leather strap loosely, glancing slightly to the side of the camera with a small natural smile that shows a slight gap between his upper front teeth.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the boots.
Image quality: clean and smooth. Gentle tonal gradients; wool, plaster and polished wood read as soft surfaces at natural scale. Natural skin in soft even light. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### s03-ring-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera indoors in the early evening. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The man in the reference image (same face, long grey hair tied low at the nape, short trimmed grey beard, eyebrows with white tips), 56 years old, lean with gently sloping shoulders and long arms.
He sits at a plain wooden table against a smooth pale lime-plastered wall in a clean, restored medieval room at a living-history museum. On the table: one small brass lamp with pale blue glass, lit, giving a soft cool glow, a folded linen cloth, and a worn leather bag strap. Nothing else, no clutter.
Action: he is fitting a small brass ring onto the leather strap. He holds the ring up between his fingertips and turns it toward the lamp, narrowing his eyes to see it better. The fleshy pad at the base of his thumb is visible.
Costume: ash-grey coat of smooth, softly worn wool over a dark buttoned waistcoat and a light linen shirt, sleeves pushed back a little.
Light: warm dim room light from a window at dusk plus the cool glow of the lamp on his face and hands; soft, no harsh shadows.
Framing: phone held at eye level about 1.2 meters away, three-quarter view of his face, hands and the ring clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; wool, plaster and wood read as soft surfaces. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```
