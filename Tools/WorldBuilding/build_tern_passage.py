"""Build the Veles–Tarnel–Roden river corridor in the original Veles metre grid."""
import hashlib
import json
import math
import re
from bisect import bisect_left
from pathlib import Path
from xml.etree import ElementTree as ET

from build_city_plans import ATLAS, ATLAS_DATA, GRID, W, H, X0, Y0, read, curve

OUT = ATLAS / "TernPassage"
CITIES = {c:read(ATLAS/"CityPlans"/c/"coordinates.json") for c in ("Tarnel","Roden")}
LEGS = (("VT","벨레스 → 타르넬","i-vt",363),("TR","타르넬 → 로덴","i-tr",593))
NAMES = {"west":"서안","east":"동안","channel":"테른 강 항로","track1":"강변길","track2":"강변길",
         "crossing":"수레 나루 도선","market":"시장 동문 연결길"}
STYLE = {"west":("#81896b",1.4),"east":("#81896b",1.4),"channel":("#d8e8d8",1.1),
         "crossing":("#5a6260",1.2)}
NS="{http://www.w3.org/2000/svg}"


def key(ij):
    return f"{ij[0]}_{ij[1]}"


def tile(p):
    return math.floor((p[0]-X0)/W),math.floor((p[1]-Y0)/H)


def corner(ij):
    return X0+ij[0]*W,Y0+ij[1]*H


def box(ij):
    return (*corner(ij),*corner((ij[0]+1,ij[1]+1)))


def legacy():
    result={}
    for name,t in GRID["tiles"].items():
        url="../02-veles-village.svg" if name=="center" else f"../VelesCardinalTiles/{name}.svg"
        result[tuple(t["offset"])]=["벨레스",url]
    for c,data in CITIES.items():
        for k in data["tiles"]:
            ij=tuple(map(int,k.split("_")))
            assert ij not in result
            result[ij]=[data["name"],f"../CityPlans/{c}/{k}.svg"]
    assert len(result)==246
    return result


def source_hashes():
    paths=[*ATLAS.glob("*.svg"),ATLAS/"coordinates.json",*(ATLAS/"CityPlans").rglob("*"),
           *(ATLAS/"VelesCardinalTiles").rglob("*")]
    return {p.relative_to(ATLAS).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(paths) if p.is_file()}


def cells_hit(ps):
    result=set()
    for p,q in zip(ps,ps[1:]):
        a,b=tile(p),tile(q)
        ts=[0,1]
        for axis,span,origin in ((0,W,X0),(1,H,Y0)):
            for n in range(min(a[axis],b[axis])+1,max(a[axis],b[axis])+1):
                ts.append((origin+n*span-p[axis])/(q[axis]-p[axis]))
        ts.sort()
        for lo,hi in zip(ts,ts[1:]):
            if hi-lo>1e-12:
                t=(lo+hi)/2
                result.add(tile(tuple(x+t*(y-x) for x,y in zip(p,q))))
    return result


def tube(ps):
    return {(i+a,j+b) for i,j in cells_hit(ps) for a in (-1,0,1) for b in (-1,0,1)}


def crop(p,q,b):
    lo,hi=0.,1.
    for axis in (0,1):
        v=q[axis]-p[axis]
        if abs(v)<1e-12:
            if not b[axis]<=p[axis]<=b[axis+2]:
                return None
        else:
            u=(b[axis]-p[axis])/v; w=(b[axis+2]-p[axis])/v
            lo=max(lo,min(u,w));hi=min(hi,max(u,w))
    if hi-lo<1e-10:
        return None
    return [tuple(p[i]+t*(q[i]-p[i]) for i in (0,1)) for t in (lo,hi)]


def clip_lines(ps,b):
    chunks=[]
    for a,z in zip(ps,ps[1:]):
        hit=crop(a,z,b)
        if hit:
            if chunks and math.dist(chunks[-1][-1],hit[0])<1e-7:
                chunks[-1].append(hit[1])
            else:
                chunks.append(hit)
    return chunks


def moved(p,v,t):
    return [a+b*t for a,b in zip(p,v)]


def unit(a,b):
    l=math.dist(a,b)
    return [(y-x)/l for x,y in zip(a,b)]


def veles_port(sheet,name,axis,edge):
    el=ET.parse(ATLAS/"VelesCardinalTiles"/(sheet+".svg")).find(f'.//*[@data-line="{name}"]')
    nums=list(map(float,re.findall(r'-?\d+(?:\.\d+)?',el.get("d"))))
    p=nums[:2]
    for j in range(2,len(nums),6):
        a,b,q=nums[j:j+2],nums[j+2:j+4],nums[j+4:j+6]
        if (p[axis]-edge)*(q[axis]-edge)<=0:
            def at(t):
                u=1-t
                return [u**3*x+3*t*u*u*y+3*t*t*u*z+t**3*w for x,y,z,w in zip(p,a,b,q)]
            lo,hi=0,1
            for _ in range(48):
                mid=(lo+hi)/2
                if (at(mid)[axis]-edge)*(p[axis]-edge)>0:lo=mid
                else:hi=mid
            pos=at(mid);v=unit(at(mid+1e-5),at(mid-1e-5))
            return {"world_m":pos,"outward_unit":v,"width_m":float(el.get("stroke-width")),
                    "tile":key(GRID["tiles"][sheet]["offset"]),"source":sheet+":"+name}
        p=q
    raise ValueError(name)


def cityport(city,name,inlet):
    matches=[s for s in CITIES[city]["boundary_connections"] if s["feature"]==name]
    return sorted(matches,key=lambda v:v["world_m"][1],reverse=inlet)[0]


def channel_ports(city,inlet):
    return [cityport(city,name,inlet) for name in ("서안","동안","테른 강 항로")]


def port_x(port,y):
    x,yp=port["world_m"];vx,vy=port["outward_unit"]
    return x+(y-yp)*vx/vy


def smoothstep(t):
    t=max(0,min(1,t))
    return t*t*(3-2*t)


def source_geo():
    s=(ATLAS/"TernBasin377/geography.js").read_text(encoding="utf-8")
    return json.loads(s[s.index("{"):s.rindex("}")+1])


class Passage:
    def __init__(self,config,geo,old):
        self.id,self.name,route,self.quota=config
        self.old=old;self.geo=geo
        vx,vy=ATLAS_DATA["frames"]["V"]["origin_km"]
        self.route=[((x-vx)*1000,(y-vy)*1000) for x,y in ATLAS_DATA["features"][route]["geometry"]]
        self.spine=cells_hit(self.route)
        self.seed=tube(self.route)-old.keys()
        assert len(self.seed)==self.quota
        self.ports={}
        if self.id=="VT":
            edge=box((-2,-2))[1]
            start=[veles_port("outer-sw","bank"+str(k),1,edge) for k in (0,1)]
            start.append({"world_m":[(start[0]["world_m"][0]+start[1]["world_m"][0])/2,edge],
                          "outward_unit":unit([0,0],[(sum(s["outward_unit"][0]/abs(s["outward_unit"][1]) for s in start)/2),-1]),
                          "width_m":None,"tile":"-2_-2","source":"양안 중점"})
            end=channel_ports("Tarnel",True)
            roads=[veles_port("outer-w","westpass",0,box((-2,0))[0]),cityport("Tarnel","상류 둑길",True)]
        else:
            start=channel_ports("Tarnel",False)
            end=channel_ports("Roden",True)
            roads=[cityport("Tarnel","하류 강변길",False),cityport("Roden","상류 수레길",True)]
        self.start,self.end,self.roadports=start,end,roads
        self.water=geo["water"];self.water_y=[r[0] for r in self.water]
        self.upper=max(s["world_m"][1] for s in start)
        self.lower=min(s["world_m"][1] for s in end)
        self.ferry=-10800 if self.id=="VT" else -40300
        self.samples=sorted({*range(math.floor(self.lower/7)*7-70,math.ceil(self.upper/7)*7+72,7),
            *[s["world_m"][1]+dy for s in start+end for dy in (-4,0,4)],self.ferry})
        self.rows=[(y,*self.section(y)) for y in self.samples]
        self.lines={}
        for n,k in enumerate(("west","east","channel"),1):
            self.lines[k]=[(r[n],r[0]) for r in reversed(self.rows)]
            self.ports[k]=[start[n-1],end[n-1]]
        self.roads()
        self.choose_tiles()

    def surveyed_banks(self,y):
        i=max(1,min(len(self.water)-1,bisect_left(self.water_y,y)))
        pre,nxt=self.water[i-1:i+1]
        t=(y-pre[0])/(nxt[0]-pre[0])
        return [pre[k]+t*(nxt[k]-pre[k]) for k in (1,2)]

    def section(self,y):
        samples=[self.surveyed_banks(y+d) for d in (-80,-40,0,40,80)]
        west,east=[sum(row[k]*weight for row,weight in zip(samples,(1,4,6,4,1)))/16 for k in (0,1)]
        bend=19*math.sin(y/146)+12*math.sin(y/301+1.4)
        west+=bend+7*math.sin(y/92)
        east+=bend+10*math.sin(y/125+1.1)
        original=[west,east,(west+east)/2]
        out=[]
        for k,x in enumerate(original):
            # Regional geography governs the interior; the legacy endpoint and tangent
            # govern the last metres on each approach.
            for port in (self.start[k],self.end[k]):
                dist=abs(y-port["world_m"][1])
                if dist<720:
                    f=smoothstep((dist-35)/685)
                    x=f*x+(1-f)*port_x(port,y)
            out.append(x)
        assert out[0]<out[2]<out[1],(y,out)
        return out

    def land_x(self,y,side):
        bank=self.section(y)[0 if side<0 else 1]
        spacing=36+14*math.sin(y/151)+9*math.sin(y/79)+14*(1+math.cos(y/629))
        return bank+side*spacing

    def track(self,from_y,to_y,side):
        n=max(1,math.ceil(abs(to_y-from_y)/9))
        ys=[from_y+(to_y-from_y)*j/n for j in range(n+1)]
        return [(self.land_x(y,side),y) for y in ys]

    def stub(self,port,length=38):
        p,v=port["world_m"],port["outward_unit"]
        return [moved(p,v,-3),p,moved(p,v,4),moved(p,v,length)]

    def roads(self):
        p,q=self.roadports
        a=self.stub(p);z=self.stub(q)
        f=self.ferry
        first_side=-1 if self.id=="VT" else 1
        second_side=-first_side
        y=self.upper-650
        if self.id=="VT":
            approach=[a[-1],(-734,36),(-825,-75),(-842,-250),(-830,-461),
                      (-862,-630),(-912,-827),(self.land_x(y,first_side),y)]
        else:
            approach=[a[-1],(self.land_x(self.upper-185,1),self.upper-185),
                      (self.land_x(y,1),y)]
        # Both road ends run onto a landing at the adjoining bank, never into open water.
        west,east,_=self.section(f)
        landing1=(west if first_side<0 else east,f)
        landing2=(east if first_side<0 else west,f)
        lane1=self.track(y,f+85,first_side)
        lane2=self.track(f-90,self.lower+(900 if self.id=="VT" else 1300),second_side)
        self.lines["track1"]=a[:3]+curve(approach,6)+lane1[1:]+curve([lane1[-1],(landing1[0]+first_side*17,f+19),landing1],4)[1:]
        self.lines["crossing"]=[landing1,landing2]
        if self.id=="VT":
            finish=self.track(lane2[-1][1],z[-1][1],1)
            for i,(x,y) in enumerate(finish):
                f=smoothstep((y-z[-1][1])/240)
                finish[i]=(x*f+port_x(q,y)*(1-f),y)
        else:
            # Approach the northern edge of the Roden plans on dry land.
            finish=[lane2[-1],(-37340,-47930),(-37810,-48200),(-38490,-48238),
                    (-39170,-48200),(-39970,-48258),(-40580,-48212),z[-1]]
        head=curve([landing2,(landing2[0]+second_side*18,self.ferry-26),lane2[0]],4)
        self.lines["track2"]=head+lane2[1:]+(finish if self.id=="VT" else curve(finish,6))[1:]+z[2::-1]
        self.ports["track1"]=[p];self.ports["track2"]=[q]
        if self.id=="VT":
            # The east market gate opens into another tile of this same corridor.
            gate=cityport("Tarnel","시장 동문길",True)
            s=self.stub(gate)
            dest=z[-1]
            bx,by=corner((-44,-72))
            self.lines["market"]=s[:3]+curve([s[-1],(bx+94,by+125),(bx+73,by+H+41),
                (bx-120,by+H+84),(bx-158,corner((-46,-70))[1]+63),dest],7)
            self.ports["market"]=[gate]

    def choose_tiles(self):
        required=set()
        for k,ps in self.lines.items():
            required.update(cells_hit(ps))
        # Keep the entire wet cross-section and a small land margin at each bank.
        for y,w,e,_ in self.rows:
            required.update(cells_hit([(w-10,y),(e+10,y)]))
        required-=self.old.keys()
        selected=self.seed|required
        def distance(ij):
            return min((ij[0]-a)**2+(ij[1]-b)**2 for a,b in self.spine)
        def connected(cells):
            seen=set();stack=[next(iter(cells))]
            while stack:
                k=stack.pop()
                if k in seen or k not in cells:continue
                seen.add(k);i,j=k;stack.extend([(i+1,j),(i-1,j),(i,j+1),(i,j-1)])
            return len(seen)==len(cells)
        # Prefer slimming the outer margin on the same reach; do not remove a spine tile.
        extras=required-self.seed
        def priority(k):
            near=min((k[0]-a)**2+(k[1]-b)**2 for a,b in extras) if extras else 0
            return (near,-distance(k),k)
        neighbors={"벨레스","타르넬"} if self.id=="VT" else {"타르넬","로덴"}
        continuous=selected|{k for k,(name,_) in self.old.items() if name in neighbors}
        for ij in sorted(selected-required-self.spine,key=priority):
            if len(selected)<=self.quota:break
            if connected(continuous-{ij}):
                selected.remove(ij);continuous.remove(ij)
        assert len(selected)==self.quota,(self.id,len(selected),len(required))
        self.tiles=selected
        self.swaps={"added":sorted(map(key,selected-self.seed)),"removed":sorted(map(key,self.seed-selected))}


def city_lines(city,ij):
    root=ET.parse(ATLAS/"CityPlans"/city/(ij+".svg"))
    result={}
    for el in root.findall(".//*[@data-feature]"):
        result[el.get("data-feature")]=[tuple(map(float,v.split(","))) for v in el.get("points").split()]
    return result


def old_route(name,features,offset):
    for k,ps in features.items():
        if k==name:
            return [(x+offset[0],y+offset[1]) for x,y in ps]
    raise KeyError(name)


def itinerary(legs,geo):
    t,r=legs
    # The old plates carry full, uncut metric streets; collect each named route once.
    roads={}
    for city in CITIES:
        lines={}
        for ij in CITIES[city]["tiles"]:
            for k,ps in city_lines(city,ij).items():
                lines.setdefault(k,ps)
        roads[city]=lines
    def route(city,name):
        return old_route(name,roads[city],CITIES[city]["offset"])
    def between(ps,p,q):
        def position(point):
            candidates=[]
            for n,(a,b) in enumerate(zip(ps,ps[1:])):
                dx,dy=b[0]-a[0],b[1]-a[1]
                t=max(0,min(1,((point[0]-a[0])*dx+(point[1]-a[1])*dy)/max(1e-12,dx*dx+dy*dy)))
                candidates.append((math.dist(point,(a[0]+t*dx,a[1]+t*dy)),n+t))
            return min(candidates)[1]
        i,j=position(p),position(q)
        middle=ps[math.floor(min(i,j))+1:math.ceil(max(i,j))]
        return [p]+(middle if i<=j else middle[::-1])+[q]
    def water(city,begin,end):
        return between(route(city,"테른 강 항로"),begin,end)
    def roadend(leg,k):
        return leg.roadports[k]["world_m"]
    channel_t=route("Tarnel","테른 강 항로")
    t_dock=[-12841,-19920];r_dock=[-39903,-49911]
    t_channel=min(channel_t,key=lambda p:abs(p[1]-t_dock[1]))
    boat=[between(geo["voyage"],geo["voyage"][0],t.start[2]["world_m"]),t.lines["channel"],
          water("Tarnel",t.end[2]["world_m"],t_channel),[t_channel,t_dock,t_channel],
          water("Tarnel",t_channel,r.start[2]["world_m"]),r.lines["channel"],
          water("Roden",r.end[2]["world_m"],[-39881.333,-49911]),[[-39881.333,-49911],r_dock]]
    path=next(p["points"] for p in geo["lines"] if p["id"]=="westpass")
    walk=[between(path,path[0],roadend(t,0)),t.lines["track1"],t.lines["crossing"],t.lines["track2"],
          between(route("Tarnel","상류 둑길"),roadend(t,1),[-12278,-19792]),
          route("Tarnel","곡물 수레길")[::-1],route("Tarnel","목재 검사길"),route("Tarnel","낮은 조사길"),
          route("Tarnel","어살 둑길"),between(route("Tarnel","하류 강변길"),[-13360,-21510],roadend(r,0)),
          r.lines["track1"],r.lines["crossing"],r.lines["track2"],
          between(route("Roden","상류 수레길"),roadend(r,1),[-40700,-48880]),
          between(route("Roden","외곽 점검길"),[-40700,-48880],[-40320,-49240]),
          route("Roden","조합과 작업구")[::-1]]
    # The sampled channels extend past the boundary to paint the full water cross-section.
    boat[1]=between(t.lines["channel"],t.start[2]["world_m"],t.end[2]["world_m"])
    boat[5]=between(r.lines["channel"],r.start[2]["world_m"],r.end[2]["world_m"])
    permitted=set(t.tiles)|r.tiles|set(t.old)
    out={};ink={}
    for mode,sections in (("boat",boat),("road",walk)):
        steps=[]
        regions=(["벨레스","VT","타르넬","타르넬","타르넬","TR","로덴","로덴"] if mode=="boat" else
                 ["벨레스","VT","VT","VT",*(["타르넬"]*6),"TR","TR","TR","로덴","로덴","로덴"])
        assert len(regions)==len(sections)
        ink[mode]=[]
        for region,ps in zip(regions,sections):
            segments=[]
            for p,q in zip(ps,ps[1:]):
                crossed=cells_hit([p,q])
                crossed=sorted(crossed,key=lambda ij:math.dist(p,crop(p,q,box(ij))[0]))
                for ij in crossed:
                    assert ij in permitted,(mode,ij)
                    owner=t.old[ij][0] if ij in t.old else ("VT" if ij in t.tiles else "TR")
                    if owner!=region:continue
                    a,b=crop(p,q,box(ij))
                    if not segments or math.dist(segments[-1][-1],a)>.01:segments.append([a])
                    segments[-1].append(b)
                    if not steps or steps[-1]!=ij:
                        steps.append(ij)
            ink[mode].extend(preview_points(segment,45) for segment in segments)
        assert all(abs(a[0]-b[0])+abs(a[1]-b[1])==1 for a,b in zip(steps,steps[1:])),mode
        out[mode]=list(map(key,steps))
    return out,ink


def preview_points(ps,spacing):
    out=[ps[0]]
    for p in ps[1:-1]:
        if math.dist(p,out[-1])>=spacing:out.append(p)
    return [[round(x,2),round(y,2)] for x,y in out+[ps[-1]]]


def build():
    from tern_passage_art import Drawing
    old=legacy();before=source_hashes()
    baseline=OUT/"baseline.json"
    if baseline.is_file():
        assert read(baseline)==before,"Original plates or their source data changed"
    geo=source_geo()
    legs=[Passage(cfg,geo,old) for cfg in LEGS]
    assert not legs[0].tiles.intersection(legs[1].tiles)
    # Verify that every port and the dry road are on the selected side of the boundary.
    for leg in legs:
        for name in ("track1","track2","market"):
            ps=leg.lines.get(name,[])
            for i,(x,y) in enumerate(ps):
                if leg.lower<=y<=leg.upper:
                    w,e,_=leg.section(y)
                    assert not w+.02<x<e-.02,(leg.id,name,i,x,y)
        for name,ports in leg.ports.items():
            for port in ports:
                p,v=port["world_m"],port["outward_unit"]
                assert tile(moved(p,v,.05)) in leg.tiles,(name,p)
    OUT.mkdir(exist_ok=True)
    index={key(ij):{"region":name,"url":url} for ij,(name,url) in old.items()}
    journey,ink=itinerary(legs,geo)
    meta={"size":[W,H],"origin":[X0,Y0],"tiles":index,"legs":{},"routes":journey,"paths":ink}
    for leg in legs:
        folder=OUT/leg.id;folder.mkdir(exist_ok=True)
        drawing=Drawing(leg,(tile,box,crop))
        for ij in sorted(leg.tiles):
            k=key(ij);url=leg.id+"/"+k+".svg"
            (OUT/url).write_text(drawing.sheet(ij),encoding="utf-8")
            index[k]={"region":leg.id,"url":url}
        meta["legs"][leg.id]={"name":leg.name,"swaps":leg.swaps,"ports":leg.ports,
            "overview":{k:preview_points(ps,95) for k,ps in leg.lines.items()},
            "places":[{"p":[x,y],"name":name} for x,y,name,_ in drawing.labels],
            "bridges":[p for p,_,_,_ in drawing.bridges]}
        print(leg.id,len(leg.tiles),"sheets;",len(drawing.bridges),"tributary bridges",flush=True)
    assert len(index)==1202 and before==source_hashes()
    (OUT/"atlas.js").write_text("const atlas = "+json.dumps(meta,ensure_ascii=False,separators=(",",":"))+";\n",encoding="utf-8")
    if not baseline.exists():
        baseline.write_text(json.dumps(before,indent=1),encoding="utf-8")


if __name__=="__main__":
    build()

