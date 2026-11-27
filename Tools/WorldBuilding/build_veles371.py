"""Publish metric SVG sheets in independently loadable ZIP blocks."""
import base64
import io
import json
import math
from pathlib import Path
import subprocess
from zipfile import ZipFile, ZIP_DEFLATED, ZipInfo
from collections import defaultdict, Counter
from html import escape
import numpy as np
from PIL import Image

from build_tern_passage import ATLAS, CITIES, box, tile, crop, key, source_geo, curve, moved, W, H, X0, Y0, Passage, LEGS, legacy
from tern_passage_art import intersect

OUT = ATLAS/"Veles371"
R = 185
SIDE = 2*R+1
BLOCK = 16
PW, PH = 48, 52
FEATURES = defaultdict(list)
OVERPRINT = defaultdict(list)


def load_js(path):
    data = path.read_text(encoding="utf-8")
    return json.loads(data[data.index("{"):data.rindex("}")+1])


def inside(ij):
    return max(map(abs, ij)) <= R


def region(x, y, zones):
    xw = x+1000*np.sin(y/6800)+490*np.cos((x+y)/3600)
    yw = y+850*np.sin(x/5100)
    best = np.full_like(xw+yw, np.inf)
    result = np.zeros(best.shape, dtype=np.uint8)
    for i, (_, u, v, *_) in enumerate(zones):
        diff = (xw-u)**2+(yw-v)**2
        result = np.where(diff<best, i, result)
        best = np.minimum(diff,best)
    return result


def xy(points):
    return " ".join(f"{x:.3f},{y:.3f}" for x,y in points)


def stroke(points, color, width):
    return f'<polyline points="{xy(points)}" fill="none" stroke="{color}" stroke-width="{width}"/>'


def patch(points, color):
    return f'<polygon points="{xy(points)}" fill="{color}"/>'


def bins(bounds):
    i,j=tile(bounds[:2]); u,v=tile(bounds[2:])
    for a in range(max(-R,i),min(R,u)+1):
        for b in range(max(-R,j),min(R,v)+1):
            yield a,b


def line(points, style, layer, tag, transfer=False):
    parts = defaultdict(list)
    halo = max(w for _,w in style)/2+1
    for p,q in zip(points,points[1:]):
        if p==q: continue
        for ij in bins((min(p[0],q[0])-halo,min(p[1],q[1])-halo,max(p[0],q[0])+halo,max(p[1],q[1])+halo)):
            a,b,c,d=box(ij)
            hit=crop(p,q,(a-halo,b-halo,c+halo,d+halo))
            if not hit: continue
            chunks=parts[ij]
            if chunks and math.dist(chunks[-1][-1],hit[0])<.0001: chunks[-1].append(hit[1])
            else: chunks.append(hit)
    for ij,chunks in parts.items():
        ink=''.join(stroke(ps,color,w).replace('<polyline ',f'<polyline data-trace="{escape(tag)}" ',1)
                    if n==len(style)-1 else stroke(ps,color,w) for n,(color,w) in enumerate(style) for ps in chunks)
        record=(layer,tag,ink)
        FEATURES[ij].append(record)
        if transfer:
            OVERPRINT[ij].append(record)


def shape(ps, ink, layer, tag):
    if not ps:return
    xs,ys=zip(*ps)
    for ij in bins((min(xs)-1,min(ys)-1,max(xs)+1,max(ys)+1)):
        FEATURES[ij].append((layer,tag,ink))


def yard(x,y,w,h,angle):
    cs,sn=math.cos(angle),math.sin(angle)
    return [(x+dx*cs-dy*sn,y+dx*sn+dy*cs) for dx,dy in ((-w/2,-h/2),(w/2,-h/2),(w/2,h/2),(-w/2,h/2))]


def survey():
    g=source_geo()
    s=json.loads(subprocess.check_output(["node",str(Path(__file__).with_name("veles371_survey.cjs"))]))
    water=g["water"]
    # 수면과 양안을 따로 잘라 물 너비와 굽이를 보존한다.
    for i in range(0,len(water)-1,12):
        rows=water[i:i+13]
        left=[(w,y) for y,w,e in rows]; right=[(e,y) for y,w,e in rows]
        poly=left+right[::-1]
        shape(poly,patch(poly,"#87aaa1"),8,"테른 강")
    for k in (1,2):
        line([(r[k],r[0]) for r in water],[("#bcb897",3.2),("#848a71",.9)],9,"테른 강안")
    legs=[Passage(cfg,g,legacy()) for cfg in LEGS]
    roads={w["id"]:w for w in s["ways"]}
    incoming=roads["westpass"]["points"]
    join=legs[0].lines["track1"][3]
    keep=next(i for i,p in enumerate(incoming) if p[0]<-740)
    roads["westpass"]["points"]=curve([join,(-708,37),incoming[keep]],4)+incoming[keep+1:]
    market=roads["market"]["points"]
    stop=max(i for i,p in enumerate(market) if p[0]<-13950)
    approach=curve([market[stop],(-13670,-19924),(-13370,-19941),(-13128,-19918),(-12960,-19950),(-12871,-19920)],6)
    roads["market"]["points"]=market[:stop]+approach
    oldway=roads["millgate"]["points"]
    start=next(i for i,p in enumerate(oldway) if p[0]<-13930)
    hits=[]
    for i,(a,b) in enumerate(zip(oldway,oldway[1:])):
        if a[1]>-47600:continue
        for p,q in zip(legs[1].lines["track2"],legs[1].lines["track2"][1:]):
            if max(p[1],q[1])<min(a[1],b[1]) or min(p[1],q[1])>max(a[1],b[1]):continue
            h=intersect(a,b,p,q)
            if h:hits.append((i,h))
    i,h=hits[0]
    roads["millgate"]["points"]=curve([(-13370,-19941),(-13535,-20170),(-13690,-20580),oldway[start]],7)+oldway[start+1:i+1]+[h]
    line([(-12871,-19920),(-12841,-19920)],[("#625f4e",1.1)],14,"공동 나루 건너편 · 수레 도선",True)
    through={"westpass","grain","market","millgate"}
    for f in s["ways"]:
        w=f["width"]
        stream=f["kind"]=="stream"
        style=[("#b3ac8c",w+1.2),("#84a8a1",w)] if stream else [("#99886a",w+1.1),("#e2cba2",w)]
        line(f["points"],style,10 if stream else 12,f["name"],f["id"] in through)
    for bridge in s["crossings"]:
        if bridge["road"] in through and bridge["river"]=="H01":continue
        ferry=bridge["mode"]=="나루"
        line(bridge["points"],[("#667f78",2)] if ferry else [("#7b6e55",5.1),("#c5ae89",3.9)],13,bridge["mode"],bridge["road"] in through)
    for name in through:
        path=roads[name]["points"]
        hits=[]
        for i,(a,b) in enumerate(zip(path,path[1:])):
            for leg in legs:
                if not leg.lower<=a[1]<=leg.upper or not leg.lower<=b[1]<=leg.upper:continue
                for bank in (0,1):
                    def side(t):
                        x,y=[p+t*(q-p) for p,q in zip(a,b)]
                        return x-leg.section(y)[bank]
                    if side(0)*side(1)>=0:continue
                    lo,hi=0,1
                    for _ in range(28):
                        mid=(lo+hi)/2
                        if side(lo)*side(mid)>0:lo=mid
                        else:hi=mid
                    hits.append((i+mid,[p+mid*(q-p) for p,q in zip(a,b)]))
        hits.sort()
        assert len(hits)%2==0,(name,hits)
        for (f,p),(t,q) in zip(hits[::2],hits[1::2]):
            ferry=math.dist(p,q)>60
            line([p,*path[math.floor(f)+1:math.ceil(t)],q],[("#6c8481",2)] if ferry else [("#73664f",5),("#c5aa80",3.8)],14,"수레 도선" if ferry else "둑길 다리",True)
    # 로덴 원도의 남서 시장문은 이 범위에서 유일하게 회랑 밖으로 열린 도시 출입구다.
    port=next(p for p in CITIES["Roden"]["boundary_connections"] if p["feature"]=="서쪽 시장길")
    p,v=port["world_m"],port["outward_unit"]
    end=(-42820,-50510)
    line([moved(p,v,-3),p,moved(p,v,24)]+curve([moved(p,v,48),(-42290,-50445),(-42510,-50430),(-42680,-50515),end],6),
         [("#99886a",8.1),("#e2cba2",7)],12,"서쪽 시장길 · 교외 집재장")
    for dx,dy in ((7,9),(7,13),(7,17),(-6,-8)):
        ps=yard(end[0]+dx,end[1]+dy,12,2,0)
        shape(ps,patch(ps,"#977e5d"),15,"시장길 집재장")
    places=[["벨레스",0,0],["타르넬",-13000,-20000],["로덴",-40000,-50000]]
    for plan in s["localPlans"]:
        _,name,x,y,*_=plan["site"]
        places.append([name,x,y])
        for lane in plan["lanes"]:line(lane,[("#a39070",5.3),("#dfc9a1",4)],13,name+" 마을길")
        for u,v,w,h,angle in plan["fields"]:
            ps=yard(u,v,w,h,angle)
            shape(ps,patch(ps,"url(#field)"),11,name+" 경작지")
        for u,v,w,h,angle,ax,ay in plan["homes"]:
            garden=yard(u,v,w+4.5,h+5.5,angle)
            shape(garden,patch(garden,"#bbb294"),12,"집 마당")
            line([(u,v),(ax,ay)],[("#d4bf96",1.75)],13,"마당 진입")
            ps=yard(u,v,w,h,angle)
            shape(ps,patch(ps,"#a58463")+stroke([ps[0],ps[1]],"#695f4b",.65)+
                  stroke([((ps[0][0]+ps[3][0])/2,(ps[0][1]+ps[3][1])/2),((ps[1][0]+ps[2][0])/2,(ps[1][1]+ps[2][1])/2)],"#dbbd90",.58),15,name+" 집터")
        for label,u,v,r,kind in plan["land"]:
            ps=yard(u,v,r*1.7,r,0)
            shade={"tree":"#788963","well":"#6f9390","mouth":"#665f4e","chamber":"#787362"}.get(kind,"#a88c68")
            if kind=="tree":
                mark=f'<use xlink:href="#oak" transform="translate({u:.3f} {v:.3f}) scale({r/6:.2f})"/>'
            else:
                mark=f'<ellipse cx="{u:.2f}" cy="{v:.2f}" rx="{r:.2f}" ry="{r*.72:.2f}" fill="{shade}" stroke="#796e58" stroke-width=".6"/>'
            shape(ps,mark,16,label)
            shape([(u-28,v+r+2),(u+28,v+r+8)],f'<text transform="translate({u:.3f} {v+r+3:.3f}) scale(1 -1)" text-anchor="middle" font-family="Malgun Gothic,sans-serif" font-size="3.6" fill="#414636" stroke="#eae3c9" stroke-width=".8" paint-order="stroke">{escape(label)}</text>',17,label)
    for d in g["details"]:
        if d["kind"]=="point":
            places.append([d["name"],*d["p"][0]])
        if d["frame"]!="V":continue
        ps=d["p"]
        if d["kind"] in ("building","facility","rock","wood","water") and len(ps)>=3:
            col={"building":"#9e7e60","facility":"#b49e77","rock":"#a6a08a","wood":"#899776","water":"#86a8a0"}[d["kind"]]
            shape(ps,patch(ps,col),14,d["name"])
        if d["kind"] in ("stream","road","walk","haul","back","detour","retreat") and len(ps)>=2:
            line(ps,[("#85a7a1",3)] if d["kind"]=="stream" else [("#dacaab",2.6)],12,d["name"])
    return s["regions"],places,s


# 한 팔레트에서 각 토지 피복의 바탕색과 나무 그늘, 지형 음영을 함께 만든다.
COLORS=np.array([[189,186,150],[146,156,119],[122,141,105],[183,175,139],
                 [158,164,126],[185,174,138],[171,171,143],[167,157,126]],dtype=float)
PALETTE=np.clip((COLORS[:,None,:]+np.linspace(-20,18,16)[None,:,None]),0,255).astype('uint8').reshape(-1).tolist()
PALETTE+=([0]*(768-len(PALETTE)))


def noise(x,y,seed):
    a=np.floor(x).astype('int32')
    b=np.floor(y).astype('int32')
    def h(i,j):
        k=i.astype('uint32')*np.uint32(374761393)+j.astype('uint32')*np.uint32(668265263)+np.uint32(seed)
        m=(k^(k>>13))*np.uint32(1274126177)
        return ((m^(m>>16)) & 65535).astype('float32')/65535
    u=(x-a).astype('float32');v=(y-b).astype('float32')
    u=u*u*(3-2*u);v=v*v*(3-2*v)
    return (h(a,b)*(1-u)+h(a+1,b)*u)*(1-v)+(h(a,b+1)*(1-u)+h(a+1,b+1)*u)*v


def terrain(i,j,nx,ny,zones):
    x=(X0+(i+(np.arange(nx*PW)+.5)/PW)*W)[None,:]
    y=(Y0+(j+ny-(np.arange(ny*PH)+.5)/PH)*H)[:,None]
    # 모든 잡음과 능선은 월드 좌표에서 계산해서 블록이 바뀌어도 이어진다.
    z=region(x[:,::PW]+W*(.5-.5/PW),y[::PH,:]-H*(.5-.5/PH),zones)
    reg=region(x[:,::4],y[::4,:],zones).repeat(4,0).repeat(4,1)
    broad=noise(x/710,y/710,19)
    mid=noise(x/148,y/148,731)
    fine=noise(x/49,y/49,951)
    stand=.44*broad+.36*mid+.20*fine
    marsh=(reg==10)|(reg==12)
    rocky=(reg==8)|(reg==11)
    wood=stand>np.where(reg==3,.65,np.where(marsh,.59,.46))
    deep=wood&(stand>.59)
    kind=np.where(marsh,4,np.where(rocky,6,np.where(reg==9,7,0)))
    kind=np.where((reg==3)&(stand<.48),3,kind)
    kind=np.where(wood,1,kind);kind=np.where(deep,2,kind)
    crown=noise(x/10.7,y/9.4,715)
    grain=noise(x/19,y/18,123)
    slope=np.sin(x/370+y/514+4*broad)+.4*np.cos(y/143-x/250+mid*3)
    ridge=np.abs(np.sin(x/1130+y/700+3*mid+2*broad))
    hatch=(np.sin((x*.038+y*.024)+fine*3)>.85)&~wood
    shade=np.clip(7.7+2.8*slope+np.where(wood,(crown-.5)*18+(grain-.5)*4,(fine-.5)*8+(grain-.5)*5-1.7*hatch)-np.where(rocky&~wood,2.4*(ridge<.06),0),0,15).astype('uint8')
    return z,(kind.astype('uint8')*16+shade)


SYMBOLS = '''<g id="oak"><path d="M-6 .2Q-7 -2 -4.9 -3.3Q-4.8 -5.9 -2.5 -5.1Q-.1 -7.1 1.3 -5.2Q4.2 -6.2 4.6 -3.7Q7 -3.2 5.8 -.6Q7.1 1.7 4.9 3Q4.6 5.8 2 5Q.1 7 -1.9 5.2Q-4.9 6 -5 3.5Q-7.4 2.5 -6 .2Z" fill="#899571" stroke="#657456" stroke-width=".44"/><path d="M-4.7 1.2Q-5 3.1 -3.5 3Q-2.8 4.8 -1.2 4M.2 3Q2 4.6 3.2 2.6M.4 -.6Q2 -1.6 2.9 -.7" fill="none" stroke="#acb38a" stroke-width=".55"/></g>
<g id="fir"><path d="M.2 6.6 1.9 3.5 3.7 4.3 3.6 1.7 6.2 1.1 4.5 -.5 5 -3.1 2.6 -2.9 1.4 -5.5 -.4 -4.3 -2.6 -5.3 -2.8 -3.1 -5.3 -2.9 -4.3 -.8 -5.8 .9 -3.2 1.6 -3.4 4 -1.8 3.2Z" fill="#72826a" stroke="#5d6c54" stroke-width=".4"/><path d="M-3.2 1.6 -.4 .5 .2 5.2M-.4 .5 2.5 2.5M-.4 .5 2.1 -2.4" fill="none" stroke="#a2ad8c" stroke-width=".45"/></g>
<g id="copse"><use xlink:href="#oak" transform="translate(-2.7 1.4) scale(.77)"/><use xlink:href="#oak" transform="translate(3 -2) scale(.69)"/></g>
<g id="stone"><path d="M-3 -.3 -1.8 -2 1 -1.7 3 -.1 1.8 1.4 -.7 1.9Z" fill="#a6a18c" stroke="#827f6c" stroke-width=".3"/><path d="M-2.3 .2 -.8 1.4 1.6 .9" fill="none" stroke="#d0c8aa" stroke-width=".47"/></g>
<path id="reed" d="M-1 -.9q-.1 2 -1.8 3.3M-.6 -1q.3 2.1 .1 4.3M0 -1q1.6 1.9 2.1 3.5m.6 -3.7q1.1 1.7 2.1 2.1" fill="none" stroke="#858d68" stroke-width=".51"/>
<path id="grass" d="M-2 0q.6 -.8 1.7 -.8l1.1 1.3m.4 -.5 1 .5" fill="none" stroke="#a5a789" stroke-width=".46"/>'''


def vegetation(i,j,nx,ny,zones):
    a,b,_,_=box((i,j));_,_,c,d=box((i+nx-1,j+ny-1))
    step=19
    xs=np.arange(math.floor((a-22)/step),math.ceil((c+22)/step)+1,dtype='int64')[None,:]
    ys=np.arange(math.floor((b-22)/step),math.ceil((d+22)/step)+1,dtype='int64')[:,None]
    def random(seed):
        h=(xs*92837111 ^ ys*689287499 ^ seed).astype('uint32')
        h=(h^(h>>16))*np.uint32(2246822519)
        return ((h^(h>>13))&65535)/65536
    x=np.round((xs+random(863))*step,1);y=np.round((ys+random(4157))*step,1)
    stand=.44*noise(x/710,y/710,19)+.36*noise(x/148,y/148,731)+.2*noise(x/49,y/49,951)
    z=region(x,y,zones);wet=(z==10)|(z==12);rock=(z==8)|(z==11);seed=random(71)
    threshold=np.where(z==3,.65,np.where(wet,.59,.46))
    spec=np.where(stand>threshold-.04,np.where(seed<.17,2,np.where((seed>.49)&(stand>.55),1,0)),
            np.where(wet,4,np.where(rock&(seed<.63),3,5)))
    mask=(seed<np.clip((stand-threshold+.16)*4,.18,.96)) | (wet&(seed<.6)) | (rock&(seed<.52))
    angle=np.floor(random(8181)*8).astype(int)*45
    size=np.round(.74+random(8182)*.48,1)
    result=defaultdict(list);names=("oak","fir","copse","stone","reed","grass")
    for px,py,k,rot,scale in zip(x[mask],y[mask],spec[mask],angle[mask],size[mask]):
        ink=f'<use xlink:href="#{names[k]}" transform="translate({px} {py}) rotate({rot}) scale({scale})"/>'
        left,bottom=tile((px-10,py-10));right,top=tile((px+10,py+10))
        for col in range(max(i,left),min(i+nx-1,right)+1):
            for row in range(max(j,bottom),min(j+ny-1,top)+1):
                result[col,row].append(ink)
    return result


def sheet(i,j,rname,pixels,features,plants):
    # 7비트 팔레트 PNG는 물길·도로 벡터 아래의 토지 피복만 담는다.
    im=Image.frombytes('P',(PW,PH),pixels.tobytes())
    im.putpalette(PALETTE)
    mem=io.BytesIO();im.save(mem,format='PNG',bits=7,compress_level=2)
    img=base64.b64encode(mem.getvalue()).decode('ascii')
    a,b,c,d=box((i,j))
    ink=''.join(plants)+''.join(s for _,_,s in sorted(features,key=lambda v:v[0]))
    names=list(dict.fromkeys(s for _,s,_ in sorted(features,key=lambda v:-v[0])))
    legend=" · ".join(names[:3]) or rname
    return f'''<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="900" height="1056" viewBox="0 0 300 352" data-tile="{i}_{j}">
<title>{escape(rname)} ({i}, {j})</title><defs>{SYMBOLS}<clipPath id="frame"><rect x="22" y="26" width="{W}" height="{H}"/></clipPath><pattern id="field" patternUnits="userSpaceOnUse" patternTransform="rotate(11)" width="3" height="4"><rect width="3" height="4" fill="#b5a378"/><path d="M0 0v4" stroke="#d0bc8c" stroke-width="1"/></pattern></defs>
<rect width="300" height="352" fill="#efe9d8"/><g font-family="Malgun Gothic,sans-serif" fill="#444a3c"><text x="22" y="15" font-size="6">{escape(rname)}　 /　({i}, {j})</text><text x="276" y="15" text-anchor="end" font-size="5">북 ↑</text></g>
<g clip-path="url(#frame)"><image x="22" y="26" width="{W}" height="{H}" preserveAspectRatio="none" xlink:href="data:image/png;base64,{img}"/><g transform="translate({22-a:.6f} {26+d:.6f}) scale(1 -1)" stroke-linecap="round" stroke-linejoin="round">{ink}</g></g>
<g font-family="Malgun Gothic,sans-serif" fill="#454a3d" font-size="4.5"><text x="22" y="308">X {a:.1f} → {c:.1f} m　 /　Y {d:.1f} → {b:.1f} m</text><text x="22" y="320">{escape(legend[:48])}</text><path d="M22 335h50m-50 -1.5v3m50 -3v3" fill="none" stroke="#4f5644" stroke-width=".85"/><text x="76" y="337">50 m</text><text x="277" y="337" text-anchor="end">벨레스 세계 격자</text></g></svg>'''.encode('utf-8')


def build_block(args):
    bx,by,zones,originals=args
    i0,j0=bx*BLOCK-R,by*BLOCK-R
    nx,ny=min(BLOCK,R-i0+1),min(BLOCK,R-j0+1)
    reg,raster=terrain(i0,j0,nx,ny,zones)
    plants=vegetation(i0,j0,nx,ny,zones)
    output=io.BytesIO()
    with ZipFile(output,'w',compression=ZIP_DEFLATED,compresslevel=4) as archive:
        for jj in range(ny):
            for ii in range(nx):
                ij=i0+ii,j0+jj;k=key(ij)
                if k in originals:
                    data=(ATLAS/"TernPassage"/originals[k]["url"]).read_bytes()
                    extra=OVERPRINT.get(ij)
                    if extra:
                        a,b,c,d=box(ij)
                        top=142 if originals[k]["region"]=="벨레스" else 86
                        ink=''.join(v for _,_,v in sorted(extra,key=lambda a:a[0]))
                        overlay=f'<g id="regional-transfers"><defs><clipPath id="survey371"><rect x="48" y="{top}" width="1084" height="1160"/></clipPath></defs><g clip-path="url(#survey371)"><g transform="translate({48-a*4.25:.6f} {top+d*4.25:.6f}) scale(4.25 -4.25)" stroke-linejoin="round" stroke-linecap="round">{ink}</g></g></g>'
                        pos=data.rindex(b'</svg>')
                        data=data[:pos]+overlay.encode('utf-8')+data[pos:]
                else:
                    row=ny-1-jj
                    pixels=raster[row*PH:(row+1)*PH,ii*PW:(ii+1)*PW]
                    data=sheet(*ij,zones[reg[row,ii]][0],pixels,FEATURES.get(ij,()),plants.get(ij,()))
                info=ZipInfo(k+'.svg',date_time=(2026,1,1,0,0,0));info.compress_type=ZIP_DEFLATED
                archive.writestr(info,data,compresslevel=4)
    ident=f'{bx}_{by}'
    packed=base64.b64encode(output.getvalue()).decode('ascii')
    (OUT/"blocks"/(ident+'.js')).write_text(f'plateBlock("{ident}","{packed}");\n',encoding='ascii')
    return nx*ny


def overview(data,atlas):
    g=source_geo()
    a,b,_,_=box((-R,-R));_,_,c,d=box((R,R))
    l,s,r,t=g["limits"]
    relief=base64.b64encode((ATLAS/"TernBasin377/relief.webp").read_bytes()).decode("ascii")
    world=[f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="1800" height="{1800*(d-b)/(c-a):.4f}" viewBox="{a} {-d} {c-a} {d-b}"><title>벨레스 중심 371×371 유역도</title>',
           f'<image x="{l}" y="{-t}" width="{r-l}" height="{t-s}" preserveAspectRatio="none" xlink:href="data:image/webp;base64,{relief}"/><g transform="scale(1 -1)" stroke-linejoin="round" stroke-linecap="round">']
    for f in data["ways"]:
        water=f["kind"]=="stream"
        ps=f["points"]
        sampled=[ps[0]]
        for p in ps[1:-1]:
            if math.dist(p,sampled[-1])>=55:sampled.append(p)
        world.append(stroke(sampled+[ps[-1]],"#528481" if water else "#a68b62",190 if water else 105))
    world.append(stroke([((w+e)/2,y) for y,w,e in g["water"]],"#507e7b",320))
    for leg in atlas["legs"].values():
        world.append(stroke(leg["overview"]["channel"],"#416f73",210))
    world.append("</g></svg>")
    (OUT/"overview.svg").write_text("".join(world),encoding="utf-8")


def main():
    zones,places,survey_data=survey()
    atlas=load_js(ATLAS/"TernPassage/atlas.js")
    old={k:v for k,v in atlas["tiles"].items() if inside(tuple(map(int,k.split("_"))))}
    state={"radius":R,"block":BLOCK,"size":[W,H],"origin":[X0,Y0],"regions":zones,
           "places":[p for p in places if inside(tile(p[1:]))],"originals":old,
           "journeys":atlas["paths"],"counts":dict(Counter(p["region"] for p in old.values())),
           "overprints":[key(ij) for ij in OVERPRINT if key(ij) in old]}
    (OUT/"blocks").mkdir(exist_ok=True,parents=True)
    overview(survey_data,atlas)
    (OUT/"catalog.js").write_text("const world = "+json.dumps(state,ensure_ascii=False,separators=(',',':'))+";\n",encoding='utf-8')
    # 스레드별 처리 구역은 겹치지 않으며, 물길과 길의 좌표는 위에서 한 번만 확정한다.
    from concurrent.futures import ThreadPoolExecutor
    total=0
    with ThreadPoolExecutor(max_workers=4) as pool:
        for idx,num in enumerate(pool.map(build_block,((x,y,zones,old) for y in range(math.ceil(SIDE/BLOCK)) for x in range(math.ceil(SIDE/BLOCK))))):
            total+=num
            if (idx+1)%48==0: print(idx+1,"blocks",total,"sheets",flush=True)
    assert total==SIDE**2
    print(total,"SVGs",len(old),"originals",flush=True)


if __name__=='__main__':
    main()

