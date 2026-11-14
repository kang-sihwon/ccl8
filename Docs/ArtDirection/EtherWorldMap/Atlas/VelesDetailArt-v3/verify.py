"""Read and verify existing exports independently; never regenerate tiles during checks."""
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parent
m = json.loads((ROOT / 'manifest.json').read_text(encoding='utf-8'))
previous = json.loads((ROOT / '../VelesDetailArt-v2/manifest.json').read_text(encoding='utf-8'))
checks = []

def check(name, passed):
    checks.append(dict(name=name, passed=bool(passed)))

check('Geometry locks match the previously recorded V2 locks', m['geometry_sha256'] == previous['geometry_sha256'])
for file, digest in m['geometry_sha256'].items():
    check('Unchanged geometry: ' + file, hashlib.sha256((ROOT / '../VelesDetailTerrain-v1' / file).read_bytes()).hexdigest() == digest)
for name, spec in m['images'].items():
    path = ROOT / spec['file']
    check('Image dimensions and digest: ' + name,
          Image.open(path).size == (spec['width'], spec['height']) and hashlib.sha256(path.read_bytes()).hexdigest() == spec['sha256'])

base = Image.open(ROOT / m['images']['base']['file']).convert('RGB')
art = Image.open(ROOT / 'previews/composite.png').convert('RGB')
check('Original world frame and export dimensions', m['bounds'] == [-2048, -2560, 2048, 3584] and m['origin'] == [150000, 145000] and base.size == art.size == (2048, 3072))
edges = [(0, 0, 2048, 3), (0, 3069, 2048, 3072), (0, 0, 3, 3072), (2045, 0, 2048, 3072)]
check('Four outer 3px strips match the original exactly', all(ImageChops.difference(base.crop(b), art.crop(b)).getbbox() is None for b in edges))
tiles = json.loads((ROOT / 'tiles.json').read_text(encoding='utf-8'))['tiles']
check('Exactly 24 unique expected tile IDs', len(tiles) == 24 and {t['id'] for t in tiles} == {f'C{c}-R{r}' for r in range(6) for c in range(4)})
rebuilt = Image.new('RGB', art.size)
correct = True
for t in tiles:
    c, r = map(int, t['id'][1:].split('-R'))
    b = [max(0, c*512-4), max(0, r*512-4), min(2048, (c+1)*512+4), min(3072, (r+1)*512+4)]
    core = [c*512-b[0], r*512-b[1], (c+1)*512-b[0], (r+1)*512-b[1]]
    im = Image.open(ROOT / t['file']).convert('RGB')
    correct &= (t['bounds_pixels'] == b and t['core_pixels'] == core and t['world_bounds'] == [-2048+c*1024, 3584-(r+1)*1024, -2048+(c+1)*1024, 3584-r*1024] and im.size == (b[2]-b[0], b[3]-b[1]))
    correct &= ImageChops.difference(im, art.crop(b)).getbbox() is None
    rebuilt.paste(im.crop(core), (c*512, r*512))
check('Every existing tile matches its declared world frame and composite crop', correct)
check('24 saved tile cores reconstruct the composite exactly', ImageChops.difference(rebuilt, art).getbbox() is None)
overlaps = []
for i, a in enumerate(tiles):
    for b in tiles[i+1:]:
        ca, ra = map(int, a['id'][1:].split('-R')); cb, rb = map(int, b['id'][1:].split('-R'))
        if abs(ca-cb)+abs(ra-rb) != 1:
            continue
        aa, bb = a['bounds_pixels'], b['bounds_pixels']
        overlap = [max(aa[0],bb[0]),max(aa[1],bb[1]),min(aa[2],bb[2]),min(aa[3],bb[3])]
        def crop(t):
            p = t['bounds_pixels']
            return Image.open(ROOT / t['file']).convert('RGB').crop((overlap[0]-p[0],overlap[1]-p[1],overlap[2]-p[0],overlap[3]-p[1]))
        overlaps.append(dict(a=a['id'], b=b['id'], identical=ImageChops.difference(crop(a),crop(b)).getbbox() is None))
check('All 38 existing shared margins are pixel-identical', len(overlaps) == 38 and all(x['identical'] for x in overlaps))

def area(ids):
    rectangles = [l['bounds'] for l in m['layers'] if l['id'] in ids]
    xs = sorted({x for b in rectangles for x in (b[0], b[2])})
    ys = sorted({y for b in rectangles for y in (b[1], b[3])})
    return sum((x1-x0)*(y1-y0) for x0,x1 in zip(xs,xs[1:]) for y0,y1 in zip(ys,ys[1:]) if any(b[0] <= x0 and b[2] >= x1 and b[1] <= y0 and b[3] >= y1 for b in rectangles)) / 1e6

report = dict(data_and_assembly_status='PASS' if all(c['passed'] for c in checks) else 'FAIL', checks=checks, overlaps=overlaps,
    detail_image_footprint_km2=dict(v2=area({'core','forest'}), v3=area({'core','forest','west','east'}), includes_feather_transitions=True),
    visual_registration_status='PARTIAL',
    manual_visual_review=dict(roof_audit_pages=4, roof_locations_reviewed=86, finding='Roof presence and outline placement generally improved against authoritative cyan footprints; eaves, shadows and path edges still differ. No exact pixel registration or automated building count claimed.', forest='West/east strips reduce coarse canopy beside the existing central forest patch. Remaining regional background still has lower native resolution.', road_alignment='Not exhaustively checked against all 103 road records.'),
    browser_interaction_status='NOT_RUN_FOR_V3', runtime_ue_status='NOT_TESTED')
(ROOT / 'records/verification.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
print(json.dumps(dict(status=report['data_and_assembly_status'], checks=len(checks), failed=[c for c in checks if not c['passed']], coverage=report['detail_image_footprint_km2'])))
if report['data_and_assembly_status'] != 'PASS':
    raise SystemExit(1)
