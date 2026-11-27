# 벨레스 주민 15명 시안 지시문

[세브린 인물 시안](../Sevrin/README.md)과 같은 방식으로 나머지 15명을 만든 지시문 전체다. 실행 방법과 감싸는 지시는 [세브린 지시문](../Sevrin/prompts.md#실행-방법)과 같다. 결과와 검토는 [README](README.md)에 있다.

## 세브린과 다른 점

세브린은 사용자가 고른 이전 시안의 외모에서 얼굴 기준을 옮겼다. 15명은 그런 원본이 없으므로 얼굴 기준(r01)을 인물표의 얼굴 설명만으로 참조 없이 만들었다. 이후 순서는 같다.

| 파일 | 참조 | 내용 |
|---|---|---|
| r01-face.png | 없음 | 얼굴 기준, 단색 배경 |
| s01-body-ref.png | r01 | 전신 기준, 단색 배경, 키·몸무게·머리 수 지시 |
| s02-room-fullbody.png | r01, s01 | 세브린 S2와 같은 깨끗한 방의 전신 스냅 |
| s03-habit.png | r01 | 인물표의 버릇을 보이는 장면, 잔무늬가 적은 장소 |

머리 수는 지시문에 키를 그대로 쓰면 반영되지 않으므로, 나이·키·체형에 맞춘 보통 비율을 적었다. 니모(9세)는 6등신, 성인은 6.9~7.8등신이다.

## 인물별 지시문

## 린 · 25세 · 나루지기

### rin/r01-face
```text
Clean identity reference photo of a real person: Rin, 25, a woman, ferry-keeper who loads riverboats. sun-tanned round cheeks, unusually straight dark eyebrows, black hair twisted up high on her head. Build: short waist, firm forearms and calves.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: short navy-blue over-jacket with a red waist sash whose two long ends hang behind her, plain dark linen trousers, worn leather ankle boots.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### rin/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Rin, 25, a woman, ferry-keeper who loads riverboats. Keep the face, age, hair and features exactly as in the reference: sun-tanned round cheeks, unusually straight dark eyebrows, black hair twisted up high on her head.
Body: 166 cm tall and 62 kg; short waist, firm forearms and calves. Realistic proportions for this age, height and weight: about 7.3 heads tall.
Pose: standing naturally, front view, knees slightly loose, weight shared evenly on both feet as if on a rocking boat, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: short navy-blue over-jacket with a red waist sash whose two long ends hang behind her, plain dark linen trousers, worn leather ankle boots.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### rin/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Rin, 25, a woman, ferry-keeper who loads riverboats, 166 cm tall and 62 kg; short waist, firm forearms and calves; about 7.3 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: short navy-blue over-jacket with a red waist sash whose two long ends hang behind her, plain dark linen trousers, worn leather ankle boots.
Pose: standing relaxed near the window, knees slightly loose, weight shared evenly on both feet as if on a rocking boat, whole body visible from head to feet, a coil of rope hung over one forearm, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### rin/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Rin, 25, a woman, ferry-keeper who loads riverboats; short waist, firm forearms and calves.
Setting: a clean, smooth stone quay beside a calm river with glassy water and a soft overcast morning sky, nothing else in frame, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: short navy-blue over-jacket with a red waist sash whose two long ends hang behind her, plain dark linen trousers, worn leather ankle boots.
Action: She stands at the edge of the quay seeing someone off: a coil of rope still hung over her forearm, she raises only that hand slightly in farewell, holding back a laugh so that one cheek rises first.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 오델 · 48세 · 공동 창고 보관인

### odel/r01-face
```text
Clean identity reference photo of a real person: Odel, 48, a man, keeper of the shared grain store. dark brown skin, short tightly curled hair, a broad square flat jaw, beard shaved very short; when he smiles a dimple appears in ONE cheek only. Build: deep barrel chest and some weight at the waist, a short neck so his head sits close to his shoulders.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: loose linen shirt with wide sleeves folded up short, dark wool waistcoat, plain trousers, several small wooden tally tags hanging from his belt, leather shoes.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### odel/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Odel, 48, a man, keeper of the shared grain store. Keep the face, age, hair and features exactly as in the reference: dark brown skin, short tightly curled hair, a broad square flat jaw, beard shaved very short; when he smiles a dimple appears in ONE cheek only.
Body: 171 cm tall and 84 kg; deep barrel chest and some weight at the waist, a short neck so his head sits close to his shoulders. Realistic proportions for this age, height and weight: about 7.3 heads tall.
Pose: standing naturally, front view, standing square and solid, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: loose linen shirt with wide sleeves folded up short, dark wool waistcoat, plain trousers, several small wooden tally tags hanging from his belt, leather shoes.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### odel/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Odel, 48, a man, keeper of the shared grain store, 171 cm tall and 84 kg; deep barrel chest and some weight at the waist, a short neck so his head sits close to his shoulders; about 7.3 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: loose linen shirt with wide sleeves folded up short, dark wool waistcoat, plain trousers, several small wooden tally tags hanging from his belt, leather shoes.
Pose: standing relaxed near the window, standing square and solid, whole body visible from head to feet, one hand resting on the wooden tally tags at his belt, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### odel/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Odel, 48, a man, keeper of the shared grain store; deep barrel chest and some weight at the waist, a short neck so his head sits close to his shoulders.
Setting: a clean whitewashed storeroom with a smooth plaster wall and a neat row of three clean, plump linen grain sacks on a smooth stone floor, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: loose linen shirt with wide sleeves folded up short, dark wool waistcoat, plain trousers, several small wooden tally tags hanging from his belt, leather shoes.
Action: While counting, he keeps his palm resting flat on the grain sack he has just touched and runs the fingers of his other hand over the wooden tally tags at his belt, smiling slightly so the single dimple shows.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 미라 · 34세 · 나무통 제작자

### mira/r01-face
```text
Clean identity reference photo of a real person: Mira, 34, a woman, cooper who also checks the village water station. copper-tinged auburn curly hair tied at the nape, a cluster of light freckles across the bridge of her nose; when she smiles her left canine slightly overlaps the tooth beside it. Build: short torso and strong, solid thighs.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: linen shirt with rolled sleeves, a heavy canvas work apron, brown trousers, a thin iron barrel hoop hanging from her belt, leather boots.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### mira/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Mira, 34, a woman, cooper who also checks the village water station. Keep the face, age, hair and features exactly as in the reference: copper-tinged auburn curly hair tied at the nape, a cluster of light freckles across the bridge of her nose; when she smiles her left canine slightly overlaps the tooth beside it.
Body: 160 cm tall and 60 kg; short torso and strong, solid thighs. Realistic proportions for this age, height and weight: about 7.2 heads tall.
Pose: standing naturally, front view, standing with weight on one leg, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: linen shirt with rolled sleeves, a heavy canvas work apron, brown trousers, a thin iron barrel hoop hanging from her belt, leather boots.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### mira/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Mira, 34, a woman, cooper who also checks the village water station, 160 cm tall and 60 kg; short torso and strong, solid thighs; about 7.2 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: linen shirt with rolled sleeves, a heavy canvas work apron, brown trousers, a thin iron barrel hoop hanging from her belt, leather boots.
Pose: standing relaxed near the window, standing with weight on one leg, whole body visible from head to feet, the thin iron hoop at her belt, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### mira/s03-habit
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Mira, 34, a woman, cooper who also checks the village water station; short torso and strong, solid thighs.
Setting: a clean courtyard corner with a smooth pale plaster wall and smooth flagstones, only the small barrel and the low block, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: linen shirt with rolled sleeves, a heavy canvas work apron, brown trousers, a thin iron barrel hoop hanging from her belt, leather boots.
Action: She inspects a small wooden water barrel standing slightly tilted on a low stand: one foot up on a low block, her elbow resting on that knee, and her head tilted the same way the barrel leans as her eyes follow its slant.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, the whole body visible from head to feet, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 하겐 · 51세 · 운송선 선장

### hagen/r01-face
```text
Clean identity reference photo of a real person: Hagen, 51, a man, captain of the cargo riverboat from Tarnel. brown hair swept straight back, turning white from the temples, thin round wire-rimmed spectacles, a long nose bridge, a deep philtrum, a moustache trimmed so it does not cover the upper lip. Build: a long neck and straight, level shoulders.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: short oiled-canvas cape over a dark wool coat, a white cloth wrapped only around his neck, dark trousers, sturdy boots.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### hagen/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Hagen, 51, a man, captain of the cargo riverboat from Tarnel. Keep the face, age, hair and features exactly as in the reference: brown hair swept straight back, turning white from the temples, thin round wire-rimmed spectacles, a long nose bridge, a deep philtrum, a moustache trimmed so it does not cover the upper lip.
Body: 175 cm tall and 72 kg; a long neck and straight, level shoulders. Realistic proportions for this age, height and weight: about 7.6 heads tall.
Pose: standing naturally, front view, upright, one foot set half a step behind the other, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: short oiled-canvas cape over a dark wool coat, a white cloth wrapped only around his neck, dark trousers, sturdy boots.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### hagen/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Hagen, 51, a man, captain of the cargo riverboat from Tarnel, 175 cm tall and 72 kg; a long neck and straight, level shoulders; about 7.6 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: short oiled-canvas cape over a dark wool coat, a white cloth wrapped only around his neck, dark trousers, sturdy boots.
Pose: standing relaxed near the window, upright, one foot set half a step behind the other, whole body visible from head to feet, nothing in his hands, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### hagen/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Hagen, 51, a man, captain of the cargo riverboat from Tarnel; a long neck and straight, level shoulders.
Setting: the freshly varnished, smooth clean deck of a small wooden riverboat on calm glassy water under a soft overcast sky, no clutter, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: short oiled-canvas cape over a dark wool coat, a white cloth wrapped only around his neck, dark trousers, sturdy boots.
Action: Standing on deck, he points with one arm to the spot where a passenger should place their luggage, while his torso stays facing the bow; one foot half a step behind the other.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 베사 · 43세 · 여관 주인

### besa/r01-face
```text
Clean identity reference photo of a real person: Besa, 43, a woman, keeper of the 'Wet Oar' inn. short black curly hair half covering her ears, full plump cheeks, eyes whose outer corners always curve upward. Build: rounded shoulders, wide hips, fleshy arms.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: long-sleeved linen dress in a muted color, a cream apron tied at the SIDE, a folded dish towel draped over her free arm, soft leather shoes.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### besa/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Besa, 43, a woman, keeper of the 'Wet Oar' inn. Keep the face, age, hair and features exactly as in the reference: short black curly hair half covering her ears, full plump cheeks, eyes whose outer corners always curve upward.
Body: 157 cm tall and 70 kg; rounded shoulders, wide hips, fleshy arms. Realistic proportions for this age, height and weight: about 7 heads tall.
Pose: standing naturally, front view, standing relaxed, turned a little sideways, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: long-sleeved linen dress in a muted color, a cream apron tied at the SIDE, a folded dish towel draped over her free arm, soft leather shoes.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### besa/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Besa, 43, a woman, keeper of the 'Wet Oar' inn, 157 cm tall and 70 kg; rounded shoulders, wide hips, fleshy arms; about 7 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: long-sleeved linen dress in a muted color, a cream apron tied at the SIDE, a folded dish towel draped over her free arm, soft leather shoes.
Pose: standing relaxed near the window, standing relaxed, turned a little sideways, whole body visible from head to feet, the folded dish towel over her arm, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### besa/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Besa, 43, a woman, keeper of the 'Wet Oar' inn; rounded shoulders, wide hips, fleshy arms.
Setting: a clean inn dining room with a smooth pale plaster wall and one plain wooden table, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: long-sleeved linen dress in a muted color, a cream apron tied at the SIDE, a folded dish towel draped over her free arm, soft leather shoes.
Action: She holds a small hot iron pot with a single handle close to her body and offers a ladle of stew toward the viewer, saying nothing, only raising her eyebrows while she waits for the answer.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 라우트 · 63세 · 촌장 겸 판자길 목수

### raut/r01-face
```text
Clean identity reference photo of a real person: Raut, 63, a man, village head who still repairs the plank boardwalks. hollow temples, thick bushy silver eyebrows over deep-set eyes, white hair swept back with the ends lifting behind his ears. Build: long legs with visibly bony knees and ankles, lean.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: short chestnut-brown wool vest over a linen shirt, dark trousers, worn boots.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### raut/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Raut, 63, a man, village head who still repairs the plank boardwalks. Keep the face, age, hair and features exactly as in the reference: hollow temples, thick bushy silver eyebrows over deep-set eyes, white hair swept back with the ends lifting behind his ears.
Body: 183 cm tall and 73 kg; long legs with visibly bony knees and ankles, lean. Realistic proportions for this age, height and weight: about 7.8 heads tall.
Pose: standing naturally, front view, tall and loose, long stride, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: short chestnut-brown wool vest over a linen shirt, dark trousers, worn boots.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### raut/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Raut, 63, a man, village head who still repairs the plank boardwalks, 183 cm tall and 73 kg; long legs with visibly bony knees and ankles, lean; about 7.8 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: short chestnut-brown wool vest over a linen shirt, dark trousers, worn boots.
Pose: standing relaxed near the window, tall and loose, long stride, whole body visible from head to feet, a plain wooden walking stick, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### raut/s03-habit
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Raut, 63, a man, village head who still repairs the plank boardwalks; long legs with visibly bony knees and ankles, lean.
Setting: a freshly built boardwalk of pale, smoothly planed new planks beside calm water under a soft overcast sky, nothing else in frame, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: short chestnut-brown wool vest over a linen shirt, dark trousers, worn boots.
Action: Stopping on a boardwalk, he holds his walking stick horizontally at waist height in both hands, gauging the width people will need to pass.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, the whole body visible from head to feet, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 타비 · 38세 · 대장장이

### tavi/r01-face
```text
Clean identity reference photo of a real person: Tavi, 38, a woman, blacksmith. dark brown skin, hair cropped very close to the head, a broad nose, a copper ring in her RIGHT earlobe. Build: broad shoulders and thick upper arms down to the elbows, very stable upper body.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: black leather apron over a sleeveless linen shirt, ochre trousers rolled up at the hems, leather boots.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### tavi/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Tavi, 38, a woman, blacksmith. Keep the face, age, hair and features exactly as in the reference: dark brown skin, hair cropped very close to the head, a broad nose, a copper ring in her RIGHT earlobe.
Body: 169 cm tall and 78 kg; broad shoulders and thick upper arms down to the elbows, very stable upper body. Realistic proportions for this age, height and weight: about 7.3 heads tall.
Pose: standing naturally, front view, feet set close together yet steady, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: black leather apron over a sleeveless linen shirt, ochre trousers rolled up at the hems, leather boots.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### tavi/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Tavi, 38, a woman, blacksmith, 169 cm tall and 78 kg; broad shoulders and thick upper arms down to the elbows, very stable upper body; about 7.3 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: black leather apron over a sleeveless linen shirt, ochre trousers rolled up at the hems, leather boots.
Pose: standing relaxed near the window, feet set close together yet steady, whole body visible from head to feet, a short forging hammer, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### tavi/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Tavi, 38, a woman, blacksmith; broad shoulders and thick upper arms down to the elbows, very stable upper body.
Setting: a clean, whitewashed forge corner with a smooth stone wall and a single anvil, no clutter, soft daylight, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: black leather apron over a sleeveless linen shirt, ochre trousers rolled up at the hems, leather boots.
Action: Lips pressed together, she raises a small test offcut of wood with a few nails driven into it up to eye level and studies how the nails sit, in three-quarter profile so the copper earring shows.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 에다 · 46세 · 배 목수

### eda/r01-face
```text
Clean identity reference photo of a real person: Eda, 46, a woman, boatwright. ash-blonde hair cut short at cheek height, blue eyes with fine sun creases at the corners, a narrow chin. Build: a long torso and a flat back.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: canvas work shirt with rolled sleeves, a leather work apron, sturdy trousers, boots.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### eda/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Eda, 46, a woman, boatwright. Keep the face, age, hair and features exactly as in the reference: ash-blonde hair cut short at cheek height, blue eyes with fine sun creases at the corners, a narrow chin.
Body: 173 cm tall and 67 kg; a long torso and a flat back. Realistic proportions for this age, height and weight: about 7.5 heads tall.
Pose: standing naturally, front view, one shoulder slightly forward, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: canvas work shirt with rolled sleeves, a leather work apron, sturdy trousers, boots.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### eda/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Eda, 46, a woman, boatwright, 173 cm tall and 67 kg; a long torso and a flat back; about 7.5 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: canvas work shirt with rolled sleeves, a leather work apron, sturdy trousers, boots.
Pose: standing relaxed near the window, one shoulder slightly forward, whole body visible from head to feet, a small wooden hand plane, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### eda/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Eda, 46, a woman, boatwright; a long torso and a flat back.
Setting: a bright, clean boat shed with a smooth plaster wall and a freshly built boat of pale, smoothly planed wood, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: canvas work shirt with rolled sleeves, a leather work apron, sturdy trousers, boots.
Action: Fitting a hatch cover on a boat, she leans in with one shoulder forward and her chin almost level with the edge of the board; as the cover closes she keeps her eyes on the hinge and brings one ear close to listen.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 울모 · 58세 · 강고기잡이

### ulmo/r01-face
```text
Clean identity reference photo of a real person: Ulmo, 58, a man, river fisherman. a flat nose bridge, thick dark eyelashes, a short grey-streaked chin beard; his eyes crinkle a beat before he smiles. Build: a stocky torso, hands broad for his size with thick knuckles.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: long leather waders pulled up to the thighs, linen shirt with sleeves rolled tight above the elbows, a willow basket hooked at his hip.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### ulmo/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Ulmo, 58, a man, river fisherman. Keep the face, age, hair and features exactly as in the reference: a flat nose bridge, thick dark eyelashes, a short grey-streaked chin beard; his eyes crinkle a beat before he smiles.
Body: 164 cm tall and 68 kg; a stocky torso, hands broad for his size with thick knuckles. Realistic proportions for this age, height and weight: about 7.1 heads tall.
Pose: standing naturally, front view, toes turned slightly outward, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: long leather waders pulled up to the thighs, linen shirt with sleeves rolled tight above the elbows, a willow basket hooked at his hip.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### ulmo/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Ulmo, 58, a man, river fisherman, 164 cm tall and 68 kg; a stocky torso, hands broad for his size with thick knuckles; about 7.1 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: long leather waders pulled up to the thighs, linen shirt with sleeves rolled tight above the elbows, a willow basket hooked at his hip.
Pose: standing relaxed near the window, toes turned slightly outward, whole body visible from head to feet, the willow basket at his hip, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### ulmo/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Ulmo, 58, a man, river fisherman; a stocky torso, hands broad for his size with thick knuckles.
Setting: the edge of a calm river with glassy smooth water and a soft pale dawn sky, nothing else in frame, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: long leather waders pulled up to the thighs, linen shirt with sleeves rolled tight above the elbows, a willow basket hooked at his hip.
Action: Holding up a wet fishing net in both hands, he spreads his broad fingers over the mesh to gauge its size, eyes crinkling.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 디나 · 39세 · 밭농사와 종자 갈무리

### dina/r01-face
```text
Clean identity reference photo of a real person: Dina, 39, a woman, field farmer who keeps the seed stock. angular cheekbones narrowing toward the cheeks; when she smiles her eyes become half-moons; black hair in a single braid laid forward over one shoulder. Build: thighs and calves evenly fleshed, firm ankles.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: linen blouse, a long work skirt with one side of the hem pinned up by a wooden clip, a rolled straw hat hanging on her back, leather boots.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### dina/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Dina, 39, a woman, field farmer who keeps the seed stock. Keep the face, age, hair and features exactly as in the reference: angular cheekbones narrowing toward the cheeks; when she smiles her eyes become half-moons; black hair in a single braid laid forward over one shoulder.
Body: 162 cm tall and 64 kg; thighs and calves evenly fleshed, firm ankles. Realistic proportions for this age, height and weight: about 7.1 heads tall.
Pose: standing naturally, front view, grounded, feet apart, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: linen blouse, a long work skirt with one side of the hem pinned up by a wooden clip, a rolled straw hat hanging on her back, leather boots.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### dina/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Dina, 39, a woman, field farmer who keeps the seed stock, 162 cm tall and 64 kg; thighs and calves evenly fleshed, firm ankles; about 7.1 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: linen blouse, a long work skirt with one side of the hem pinned up by a wooden clip, a rolled straw hat hanging on her back, leather boots.
Pose: standing relaxed near the window, grounded, feet apart, whole body visible from head to feet, a small linen seed bag, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### dina/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Dina, 39, a woman, field farmer who keeps the seed stock; thighs and calves evenly fleshed, firm ankles.
Setting: a clean seed room with a smooth pale plaster wall and a plain wooden table with two small linen seed bags, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: linen blouse, a long work skirt with one side of the hem pinned up by a wooden clip, a rolled straw hat hanging on her back, leather boots.
Action: She has spread seeds on her open palm and with her thumb pushes just a few of them aside, smiling with half-moon eyes.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 켈 · 28세 · 나루 하역꾼

### kel/r01-face
```text
Clean identity reference photo of a real person: Kel, 28, a man, landing porter. thick, strongly waved dark brown hair, a short beard, a vertical cleft in the middle of the chin that shows even in profile. Build: the tallest of the villagers, a very broad back, long from shoulders to waist, heavy and strong.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: a mustard-yellow vest padded on ONE shoulder only, linen shirt, dark trousers, boots.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### kel/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Kel, 28, a man, landing porter. Keep the face, age, hair and features exactly as in the reference: thick, strongly waved dark brown hair, a short beard, a vertical cleft in the middle of the chin that shows even in profile.
Body: 185 cm tall and 91 kg; the tallest of the villagers, a very broad back, long from shoulders to waist, heavy and strong. Realistic proportions for this age, height and weight: about 7.7 heads tall.
Pose: standing naturally, front view, standing tall and easy, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: a mustard-yellow vest padded on ONE shoulder only, linen shirt, dark trousers, boots.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### kel/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Kel, 28, a man, landing porter, 185 cm tall and 91 kg; the tallest of the villagers, a very broad back, long from shoulders to waist, heavy and strong; about 7.7 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: a mustard-yellow vest padded on ONE shoulder only, linen shirt, dark trousers, boots.
Pose: standing relaxed near the window, standing tall and easy, whole body visible from head to feet, nothing in his hands, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### kel/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Kel, 28, a man, landing porter; the tallest of the villagers, a very broad back, long from shoulders to waist, heavy and strong.
Setting: clean, smooth stone steps beside calm water under a soft overcast sky, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: a mustard-yellow vest padded on ONE shoulder only, linen shirt, dark trousers, boots.
Action: He carries one side of a full water barrel by its handle with his arms bent low, lowering his handle to match the height of the other carrier (only their hand on the other handle visible).
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 피아 · 17세 · 에테르 공방 견습

### pia/r01-face
```text
Clean identity reference photo of a real person: Pia, 17, a teenage boy, apprentice at the ether workshop. a short dark-brown fringe with straight eyebrows visible beneath it, large ears clearly showing. Build: narrow shoulders, thin wrists and ankles, still growing.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: a work apron far too wide for him, wrapped twice around his waist so spare cloth sticks out at both sides, linen shirt, dark trousers, a worn wooden ruler tucked into the back of the apron strings.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### pia/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Pia, 17, a teenage boy, apprentice at the ether workshop. Keep the face, age, hair and features exactly as in the reference: a short dark-brown fringe with straight eyebrows visible beneath it, large ears clearly showing.
Body: 165 cm tall and 53 kg; narrow shoulders, thin wrists and ankles, still growing. Realistic proportions for this age, height and weight: about 7.2 heads tall.
Pose: standing naturally, front view, slightly awkward, upright, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: a work apron far too wide for him, wrapped twice around his waist so spare cloth sticks out at both sides, linen shirt, dark trousers, a worn wooden ruler tucked into the back of the apron strings.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### pia/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Pia, 17, a teenage boy, apprentice at the ether workshop, 165 cm tall and 53 kg; narrow shoulders, thin wrists and ankles, still growing; about 7.2 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: a work apron far too wide for him, wrapped twice around his waist so spare cloth sticks out at both sides, linen shirt, dark trousers, a worn wooden ruler tucked into the back of the apron strings.
Pose: standing relaxed near the window, slightly awkward, upright, whole body visible from head to feet, the worn wooden ruler, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### pia/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Pia, 17, a teenage boy, apprentice at the ether workshop; narrow shoulders, thin wrists and ankles, still growing.
Setting: a clean workshop corner with a smooth pale plaster wall and a plain wooden table with a sheet of paper and the ruler, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: a work apron far too wide for him, wrapped twice around his waist so spare cloth sticks out at both sides, linen shirt, dark trousers, a worn wooden ruler tucked into the back of the apron strings.
Action: Copying the marks of a worn wooden ruler onto paper at a table, mouth pressed shut, only his eyes moving between the ruler and the paper.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 니모 · 9세 · 창고 곁집 아이

### nimo/r01-face
```text
Clean identity reference photo of a real person: Nimo, a 9-year-old boy, the storekeeper's grandson. light-brown curly hair falling onto his forehead; one front tooth missing so a gap shows when he smiles. Build: round cheeks, small shoulders, short fingers, a normal healthy child.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: a lime-green over-jacket with big pockets, short trousers, wool socks with only ONE sock dyed a different color, small leather shoes.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### nimo/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Nimo, a 9-year-old boy, the storekeeper's grandson. Keep the face, age, hair and features exactly as in the reference: light-brown curly hair falling onto his forehead; one front tooth missing so a gap shows when he smiles.
Body: 132 cm tall and 29 kg; round cheeks, small shoulders, short fingers, a normal healthy child. Realistic proportions for this age, height and weight: about 6 heads tall.
Pose: standing naturally, front view, back against a pillar, heels lifted without noticing, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: a lime-green over-jacket with big pockets, short trousers, wool socks with only ONE sock dyed a different color, small leather shoes.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### nimo/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Nimo, a 9-year-old boy, the storekeeper's grandson, 132 cm tall and 29 kg; round cheeks, small shoulders, short fingers, a normal healthy child; about 6 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: a lime-green over-jacket with big pockets, short trousers, wool socks with only ONE sock dyed a different color, small leather shoes.
Pose: standing relaxed near the window, back against a pillar, heels lifted without noticing, whole body visible from head to feet, a small carved wooden toy boat held in both hands, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### nimo/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Nimo, a 9-year-old boy, the storekeeper's grandson; round cheeks, small shoulders, short fingers, a normal healthy child.
Setting: a clean storeroom with a smooth plaster wall and a smooth wooden pillar, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: a lime-green over-jacket with big pockets, short trousers, wool socks with only ONE sock dyed a different color, small leather shoes.
Action: He imitates a ship's horn: upper lip lifted so the gap of his missing front tooth shows, puffing air through it, holding a small carved wooden toy boat by its hull in both hands.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 사엘 · 32세 · 돛감·멜빵 수선 장인

### sael/r01-face
```text
Clean identity reference photo of a real person: Sael, 32, a woman, sailcloth and harness mender. a thin long face, ONE eyelid with a deep fold while the other is single, black hair wound up at the nape. Build: a long neck and long fingers, narrow shoulders, slender.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: a dress with deep purple sleeves, a needle case with threaded needles pinned on one sleeve, a plain linen apron.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### sael/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Sael, 32, a woman, sailcloth and harness mender. Keep the face, age, hair and features exactly as in the reference: a thin long face, ONE eyelid with a deep fold while the other is single, black hair wound up at the nape.
Body: 168 cm tall and 56 kg; a long neck and long fingers, narrow shoulders, slender. Realistic proportions for this age, height and weight: about 7.6 heads tall.
Pose: standing naturally, front view, composed, elbows close to her body, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: a dress with deep purple sleeves, a needle case with threaded needles pinned on one sleeve, a plain linen apron.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### sael/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Sael, 32, a woman, sailcloth and harness mender, 168 cm tall and 56 kg; a long neck and long fingers, narrow shoulders, slender; about 7.6 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: a dress with deep purple sleeves, a needle case with threaded needles pinned on one sleeve, a plain linen apron.
Pose: standing relaxed near the window, composed, elbows close to her body, whole body visible from head to feet, a folded piece of pale sailcloth, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### sael/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Sael, 32, a woman, sailcloth and harness mender; a long neck and long fingers, narrow shoulders, slender.
Setting: a clean room with a smooth pale plaster wall beside a window with soft daylight, a plain wooden chair, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: a dress with deep purple sleeves, a needle case with threaded needles pinned on one sleeve, a plain linen apron.
Action: Seated with a pale sailcloth spread over her knees, elbows tucked against her body, she turns her wrist to look at the inside of a seam, head bowed; the asymmetric eyelids still show.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```

## 토르마 · 72세 · 은퇴한 옹기장이

### torma/r01-face
```text
Clean identity reference photo of a real person: Torma, 72, an old woman, retired potter. an indigo headscarf over a round forehead with wisps of silver hair, thin lips with vertical wrinkles around the mouth, lively old eyes. Build: short, with plump forearms and palms.
Head-and-shoulders portrait, front view, looking straight into the camera, neutral relaxed expression with the mouth closed. An ordinary, non-model face with natural asymmetry. Clothing visible at the shoulders: a dark wool dress with sleeves pushed up, a clay-stained linen apron, the indigo headscarf, soft leather shoes.
Plain light-grey seamless studio backdrop. Soft, even front light, no strong shadows, neutral white balance. Natural real skin, soft natural processing, no HDR, no sharpening, no grain, no beauty retouching.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### torma/s01-body-ref
```text
Clean full-body identity reference photo of the same person as in the reference image: Torma, 72, an old woman, retired potter. Keep the face, age, hair and features exactly as in the reference: an indigo headscarf over a round forehead with wisps of silver hair, thin lips with vertical wrinkles around the mouth, lively old eyes.
Body: 152 cm tall and 54 kg; short, with plump forearms and palms. Realistic proportions for this age, height and weight: about 6.9 heads tall.
Pose: standing naturally, front view, back held perfectly straight, one hand behind her back, arms relaxed at the sides, looking at the camera with a neutral expression and the mouth closed. Whole body visible from head to feet.
Costume: a dark wool dress with sleeves pushed up, a clay-stained linen apron, the indigo headscarf, soft leather shoes.
Plain light-grey seamless studio backdrop and floor. Soft, even light, no strong shadows, neutral white balance. Long lens at waist height, minimal perspective distortion.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Photorealistic photograph, not a painting, not CGI. Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### torma/s02-room-fullbody
```text
Casual smartphone photo, a candid full-body snapshot taken by a friend with a modern phone camera on a bright day. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference images: the first image fixes the face, hair and features; the second image fixes the height, body proportions and costume. Torma, 72, an old woman, retired potter, 152 cm tall and 54 kg; short, with plump forearms and palms; about 6.9 heads tall.
They stand in the great chamber of a carefully restored medieval manor house at a living-history museum. The room is clean and calm: smooth pale lime-plastered walls with no cracks or stains, wide polished oak floorboards with a soft sheen, a tall arched window letting in diffuse daylight, a plain wooden door about 2 meters high in the side wall for scale, and only one carved wooden chair and a simple table with a plain linen cloth. Large empty wall areas, no tapestries, no clutter, no small objects. Everything in frame is pre-industrial: no modern objects, no visitors, no signs.
Costume: a dark wool dress with sleeves pushed up, a clay-stained linen apron, the indigo headscarf, soft leather shoes.
Pose: standing relaxed near the window, back held perfectly straight, one hand behind her back, whole body visible from head to feet, a small clay bowl, glancing slightly to the side of the camera with a small natural expression.
Light: soft, even daylight from the tall window, gentle falloff across the plaster wall, no harsh shadows.
Framing: phone held at chest height about three meters away, full body centered with a little space above the head and below the feet.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Portrait 2:3 (vertical). No text, letters, logo, watermark or border.
```

### torma/s03-habit
```text
Casual smartphone photo, a candid waist-up snapshot taken by a friend with a modern phone camera. Soft natural phone processing, no HDR, no sharpening, true-to-life colors, no cinematic color grading. Not a painting, not CGI, not a game render.
The person in the reference image (same face, hair and features): Torma, 72, an old woman, retired potter; short, with plump forearms and palms.
Setting: a clean room with a smooth pale plaster wall and a smooth stone floor, only the low stool, at a living-history museum. Everything in frame is pre-industrial: no modern objects, no visitors, no signs, no clutter.
Costume: a dark wool dress with sleeves pushed up, a clay-stained linen apron, the indigo headscarf, soft leather shoes.
Action: Seated very upright on a low stool, she holds a small clay bowl in both hands and runs both thumbs around its rim in opposite directions.
Light: soft, even natural daylight, no harsh shadows.
Framing: phone held at eye level, waist-up, face, hands and the action clearly visible.
Image quality: clean and smooth. Gentle tonal gradients; fabric, plaster, wood and skin read as soft surfaces at natural scale. Natural skin. No crunchy micro-texture, no embossed or patterned fabric, no grain, no noise, no HDR, no oversharpening or edge halos.
Landscape 3:2. No text, letters, logo, watermark or border.
```
