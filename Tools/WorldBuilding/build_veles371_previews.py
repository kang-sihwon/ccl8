"""Build the wide-grid previews from the published SVG tiles (Pillow, resvg-py)."""
import base64
import io
import json
import re
from concurrent.futures import ProcessPoolExecutor
from itertools import repeat
from pathlib import Path
from zipfile import ZipFile

from PIL import Image
from resvg_py import svg_to_bytes


ROOT = Path(__file__).resolve().parents[2] / "Docs/ArtDirection/EtherWorldMap/Atlas/VelesTarnelRoden-v1/Veles371"
OUTPUT = ROOT / "previews"
TILE_WIDTH, TILE_HEIGHT = 48, 52
WORLD_TILE = 6


def render_block(path, world):
    bx, by = map(int, path.stem.split("_"))
    radius, block = world["radius"], world["block"]
    side = 2 * radius + 1
    col, row = bx * block, by * block
    cols, rows = min(block, side - col), min(block, side - row)
    mosaic = Image.new("RGB", (cols * TILE_WIDTH, rows * TILE_HEIGHT))
    packed = base64.b64decode(path.read_bytes().split(b'"')[3], validate=True)
    with ZipFile(io.BytesIO(packed)) as archive:
        assert len(archive.namelist()) == cols * rows
        for name in archive.namelist():
            key = name.removesuffix(".svg")
            x, y = map(int, key.split("_"))
            src = world["originals"].get(key)
            if src:
                box = "48 142 1084 1160" if src["region"] == "벨레스" else "48 86 1084 1160"
            else:
                w, h = world["size"]
                box = f"22 26 {w} {h}"
            svg = archive.read(name).decode("utf-8")
            head, body = svg.split("<svg ", 1)[1].split(">", 1)
            head = re.sub(r'\b(width|height|viewBox)="[^"]*"', "", head)
            svg = f'<svg {head} width="{TILE_WIDTH}" height="{TILE_HEIGHT}" viewBox="{box}" preserveAspectRatio="none">{body}'
            png = svg_to_bytes(svg_string=svg, skip_system_fonts=True)
            with Image.open(io.BytesIO(png)) as tile:
                mosaic.paste(tile.convert("RGB"), ((x + radius - col) * TILE_WIDTH,
                             (rows - 1 - (y + radius - row)) * TILE_HEIGHT))
    mosaic.save(OUTPUT / (path.stem + ".webp"), quality=82, method=2)
    small = mosaic.resize((cols * WORLD_TILE, rows * WORLD_TILE), Image.Resampling.LANCZOS)
    return col, side - row - rows, small


def main():
    catalog = (ROOT / "catalog.js").read_text(encoding="utf-8")
    world = json.loads(catalog.split("=", 1)[1].strip().rstrip(";"))
    side = 2 * world["radius"] + 1
    files = sorted((ROOT / "blocks").glob("*.js"))
    OUTPUT.mkdir(exist_ok=True)
    whole = Image.new("RGB", (side * WORLD_TILE, side * WORLD_TILE))
    with ProcessPoolExecutor(max_workers=8) as pool:
        for count, (x, y, small) in enumerate(pool.map(render_block, files, repeat(world)), 1):
            whole.paste(small, (x * WORLD_TILE, y * WORLD_TILE))
            if count % 48 == 0:
                print(f"{count}/{len(files)}", flush=True)
    whole.save(OUTPUT / "world.webp", quality=86, method=4)


if __name__ == "__main__":
    main()
