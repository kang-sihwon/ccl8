"""Compare independently clipped ink in the published city SVGs, in local metres."""
import json
import math
from collections import Counter, defaultdict
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[2]
ATLAS=ROOT/"Docs/ArtDirection/EtherWorldMap/Atlas/VelesTarnelRoden-v1"
NS="{http://www.w3.org/2000/svg}"


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def verify(city,frame,expected):
    folder=ATLAS/"CityPlans"/city
    data=read(folder/"coordinates.json")
    tile_ids={tuple(map(int,k.split("_"))) for k in data["tiles"]}
    assert len(tile_ids)==expected
    assert {p.stem for p in folder.glob("*_*.svg")}==set(data["tiles"])
    overview=ET.parse(folder/"overview.svg")
    assert {e.get("data-tile") for e in overview.iter(NS+"a")}==set(data["tiles"])
    w,h=data["tileSize"];ox,oy=data["worldTileOrigin"];dx,dy=data["offset"]
    sides=defaultdict(Counter)
    for i,j in tile_ids:
        root=ET.parse(folder/f"{i}_{j}.svg")
        x=ox+i*w-dx;y=oy+j*h-dy
        east=ox+(i+1)*w-dx;north=oy+(j+1)*h-dy
        # Every sheet carries its own page transform and independently selected geometry.
        transform=root.find('.//*[@id="metric"]').get("transform")
        tx,ty=map(float,transform.split(")")[0].split("(")[1].split())
        assert abs(tx-(48-x*4.25))<.001 and abs(ty-(86+(y+h)*4.25))<.001
        for path in root.iter(NS+"polyline"):
            key=path.get("data-feature")
            if not key:
                continue
            width=float(path.get("data-width"))
            points=[tuple(map(float,p.split(","))) for p in path.get("points").split()]
            for a,b in zip(points,points[1:]):
                vx,vy=b[0]-a[0],b[1]-a[1]
                for axis,start,end,low,high in ((0,x,east,y,north),(1,y,north,x,east)):
                    speed=(vx,vy)[axis]
                    if not speed:
                        continue
                    for sign,border in ((-1,start),(1,end)):
                        t=(border-a[axis])/speed
                        if not 0<=t<1:
                            continue
                        pos=a[1-axis]+t*(b[1-axis]-a[1-axis])
                        if low<=pos<high:
                            angle=round(math.atan2(vy,vx),6)
                            sides[i,j,axis,sign][(key,round(pos,3),angle,width)]+=1
    edges=crossings=0
    for i,j in tile_ids:
        for axis,neighbor in enumerate(((i+1,j),(i,j+1))):
            if neighbor in tile_ids:
                a=sides[i,j,axis,1];b=sides[*neighbor,axis,-1]
                assert a==b,(city,(i,j),neighbor,a-b,b-a)
                edges+=1;crossings+=sum(a.values())
    checks=data["checks"]
    assert (edges,crossings)==(checks["shared_edges"],checks["shared_crossings"])
    ports=data["boundary_connections"]
    for p in ports:
        i,j=map(int,p["tile"].split("_"));u,v=map(int,p["next"].split("_"))
        axis=int(j!=v); sign=(u-i,v-j)[axis]
        assert (u,v) not in tile_ids and abs(u-i)+abs(v-j)==1
        match=[k for k in sides[i,j,axis,sign] if k[0]==p["feature"] and
               abs(k[1]-p["local_m"][1-axis])<.015]
        assert match,(city,p)
        assert abs(math.hypot(*p["outward_unit"])-1)<1e-6
        assert p["outward_unit"][axis]*sign>0
        assert math.dist(p["world_m"],[x+y for x,y in zip(p["local_m"],(dx,dy))])<.001
    stops=data["stops"]
    source=read(ATLAS/"coordinates.json")["features"]
    for k,s in stops.items():
        if len(k)==3 and k[0] in "AB" and k[1:].isdigit():
            assert s["point"]==source[k]["geometry"]
        assert s["tile"] in data["tiles"]
    for trip in data["trips"]:
        assert all(k in stops for k in trip["stops"])
    surface=data["surface"]
    assert abs(sum(surface[k] for k in ("river_ha","settled_ha","woodland_ha","other_land_ha"))-expected*w*h/10000)<.03
    assert abs(sum(d["land_ha"] for d in data["districts"])-data["land_ha"])<.01
    print(f"{city}: {expected} SVGs, {edges} shared sides, {crossings} matching crossings, {len(ports)} boundary connections")


if __name__=="__main__":
    verify("Tarnel","T",97)
    verify("Roden","R",124)

