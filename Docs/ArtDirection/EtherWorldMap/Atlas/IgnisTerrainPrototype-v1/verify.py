"""Read-only image comparison and source-coordinate checks for the prototype."""
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageChops

root = Path(__file__).resolve().parent
old = root.parent / 'IgnisSettlementDetails-v1'
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
source, plan = read(old / 'records/plan.json'), read(root / 'records/plan.json')
checks = []

def check(name, passed, detail=None):
    checks.append(dict(name=name, passed=bool(passed), detail=detail))

check('36 settlement IDs and world anchors preserved',
      [(s['id'], s['worldPixel']) for s in source['settlements']] ==
      [(s['id'], s['worldPixel']) for s in plan['settlements']])
check('All original coast, river and route vertices preserved',
      [(f['id'], f['worldPixelPoints']) for f in source['features']] ==
      [(f['id'], f['worldPixelPoints']) for f in plan['features']])
check('150 existing detail image references resolve',
      sum((root / t['path']).is_file() for s in plan['settlements'] for t in s['tiles']) == 150)
references = [s[k] for s in plan['settlements'] for k in ['source', 'context'] if k in s]
references += [t[k] for s in plan['settlements'] for t in s['tiles'] for k in ['path', 'source', 'guide', 'generated'] if k in t]
check('Source, guide, generated and display image references resolve', all((root / p).is_file() for p in references))
check('150 detail bounds preserved',
      [(t['id'], t['worldPixelBounds']) for s in source['settlements'] for t in s['tiles']] ==
      [(t['id'], t['worldPixelBounds']) for s in plan['settlements'] for t in s['tiles']])
whole = Image.open(root / 'previews/material-whole.png').convert('RGB')
for name, box in [('left', (0, 0, 512, 512)), ('right', (512, 0, 1024, 512))]:
    part = Image.open(root / f'previews/material-{name}.png').convert('RGB')
    diff = ImageChops.difference(whole.crop(box), part)
    check(f'Independently rendered {name} tile matches combined render RGB exactly',
          diff.getbbox() is None, dict(maxRGBDifference=max(v[1] for v in diff.getextrema())))
repeat = Image.open(root / 'previews/material-repeat.png').convert('RGB')
check('Same coordinates render identically after different views', ImageChops.difference(whole, repeat).getbbox() is None)
ends = next(f for f in plan['features'] if f['id'] == 'B04')['worldPixelPoints']
anchors = [next(s for s in plan['settlements'] if s['id'] == sid)['worldPixel'] for sid in ['V14', 'T11']]
endpoint_error = max(abs(a - b) for p, q in zip(ends, anchors) for a, b in zip(p, q))
check('B04 endpoints agree with settlement anchors within projection rounding',
      endpoint_error < 0.000001, dict(maxPixelError=endpoint_error, tolerancePixels=0.000001))
check('No measured elevation is claimed', plan['prototype']['hasMeasuredHeightmap'] is False)
browser = read(root / 'records/browser-verification.json')
check('Browser render and input checks pass', browser['status'] == 'PASS')
report = dict(status='PASS' if all(c['passed'] for c in checks) else 'FAIL',
              scope='source_coordinates_and_one_sample_split_render_not_all_continent_art_quality',
              checks=checks,
              materialSHA256=hashlib.sha256((root / 'materials/aerial-material-atlas.png').read_bytes()).hexdigest())
(root / 'records/verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(json.dumps(dict(status=report['status'], checks=len(checks), failed=[c for c in checks if not c['passed']]), ensure_ascii=False))
raise SystemExit(0 if report['status'] == 'PASS' else 1)
