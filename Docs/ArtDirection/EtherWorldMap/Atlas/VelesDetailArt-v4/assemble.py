"""Register generated imagery, export shared tiles and create geometry review evidence."""
import hashlib
import json
import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageChops
from prepare_inputs import ROOT, OLD, M, DATA, BUILDINGS, sample, masked, render as previous_render
B = M['bounds']

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def make_manifest():
    specs=json.loads((ROOT/'inputs/specs.json').read_text(encoding='utf-8'))
    old=[{**l,'file':str(Path('../VelesDetailArt-v3')/l['file']).replace('\\','/')} for l in M['layers']]
    new=[]
    for s in specs:
        f='assets/'+('S1-2-fixed' if s['id']=='S1-2' else s['id'])+'.png'
        new.append({**s,'file':f,'global_border_lock_m':12,'global_border_fade_m':32})
    forest=dict(id='forest-roads',file='assets/forest-roads.png',bounds=[-600,350,600,1550],fade=60,lock=0)
    layers=old[:2]+new[:-1]+old[2:]+[forest]+new[-1:]
    images={}
    for l in layers:
        p=ROOT/l['file'];im=Image.open(p);l['width'],l['height']=im.size
        l['mpp']=[(l['bounds'][2]-l['bounds'][0])/im.width,(l['bounds'][3]-l['bounds'][1])/im.height]
        images[l['id']]={'file':l['file'],'width':im.width,'height':im.height,'sha256':digest(p)}
    manifest=dict(status='north_south_detail_and_road_art_candidate',bounds=B,origin=M['origin'],geometry_sha256=M['geometry_sha256'],images=images,layers=layers,export_mpp=1,preview_mpp=2,global_border_lock_m=12)
    (ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (ROOT/'art-assets.js').write_text('const ART_ASSETS='+json.dumps({k:v['file'] for k,v in images.items()})+';\nconst ART_LAYERS='+json.dumps(layers)+';\n',encoding='utf-8')
    return manifest

def global_mask(size,b,lock=12,fade=32):
    w,h=size
    def axis(n,start,span,low,high):return [round(255*max(0,min(1,(min(start+(i+.5)*span/n-low,high-start-(i+.5)*span/n)-lock)/fade))) for i in range(n)]
    x=Image.new('L',(w,1));x.putdata(axis(w,b[0],b[2]-b[0],B[0],B[2]));x=x.resize((w,h))
    y=Image.new('L',(1,h));y.putdata(list(reversed(axis(h,b[1],b[3]-b[1],B[1],B[3]))));y=y.resize((w,h))
    return ImageChops.multiply(x,y)

def load_layers(m):
    result=[]
    for l in m['layers']:
        im=masked(Image.open(ROOT/l['file']),l['bounds'],l['fade'],l['lock'])
        if l.get('global_border_lock_m'):
            im.putalpha(ImageChops.multiply(im.getchannel('A'),global_mask(im.size,l['bounds'])))
        result.append((l,im))
    return result

def render(bounds,size,layers):
    result=Image.new('RGBA',size,(30,40,35,255))
    for l,im in layers:
        b=l['bounds']
        if b[2]<bounds[0] or b[0]>bounds[2] or b[3]<bounds[1] or b[1]>bounds[3]:continue
        result.alpha_composite(sample(im,b,bounds,size))
    return result.convert('RGB')

def review_image(im,path,maxsize=(1200,850)):
    im=im.copy();im.thumbnail(maxsize);im.save(path,quality=90)

def road_overlay(im,b,roads):
    im=im.copy();d=ImageDraw.Draw(im)
    p=lambda q:((q[0]-b[0])/(b[2]-b[0])*im.width,(b[3]-q[1])/(b[3]-b[1])*im.height)
    for r in roads:d.line([p(q) for q in r['points']],fill='#ffd163',width=1)
    return im

def main():
    m=make_manifest();layers=load_layers(m)
    full=render(B,(4096,6144),layers);full.save(ROOT/'previews/composite-1m.png')
    preview=render(B,(2048,3072),layers);preview.save(ROOT/'previews/composite.png')
    tiles=[]
    for r in range(6):
        for c in range(4):
            b=[max(0,c*1024-8),max(0,r*1024-8),min(4096,(c+1)*1024+8),min(6144,(r+1)*1024+8)]
            core=[c*1024-b[0],r*1024-b[1],(c+1)*1024-b[0],(r+1)*1024-b[1]]
            f=f'tiles/C{c}-R{r}.png';full.crop(b).save(ROOT/f)
            tiles.append(dict(id=f'C{c}-R{r}',file=f,bounds_pixels=b,core_pixels=core,world_bounds=[-2048+c*1024,3584-(r+1)*1024,-2048+(c+1)*1024,3584-r*1024]))
    (ROOT/'tiles.json').write_text(json.dumps(dict(meters_per_export_pixel=1,margin_pixels=8,tiles=tiles),indent=2)+'\n',encoding='utf-8')
    grid=preview.copy();d=ImageDraw.Draw(grid)
    for x in range(0,2048,512):d.line((x,0,x,3071),fill='#b8c8c6')
    for y in range(0,3072,512):d.line((0,y,2047,y),fill='#b8c8c6')
    grid.save(ROOT/'previews/assembled-grid.png');review_image(grid,ROOT/'previews/assembled-review.jpg')
    frames={'village':[-458,-40,218,300],'forest-roads':[-600,350,600,1550],'north':[-2048,1250,2048,3584],'south':[-2048,-2560,2048,350],'north-road':[-500,1800,900,3200],'south-junction':[-1650,-1000,-650,0]}
    for name,b in frames.items():
        size=(1600,round(1600*(b[3]-b[1])/(b[2]-b[0])))
        new=render(b,size,layers);old=previous_render(b,size)
        new.save(ROOT/f'previews/{name}.png')
        comp=Image.new('RGB',(size[0]*2,size[1]+25),'#1e2020');comp.paste(old,(0,25));comp.paste(new,(size[0],25));d=ImageDraw.Draw(comp);d.text((10,6),'V3',fill='white');d.text((size[0]+10,6),'V4',fill='white');comp.save(ROOT/f'previews/{name}-comparison.png');review_image(comp,ROOT/f'previews/{name}-comparison-review.jpg')
    road_groups=[DATA['roads'][i:i+20] for i in range(0,len(DATA['roads']),20)]
    audit=[]
    for page,roads in enumerate(road_groups):
        sheet=Image.new('RGB',(1200,1000),'#1e2020');d=ImageDraw.Draw(sheet)
        for i,r in enumerate(roads):
            pts=r['points'];xs=[q[0] for q in pts];ys=[q[1] for q in pts]
            b=[max(B[0],min(xs)-10),max(B[1],min(ys)-10),min(B[2],max(xs)+10),min(B[3],max(ys)+10)]
            cx=(b[0]+b[2])/2;cy=(b[1]+b[3])/2;span=max(b[2]-b[0],b[3]-b[1],24);b=[cx-span/2,cy-span/2,cx+span/2,cy+span/2]
            old=road_overlay(previous_render(b,(140,170)),b,[r]);new=road_overlay(render(b,(140,170),layers),b,[r]);x=(i%4)*300;y=(i//4)*200
            sheet.paste(old,(x,y+24));sheet.paste(new,(x+150,y+24));d.text((x+3,y+6),r['id']+'  V3 | V4',fill='white')
            audit.append(dict(id=r['id'],page=page+1,bounds=b))
        sheet.save(ROOT/f'previews/roads-audit-{page+1}.png');review_image(sheet,ROOT/f'previews/roads-audit-{page+1}-review.jpg')
    (ROOT/'records/road-audit-index.json').write_text(json.dumps(audit,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'images':len(m['images']),'tiles':len(tiles),'road_audit_records':len(audit),'export':[4096,6144]}))

if __name__=='__main__':main()
