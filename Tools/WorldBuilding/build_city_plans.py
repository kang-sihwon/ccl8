"""Build the two independent Tern city plans from one metric drawing per city."""
import json
import math
import random
import re
import xml.etree.ElementTree as ET
from collections import defaultdict
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ATLAS = ROOT / "Docs/ArtDirection/EtherWorldMap/Atlas/VelesTarnelRoden-v1"
OUTPUT = ATLAS / "CityPlans"


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


ATLAS_DATA = read(ATLAS / "coordinates.json")
FEATURES = ATLAS_DATA["features"]
FRAMES = read(ATLAS.parents[1] / "Data/tarnel-roden-exploration-v1.json")["frames"]
GRID = read(ATLAS / "VelesCardinalTiles/tiles.json")
W, H = GRID["tile_size_m"]
X0, Y0 = GRID["tiles"]["center"]["bounds_xyxy"][:2]
CITIES = {
    "T": ("타르넬", "Tarnel", (100,-240,1000,510), [
        ("높은 시장단",1850,23,(777,225)), ("운반꾼 동네",900,14,(655,-115)),
        ("나루와 하역장",450,11,(317,35))]),
    "R": ("로덴", "Roden", (-1500,-650,600,850), [
        ("식료 시장과 주택가",10400,66,(-1160,-250)), ("조합과 공방 골목",7200,52,(-690,365)),
        ("부두 노동자 동네",4800,49,(-260,70)), ("작업장 외곽",1600,43,(-270,705))])
}
INK = ["#e2c99d","#d1bf98","#d6ba98","#bfbfa0"]


def fmt(n):
    return f"{n:.3f}".rstrip("0").rstrip(".") or "0"


def points(ps):
    return " ".join(f"{fmt(x)},{fmt(y)}" for x,y in ps)


def bbox(ps, pad=0):
    xs,ys = zip(*ps)
    return min(xs)-pad,min(ys)-pad,max(xs)+pad,max(ys)+pad


def overlaps(a,b):
    return a[0]<=b[2] and a[2]>=b[0] and a[1]<=b[3] and a[3]>=b[1]


def inside(x,y,ps):
    hit=False
    for (a,b),(c,d) in zip(ps,ps[1:]+ps[:1]):
        if (b>y)!=(d>y) and x<(c-a)*(y-b)/(d-b)+a:
            hit=not hit
    return hit


def area(ps):
    return abs(sum(a*d-c*b for (a,b),(c,d) in zip(ps,ps[1:]+ps[:1])))/2


def spans(poly,y):
    hits=sorted(a+(y-b)*(c-a)/(d-b) for (a,b),(c,d) in zip(poly,poly[1:]+poly[:1]) if (b<=y<d or d<=y<b))
    return list(zip(hits[::2],hits[1::2]))


def clip_plane(ps,nx,ny,offset):
    out=[]
    for a,b in zip(ps,ps[1:]+ps[:1]):
        da=nx*a[0]+ny*a[1]-offset
        db=nx*b[0]+ny*b[1]-offset
        if da<=0:
            out.append(a)
        if (da<0<db) or (db<0<da):
            t=da/(da-db)
            out.append((a[0]+t*(b[0]-a[0]),a[1]+t*(b[1]-a[1])))
    return out


def curve(knots,step=10):
    """Sample a shared Catmull-Rom spline, including every survey knot."""
    result=[]
    for i,(a,b) in enumerate(zip(knots,knots[1:])):
        p=knots[max(0,i-1)]
        q=knots[min(len(knots)-1,i+2)]
        count=max(1,math.ceil(math.dist(a,b)/step))
        for j in range(count):
            t=j/count
            result.append(tuple(((2*a[k])+(-p[k]+b[k])*t+(2*p[k]-5*a[k]+4*b[k]-q[k])*t*t+
                                 (-p[k]+3*a[k]-3*b[k]+q[k])*t*t*t)/2 for k in (0,1)))
    return result+[knots[-1]]


def lerp_rows(rows,y):
    for (a,x),(b,z) in zip(rows,rows[1:]):
        if y<=b:
            return x+(z-x)*(y-a)/(b-a)
    return rows[-1][1]


def lower_survey():
    root=ET.parse(ATLAS/"05-tarnel-lower.svg").getroot()
    def sample(path):
        nums=lambda s:list(map(float,re.findall(r"-?\d+(?:\.\d+)?",s)))
        pts=[]; p=prev=(0,0)
        for cmd,args in re.findall(r"([MLCSQZ])([^MLCSQZ]*)",path):
            v=nums(args)
            controls=list(zip(v[::2],v[1::2]))
            if cmd=="Z":
                break
            if cmd in ("M","L"):
                p=controls[-1];pts.append(p);prev=p;continue
            if cmd=="S":
                controls.insert(0,(2*p[0]-prev[0],2*p[1]-prev[1]))
            chain=[p]+controls
            for j in range(1,25):
                t=j/24; work=chain
                while len(work)>1:
                    work=[(a+(c-a)*t,b+(d-b)*t) for (a,b),(c,d) in zip(work,work[1:])]
                pts.append(work[0])
            p=controls[-1];prev=controls[-2]
        return [(x*.612-650,-1018-y*.612) for x,y in pts]
    def path(k):
        return root.find(f'.//*[@id="{k}"]')[0].get("d")
    east,west=path("river").split(" L")
    return ([sample("M"+west),sample(east)],{k:sample(path(k)) for k in ("from-A10","A07-to-A08","toward-A09")})


def draw_path(ps,color,width,extra=""):
    return f'<polyline points="{points(ps)}" fill="none" stroke="{color}" stroke-width="{fmt(width)}" {extra}/>'


def polygon(ps,color,outline="none",width=.5):
    return f'<polygon points="{points(ps)}" fill="{color}" stroke="{outline}" stroke-width="{fmt(width)}"/>'


def label(p,s,size=6,color="#332f27"):
    x,y=p
    return (f'<text transform="translate({fmt(x)} {fmt(y)}) scale(1 -1)" text-anchor="middle" '
            f'font-size="{size}" fill="{color}" stroke="#f5ecd3" stroke-width="{size*.22:.2f}" '
            f'paint-order="stroke">{escape(s)}</text>')


def tile_name(ij):
    return f"{ij[0]}_{ij[1]}"


class CorridorIndex:
    def __init__(self):
        self.cells=defaultdict(list)

    def keys(self,box):
        a,b,c,d=box
        for i in range(math.floor(a/40),math.floor(c/40)+1):
            for j in range(math.floor(b/40),math.floor(d/40)+1):
                yield i,j

    def add(self,ps,r):
        for a,b in zip(ps,ps[1:]):
            for key in self.keys(bbox([a,b],r)):
                self.cells[key].append((a,b,r))

    def near(self,p,r):
        x,y=p
        for key in self.keys(bbox([p],r)):
            for (a,b),(c,d),w in self.cells.get(key,()):
                dx,dy=c-a,d-b
                t=max(0,min(1,((x-a)*dx+(y-b)*dy)/max(.00001,dx*dx+dy*dy)))
                if (x-a-t*dx)**2+(y-b-t*dy)**2<(r+w)**2:
                    return True
        return False


class Plan:
    def __init__(self,code):
        self.code=code
        self.t=code=="T"
        self.name,self.slug,self.urban,self.districts=CITIES[code]
        self.out=OUTPUT/self.slug
        self.out.mkdir(parents=True,exist_ok=True)
        self.features={k:v for k,v in FEATURES.items() if v["frame"]==code}
        v=ATLAS_DATA["frames"]["V"]["origin_km"]
        self.delta=[1000*(a-b) for a,b in zip(FRAMES[code]["anchor"],v)]
        self.core=self.cover(self.urban)
        ring={(i+x,j+y) for i,j in self.core for x in (-1,0,1) for y in (-1,0,1)}
        self.tiles=ring|self.cover(FRAMES[code]["bounds"])
        assert (len(self.core),len(ring),len(self.tiles))==((20,42,97) if self.t else (70,108,124))
        self.ext=bbox([p for ij in self.tiles for p in self.corners(ij)])
        self.layers=[]
        self.lines={}
        self.stops={}
        self.trips=[]
        self.roads=CorridorIndex()
        self.solids=CorridorIndex()
        self.housecount=0
        self.rng=random.Random(517 if self.t else 941)

    def tile(self,p):
        return (math.floor((p[0]+self.delta[0]-X0)/W),
                math.floor((p[1]+self.delta[1]-Y0)/H))

    def cover(self,r):
        a,b=self.tile(r[:2]); c,d=self.tile((r[2]-1e-7,r[3]-1e-7))
        return {(i,j) for i in range(a,c+1) for j in range(b,d+1)}

    def corners(self,ij):
        i,j=ij
        x=X0+i*W-self.delta[0]; y=Y0+j*H-self.delta[1]
        return [(x,y),(x+W,y+H)]

    def add(self,level,ps,svg,pad=0,overview=True):
        self.layers.append((level,bbox(ps,pad),svg,overview))

    def road(self,key,ps,width=6,smooth=True):
        ps=curve(ps,5) if smooth else ps
        self.lines[key]=(ps,width,"길")
        self.roads.add(ps,width/2)
        self.add(5,ps,draw_path(ps,"#908065",width+1.5)+draw_path(ps,"#e6d4ac",width,
                 f'data-feature="{escape(key)}" data-width="{width}"'),width)
        return ps

    def mark(self,key,name,p):
        self.stops[key]={"name":name,"point":p,"tile":tile_name(self.tile(p))}

    def terrain(self):
        # Local quay plans pin the occupied bank at the pier. Between surveyed reaches,
        # the two banks have independent bends.
        if self.t:
            east=[(-4000,-1190),(-2100,-590),(-1750,-515),(-1510,-380),(-1310,-340),(-1170,-290),
                  (-950,-260),(-812.5,-235),(-650,-169),(-470,-114),(-210,-49),(0,15),(40,87),(80,159),
                  (120,231),(150,285),(300,576),(400,746),(550,890),(900,1150),(3000,3800)]
            west=[(-4000,-1278),(-2100,-681),(-1750,-570),(-1510,-480),(-1310,-435),(-1170,-379),
                  (-950,-325),(-812.5,-277),(-650,-232),(-470,-174),(-210,-90),(0,-16),(40,57),(80,129),
                  (120,201),(150,255),(300,510),(400,680),(550,814),(900,1080),(3000,3700)]
            surveyed,self.lower_paths=lower_survey()
            for knots,samples in zip((west,east),surveyed):
                rows=sorted((y,x) for x,y in samples)
                knots[:]=sorted([p for p in knots if p[0]<rows[0][0] or p[0]>rows[-1][0]]+rows)
        else:
            west=[(-4000,-5700),(-1400,-2170),(-800,-1320),(-650,-1040),(-400,-652),(-180,-331),
                  (0,-22),(49,43.6667),(89,97),(129,150.3333),(270,335),(400,504),(519,676.6667),
                  (559,730),(599,783.3333),(800,1030),(1100,1410),(3000,3740)]
            east=[(-4000,-5610),(-1400,-2085),(-800,-1220),(-650,-950),(-400,-562),(-180,-280),
                  (0,24),(49,87),(89,140.3333),(129,193.6667),(270,391),(400,567),(519,707.3333),
                  (559,760.6667),(599,814),(800,1100),(1100,1480),(3000,3850)]
        self.banks=[]
        for knots in (west,east):
            # Hermite interpolation at a shared northing; exact knot positions retained.
            rows=[]
            for i,((a,x),(b,z)) in enumerate(zip(knots,knots[1:])):
                last,after=knots[max(0,i-1)],knots[min(len(knots)-1,i+2)]
                m=(z-last[1])/(b-last[0]); n=(after[1]-x)/(after[0]-a)
                count=math.ceil((b-a)/6)
                for j in range(count):
                    t=j/count
                    rows.append((a+(b-a)*t,(2*t**3-3*t*t+1)*x+(t**3-2*t*t+t)*(b-a)*m+
                                 (-2*t**3+3*t*t)*z+(t**3-t*t)*(b-a)*n))
            rows.append(knots[-1])
            self.banks.append(rows)
        lines=[[(x,y) for y,x in rows if self.ext[1]-50<=y<=self.ext[3]+50] for rows in self.banks]
        water=lines[0]+lines[1][::-1]
        self.add(3,water,polygon(water,"url(#water)"))
        for side,ps in zip(("서안","동안"),lines):
            self.lines[side]=(ps,0,"강안")
            self.add(4,ps,draw_path(ps,"#688c85",1.2,f'data-feature="{side}" data-width="0"'),2)
        if self.t:
            ps=curve(self.features["T-stream"]["geometry"],4)
            self.add(3,ps,draw_path(ps,"#9aa88e",8)+draw_path(ps,"#8eb2ab",5),5)
            self.roads.add(ps,4)
        self.shore=self.banks[1 if self.t else 0]
        mid=[]
        for y in range(math.floor(self.ext[1])-12,math.ceil(self.ext[3])+13,6):
            west,east=self.bank(y,0),self.bank(y,1)
            mid.append(((west+east)/2,y))
        self.lines["테른 강 항로"]=(mid,0,"항로")
        self.add(4,mid,draw_path(mid,"#abc7bc",.85,'stroke-dasharray="16 10" data-feature="테른 강 항로" data-width="0"'),1)

    def bank(self,y,side=None):
        from bisect import bisect_left
        rows=self.shore if side is None else self.banks[side]
        i=max(1,min(len(rows)-1,bisect_left(rows,(y,-math.inf))))
        return lerp_rows(rows[i-1:i+1],y)

    def town(self):
        x0,y0,x1,y1=self.urban
        target=sum(d[2] for d in self.districts)*10000
        self.rows=[]
        for i in range(151):
            v=i/150; y=y0+(y1-y0)*v
            end=abs(2*v-1)**7
            if self.t:
                a=max(x0+4*math.sin(14*v)**2,self.bank(y)+2)
                b=x1-5*math.sin(12*v)**2-11*end
            else:
                a=x0+21*math.sin(15*v)**2+52*end
                b=min(x1-13*math.sin(19*v)**2,self.bank(y)-2)
            self.rows.append((y,a,b))
        def boundary():
            return [(a,y) for y,a,b in self.rows]+[(b,y) for y,a,b in reversed(self.rows)]
        factor=target/area(boundary())
        # Keep the quays fixed; trim the inland edge to the planning acreage.
        assert .6<factor<=1.08,factor
        factor=min(1,factor)
        self.rows=[(y,a,a+(b-a)*factor) if self.t else (y,b-(b-a)*factor,b) for y,a,b in self.rows]
        self.boundary=boundary()
        self.measured=area(self.boundary)/10000
        sites=[d[3] for d in self.districts]
        weights=[0.]*len(sites)
        def cell(i):
            poly=self.boundary
            x,y=sites[i]
            for j,(u,v) in enumerate(sites):
                if i!=j:
                    poly=clip_plane(poly,2*(u-x),2*(v-y),u*u+v*v-x*x-y*y+weights[i]-weights[j])
            return poly
        for _ in range(180):
            errors=[d[2]*10000*area(self.boundary)/target-area(cell(i)) for i,d in enumerate(self.districts)]
            if max(map(abs,errors))<30:
                break
            weights=[w+.28*e for w,e in zip(weights,errors)]
        self.cells=[cell(i) for i in range(len(sites))]
        for i,ps in enumerate(self.cells):
            self.add(2,ps,polygon(ps,INK[i],"#a3906d",.9))
        self.add(4,self.boundary,draw_path(self.boundary+[self.boundary[0]],"#736f50",1.2,'stroke-dasharray="5 3"'),2)

    def strip(self,u,v):
        t=min(1,max(0,v))*150
        i=min(149,int(t)); f=t-i
        p=self.rows[i]; q=self.rows[i+1]
        a=p[1]+f*(q[1]-p[1]); b=p[2]+f*(q[2]-p[2])
        return (a+(b-a)*u,p[0]+f*(q[0]-p[0]))

    def routes(self):
        for key,f in self.features.items():
            if len(key)==3 and key[0] in "AB" and key[1:].isdigit():
                self.mark(key,f["name"],f["geometry"])
        def g(k):
            return self.features[k]["geometry"]
        def trip(name,keys,state=""):
            self.trips.append({"name":name,"stops":keys.split(),"state":state})
        def link(key,knots,width=6,smooth=True):
            return self.road(key,knots,width,smooth)
        if self.t:
            self.mark("LAND","나룻배 접안",g("T-LAND"))
            self.mark("STORE","곡물 창고",g("STORE"))
            self.mark("MARKET","높은 단 곡물 시장",(722,208))
            self.mark("TIMBER","검수목 적치 마당",(680,-140))
            link("선적길",g("t-loadway"),5,False)
            link("곡물 수레길",[g("A01"),(401,89),(436,115),(482,119),(536,151),(595,163),(657,190),(722,208)],8)
            link("목재 검사길",[g("A01"),(387,52),(414,8),(475,-61),(491,-118),g("A02")],9)
            link("적치장 진입",[g("A02"),(608,-171),(680,-140)],8)
            approach=self.lower_paths["from-A10"]; quay=self.lower_paths["toward-A09"]
            link("낮은 조사길",curve([g("A02"),(551,-296),(485,-420),g("A03"),(449,-814),g("A10"),
                              (315,-1062),(103,-1104),approach[0]],5)+approach[1:],5,False)
            low=self.lower_paths["A07-to-A08"];low[0]=g("A07");low[-1]=g("A08")
            link("어살 둑길",low,3.5,False)
            link("높은 귀환길",[g("A08")]+quay[1:]+curve([quay[-1],(230,-1455),(475,-1442),g("A09"),
                            (865,-1120),(943,-731),g("A04"),(785,-366),g("A02")],5)[1:],6,False)
            link("수액 숲길",[g("A04"),(1128,-674),g("A05"),(1150,-1110),g("A06"),(933,-1370),g("A09")],3)
            link("시장 동문길",[(722,208),(855,234),(992,255),(1187,240),(1420,303),(1900,338)],7)
            link("상류 둑길",[(722,208),(797,294),(863,371),(914,485),(1047,602),(1150,771),(1310,1000)],5)
            link("하류 강변길",[g("A08"),(-378,-1580),(-425,-1685),(-470,-1810),(-520,-1930),(-560,-2050)],4)
            trip("나루에서 높은 시장까지","LAND A01 STORE MARKET")
            trip("검수와 목재 적치","A01 A02 TIMBER")
            trip("어살에서 쉼나루로, 높은 길로 귀환","A01 A02 A03 A10 A07 A08 A09 A04 A02")
        else:
            for k,src,name in [("QUAY","R-QUAY","공공 접안"),("SUPPLY","r-supply","급수·보온 배분소"),
                               ("GATE","R-GATE","들것용 수동문"),("MOOR","R-MOOR","구조선 접안")]:
                p=g(src)
                if isinstance(p[0],list):
                    p=[sum(v[i] for v in p)/len(p) for i in (0,1)]
                self.mark(k,name,p)
            self.mark("STORE","제방 곡물 창고",(32,103))
            self.mark("MARKET","안쪽 식료 시장",(-1050,-160))
            link("하역 연결",g("r-unload"),6,False)
            link("창고 진입",[g("B01"),(31,96),(32,103)],5)
            link("식료 수레길",[(32,103),(12,99),(-39,112),(-113,83),(-258,64),(-399,26),(-537,-5),
                              (-718,-31),(-872,-99),(-1050,-160)],9)
            link("조합과 작업구",g("r-publicwalk"),7)
            link("생활 급수길",[g("B02"),(-634,394),self.stops["SUPPLY"]["point"]],3)
            link("외곽 점검길",g("drain"),5)
            link("점검 수레길",g("cartway"),7)
            link("제방 귀환길",g("levee"),7)
            link("구조선 연결",g("r-pier-link"),5,False)
            for key in ("stairs","foot","bypass","exit","r-stretcher"):
                link(key,g(key),3 if key!="stairs" else 1.5,False)
            link("서쪽 시장길",[(-1050,-160),(-1160,-205),(-1365,-247),(-1515,-327),(-1800,-352),(-2200,-430)],7)
            link("상류 수레길",[g("B04"),(-813,1260),(-875,1410),(-780,1640),(-845,2070)],5)
            trip("부두에서 창고와 식료 시장까지","QUAY B01 STORE MARKET")
            trip("조합의 급수와 작업구","B01 B02 SUPPLY B03")
            trip("사고 후 보행 우회","B07 B12 B09","closed")
            trip("문 개방 뒤 들것 이송","B08 GATE B09 B10 MOOR","rescued")
            trip("구조선 출발 뒤 조합 귀환","B10 B11 B01 B02","rescued")
            self.trips[-1]["guide"]="린이 부상자와 함께 떠나면 주인공은 육지에 남는다. B11에서 한네를 지나 B01에서 보렌을 다시 보고 B02 조합으로 오른다."
            self.trips[-1]["notes"]={
                "B10":"가넬은 열린 길을 확인해 배를 요청한다. 접안과 출발은 린이 판단하고, 주인공은 배를 바라본 뒤 제방으로 향한다.",
                "B11":"제방 경비 한네가 수레와 보행자의 통행을 나눈다. 작업구 입구에서 점검조를 확인하는 사비는 B03에 있다.",
                "B01":"보렌이 낮은 받침에 앉아 두 번째 짐을 가리킨다. 이번에는 주인공이 먼저 어깨로 짐을 나르는 사람에게 길을 비킨다.",
                "B02":"계단에서는 에브라가 급수와 보온 몫을 남긴다. 접수대에서는 베딕이 마렌의 우회 승인과 디셀의 목격을 되읽는다. 다음 교대자가 확인 못 한 빈 줄까지 읽고 종이를 받아 가면 베딕의 대표 위임이 끝난다. 디셀이 떠난 뒤 주인공과 마렌은 조합 작업대(R-GUILD)에서 각자 본 선이 만나는 자리를 도면에 그린다."}
        for k,f in self.features.items():
            ps=f["geometry"]
            if f["kind"] in ("facility","building","pad","rock") and len(ps)>=3:
                self.solids.add(ps+[ps[0]],2)
                self.add(7,ps,polygon(ps,"#ad8b69" if f["kind"]=="building" else "#c5ac84","#6e624d",.8))
        anchors=([("나루 경비소",(348,63)),("검수 망대",(549,-169))] if self.t else
                 [("나루 경비소",(52,78)),("제방 망루",(66,88)),("작업구 경비실",(-332,750))])
        for name,(x,y) in anchors:
            ps=[(x-3,y-3),(x+3,y-3),(x+3,y+3),(x-3,y+3)]
            self.solids.add(ps+[ps[0]],2)
            self.add(9,ps,polygon(ps,"#8e7050","#594735",.9)+label((x,y+5),name,4),16,False)
        for s in self.stops.values():
            self.solids.add([s["point"],s["point"]],9)
        # Put market activity on a paved square, with the road entering through its centre.
        for key in ("MARKET","TIMBER") if self.t else ("MARKET",):
            x,y=self.stops[key]["point"]
            ps=[(x-29,y-14),(x+26,y-17),(x+31,y+8),(x+12,y+19),(x-26,y+15)]
            self.add(4,ps,polygon(ps,"url(#paving)","#a48d66",1))
            self.solids.add([(x-23,y),(x+23,y)],16)
            for k in range(5):
                a=x-18+k*9
                small=[(a,y+6),(a+7,y+6),(a+7,y+11),(a,y+11)]
                self.add(7,small,polygon(small,"#a87756" if key=="MARKET" else "#8c764d","#745c45",.6))
        ps=self.features["A03"]["geometry"] if self.t else None
        if ps:
            x,y=ps
            self.add(7,[ps],f'<g transform="translate({x} {y}) rotate(-7)"><rect x="-2.4" y="-6" width="4.8" height="12" fill="url(#timber)" stroke="#6d583c"/></g>',8)

    def streets(self):
        rows=11 if self.t else 25
        cols=10 if self.t else 23
        # Warped blocks follow the bank. Alternating short cross streets open into courts.
        def node(i,j):
            u=.045+.91*i/cols; v=.035+.93*j/rows
            u+=(.28*math.sin(j*.66+i*.57)+.13*math.sin(i*1.71+j*.37))/cols
            v+=(.31*math.sin(i*.61+j*.71)+.17*math.cos(j*1.19-i*.41))/rows
            return self.strip(u,v)
        self.frontages=[]
        def frontage(name,knots,width):
            # Keep generic residential lanes out of the controlled works and out of existing yards.
            chunks=[]; run=[]
            for x,y in curve(knots,4):
                restricted=not self.t and inside(x,y,[(-330,753),(110,548),(455,510),(630,545),(680,870),(-260,990)])
                if restricted or self.solids.near((x,y),width/2+.8):
                    if len(run)>3:
                        chunks.append(run)
                    run=[]
                else:
                    run.append((x,y))
            if len(run)>3:
                chunks.append(run)
            for n,ps in enumerate(chunks):
                self.frontages.append(self.road(f"{name} · {n+1}",ps,width,False))
        for j in range(rows+1):
            knots=[node(i,j) for i in range(cols+1)]
            frontage(f"안길 {j+1}",knots,4.8 if j%4 else 6.8)
        for i in range(cols+1):
            runs=[];run=[]
            for j in range(rows+1):
                run.append(node(i,j))
                if j==rows or (1<j<rows-1 and (j+3*i)%7==0 and i%4):
                    if len(run)>1:
                        runs.append(run)
                    run=[]
            for k,run in enumerate(runs):
                frontage(f"골목 {i+1}-{k+1}",run,3.5 if i%4 else 6)
        # Connect the network at named destinations instead of relying on coincidental crossings.
        for key in (("MARKET","A01","A02") if self.t else ("MARKET","B01","B02","B03")):
            p=self.stops[key]["point"]
            q=min((q for lane in self.frontages for q in lane),key=lambda q:math.dist(p,q))
            self.road(key+" 마당길",[p,q],4,False)
        for lane in self.frontages:
            next_at=8; walked=0
            for p,q in zip(lane,lane[1:]):
                length=math.dist(p,q)
                if length<.01:
                    continue
                walked+=length
                if walked<next_at:
                    continue
                next_at=walked+self.rng.uniform(12,21 if self.t else 16)
                ux,uy=(q[0]-p[0])/length,(q[1]-p[1])/length
                for side in (-1,1):
                    nx,ny=-uy*side,ux*side
                    a=self.rng.uniform(3,5.8); b=self.rng.uniform(3.2,6.3)
                    setback=self.rng.uniform(3.8,7)
                    x=p[0]+nx*(setback+b); y=p[1]+ny*(setback+b)
                    if self.roads.near((x,y),max(a,b)+.8) or self.solids.near((x,y),math.hypot(a,b)+1):
                        continue
                    corners=[(x+u*ux+v*nx,y+u*uy+v*ny) for u in (-a,a) for v in (-b,b)]
                    if any(not inside(px,py,self.boundary) or self.roads.near((px,py),.45) for px,py in corners):
                        continue
                    ang=math.degrees(math.atan2(uy,ux))
                    fill=self.rng.choice(("#a37454","#b08260","#9b8061","#927b60","#ae8868","#aa765b"))
                    tr=f'translate({fmt(x)} {fmt(y)}) rotate({fmt(ang)}) scale(1 {side})'
                    roof=(f'<g transform="{tr}"><rect x="{-a:.2f}" y="{-b:.2f}" width="{2*a:.2f}" height="{2*b:.2f}" fill="{fill}" stroke="#65553f" stroke-width=".6"/>'
                          f'<path d="M{-a:.2f} 0H{a:.2f}V{b:.2f}H{-a:.2f}Z" fill="#4f453b" opacity=".22"/>'
                          f'<path d="M{-a:.2f} 0H{a:.2f}" stroke="#e0bd8e" stroke-width=".5"/>'
                          f'<path d="M-1 {-b:.2f}h2v-1.2h-2z" fill="#eacb98"/></g>')
                    yard=(f'<g transform="{tr}"><rect x="{-a-1:.2f}" y="{-b-setback+1.8:.2f}" width="{2*a+2:.2f}" height="{setback+b*2:.2f}" fill="#cabc94"/>'
                          f'<path d="M{-a-1:.2f} {-b-1:.2f}v{b*2+2:.2f}h{2*a+2:.2f}" fill="none" stroke="#928265" stroke-width=".45"/></g>')
                    pitch=(f'<g transform="{tr}" fill="none" stroke="#795e43" stroke-width=".20" opacity=".8">'
                           +''.join(f'<path d="M{-a+.3:.2f} {b*i/4:.2f}h{2*a-.6:.2f}"/>' for i in (-3,-2,-1,1,2,3))
                           +f'<path d="M{-a:.2f} {-b:.2f}l{a*.45:.2f} {b:.2f}l{-a*.45:.2f} {b:.2f} M{a:.2f} {-b:.2f}l{-a*.45:.2f} {b:.2f}l{a*.45:.2f} {b:.2f}" stroke-width=".38"/></g>')
                    extras=(f'<g transform="{tr}"><rect x="{a*.38:.2f}" y="{b*.3:.2f}" width="1.05" height="1.25" fill="#715e4e" stroke="#c4ab87" stroke-width=".25"/>'
                            f'<path d="M0 {-b:.2f}v{-setback+1:.2f}" stroke="#d8c6a2" stroke-width="1.65"/></g>')
                    district=next((i for i,poly in enumerate(self.cells) if inside(x,y,poly)),0)
                    fixture=""
                    if district==0 and self.housecount%3==0:
                        # A street-facing stall below the upper floor.
                        fixture=(f'<path d="M{-a+.6:.2f} {-b:.2f}h{a:.2f}l-.45 -1.6h{-a:.2f}Z" fill="#b99565" stroke="#716045" stroke-width=".28"/>'
                                 f'<path d="M{-a+1:.2f} {-b-1.65:.2f}v-.6m{a-1:.2f} 0v.6" stroke="#68593b" stroke-width=".3"/>')
                    elif district==1 and self.housecount%3:
                        fixture=(f'<path d="M{-a:.2f} {b*.2:.2f}h{a*.6:.2f}v{b+1:.2f}h{-a*.6:.2f}Z" fill="#a78763" stroke="#6e5e49" stroke-width=".35"/>'
                                 f'<path d="M{-a*.7:.2f} {b*.2:.2f}v{b+1:.2f}" stroke="#dfbd92" stroke-width=".42"/>')
                    elif self.housecount%4==0:
                        fixture=f'<rect x="{-a+.4:.2f}" y="{-b-2:.2f}" width="1.4" height="1.2" fill="#947753" stroke="#766044" stroke-width=".25"/>'
                    self.add(4,[(x,y)],yard,a+b+6,False)
                    self.add(7,[(x,y)],roof,a+b+2)
                    self.add(7,[(x,y)],pitch+extras+f'<g transform="{tr}">{fixture}</g>',a+b+6,False)
                    self.solids.add([(x,y),(x,y)],math.hypot(a,b))
                    self.housecount+=1

    def outskirts(self):
        rng=self.rng
        a,b,c,d=self.ext
        self.woods=[self.features[self.code+"-wood"]["geometry"]]
        groves=([(-465,-1090,245,520),(-245,-460,185,275),(1390,-720,175,415),(500,570,265,175)]
                if self.t else [(-1350,1240,340,470),(-1750,120,155,430),(850,1240,440,250),(-620,-675,265,265)])
        for cx,cy,rx,ry in groves:
            edge=[]
            for j in range(22):
                theta=2*math.pi*j/22
                r=1+.13*math.sin(3*theta+cy)+.08*math.sin(7*theta+cx)
                edge.append((cx+rx*r*math.cos(theta),cy+ry*r*math.sin(theta)))
            self.woods.append(edge)
        for wood in self.woods:
            self.add(1,wood,polygon(wood,"#a5ad89","#9ba37f",1))
        if self.t:
            farms=[(1110,210),(1310,330),(1157,590),(-80,-1500),(451,-1400)]
        else:
            farms=[(-1635,-345),(-1760,-205),(-892,1530),(-758,1360),(-1320,-787)]
        for i,(x,y) in enumerate(farms):
            key=("시장 동문길","시장 동문길","상류 둑길","높은 귀환길","높은 귀환길")[i] if self.t else (
                "서쪽 시장길","서쪽 시장길","상류 수레길","상류 수레길","서쪽 시장길")[i]
            q=min(self.lines[key][0],key=lambda p:math.dist(p,(x,y)))
            self.road(f"농가 진입로 {i+1}",[q,((q[0]+x)/2+8,(q[1]+y)/2),(x,y)],3)
            self.add(4,[(x,y)],f'<ellipse cx="{x}" cy="{y}" rx="13" ry="9" fill="#ccb995"/>',15)
            for ox,oy,angle in [(7,7,-19),(-8,4,13)]:
                self.add(7,[(x+ox,y+oy)],f'<use href="#barn" transform="translate({x+ox} {y+oy}) rotate({angle})"/>',10)
            self.solids.add([(x,y),(x,y)],16)
            for j in range(7):
                px=x+(j//4)*44-44; py=y+(j%4)*11+22
                field=[(px,py),(px+39,py+4),(px+37,py+12),(px-1,py+8)]
                if all(not self.roads.near(z,3) and not self.bank(z[1],0)-4<z[0]<self.bank(z[1],1)+4 for z in field):
                    self.add(3,field,polygon(field,"url(#furrow)","#a2916d",.65))
                    self.solids.add([field[0],field[1]],9)
        for gx in range(math.floor(a/38),math.ceil(c/38)):
            for gy in range(math.floor(b/38),math.ceil(d/38)):
                x=(gx+rng.random())*38; y=(gy+rng.random())*38
                if self.tile((x,y)) not in self.tiles or inside(x,y,self.boundary):
                    continue
                west,east=self.bank(y,0),self.bank(y,1)
                if west-12<x<east+12 or self.roads.near((x,y),7) or self.solids.near((x,y),12):
                    continue
                relief=math.sin(x/159+y/105)+.8*math.sin(y/174-x/130)+.4*math.cos(x/58+y/91)
                wooded=any(inside(x,y,poly) for poly in self.woods)
                if wooded or relief>.25:
                    for _ in range(6 if wooded else 2 if relief>.95 else 1):
                        px=x+rng.uniform(-10,10);py=y+rng.uniform(-10,10)
                        if self.roads.near((px,py),6) or inside(px,py,self.boundary) or self.bank(py,0)-5<px<self.bank(py,1)+5:
                            continue
                        r=rng.uniform(3.1,6.8)
                        self.add(4,[(px,py)],f'<use href="#tree" transform="translate({fmt(px)} {fmt(py)}) rotate({rng.randrange(360)}) scale({r:.2f})"/>',r*1.4)
                elif gx%2==0 and gy%2==0 and min(math.dist((x,y),z) for z in farms)<160:
                    field=[(x-17,y-10),(x+18,y-6),(x+15,y+12),(x-18,y+7)]
                    if all(not inside(px,py,self.boundary) and not self.roads.near((px,py),14) and
                           not self.bank(py,0)-10<px<self.bank(py,1)+10 for px,py in field):
                        self.add(2,field,polygon(field,"url(#furrow)","#aa976f",.7))

    def surface_inventory(self):
        # Four-metre northing bands; intersect the forest, settlement and banks on each scanline.
        totals=defaultdict(float)
        ex=FRAMES[self.code]["bounds"]
        for ij in self.tiles:
            (a,b),(c,d)=self.corners(ij)
            count=math.ceil(H/4); step=H/count
            breaks=sorted({b+j*step for j in range(count+1)} |
                          {y for y in (self.urban[1],self.urban[3],ex[1],ex[3]) if b<y<d})
            for y0,y1 in zip(breaks,breaks[1:]):
                y=(y0+y1)/2
                forest=[s for poly in self.woods for s in spans(poly,y)]
                settled=spans(self.boundary,y)
                wet=[(self.bank(y,0),self.bank(y,1))]
                cuts=sorted({a,c,*[min(c,max(a,x)) for s in forest+settled+wet+[(ex[0],ex[2])] for x in s]})
                for left,right in zip(cuts,cuts[1:]):
                    x=(left+right)/2; parcel=(right-left)*(y1-y0)/10000
                    def contains(intervals):
                        return any(lo<=x<=hi for lo,hi in intervals)
                    if contains(wet):
                        totals["river_ha"]+=parcel
                    elif contains(settled):
                        totals["settled_ha"]+=parcel
                    else:
                        k="woodland_ha" if contains(forest) else "other_land_ha"
                        totals[k]+=parcel
                        if ex[0]<=x<=ex[2] and ex[1]<=y<=ex[3]:
                            totals["exploration_land_outside_settlement_ha"]+=parcel
        assert abs(totals["settled_ha"]-self.measured)<.25
        return {k:round(v,2) for k,v in totals.items()}

    def signs(self):
        for k,f in self.features.items():
            p=f["geometry"]
            if "radius" in f:
                x,y=p; r=f["radius"]
                note=f'{k} {f["name"]} · {f.get("stop","")}'
                art=(f'<g class="ecology"><title>{escape(note)}</title><circle cx="{x}" cy="{y}" r="{r}" '
                     f'fill="#a36956" fill-opacity=".10" stroke="#925f4d" stroke-width="1.6" stroke-dasharray="7 5"/>'
                     +label((x,y+r*.52),k,8,"#784933")+'</g>')
                self.add(8,[p],art,r)
            if k.endswith("-END") or f["kind"]=="patrol":
                ds=draw_path(p,"#985a46",2.1,'stroke-dasharray="5 3"')
                self.add(8,p,'<g class="ecology"><title>'+escape(f["name"])+"</title>"+ds+"</g>",3)
        if not self.t:
            for stage,ks,col in [("normal",["stairs"],"#39675a"),("normal",["r-lock"],"#9a4535"),
                                 ("broken",["r-cross","r-lock"],"#9a4535"),("closed",["bypass"],"#9b542c"),
                                 ("closed",["r-cross","r-lock","RU2-move"],"#9a4535"),("rescued",["bypass"],"#a57049"),
                                 ("rescued",["r-cross","r-cordon"],"#9a4535"),
                                 ("rescued",["r-stretcher","r-pier-link"],"#346d6e")]:
                for k in ks:
                    ps=self.features[k]["geometry"]
                    self.add(9,ps,f'<g class="state {stage}">'+draw_path(ps,col,3,'stroke-dasharray="7 3"')+"</g>",4)
            for stage,k,word in [("broken","R-BREAK","계단 붕괴"),("closed","R-GATE","들것 문 닫힘"),
                                 ("rescued","R-GATE","들것 문 개방"),("rescued","R-GRID","추격 종료")]:
                p=self.features[k]["geometry"]
                self.add(10,[p],f'<g class="state {stage}">'+label((p[0],p[1]-5),word,5)+"</g>",25,False)
        for key,s in self.stops.items():
            x,y=s["point"]
            dot=f'<circle cx="{x}" cy="{y}" r="2.3" fill="#503f32" stroke="#f6ecd5" stroke-width="1"/>'
            self.add(10,[(x,y)],dot+label((x,y+5),s["name"],5),33,False)

    def workplaces(self):
        def place(p,symbol,angle=0,size=18):
            x,y=p
            self.add(7,[p],f'<use href="#{symbol}" transform="translate({x} {y}) rotate({angle})"/>',size)
            self.solids.add([p,p],size*.65)
        if self.t:
            for p,angle in [((705,-145),-9),((704,-127),-9),((669,-122),12),((981,-424),23),((1003,-403),18)]:
                place(p,"logs",angle)
            for p in [(979,-451),(1023,-439),(572,-178),(653,-155)]:
                place(p,"barn",14)
            # Fixed fish stakes stand in the shallows just below the observation ledge.
            for i in range(15):
                x,y=-298-i*1.12,-1166+i*1.28
                self.add(7,[(x,y)],f'<circle cx="{x:.2f}" cy="{y:.2f}" r=".45" fill="#65573d"/>',1)
            self.add(6,[(-315,-1148),(-298,-1166)],draw_path([(-315,-1148),(-308,-1157),(-298,-1166)],"#887b59",.5),1)
            for p in [(-365,-1500),(-262,-1165),(1211,-958),(1059,-1302)]:
                place(p,"shelter",7,6)
            x=self.bank(-1510)
            self.road("쉼나루 잔교길",[self.stops["A08"]["point"],(x,-1510)],3,False)
            piers=[((159,80),157),((x,-1510),170)]
            stores=[((433,129),9),((461,120),9)]
        else:
            piers=[((97,89),-36),((730,559),-36)]
            stores=[((-5,108),-20),((-23,116),-20),((-678,421),24),((-677,446),24)]
            for p in [(-695,1132),(-729,1107)]:
                place(p,"logs",19)
            place((-880,686),"barn",-11)
            for k in ("r-pipe0","r-pipe1","r-clear","r-drain-water","r-shelf-route"):
                ps=self.features[k]["geometry"]
                self.add(8,ps,draw_path(ps,"#78948b" if k=="r-drain-water" else "#816e52",.85),1,False)
        for p,angle in stores:
            place(p,"barn",angle)
        for (x,y),angle in piers:
            place((x,y),"pier",angle,15)
            v=math.radians(angle)
            place((x+12*math.cos(v),y+12*math.sin(v)),"boat",angle+89,10)
        for k,f in self.features.items():
            if f["kind"]=="building":
                ps=f["geometry"]; a,b,c,d=bbox(ps)
                self.add(7,ps,draw_path([(a,(b+d)/2),(c,(b+d)/2)],"#e8c89c",.7),1)
        for name,_,_,site in self.districts:
            self.add(10,[site],label(site,name,6.5,"#624d35"),34,False)

    def seams(self):
        outer=[]; internal=defaultdict(list)
        for name,(ps,width,kind) in self.lines.items():
            for p,q in zip(ps,ps[1:]):
                a=self.tile(p); b=self.tile(q)
                for axis,span in enumerate((W,H)):
                    change=q[axis]-p[axis]
                    if abs(change)<1e-10:
                        continue
                    for line in range(min(a[axis],b[axis])+1,max(a[axis],b[axis])+1):
                        coord=(X0,Y0)[axis]+line*span-self.delta[axis]
                        t=(coord-p[axis])/change
                        cross=[p[0]+t*(q[0]-p[0]),p[1]+t*(q[1]-p[1])]
                        cross[axis]=coord
                        low=list(self.tile(cross));high=list(low)
                        low[axis]=line-1;high[axis]=line
                        low,high=tuple(low),tuple(high)
                        if low not in self.tiles and high not in self.tiles:
                            continue
                        v=[q[0]-p[0],q[1]-p[1]]; length=math.hypot(*v)
                        v=[z/length for z in v]
                        if low in self.tiles and high in self.tiles:
                            internal[low,high].append((name,cross,v,width))
                        else:
                            tile=low if low in self.tiles else high
                            neighbor=high if tile==low else low
                            if v[axis]*(neighbor[axis]-tile[axis])<0:
                                v=[-z for z in v]
                            outer.append({"feature":name,"kind":kind,"tile":tile_name(tile),
                                          "next":tile_name(neighbor),"local_m":[round(z,4) for z in cross],
                                          "world_m":[round(z+s,4) for z,s in zip(cross,self.delta)],
                                          "outward_unit":[round(z,7) for z in v],"width_m":width if kind=="길" else None})
        edges=sum(1 for i,j in self.tiles for q in ((i+1,j),(i,j+1)) if q in self.tiles)
        for (lo,hi),crossings in internal.items():
            axis=int(lo[1]!=hi[1])
            for _,pt,_,_ in crossings:
                assert abs(self.corners(lo)[1][axis]-pt[axis])<1e-7
                assert abs(self.corners(hi)[0][axis]-pt[axis])<1e-7
        for needed in ("서안","동안"):
            assert sum(p["feature"]==needed for p in outer)>=2
        return outer,{"shared_edges":edges,"shared_crossings":sum(map(len,internal.values()))}

    def itineraries(self):
        # Route highlighting uses the same curves as the underlying streets.
        keys=(["선적길|곡물 수레길","목재 검사길|적치장 진입","목재 검사길|낮은 조사길|어살 둑길|높은 귀환길"] if self.t else
              ["하역 연결|창고 진입|식료 수레길","조합과 작업구|생활 급수길","bypass","r-stretcher|구조선 연결","제방 귀환길|조합과 작업구"])
        for i,(trip,names) in enumerate(zip(self.trips,keys)):
            for name in names.split("|"):
                ps=self.lines[name][0]
                if trip["stops"][-1]=="B02" and name=="조합과 작업구":
                    end=ps.index(tuple(self.stops["B02"]["point"]))
                    ps=ps[:end+1]
                self.add(9,ps,f'<g class="travel trip{i}">'+draw_path(ps,"#fff4ca",5.4)+
                         draw_path(ps,"#894826" if trip["state"]!="rescued" else "#226c68",2.8)+'</g>',4)

    def base_svg(self,bounds,overview=False):
        a,b,c,d=bounds
        selection=[]
        for level,bb,art,show in self.layers:
            if overlaps(bb,bounds) and (not overview or show):
                selection.append(art)
        return (f'<rect x="{fmt(a)}" y="{fmt(b)}" width="{fmt(c-a)}" height="{fmt(d-b)}" fill="url(#ground)"/>'
                +"".join(selection))

    def detail(self,ij):
        (a,b),(c,d)=self.corners(ij)
        x,y=ij; key=tile_name(ij)
        bounds=(a,b,c,d)
        frame='<rect x="48" y="86" width="1084" height="1160"/>'
        matrix=f'translate({fmt(48-a*4.25)} {fmt(86+d*4.25)}) scale(4.25 -4.25)'
        title=f'{self.name} · 세계 타일 {x}, {y}'
        art=[svg_head(1180,1344,title,frame,len(self.trips)),text_at(48,51,title,30),
             text_at(1128,48,"북 ↑　　도시 로컬 m","17","end"),
             f'<g clip-path="url(#crop)"><g id="metric" transform="{matrix}">',
             self.base_svg(bounds),'</g></g>']
        for tick in range(math.ceil(a/50)*50,math.ceil(c/50)*50,50):
            px=48+(tick-a)*4.25
            art.append(text_at(px,1267,str(tick),13,"middle"))
        for tick in range(math.ceil(b/50)*50,math.ceil(d/50)*50,50):
            py=86+(d-tick)*4.25
            art.append(text_at(42,py+4,str(tick),13,"end"))
        art += ['<path d="M48 1294h212.5m-212.5 -4v8m212.5 -8v8" stroke="#414b3c" stroke-width="2"/>',
                text_at(270,1300,"50 m",15)]
        for n,(dx,dy) in enumerate([(-1,0),(0,1),(0,-1),(1,0)]):
            nxt=x+dx,y+dy
            if nxt in self.tiles:
                k=tile_name(nxt)
                art.append(f'<a data-tile="{k}" target="_top" href="index.html?tile={k}">'+
                           text_at(580+n*130,1300,["← 서쪽","↑ 북쪽","↓ 남쪽","→ 동쪽"][n],18)+"</a>")
        art.append("</svg>")
        (self.out/(key+".svg")).write_text("".join(art),encoding="utf-8")

    def overview(self):
        a,b,c,d=self.ext
        scale=min(1180/(c-a),1090/(d-b))
        width=1260; height=math.ceil((d-b)*scale+160)
        x0=(width-(c-a)*scale)/2
        rects=[]; tiles=[]
        for ij in sorted(self.tiles):
            (x,y),(u,v)=self.corners(ij)
            box=f'x="{fmt(x)}" y="{fmt(y)}" width="{fmt(W)}" height="{fmt(H)}"'
            rects.append(f'<rect {box}/>')
            k=tile_name(ij)
            tone="#786349" if ij in self.core else "#6b705d"
            tiles.append(f'<a data-tile="{k}" target="_top" href="index.html?tile={k}"><title>세계 타일 {ij[0]}, {ij[1]} 열기</title><rect {box} fill="transparent" stroke="{tone}" stroke-opacity=".50" stroke-width="1.6"/></a>')
        title=self.name+" 도시와 근교"
        mat=f'translate({fmt(x0-a*scale)} {fmt(78+d*scale)}) scale({fmt(scale)} -{fmt(scale)})'
        art=[svg_head(width,height,title,"".join(rects),len(self.trips)),text_at(x0,48,title,32),
             text_at(width-x0,45,f"북 ↑　　세계 타일 {len(tiles)}장",18,"end"),
             f'<g transform="{mat}" id="metric"><g clip-path="url(#crop)">',self.base_svg(self.ext,True),"</g>"]
        for key,s in self.stops.items():
            if key[:1] in "AB" and key[1:].isdigit():
                px,py=s["point"]
                art.append(f'<circle cx="{px}" cy="{py}" r="8" fill="#5a4a37" stroke="#fff1cf" stroke-width="2"/>')
                art.append(label((px,py+15),key,24))
        for i,(name,_,_,site) in enumerate(self.districts):
            art.append(label(site,name,26,"#55412e"))
        art+=tiles+['</g>',text_at(x0,height-32,"칸을 누르면 상세 좌표 평면도가 열린다.",18),
                   f'<path d="M{width-x0-500*scale:.2f} {height-36}h{500*scale:.2f}" stroke="#4c5040" stroke-width="4"/>',
                   text_at(width-x0,height-44,"500 m",13,"end"),"</svg>"]
        (self.out/"overview.svg").write_text("".join(art),encoding="utf-8")

    def build(self):
        self.terrain()
        self.town()
        self.routes()
        self.workplaces()
        self.streets()
        self.outskirts()
        self.signs()
        self.itineraries()
        self.layers.sort(key=lambda item:item[0])
        outer,checks=self.seams()
        land=sum(d[2] for d in self.districts)
        data={"name":self.name,"frame":self.code,"offset":self.delta,"extent":self.ext,
              "worldTileOrigin":[X0,Y0],"tileSize":[W,H],"target_ha":land,"land_ha":round(self.measured,3),
              "urban_boundary_m":self.boundary,"districts":[{"name":n,"population":p,"target_ha":h,
              "land_ha":round(area(cell)/10000,3)} for (n,p,h,_),cell in zip(self.districts,self.cells)],
              "tiles":{tile_name(ij):("시가지" if ij in self.core else "근교") for ij in sorted(self.tiles)},
              "surface":self.surface_inventory(),"stops":self.stops,"trips":self.trips,"boundary_connections":outer,
              "basis":{"grid":"../../VelesCardinalTiles/tiles.json","anchors":"../../coordinates.json",
                       "lower_detail":"../../05-tarnel-lower.svg" if self.t else "../../08-roden-rescue.svg",
                       "placement":"거점·몬스터 활동 반경은 기존 좌표, 구획·집터·근교 수림·도시 밖 접속 방향은 평면 배치안"},
              "checks":{**checks,"frontage_buildings":self.housecount}}
        self.overview()
        for ij in sorted(self.tiles):
            self.detail(ij)
        (self.out/"index.html").write_text(page(data),encoding="utf-8")
        (self.out/"coordinates.json").write_text(json.dumps(data,ensure_ascii=False,indent=1),encoding="utf-8")
        print(self.slug,len(self.tiles),round(self.measured,3),self.housecount,len(outer),checks)


def text_at(x,y,s,size,align="start"):
    return f'<text x="{fmt(x)}" y="{fmt(y)}" font-size="{size}" text-anchor="{align}">{escape(str(s))}</text>'


def svg_head(w,h,title,clip,trip_count):
    head=f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}" lang="ko">
<title>{escape(title)}</title><defs><clipPath id="crop">{clip}</clipPath>
<pattern id="ground" patternUnits="userSpaceOnUse" width="37" height="31"><rect width="37" height="31" fill="#c4c5a1"/><path d="M3 7l1 1 2 -1m10 9l2 -.4m11 -11l1 1.5m2 21l2 -1" fill="none" stroke="#a4a780" stroke-width=".6"/></pattern>
<pattern id="water" patternUnits="userSpaceOnUse" width="43" height="22"><rect width="43" height="22" fill="#85aba5"/><path d="M1 5q7 2 13 1m4 9q7 1 16 0m-5 -13l7 1" stroke="#abc1b3" stroke-width=".8" fill="none"/></pattern>
<pattern id="furrow" patternUnits="userSpaceOnUse" width="3.5" height="5" patternTransform="rotate(13)"><rect width="3.5" height="5" fill="#bdad7d"/><path d="M0 0v5" stroke="#998b64" stroke-width="1"/></pattern>
<pattern id="timber" patternUnits="userSpaceOnUse" width="5" height="1.3"><rect width="5" height="1.3" fill="#ae9067"/><path d="M0 0h5" stroke="#776347" stroke-width=".3"/></pattern>
<pattern id="paving" patternUnits="userSpaceOnUse" width="4" height="2.4"><rect width="4" height="2.4" fill="#c4ab87"/><path d="M0 0h4m-1 0v1.2m-3 0h4m-3 0v1.2" stroke="#ad9777" stroke-width=".2"/></pattern>
<g id="barn" stroke="#66523f" stroke-width=".45"><rect x="-7" y="-3.9" width="14" height="7.8" fill="#a8805b"/><path d="M-7 0H7V3.9H-7Z" fill="#8c7155"/><path d="M-7 0H7m-12 -3.8v7.6m2 -7.6v7.6m2 -7.6v7.6m2 -7.6v7.6m2 -7.6v7.6" stroke="#79634c" stroke-width=".2"/><path d="M-7 0H7" stroke="#e9c79a"/></g>
<g id="shelter"><rect x="-4" y="-2.4" width="8" height="4.8" fill="url(#timber)" stroke="#685d45" stroke-width=".55"/></g>
<g id="logs" fill="#a8875d" stroke="#68583c" stroke-width=".4"><rect x="-10" y="-4" width="21" height="1.5" rx=".7"/><rect x="-12" y="-2" width="21" height="1.5" rx=".7"/><rect x="-11" y="0" width="23" height="1.5" rx=".7"/><rect x="-9" y="2" width="20" height="1.5" rx=".7"/><path d="M-6 -4v8m12 -8v8" stroke-width=".8"/></g>
<g id="pier"><path d="M-4 -2H10V2H-4Z" fill="url(#timber)" stroke="#65563d" stroke-width=".6"/><path d="M3 -2.3v4.6m6 -4.6v4.6" stroke="#65563d" stroke-width=".8"/></g>
<g id="boat"><path d="M-5 0Q-2 -2.45 3 -1.25L5 0Q2 2.1 -2 1.5Z" fill="#ac9168" stroke="#564c38" stroke-width=".55"/><path d="M-2 -1v2m2 -2.3v2.6m2 -2.4v2" stroke="#68593e" stroke-width=".4"/></g>
<g id="tree"><path d="M-.92 -.22Q-1.2 -.7 -.65 -.8Q-.39 -1.24 .04 -.94Q.67 -1.15 .84 -.67Q1.28 -.36 .93 .1Q1 .67 .49 .7Q.1 1.23 -.29 .9Q-.98 1.08 -.94 .44Q-1.27 .13 -.92 -.22Z" fill="#7d8969" stroke="#646e52" stroke-width=".10"/><path d="M-.74 -.33Q-.65 -.72 -.31 -.58Q-.18 -.9 .16 -.64M.35 .55Q.74 .36 .59 .12" fill="none" stroke="#b1b28a" stroke-width=".11"/></g>
</defs><style>svg{{background:#f1ead8;font-family:'Malgun Gothic',sans-serif;color:#383c31}}polyline,polygon,path{{stroke-linejoin:round;stroke-linecap:round}}text{{fill:currentColor}}a:hover rect{{fill:#fff5c5;fill-opacity:.28}}.ecology,.state,.travel{{display:none}}.normal{{display:inline}}svg.closed .normal,svg.rescued .normal{{display:none}}svg.closed .closed,svg.rescued .rescued,svg.wild .ecology{{display:inline}}svg.route0 .trip0,svg.route1 .trip1,svg.route2 .trip2,svg.route3 .trip3{{display:inline}}</style>'''
    return head + state_flags(trip_count)


def state_flags(trip_count):
    # Fragment navigation also works when the HTML is opened from disk.
    flags="".join(f'<g id="s-{s}-{wild}-{r}" class="flag" data-s="{s}" data-w="{wild}" data-r="{r}"/>'
                  for s in ("normal","broken","closed","rescued") for wild in (0,1) for r in range(-1,max(4,trip_count)))
    style='.flag[data-w="1"]:target ~ g .ecology{display:inline}'
    for s in ("broken","closed","rescued"):
        style+=f'.flag[data-s="{s}"]:target ~ g .normal{{display:none}}.flag[data-s="{s}"]:target ~ g .{s}{{display:inline}}'
    for r in range(max(4,trip_count)):
        style+=f'.flag[data-r="{r}"]:target ~ g .trip{r}{{display:inline}}'
    return "<style>"+style+"</style>"+flags


def page(data):
    story = ""
    if data["frame"] == "T":
        def stop_link(key, label):
            tile = data["stops"][key]["tile"]
            return f'<a href="?tile={tile}&amp;place={key}">{label}</a>'
        story = (
            '<section id="first-call"><h2>첫 기항의 세 장소</h2>'
            f'<p>04 · {stop_link("A02", "목재 검사길")}에서 세 경로가 만난다. 부두에 남거나 하류에서 돌아와도 '
            '에르사는 직접 확인한 묶음만 받고 모릭이 옮긴다. 다벤은 남은 물량의 납기를 로덴에 묻는다.</p>'
            f'<p>선택 · {stop_link("STORE", "곡물 창고 앞")}에서 첫 자루를 받은 오르벨이 장꾼에게 의결된 경비 급료를 설명한다. '
            '증원 요청은 시의회에 전달한다. 이 대화를 건너뛰고 선적장으로 갈 수 있다.</p>'
            f'<p>05 · 다음 날 {stop_link("A01", "공동 나루")}에서 이셀이 접안을 조율한다. '
            '린은 하역할 사람이 설 폭을 남겨 실을 양을 정하고 로덴으로 떠난다. '
            '<a href="../../05-tarnel.svg#detail-2">선적 자리 확대</a></p></section>'
        )
    front={k:v for k,v in data.items() if k not in ("urban_boundary_m","boundary_connections")}
    rows="".join(f'<tr><th>{d["name"]}</th><td>{d["population"]:,}명</td><td>{d["land_ha"]:.2f} / {d["target_ha"]} ha</td></tr>' for d in data["districts"])
    spec=("나루에서 높은 시장단까지 곡물 수레가 오르고, 검수를 마친 목재는 남쪽 마당으로 나간다. "
          "어살의 낮은 둑길이 잠기면 쉼나루에서 높은 운반길의 상황을 확인한다." if data["frame"]=="T" else
          "제방 창고에서 식료 시장으로 곡물을 보낸다. 작업구 사고 중에도 공공 나루와 조합 급수 자리는 열린다. "
          "보행자는 외곽 점검로로 돌아가고, 들것은 작업자가 문을 열고 작업팔이 멎은 뒤 선착장으로 간다.")
    stages='' if data["frame"]=="T" else '''<label>작업구 상태 <select id="stage"><option value="normal">평상시 · 점검 진입</option><option value="broken">계단 붕괴 직후</option><option value="closed">공급 차단 · 보행 우회</option><option value="rescued">수동문 개방 · 작업팔 정지</option></select></label>'''
    connections="".join(f'<tr><th>{s["feature"]}</th><td>{s["tile"]}</td><td>{s["local_m"][0]:.1f}, {s["local_m"][1]:.1f}</td><td>{s["width_m"] or "—"}</td></tr>' for s in data["boundary_connections"])
    surface=data["surface"]
    monsters=[(k,f) for k,f in FEATURES.items() if f["frame"]==data["frame"] and "radius" in f]
    ecology="".join(f'<tr><th>{k} {escape(f["name"])}</th><td>{f["radius"]} m</td><td>{escape(f.get("stop",""))}</td></tr>' for k,f in monsters)
    source="../../05-tarnel-lower.html" if data["frame"]=="T" else "../../08-roden-rescue.svg"
    return f'''<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>{data["name"]} 도시와 근교 좌표 평면도</title><link rel="stylesheet" href="../viewer.css"></head><body><main>
<nav><a href="../../index.html">테른 강 지도 목차</a><a href="../{"Roden" if data["frame"]=="T" else "Tarnel"}/index.html">{"로덴" if data["frame"]=="T" else "타르넬"} 도시와 근교</a></nav>
<header><h1>{data["name"]} 도시와 근교</h1><p>{sum(d["population"] for d in data["districts"]):,}명　·　정주지 {data["land_ha"]:.2f} ha　·　세계 타일 {len(data["tiles"])}장</p></header>
<div class="layout"><section><div class="tools"><button id="overview">전체도</button><label>세계 타일 <select id="tiles"></select></label><label>확대 <select id="zoom"><option>100%</option><option>150%</option><option>200%</option><option>300%</option></select></label></div>
<div id="port"><div id="paper"><img id="plate" src="overview.svg" alt="{data["name"]} 좌표 평면도"><div id="hits"></div><i id="pin" hidden></i></div></div>
<div class="tools"><div id="neighbors"></div><a id="original" href="overview.svg">SVG 원본</a></div><output id="position" aria-live="off"></output></section>
<aside><h2>길 따라가기</h2><label>동선 <select id="trip"></select></label>{stages}<label><input type="checkbox" id="ecology"> 몬스터 활동권·추격 경계</label><div id="stops"></div><p id="guide">{spec}</p>
{story}<h2>정주 구획</h2><table><thead><tr><th>구역</th><th>상주</th><th>도면 / 목표</th></tr></thead><tbody>{rows}</tbody></table>
<p>주거지·마당·육상 작업장을 포함한 정주 경계의 면적이다. 지붕은 제작 배치안이며 가구별 인구 집계와 별개다.</p><a href="{source}">기존 탐색 상세도</a></aside></div>
<details><summary>근교 면적과 몬스터 활동권</summary><p>전체 도면에서 본류 수면 약 {surface["river_ha"]:.1f} ha, 정주지 밖 숲 약 {surface["woodland_ha"]:.1f} ha, 나머지 비정주 육지는 약 {surface["other_land_ha"]:.1f} ha다. 기존 탐색도에 드는 비정주 육지 약 {surface["exploration_land_outside_settlement_ha"]:.1f} ha는 숲과 기타 육지에 걸쳐 있다. 강안과 숲 경계를 남북 4m 이하 간격으로 잘라 추산했다.</p><table><tr><th>무리와 먹이터</th><th>활동 반경</th><th>추격 종료</th></tr>{ecology}</table></details>
<details><summary>외곽 접속점과 좌표</summary><p>동쪽 X · 북쪽 Y(m). 길은 중심선, 강은 서안과 동안의 접점을 따로 기록했다. 세계 좌표와 바깥쪽 단위 방향 벡터는 <a href="coordinates.json">좌표 원본</a>에서 확인한다.</p><table><tr><th>길·강안</th><th>세계 타일</th><th>로컬 X, Y (m)</th><th>길 폭 (m)</th></tr>{connections}</table></details>
</main><script id="data" type="application/json">{json.dumps(front,ensure_ascii=False)}</script><script src="../viewer.js"></script></body></html>'''


if __name__=="__main__":
    for city in CITIES:
        Plan(city).build()




