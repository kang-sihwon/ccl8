"""Metric tile composition for the Tern passage atlas."""
import math
import random
from bisect import bisect_left
from collections import defaultdict
from html import escape

from build_city_plans import W,H,X0,Y0,fmt,points


def poly(ps,fill,stroke="none",width=0):
    return f'<polygon points="{points(ps)}" fill="{fill}" stroke="{stroke}" stroke-width="{fmt(width)}"/>'


def line(ps,color,width,attrs=""):
    return f'<polyline points="{points(ps)}" fill="none" stroke="{color}" stroke-width="{fmt(width)}" {attrs}/>'


def caption(x,y,s,size):
    return f'<text x="{fmt(x)}" y="{fmt(y)}" font-size="{size}">{escape(str(s))}</text>'


def intersect(a,b,c,d):
    u,v=b[0]-a[0],b[1]-a[1];s,t=d[0]-c[0],d[1]-c[1]
    det=u*t-v*s
    if abs(det)<1e-9:return None
    x,y=c[0]-a[0],c[1]-a[1]
    f=(x*t-y*s)/det;g=(x*v-y*u)/det
    if 0<=f<1 and 0<=g<1:return (a[0]+f*u,a[1]+f*v)


def distance(p,a,b):
    u,v=b[0]-a[0],b[1]-a[1]
    t=max(0,min(1,((p[0]-a[0])*u+(p[1]-a[1])*v)/max(.0000001,u*u+v*v)))
    return math.hypot(p[0]-a[0]-t*u,p[1]-a[1]-t*v)


DEFS='''
<pattern id="wash" patternUnits="userSpaceOnUse" width="41" height="37"><rect width="41" height="37" fill="#cdc9a5"/><path d="M3 9l1 1 2 -1m21 4l2 1m7 15l2 -1m-24 4l1 -.5" stroke="#adae88" stroke-width=".5" fill="none"/></pattern>
<pattern id="pond" patternUnits="userSpaceOnUse" width="41" height="24"><rect width="41" height="24" fill="#84a7a0"/><path d="M1 5q5 1 12 .4m11 10q7 1.4 14 0" fill="none" stroke="#adc0b0" stroke-width=".65"/></pattern>
<pattern id="plough" patternUnits="userSpaceOnUse" width="3.2" height="5" patternTransform="rotate(-8)"><rect width="3.2" height="5" fill="#c1b17e"/><path d="M0 0v5" stroke="#9d9064" stroke-width=".86"/></pattern>
<g id="oak"><path d="M-.9 -.23Q-1.14 -.7 -.59 -.76Q-.35 -1.2 .05 -.94Q.65 -1.12 .79 -.64Q1.21 -.39 .91 .09Q1.01 .65 .48 .72Q.09 1.17 -.34 .89Q-1 .97 -.9 .41Q-1.26 .12 -.9 -.23Z" fill="currentColor" stroke="#697253" stroke-width=".095"/><path d="M-.67 -.34Q-.54 -.67 -.22 -.58Q-.03 -.86 .21 -.59M.26 .52Q.66 .33 .53 .12" stroke="#b4b58c" stroke-width=".1" fill="none"/></g>
<g id="fir"><path d="M0 -1.25L.36 -.48 .25 -.52 .61 .15 .42 .06 .8 .71 .17 .58 0 1.02 -.15 .59 -.75 .72 -.45 .12 -.6 .17 -.26 -.47 -.38 -.44Z" fill="#768266" stroke="#5d6b55" stroke-width=".08"/><path d="M0 -1.05v1.6l.4 .08" stroke="#a1a68a" fill="none" stroke-width=".09"/></g>
<g id="rock"><path d="M-1 -.3 -.45 -.79 .61 -.58 1 .17 .32 .61 -.62 .43Z" fill="#a39d84" stroke="#847e69" stroke-width=".12"/><path d="M-.63 -.3l.29 -.25 .69 .07" fill="none" stroke="#d1c8a6" stroke-width=".15"/></g>
<g id="rush"><path d="M0 0q-1.7 2 -2.2 2.2M0 0q.2 2.1 -.3 3.3M0 0l1.35 2.45" fill="none" stroke="#90916b" stroke-width=".38"/></g>
<g id="hut"><rect x="-4.6" y="-2.6" width="9.2" height="5.2" fill="#ab8b63" stroke="#74624b" stroke-width=".45"/><path d="M-4.6 0h9.2v2.6h-9.2Z" fill="#927659"/><path d="M-4.5 0h9" stroke="#e3c39a" stroke-width=".38"/></g>
<g id="boat"><path d="M-5 0Q-2 -2.5 3 -1.45L5 0Q2 2.1 -2 1.65Z" fill="#aa8961" stroke="#5a503c" stroke-width=".52"/><path d="M-2 -1v2m2 -2.3v2.5m2 -2.3v2" stroke="#6a5842" stroke-width=".4"/></g>'''


class Drawing:
    def __init__(self,leg,api):
        self.leg=leg
        self.tile,self.box,self.crop=api
        self.grid=defaultdict(lambda:defaultdict(list))
        self.nearby=defaultdict(list);self.bridges=[];self.labels=[]
        self.widths={"west":1.4,"east":1.4,"channel":1.1,"track1":leg.roadports[0]["width_m"],
                     "track2":leg.roadports[1]["width_m"],"crossing":2}
        if "market" in leg.lines:self.widths["market"]=leg.ports["market"][0]["width_m"]
        self.geometry=dict(leg.lines)
        for item in leg.geo["lines"]:
            if item["kind"]=="stream" and leg.lower<item["points"][-1][1]<leg.upper:
                self.geometry[item["id"]]=item["points"]
                self.widths[item["id"]]=item["width"]
        self.index()
        self.landings()
        self.stream_bridges()

    def index(self):
        for name,ps in self.geometry.items():
            for a,b in zip(ps,ps[1:]):
                if a==b:continue
                r=max(14,self.widths[name]/2+2)
                lo=self.tile((min(a[0],b[0])-r,min(a[1],b[1])-r))
                hi=self.tile((max(a[0],b[0])+r,max(a[1],b[1])+r))
                for i in range(lo[0],hi[0]+1):
                    for j in range(lo[1],hi[1]+1):
                        k=i,j
                        if k not in self.leg.tiles:continue
                        x,y,u,v=self.box(k)
                        hit=self.crop(a,b,(x-r,y-r,u+r,v+r))
                        if hit:
                            batches=self.grid[k][name]
                            if batches and math.dist(batches[-1][-1],hit[0])<.00001:
                                batches[-1].append(hit[1])
                            else:batches.append(hit)
                if name in ("track1","track2","market") or name not in self.leg.lines:
                    for u in range(math.floor((min(a[0],b[0])-28)/50),math.floor((max(a[0],b[0])+28)/50)+1):
                        for v in range(math.floor((min(a[1],b[1])-28)/50),math.floor((max(a[1],b[1])+28)/50)+1):
                            self.nearby[u,v].append((a,b,name))

    def landings(self):
        leg=self.leg
        water=leg.lines["crossing"]
        for n,(x,y) in enumerate(water):
            bank=1 if x>sum(p[0] for p in water)/2 else -1
            name=("버들여울" if leg.id=="VT" else "갈대굽이")+" 도선장"+(" · 동안" if bank>0 else " · 서안")
            self.labels.append((x,y,name,bank))
        ys=(-4300,-8200,-15500) if leg.id=="VT" else (-26800,-32800,-43700)
        names=("물굽이 쉼터","자갈턱 물표","둑길 짐마당") if leg.id=="VT" else ("둑 위 쉼터","모래턱 짐마당","제방 앞 쉼터")
        for y,name in zip(ys,names):
            road="track1" if y>leg.ferry else "track2"
            p=min(leg.lines[road],key=lambda v:abs(v[1]-y))
            self.labels.append((*p,name,0))

    def stream_bridges(self):
        for name,ps in self.geometry.items():
            if name in self.leg.lines:continue
            for a,b in zip(ps,ps[1:]):
                # Source survey segments are normally <60 m; inspect all crossed buckets.
                seen=set()
                for i in range(math.floor(min(a[0],b[0])/50),math.floor(max(a[0],b[0])/50)+1):
                    for j in range(math.floor(min(a[1],b[1])/50),math.floor(max(a[1],b[1])/50)+1):
                        for p,q,road in self.nearby[i,j]:
                            k=(tuple(p),tuple(q),road)
                            if road not in ("track1","track2","market") or k in seen:continue
                            seen.add(k)
                            hit=intersect(a,b,p,q)
                            if hit:
                                u,v=q[0]-p[0],q[1]-p[1];length=math.hypot(u,v)
                                angle=abs(u*(b[1]-a[1])-v*(b[0]-a[0]))/length/math.dist(a,b)
                                span=self.widths[name]/max(.08,angle)+6
                                self.bridges.append((hit,math.degrees(math.atan2(v,u)),span,self.widths[road]+1.4))

    def trees(self,ij):
        a,b,c,d=self.box(ij);out=[]
        for i in range(math.floor((a-13)/17),math.floor((c+13)/17)+1):
            for j in range(math.floor((b-13)/17),math.floor((d+13)/17)+1):
                rng=random.Random((i*92837111)^(j*689287499))
                x=(i+rng.random())*17;y=(j+rng.random())*17
                w,e,_=self.leg.section(y)
                if w<x<e:continue
                dist=min(abs(x-w),abs(x-e))
                nearest=self.nearby.get((math.floor(x/50),math.floor(y/50)),())
                if any(distance((x,y),p,q)<self.widths[k]/2+6 for p,q,k in nearest):continue
                grove=math.sin(x/114+y/195)+math.sin(y/146-x/190)*.8+math.cos(x/55+y/73)*.36
                if dist<10:
                    spec="rush" if (math.sin(y/244)>0)==(x<w) else "rock"
                    size=rng.uniform(.9,3)
                elif grove>-.13 or (grove>-.8 and rng.random()<.25):
                    spec="oak" if grove<1.05 or rng.random()<.48 else "fir"
                    size=rng.uniform(3.1,7.4)
                elif rng.random()<.18:
                    spec="rock";size=rng.uniform(.8,2.4)
                else:continue
                color=rng.choice(("#939b72","#8a946e","#a1a47e","#84916e"))
                out.append(f'<use href="#{spec}" color="{color}" transform="translate({fmt(x)} {fmt(y)}) rotate({rng.randrange(360)}) scale({size:.2f})"/>')
        return "".join(out)

    def sheet(self,ij):
        leg=self.leg;a,b,c,d=self.box(ij)
        tile_id=f"{ij[0]}_{ij[1]}"
        title=leg.name+"　/　"+tile_id.replace("_",", ")
        top=f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1180 1344" width="1180" height="1344" xml:lang="ko"><title>{title}</title><defs><clipPath id="frame"><rect x="48" y="86" width="1084" height="1160"/></clipPath>{DEFS}</defs><style>text{{font-family:'Malgun Gothic',sans-serif;fill:#403e31}}path,polyline,polygon{{stroke-linejoin:round;stroke-linecap:round}}</style><rect width="1180" height="1344" fill="#f1ebda"/>'''
        art=[top,caption(48,52,title,29),caption(1030,51,"북 ↑",19),'<g clip-path="url(#frame)">',
             f'<g id="metric" transform="translate({fmt(48-a*4.25)} {fmt(86+d*4.25)}) scale(4.25 -4.25)">',
             f'<rect x="{fmt(a-1)}" y="{fmt(b-1)}" width="{fmt(W+2)}" height="{fmt(H+2)}" fill="url(#wash)"/>']
        ys=[b-4,*[y for y in leg.samples[bisect_left(leg.samples,b-4):bisect_left(leg.samples,d+4)] if b-4<y<d+4],d+4]
        # Outside the surveyed ends, only road-spur sheets exist. The water surface ends with the reach.
        ys=sorted(set(max(leg.samples[0],min(leg.samples[-1],y)) for y in ys))
        if len(ys)>1:
            rows=[(y,*leg.section(y)) for y in ys]
            west=[(w,y) for y,w,e,m in rows];east=[(e,y) for y,w,e,m in rows]
            if min(p[0] for p in west)<c+65 and max(p[0] for p in east)>a-65:
                for n,bank in enumerate((west,east)):
                    sign=(-1,1)[n]
                    rim=[(x+sign*(15+6*math.sin(y/93+n)+4*math.sin(y/181)),y) for x,y in bank]
                    art.append(poly(bank+rim[::-1],"#d8caa1" if n==0 else "#c1bd93"))
                    for off in (12,25,49):
                        ps=[(x+sign*(off+math.sin(y/143+n)*off*.28+math.cos(y/78)*off*.14),y) for x,y in bank]
                        art.append(line(ps,"#afa889",.42))
                art.append(poly(west+east[::-1],"url(#pond)"))
                for f in (.12,.23,.76,.9):
                    art.append(line([(w+(e-w)*(f+.012*math.sin(y/102+f)),y) for y,w,e,_ in rows],"#9bb8ac",.65))
        for k,pss in self.grid[ij].items():
            if k in leg.lines:continue
            for ps in pss:art.append(line(ps,"#8ba9a1",self.widths[k],f'data-feature="{k}"'))
        art.append(self.trees(ij))
        for k in ("west","east","channel","track1","track2","market","crossing"):
            for ps in self.grid[ij].get(k,()):
                width=self.widths[k]
                color={"west":"#848168","east":"#848168","channel":"#d9e7d6","crossing":"#545f59"}.get(k,"#e2cba3")
                if k in ("track1","track2","market"):
                    art.append(line(ps,"#918166",width+1.1))
                dash='stroke-dasharray="5 4"' if k=="crossing" else 'stroke-dasharray="11 7"' if k=="channel" else ""
                art.append(line(ps,color,width,f'data-feature="{k}" data-width="{fmt(width)}" {dash}'))
        for (x,y),angle,span,width in self.bridges:
            if a-span<x<c+span and b-span<y<d+span:
                art.append(f'<g transform="translate({fmt(x)} {fmt(y)}) rotate({fmt(angle)})"><rect x="{-span/2:.2f}" y="{-width/2:.2f}" width="{span:.2f}" height="{width:.2f}" fill="#bd9d72" stroke="#78674d" stroke-width=".7"/>')
                for j in range(-int(span/2)+1,int(span/2),2):
                    art.append(f'<path d="M{j} {-width/2:.2f}v{width:.2f}" stroke="#948160" stroke-width=".27"/>')
                art.append("</g>")
        for x,y,name,side in self.labels:
            if not a-60<x<c+60 or not b-20<y<d+20:continue
            if side:
                art+=[poly([(x-2*side,y-2),(x+7*side,y-3),(x+8*side,y+3),(x-2*side,y+2)],"#aa9268","#73664d",.6),
                      f'<use href="#boat" transform="translate({fmt(x-side*5)} {fmt(y+2)}) rotate(78)"/>']
            art.append(f'<use href="#hut" transform="translate({fmt(x+(side or 1)*12)} {fmt(y+8)}) rotate(16)"/>')
            if a<=x<c and b<=y<d:
                tx=max(a+23,min(c-23,x));ty=max(b+6,min(d-6,y+15))
                art.append(f'<text transform="translate({fmt(tx)} {fmt(ty)}) scale(1 -1)" text-anchor="middle" font-size="4" stroke="#f4ead2" stroke-width="1.1" paint-order="stroke">{escape(name)}</text>')
        art+=['</g></g>']
        for n in range(math.ceil(a/50)*50,math.ceil(c/50)*50,50):
            art.append(caption(40+(n-a)*4.25,1267,n,12))
        for n in range(math.ceil(b/50)*50,math.ceil(d/50)*50,50):
            art.append(f'<text x="44" y="{fmt(86+(d-n)*4.25)}" text-anchor="end" font-size="11">{n}</text>')
        art.extend(['<path d="M48 1296h212.5m-212.5 -5v10m212.5 -10v10" stroke="#565747" stroke-width="2"/>',
                    caption(270,1301,"50 m",15),caption(760,1301,"벨레스 기준　동 X · 북 Y (m)",16),
                    f'<a href="../index.html?tile={tile_id}">{caption(48,1327,"연결도에서 이웃 타일 열기",14)}</a></svg>'])
        return "".join(art)

