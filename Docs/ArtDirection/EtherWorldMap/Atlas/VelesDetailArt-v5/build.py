"""Register V5 art crops against immutable V4 frames; no synthetic terrain painting."""
import hashlib
import json
import sys
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parent
PREVIOUS=ROOT/'../VelesDetailArt-v4'
sys.path.insert(0,str(PREVIOUS.resolve()))
import assemble as engine
from prepare_inputs import guides
OLD=json.loads((PREVIOUS/'manifest.json').read_text(encoding='utf-8'))
SPECS=[
 dict(id='west-fill',bounds=[-2048,200,-1250,1400],fade=80,lock=0,global_border_lock_m=12,global_border_fade_m=32),
 dict(id='east-fill',bounds=[1250,200,2048,1400],fade=80,lock=0,global_border_lock_m=12,global_border_fade_m=32),
 dict(id='workshop-access',bounds=[-30,50,115,250],fade=15,lock=0),
 dict(id='west-access',bounds=[-340,60,-90,250],fade=15,lock=0)]

def load(m,base):
    return engine.load_layers({**m,'layers':[{**l,'file':str((base/l['file']).resolve())} for l in m['layers']]})

def prep():
    for d in ['inputs','assets','previews','records','tiles']:(ROOT/d).mkdir(exist_ok=True)
    layers=load(OLD,PREVIOUS)
    for s in SPECS:
        b=s['bounds'];w=1400;h=round(w*(b[3]-b[1])/(b[2]-b[0]));im=engine.render(b,(w,h),layers)
        im.save(ROOT/f"inputs/{s['id']}.png");guides(im,b).save(ROOT/f"inputs/{s['id']}-guide.png")
        engine.review_image(im,ROOT/f"previews/{s['id']}-input.jpg",(650,650))
        engine.review_image(guides(im,b),ROOT/f"previews/{s['id']}-guide.jpg",(650,650))
    (ROOT/'inputs/specs.json').write_text(json.dumps(SPECS,indent=2)+'\n',encoding='utf-8')
    print('Prepared four fixed-coordinate image edits.')

def build():
    layers=[{**l,'file':str(Path('../VelesDetailArt-v4')/l['file']).replace('\\','/')} for l in OLD['layers']]
    layers += [{**s,'file':f"assets/{s['id']}.png"} for s in SPECS]
    images={}
    for l in layers:
        p=ROOT/l['file'];im=Image.open(p);l['width'],l['height']=im.size;b=l['bounds']
        l['mpp']=[(b[2]-b[0])/im.width,(b[3]-b[1])/im.height]
        images[l['id']]=dict(file=l['file'],width=im.width,height=im.height,sha256=hashlib.sha256(p.read_bytes()).hexdigest())
    m=dict(status='outer_strip_and_access_path_art_candidate',bounds=OLD['bounds'],origin=OLD['origin'],geometry_sha256=OLD['geometry_sha256'],images=images,layers=layers,export_mpp=1,preview_mpp=2)
    (ROOT/'manifest.json').write_text(json.dumps(m,indent=2)+'\n',encoding='utf-8')
    (ROOT/'art-assets.js').write_text('const ART_ASSETS='+json.dumps({k:v['file'] for k,v in images.items()})+';\nconst ART_LAYERS='+json.dumps(layers)+';\n',encoding='utf-8')
    loaded=load(m,ROOT);old_loaded=load(OLD,PREVIOUS)
    full=engine.render(m['bounds'],(4096,6144),loaded);full.save(ROOT/'previews/composite-1m.png')
    preview=engine.render(m['bounds'],(2048,3072),loaded);preview.save(ROOT/'previews/composite.png')
    tiles=[]
    for r in range(6):
        for c in range(4):
            b=[max(0,c*1024-8),max(0,r*1024-8),min(4096,(c+1)*1024+8),min(6144,(r+1)*1024+8)]
            core=[c*1024-b[0],r*1024-b[1],(c+1)*1024-b[0],(r+1)*1024-b[1]];f=f'tiles/C{c}-R{r}.png'
            full.crop(b).save(ROOT/f);tiles.append(dict(id=f'C{c}-R{r}',file=f,bounds_pixels=b,core_pixels=core,world_bounds=[-2048+c*1024,3584-(r+1)*1024,-2048+(c+1)*1024,3584-r*1024]))
    (ROOT/'tiles.json').write_text(json.dumps(dict(meters_per_export_pixel=1,tiles=tiles),indent=2)+'\n',encoding='utf-8')
    grid=preview.copy();d=ImageDraw.Draw(grid)
    for x in range(0,2048,512):d.line((x,0,x,3071),fill='#b8c8c6')
    for y in range(0,3072,512):d.line((0,y,2047,y),fill='#b8c8c6')
    grid.save(ROOT/'previews/assembled-grid.png');engine.review_image(grid,ROOT/'previews/assembled-review.jpg')
    frames=SPECS+[dict(id='village',bounds=[-458,-40,218,300])]
    for s in frames:
        b=s['bounds'];w=1100;h=round(w*(b[3]-b[1])/(b[2]-b[0]));new=engine.render(b,(w,h),loaded);old=engine.render(b,(w,h),old_loaded)
        new.save(ROOT/f"previews/{s['id']}.png")
        comp=Image.new('RGB',(w*2,h+24),'#202626');comp.paste(old,(0,24));comp.paste(new,(w,24));d=ImageDraw.Draw(comp);d.text((8,5),'V4',fill='white');d.text((w+8,5),'V5',fill='white')
        comp.save(ROOT/f"previews/{s['id']}-comparison.png");engine.review_image(comp,ROOT/f"previews/{s['id']}-comparison-review.jpg")
        if 'access' in s['id']:
            overlay=guides(new,b);overlay.save(ROOT/f"previews/{s['id']}-geometry.png");engine.review_image(overlay,ROOT/f"previews/{s['id']}-geometry-review.jpg")
    print(json.dumps(dict(images=len(images),tiles=len(tiles),export=[4096,6144])))

if __name__=='__main__':
    if '--inputs' in sys.argv:prep()
    else:build()
