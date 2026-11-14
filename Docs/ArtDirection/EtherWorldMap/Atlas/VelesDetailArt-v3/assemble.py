"""Assemble generated map layers in their fixed world frames; no painting or terrain invention."""
import json,math
from pathlib import Path
from PIL import Image,ImageDraw,ImageChops
ROOT=Path(__file__).resolve().parent
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
def load_layer(l):
    im=Image.open(ROOT/l['file']).convert('RGBA')
    if not l['fade']:return im
    w,h=im.size;b=l['bounds'];fade=l['fade'];lock=l['lock']
    def axis(n,meters):return [round(255*max(0,min(1,(min((i+.5)*meters/n,meters-(i+.5)*meters/n)-lock)/fade))) for i in range(n)]
    x=Image.new('L',(w,1));x.putdata(axis(w,b[2]-b[0]));x=x.resize((w,h))
    y=Image.new('L',(1,h));y.putdata(axis(h,b[3]-b[1]));y=y.resize((w,h))
    im.putalpha(ImageChops.multiply(x,y));return im
layers=[(l,load_layer(l)) for l in m['layers']]
def sample(im,b,target,size):
    sx,sy=im.width/(b[2]-b[0]),im.height/(b[3]-b[1]);dx=(target[2]-target[0])/size[0];dy=(target[3]-target[1])/size[1]
    return im.transform(size,Image.Transform.AFFINE,(dx*sx,0,(target[0]-b[0])*sx,0,dy*sy,(b[3]-target[3])*sy),Image.Resampling.BICUBIC)
def render(bounds,size,old=False):
    result=Image.new('RGBA',size,(30,40,35,255))
    for l,im in layers:
        if old and l['id'] in ('west','east'):continue
        if old and l['id']=='core':im=load_layer({**l,'file':'../VelesDetailArt-v2/assets/core.png'})
        result.alpha_composite(sample(im,l['bounds'],bounds,size))
    return result.convert('RGB')
whole=render(m['bounds'],(2048,3072));whole.save(ROOT/'previews/composite.png')
tiles=[];rebuilt=Image.new('RGB',whole.size)
for r in range(6):
    for c in range(4):
        b=[max(0,c*512-4),max(0,r*512-4),min(2048,(c+1)*512+4),min(3072,(r+1)*512+4)]
        f=f'tiles/C{c}-R{r}.png';im=whole.crop(b);im.save(ROOT/f);core=[c*512-b[0],r*512-b[1],(c+1)*512-b[0],(r+1)*512-b[1]]
        rebuilt.paste(im.crop(core),(c*512,r*512));tiles.append(dict(id=f'C{c}-R{r}',file=f,bounds_pixels=b,core_pixels=core,world_bounds=[-2048+c*1024,3584-(r+1)*1024,-2048+(c+1)*1024,3584-r*1024]))
(ROOT/'tiles.json').write_text(json.dumps(dict(meters_per_export_pixel=2,tiles=tiles),indent=2)+'\n',encoding='utf-8')
d=ImageDraw.Draw(rebuilt)
for x in range(0,2048,512):d.line((x,0,x,3071),fill='#b8c8c6')
for y in range(0,3072,512):d.line((0,y,2047,y),fill='#b8c8c6')
rebuilt.save(ROOT/'previews/assembled-grid.png')
frames={'village':[-458,-40,218,300],'forest':[-1260,190,1360,1510]}
for name,b in frames.items():
    w=1300;h=round(w*(b[3]-b[1])/(b[2]-b[0]));new=render(b,(w,h));new.save(ROOT/f'previews/{name}.png')
    old=render(b,(w,h),True);compare=Image.new('RGB',(w*2,h+24),'#1e2020');compare.paste(old,(0,24));compare.paste(new,(w,24));d=ImageDraw.Draw(compare);d.text((12,5),'V2',fill='white');d.text((w+12,5),'V3',fill='white');compare.save(ROOT/f'previews/{name}-comparison.png')
buildings=json.loads((ROOT/'../VelesDetailTerrain-v1/data/placements.json').read_text(encoding='utf-8'))['buildings'];bounds=m['layers'][-1]['bounds']
oldcore=Image.open(ROOT/'../VelesDetailArt-v2/assets/core.png').convert('RGBA');newcore=Image.open(ROOT/'assets/core.png').convert('RGBA')
for page in range(4):
    sheet=Image.new('RGB',(880,660),'#1e2020');d=ImageDraw.Draw(sheet)
    for k,b in enumerate(buildings[page*24:(page+1)*24]):
        x,y=(k%4)*220,(k//4)*110;cx,cy=b['center'];radius=max(b['width'],b['depth'])*.75+3;region=[cx-radius,cy-radius,cx+radius,cy+radius]
        for j,im in enumerate([oldcore,newcore]):
            part=sample(im,bounds,region,(100,90)).convert('RGB');q=ImageDraw.Draw(part);poly=[((p[0]-region[0])/(radius*2)*100,(region[3]-p[1])/(radius*2)*90) for p in b['footprint']];q.line(poly+[poly[0]],fill='#70ecf4',width=1);sheet.paste(part,(x+j*108,y+17))
        d.text((x+3,y+2),str(page*24+k)+' '+b['id']+'  V2 | V3',fill='white')
    sheet.save(ROOT/f'previews/roof-audit-{page+1}.png')
print(json.dumps({'tiles':len(tiles),'buildings_in_audit':len(buildings),'offline_compositor':'Pillow world-coordinate assembly; browser interaction not checked in this revision'}))
