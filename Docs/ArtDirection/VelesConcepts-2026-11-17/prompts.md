# Veles concept prompts — 2026-11-17

Built-in image_gen. Three independent generation calls. No reference image or edit input. Candidate artwork, not an approved direction.

## Common prompt

```text
Use case: stylized-concept.
Asset type: exploratory environment art direction keyframe for CCL8, Ether: Dawn of the Taiga.
Create ONE landscape 16:9 environment image, not a collage, no text or labels. A small inhabited riverside craft village called Veles at the edge of a golden taiga. Show workshop, river landing, and forest entrance within the same frame. Consistent camera for comparison: wide 28mm lens, elevated human overlook, looking diagonally across a village terrace. A modest open-front timber repair workshop occupies the left third on a higher land terrace. Broad wooden walkways and practical stairs descend across the center to a small sheltered quay and one small wooden river boat in the right lower third. The substantial main river continues to the right, calm water at the mooring, no waterfall or rapids around the boat. A clearly readable path climbs toward the golden conifer forest behind the workshop in the upper center. A small separate spring-fed wooden channel follows the forest path down to a modest stone communal basin on the upper terrace; do not connect the drinking-water channel directly to the navigable river. Houses on sturdy timber posts, red roofs, teal shutters, one mustard canvas awning. Small-scale crafted woodwork, tool rack, bench, anvil, a few ropes and barrels express daily repair work without clutter. A couple of small anonymous adult residents for scale, about 1.7m tall, doorways about 2m; no character design focus. Golden conifer needles, restrained patches of red moss and a little localized frost only in shaded forest rocks, never a snowy landscape. Forested low ridges behind the village hide the world tree completely. Clear navigable ground and readable depth layers.
Same soft late-morning natural light in each variant, warm sunlight and cool open shadows, no sunset, no excessive bloom. Keep the scene welcoming but credible for a mature fantasy action RPG. Materials must remain recognizable as wood, stone, cloth and iron. No castles, palaces, steampunk machinery, exposed modern pipes, modern industrial products, colossal trees in view, glowing gold surfaces, magical glyphs, huge dock ships, UI, captions, logos, watermark. This is a visual candidate, not a construction drawing or finalized map.
```

## A — 현실감 중심

Append to the common prompt:

```text
Art direction A: grounded realistic fantasy. Natural architectural proportions, straight load-bearing timber beams, restrained roof shapes, plausible hand-built joinery. Fine physically plausible grain, stone fracture and woven canvas texture; varied roughness and localized wear, no uniform grime. Subdued warm timber and stone dominate 70 percent, restrained muted brick red roofs and deep teal shutters 20 percent, golden foliage 10 percent. Atmospheric but photographic material depth, cinematic game environment concept, not a photograph of a famous real place. Maintain clear silhouettes despite detail.
```

## B — 서정적인 반실사

Append to the common prompt:

```text
Art direction B: lyrical semi-realistic fantasy with deliberate shape and material editing. Slightly broader roof silhouettes, stout wooden posts and readable hand-made construction, mild asymmetry without crooked instability. Painterly arrangement of large coherent warm wood, terracotta red roof, teal shutters and mustard canvas color masses; golden tree crowns grouped into legible volumes. Physically grounded natural light and convincing wood/stone/cloth, but reduce small grain and damage noise by half, group textures into designed accents. Mature illustrated game environment look, believable depth and weight, neither photoreal nor cartoon. Quiet blue-green shadows, warm welcoming accents. Think production art that can translate into a simplified PBR environment.
```

## C — 형태 중심

Append to the common prompt:

```text
Art direction C: boldly designed stylized 3D fantasy, mature and adventurous, not cute, not a miniature diorama. Exaggerate the weight of roof silhouettes, use thicker squared timber posts, broad angular stone shapes and simplified clustered conifer masses. Strong readable terracotta red, deep teal, ochre and warm timber planes; keep color areas intentional instead of scattering bright accents. Greatly simplify microtexture: broad brush-like surface variation, crisp sculpted bevels and only selective wood grain. Natural believable light and contact shadows give solid physical volume. Architecture has slightly enlarged eaves and simplified practical joinery, adult scale residents and usable paths, no toy proportions, no outlines, no low-poly triangles. This should differ materially in shape language from the realistic version, not merely in saturation.
```
