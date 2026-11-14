"""Verify persisted V5 geometry locks, unaffected pixels, image coverage and tile joins."""
import json
import hashlib
from pathlib import Path
from PIL import Image,ImageChops,ImageDraw
ROOT=Path(__file__).resolve().parent
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
old=json.loads((ROOT/'../VelesDetailArt-v4/manifest.json').read_text(encoding='utf-8'))
checks=[]
def check(name,passed):checks.append(dict(name=name,passed=bool(passed)))
check('World frame and origin unchanged',m['bounds']==old['bounds']==[-2048,-2560,2048,3584] and m['origin']==old['origin']==[150000,145000])
check('Geometry hash authority unchanged',m['geometry_sha256']==old['geometry_sha256'])
for f,h in m['geometry_sha256'].items():check('Original geometry '+f,hashlib.sha256((ROOT/'../VelesDetailTerrain-v1'/f).read_bytes()).hexdigest()==h)
check('All 23 V4 source image digests preserved',all(m['images'][name]['sha256']==s['sha256'] for name,s in old['images'].items()) and len(old['images'])==23)
for name,s in m['images'].items():
    p=ROOT/s['file'];check('Asset '+name,Image.open(p).size==(s['width'],s['height']) and hashlib.sha256(p.read_bytes()).hexdigest()==s['sha256'])
specs=json.loads((ROOT/'inputs/specs.json').read_text(encoding='utf-8'));byid={l['id']:l for l in m['layers']}
check('Four edit frames match input records',len(specs)==4 and all(byid[s['id']]['bounds']==s['bounds'] for s in specs))
full=Image.open(ROOT/'previews/composite-1m.png').convert('RGB');previous=Image.open(ROOT/'../VelesDetailArt-v4/previews/composite-1m.png').convert('RGB')
small=Image.open(ROOT/'previews/composite.png').convert('RGB');oldsmall=Image.open(ROOT/'../VelesDetailArt-v4/previews/composite.png').convert('RGB')
check('1m full export and 2m preview dimensions',full.size==previous.size==(4096,6144) and small.size==oldsmall.size==(2048,3072))
def border(w,h,n):return [(0,0,w,n),(0,h-n,w,h),(0,0,n,h),(w-n,0,w,h)]
check('Outer six pixels of full export unchanged',all(ImageChops.difference(full.crop(b),previous.crop(b)).getbbox() is None for b in border(4096,6144,6)))
check('Outer three pixels of preview unchanged',all(ImageChops.difference(small.crop(b),oldsmall.crop(b)).getbbox() is None for b in border(2048,3072,3)))
allowed=Image.new('L',full.size,0);d=ImageDraw.Draw(allowed)
for s in specs:
    x0,y0,x1,y1=s['bounds'];d.rectangle((x0+2048-2,3584-y1-2,x1+2048+2,3584-y0+2),fill=255)
diff=ImageChops.difference(full,previous);outside=ImageChops.invert(allowed)
check('Pixels outside four edit frames plus 2px sampling margin unchanged',all(ImageChops.multiply(c,outside).getbbox() is None for c in diff.split()))
tiles=json.loads((ROOT/'tiles.json').read_text(encoding='utf-8'))['tiles']
check('Exactly 24 expected tile IDs',len(tiles)==24 and {t['id'] for t in tiles}=={f'C{c}-R{r}' for c in range(4) for r in range(6)})
restored=Image.new('RGB',full.size);exact=True
for t in tiles:
    c,r=map(int,t['id'][1:].split('-R'));b=[max(0,c*1024-8),max(0,r*1024-8),min(4096,(c+1)*1024+8),min(6144,(r+1)*1024+8)]
    core=[c*1024-b[0],r*1024-b[1],(c+1)*1024-b[0],(r+1)*1024-b[1]];im=Image.open(ROOT/t['file']).convert('RGB')
    exact &= t['bounds_pixels']==b and t['core_pixels']==core and t['world_bounds']==[-2048+c*1024,3584-(r+1)*1024,-2048+(c+1)*1024,3584-r*1024]
    exact &= im.size==(b[2]-b[0],b[3]-b[1]) and ImageChops.difference(im,full.crop(b)).getbbox() is None
    restored.paste(im.crop(core),(c*1024,r*1024))
check('Saved tile frames and pixels match composite',exact)
check('24 saved cores reassemble exactly',ImageChops.difference(restored,full).getbbox() is None)
overlaps=[]
for i,a in enumerate(tiles):
    for b in tiles[i+1:]:
        ca,ra=map(int,a['id'][1:].split('-R'));cb,rb=map(int,b['id'][1:].split('-R'))
        if abs(ca-cb)+abs(ra-rb)!=1:continue
        aa,bb=a['bounds_pixels'],b['bounds_pixels'];ov=[max(aa[0],bb[0]),max(aa[1],bb[1]),min(aa[2],bb[2]),min(aa[3],bb[3])]
        def cut(t):
            p=t['bounds_pixels'];return Image.open(ROOT/t['file']).convert('RGB').crop((ov[0]-p[0],ov[1]-p[1],ov[2]-p[0],ov[3]-p[1]))
        overlaps.append(dict(a=a['id'],b=b['id'],identical=ImageChops.difference(cut(a),cut(b)).getbbox() is None))
check('38 shared tile margins identical',len(overlaps)==38 and all(t['identical'] for t in overlaps))
rects=[l['bounds'] for l in m['layers'] if l['id'] not in ['base','region']]
xs=sorted({v for b in rects for v in (b[0],b[2])});ys=sorted({v for b in rects for v in (b[1],b[3])})
area=sum((x1-x0)*(y1-y0) for x0,x1 in zip(xs,xs[1:]) for y0,y1 in zip(ys,ys[1:]) if any(b[0]<=x0 and b[2]>=x1 and b[1]<=y0 and b[3]>=y1 for b in rects))
check('Detail image footprints cover the full local rectangle',area==4096*6144)
report=dict(status='PASS' if all(t['passed'] for t in checks) else 'FAIL',checks=checks,overlaps=overlaps,detail_image_footprint_km2=area/1e6,
    new_native_mpp={s['id']:byid[s['id']]['mpp'] for s in specs},
    coverage_caveat='Image footprint coverage includes feather zones and intentionally preserved outer-border pixels; not a claim of uniform optical resolution.',
    visual_registration='Not a surveyed map; generated path widths and roof outlines require visual review.',browser_interaction='NOT_RUN_FOR_V5',ue_runtime='NOT_TESTED')
(ROOT/'records/verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(status=report['status'],checks=len(checks),coverage=report['detail_image_footprint_km2'],native_mpp=report['new_native_mpp'],failed=[t for t in checks if not t['passed']])))
if report['status']!='PASS':raise SystemExit(1)
