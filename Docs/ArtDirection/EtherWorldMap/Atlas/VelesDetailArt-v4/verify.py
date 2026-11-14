"""Independent file, world-frame, source-lock and saved-tile checks. No tile regeneration."""
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageChops
ROOT=Path(__file__).resolve().parent
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
old=json.loads((ROOT/'../VelesDetailArt-v3/manifest.json').read_text(encoding='utf-8'))
checks=[]
def check(name,passed):checks.append(dict(name=name,passed=bool(passed)))
check('Previously recorded geometry hashes remain authoritative',m['geometry_sha256']==old['geometry_sha256'])
for f,h in m['geometry_sha256'].items():
    check('Unchanged geometry '+f,hashlib.sha256((ROOT/'../VelesDetailTerrain-v1'/f).read_bytes()).hexdigest()==h)
for name,s in m['images'].items():
    p=ROOT/s['file'];check('Asset dimensions and digest '+name,Image.open(p).size==(s['width'],s['height']) and hashlib.sha256(p.read_bytes()).hexdigest()==s['sha256'])
check('World origin and bounds preserved',m['bounds']==old['bounds']==[-2048,-2560,2048,3584] and m['origin']==old['origin']==[150000,145000])
specs=json.loads((ROOT/'inputs/specs.json').read_text(encoding='utf-8'))
by_id={l['id']:l for l in m['layers']}
check('All 16 planned input frames are preserved',all(by_id[s['id']]['bounds']==s['bounds'] for s in specs))
check('Forest road repair frame preserved',by_id['forest-roads']['bounds']==[-600,350,600,1550])
full=Image.open(ROOT/'previews/composite-1m.png').convert('RGB')
preview=Image.open(ROOT/'previews/composite.png').convert('RGB')
old_preview=Image.open(ROOT/'../VelesDetailArt-v3/previews/composite.png').convert('RGB')
check('1m export and 2m preview dimensions',full.size==(4096,6144) and preview.size==(2048,3072))
def edges(w,h,n):return [(0,0,w,n),(0,h-n,w,h),(0,0,n,h),(w-n,0,w,h)]
check('Four outer 3px preview strips match V3 exactly',all(ImageChops.difference(preview.crop(b),old_preview.crop(b)).getbbox() is None for b in edges(2048,3072,3)))
base=Image.open(ROOT/m['images']['base']['file']).convert('RGBA').transform((4096,6144),Image.Transform.AFFINE,(.5,0,0,0,.5,0),Image.Resampling.BICUBIC).convert('RGB')
check('Four outer 6px full-resolution strips match resampled locked base',all(ImageChops.difference(full.crop(b),base.crop(b)).getbbox() is None for b in edges(4096,6144,6)))
t=json.loads((ROOT/'tiles.json').read_text(encoding='utf-8'));tiles=t['tiles']
check('Exactly 24 expected tile IDs at 1m per pixel',t['meters_per_export_pixel']==1 and len(tiles)==24 and {s['id'] for s in tiles}=={f'C{c}-R{r}' for c in range(4) for r in range(6)})
restored=Image.new('RGB',full.size);exact=True
for s in tiles:
    c,r=map(int,s['id'][1:].split('-R'));b=[max(0,c*1024-8),max(0,r*1024-8),min(4096,(c+1)*1024+8),min(6144,(r+1)*1024+8)]
    core=[c*1024-b[0],r*1024-b[1],(c+1)*1024-b[0],(r+1)*1024-b[1]];im=Image.open(ROOT/s['file']).convert('RGB')
    exact &= s['bounds_pixels']==b and s['core_pixels']==core and s['world_bounds']==[-2048+c*1024,3584-(r+1)*1024,-2048+(c+1)*1024,3584-r*1024]
    exact &= im.size==(b[2]-b[0],b[3]-b[1]) and ImageChops.difference(im,full.crop(b)).getbbox() is None
    restored.paste(im.crop(core),(c*1024,r*1024))
check('All saved tile dimensions, world bounds and pixels match full export',exact)
check('24 saved tile cores reconstruct the full export exactly',ImageChops.difference(full,restored).getbbox() is None)
overlaps=[]
for i,a in enumerate(tiles):
    for b in tiles[i+1:]:
        ca,ra=map(int,a['id'][1:].split('-R'));cb,rb=map(int,b['id'][1:].split('-R'))
        if abs(ca-cb)+abs(ra-rb)!=1:continue
        aa,bb=a['bounds_pixels'],b['bounds_pixels'];ov=[max(aa[0],bb[0]),max(aa[1],bb[1]),min(aa[2],bb[2]),min(aa[3],bb[3])]
        def cut(s):
            p=s['bounds_pixels'];return Image.open(ROOT/s['file']).convert('RGB').crop((ov[0]-p[0],ov[1]-p[1],ov[2]-p[0],ov[3]-p[1]))
        overlaps.append(dict(a=a['id'],b=b['id'],identical=ImageChops.difference(cut(a),cut(b)).getbbox() is None))
check('All 38 shared margins are pixel-identical',len(overlaps)==38 and all(s['identical'] for s in overlaps))
roads=json.loads((ROOT/'../VelesDetailTerrain-v1/data/connections.json').read_text(encoding='utf-8'))['roads']
audit=json.loads((ROOT/'records/road-audit-index.json').read_text(encoding='utf-8'))
check('Road audit covers all 103 authoritative route IDs',len(audit)==len(roads)==103 and {s['id'] for s in audit}=={r['id'] for r in roads})
check('All six road audit pages exist',all((ROOT/f'previews/roads-audit-{n}.png').is_file() for n in range(1,7)))
check('Rejected forest-with-road asset is absent from selected layers',all(l['file']!='assets/S1-2.png' for l in m['layers']))
def area(rects):
    xs=sorted({v for b in rects for v in [b[0],b[2]]});ys=sorted({v for b in rects for v in [b[1],b[3]]})
    return sum((x1-x0)*(y1-y0) for x0,x1 in zip(xs,xs[1:]) for y0,y1 in zip(ys,ys[1:]) if any(b[0]<=x0 and b[2]>=x1 and b[1]<=y0 and b[3]>=y1 for b in rects))/1e6
new_native=[l for l in m['layers'] if l['id'].startswith(('N','S'))]
check('15 new terrain patches have native spacing no coarser than 1.14m per pixel',len(new_native)==15 and all(max(l['mpp'])<=1.14 for l in new_native))
detail=[l['bounds'] for l in m['layers'] if l['id'] not in ['base','region']]
report=dict(status='PASS' if all(c['passed'] for c in checks) else 'FAIL',checks=checks,overlaps=overlaps,
    detail_footprint_km2=area(detail),coverage_caveat='Includes feather zones and outer locked strips; does not imply equal optical detail at every pixel.',
    new_terrain_native_mpp=[min(min(l['mpp']) for l in new_native),max(max(l['mpp']) for l in new_native)],
    pixel_registration_status='PARTIAL: generated terrain is art; unchanged coordinates and shared tile pixels do not prove every painted path edge.',
    browser_interaction='NOT_RUN_FOR_V4',ue_runtime='NOT_TESTED')
(ROOT/'records/verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:report[k] for k in ['status','detail_footprint_km2','new_terrain_native_mpp']}|dict(checks=len(checks),failed=[c for c in checks if not c['passed']])))
if report['status']!='PASS':raise SystemExit(1)
