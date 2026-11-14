"""Export coordinate-registered reference crops and geometry guides, without inventing art."""
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageChops
ROOT = Path(__file__).resolve().parent
OLD = ROOT / '../VelesDetailArt-v3'
M = json.loads((OLD/'manifest.json').read_text(encoding='utf-8'))
DATA = json.loads((ROOT/'../VelesDetailTerrain-v1/data/connections.json').read_text(encoding='utf-8'))
BUILDINGS = json.loads((ROOT/'../VelesDetailTerrain-v1/data/placements.json').read_text(encoding='utf-8'))['buildings']

def sample(im, b, target, size):
    sx,sy=im.width/(b[2]-b[0]),im.height/(b[3]-b[1]);dx=(target[2]-target[0])/size[0];dy=(target[3]-target[1])/size[1]
    return im.transform(size,Image.Transform.AFFINE,(dx*sx,0,(target[0]-b[0])*sx,0,dy*sy,(b[3]-target[3])*sy),Image.Resampling.BICUBIC)

def masked(im, b, fade, lock=0):
    im=im.convert('RGBA')
    if not fade:return im
    w,h=im.size
    def axis(n,meters):return [round(255*max(0,min(1,(min((i+.5)*meters/n,meters-(i+.5)*meters/n)-lock)/fade))) for i in range(n)]
    x=Image.new('L',(w,1));x.putdata(axis(w,b[2]-b[0]));x=x.resize((w,h))
    y=Image.new('L',(1,h));y.putdata(axis(h,b[3]-b[1]));y=y.resize((w,h))
    im.putalpha(ImageChops.multiply(x,y));return im

def render(bounds,size,layers=None):
    result=Image.new('RGBA',size,(30,40,35,255))
    for l in layers or M['layers']:
        path=Path(l.get('absolute',str(OLD/l['file'])))
        im=masked(Image.open(path),l['bounds'],l['fade'],l['lock'])
        result.alpha_composite(sample(im,l['bounds'],bounds,size))
    return result.convert('RGB')

def guides(im,b):
    out=im.copy();d=ImageDraw.Draw(out);sx=im.width/(b[2]-b[0]);sy=im.height/(b[3]-b[1])
    p=lambda q:((q[0]-b[0])*sx,(b[3]-q[1])*sy)
    for r in DATA['roads']:
        pts=[p(q) for q in r['points']]
        d.line(pts,fill='#ffd163',width=max(1,round(r.get('width',3)*sx)),joint='curve')
    for house in BUILDINGS:d.line([p(q) for q in house['footprint']+[house['footprint'][0]]],fill='#00e5ff',width=1)
    for key in ['river','stream']:d.line([p(q) for q in DATA[key]['points']],fill='#469bff',width=2)
    return out

def main():
    for folder in ['inputs','assets','previews','records','tiles']:(ROOT/folder).mkdir(exist_ok=True)
    columns=[[-2048,-580],[-780,780],[580,2048]]
    rows={'N0':[2300,3584],'N1':[1250,2500],'S0':[-740,350],'S1':[-1650,-540],'S2':[-2560,-1450]}
    specs=[]
    for row,(y0,y1) in rows.items():
        for c,(x0,x1) in enumerate(columns):
            specs.append(dict(id=f'{row}-{c}',bounds=[x0,y0,x1,y1],fade=100,lock=0))
    specs.append(dict(id='roads',bounds=[-800,-250,300,450],fade=60,lock=0))
    for s in specs+[dict(id='forest-roads',bounds=[-600,350,600,1550])]:
        b=s['bounds'];size=(1560,round(1560*(b[3]-b[1])/(b[2]-b[0])))
        im=render(b,size);im.save(ROOT/f"inputs/{s['id']}.png");guide=guides(im,b);guide.save(ROOT/f"inputs/{s['id']}-guide.png")
        im.thumbnail((600,600));im.save(ROOT/f"previews/{s['id']}-input.jpg",quality=85)
    (ROOT/'inputs/specs.json').write_text(json.dumps(specs,indent=2)+'\n',encoding='utf-8')
    sheet=Image.new('RGB',(1000,5*240),'#202626');d=ImageDraw.Draw(sheet)
    for i,s in enumerate(specs[:15]):
        im=Image.open(ROOT/f"inputs/{s['id']}-guide.png");im.thumbnail((330,215));x=(i%3)*333;y=(i//3)*240
        sheet.paste(im,(x,y+20));d.text((x+5,y+4),s['id'],fill='white')
    sheet.save(ROOT/'previews/input-sheet.jpg',quality=90)
    print(json.dumps({'reference_regions':len(specs),'scope':'north/south terrain and village roads; fixed coordinates'}))

if __name__=='__main__':main()
