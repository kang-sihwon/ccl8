import json, math, hashlib
from pathlib import Path
from PIL import Image
root=Path(__file__).parent
plan=json.loads((root/'records/plan.json').read_text(encoding='utf-8-sig'))
Image.MAX_IMAGE_PIXELS=None
atlas=Image.open(root/'previews/m01-assembled.png').convert('RGB')
checks=[]
def check(name,value):
    checks.append({'name':name,'pass':bool(value)})
check('atlas size equals plan',list(atlas.size)==plan['atlasSize'])
check('unique IDs',len({t['id'] for t in plan['tiles']})==12)
check('complete 6 by 2 coverage',{(t['column'],t['row']) for t in plan['tiles']}=={(c,r) for c in range(16316,16322) for r in range(3286,3288)})
sources=[]
for t in plan['tiles']:
    x=(t['column']-16316)*2048;y=(t['row']-3286)*2048
    with Image.open(root/'tiles'/f"{t['id']}.png") as im:
        check(t['id']+' core size',im.size==(2048,2048))
        check(t['id']+' assembly RGB equality',atlas.crop((x,y,x+2048,y+2048)).tobytes()==im.convert('RGB').tobytes())
    with Image.open(root/'generated'/f"{t['id']}.png") as im:
        sources.append({'id':t['id'],'nativeSize':list(im.size),'sha256':hashlib.sha256((root/'generated'/f"{t['id']}.png").read_bytes()).hexdigest()})
    check(t['id']+' world address',t['worldPixelBounds']==[t['column']*2048,t['row']*2048,(t['column']+1)*2048,(t['row']+1)*2048])
for p in plan['points']:
    check(p['id']+' inside atlas',0<=p['atlasPixel'][0]<12288 and 0<=p['atlasPixel'][1]<4096)
    check(p['id']+' world to atlas offset',abs(p['worldPixel'][0]-plan['worldPixelBounds'][0]-p['atlasPixel'][0])<1e-8 and abs(p['worldPixel'][1]-plan['worldPixelBounds'][1]-p['atlasPixel'][1])<1e-8)
report={'status':'PASS' if all(c['pass'] for c in checks) else 'FAIL','checks':checks,'sourceImages':sources,'internalSharedEdges':16,'sourceSizeIsNotOutputSize':True,'limitations':['Pixel equality verifies assembly only, not road or roof semantic continuity.','World coordinate registration preserves map addresses; generated microgeometry is a visual candidate.','No UE execution or gameplay scale verification.']}
(root/'records/assembly-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
print(json.dumps({'status':report['status'],'checks':len(checks),'failed':[c for c in checks if not c['pass']],'nativeSizes':sorted({tuple(s['nativeSize']) for s in sources})},ensure_ascii=False))
raise SystemExit(0 if report['status']=='PASS' else 1)
