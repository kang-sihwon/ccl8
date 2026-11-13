"""Independent PNG/coordinate verification and a geographic overview from numerical heights."""
import hashlib
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, ImageChops

ROOT = Path(__file__).resolve().parent
load = lambda p: json.loads(p.read_text(encoding="utf-8-sig"))
m = load(ROOT / "manifest.json")
g = load(ROOT / "../../Data/ignis-continent-georeference-v1.json")
world = load(ROOT / "../../Data/ether-world-map-land35-precise-v3.json")
details = load(ROOT / "../IgnisSettlementDetails-v1/records/plan.json")
coast = load(ROOT / "data/coastline.json")
settlements = load(ROOT / "data/settlements.json")["settlements"]
rivers = load(ROOT / "data/rivers.json")["rivers"]
roads = load(ROOT / "data/roads.json")["roads"]
navigation = load(ROOT / "data/navigation.json")["routes"]
interactions = load(ROOT / "data/interactions.json")
height = Image.open(ROOT / m["height"]["file"])
mask = Image.open(ROOT / m["layers"]["land"]["file"])
hp, mp = height.load(), mask.load()
W, H = height.size
gx, gy = m["grid"]["origin_xy_m"]
step = m["grid"]["spacing_m"]
R = g["coordinate_system"]["radius_km"] * 1000
p0, l0 = math.radians(54), math.radians(-1)
checks = []


def check(name, passed, detail=None):
    checks.append({"name": name, "pass": bool(passed), "detail": detail})


def project(uv):
    lon, lat = math.radians(uv[0] * 360 - 180), math.radians(90 - uv[1] * 180)
    c = math.acos(max(-1, min(1, math.sin(p0) * math.sin(lat) + math.cos(p0) * math.cos(lat) * math.cos(lon-l0))))
    k = c / math.sin(c) if c > 1e-12 else 1
    return [R*k*math.cos(lat)*math.sin(lon-l0), R*k*(math.cos(p0)*math.sin(lat)-math.sin(p0)*math.cos(lat)*math.cos(lon-l0))]


def inverse(p):
    x, y = p
    rho = math.hypot(x, y)
    if not rho:
        return [-1, 54]
    c = rho / R
    lat = math.asin(math.cos(c)*math.sin(p0)+y*math.sin(c)*math.cos(p0)/rho)
    lon = l0 + math.atan2(x*math.sin(c), rho*math.cos(p0)*math.cos(c)-y*math.sin(p0)*math.sin(c))
    return [math.degrees(lon), math.degrees(lat)]


def sample(p):
    u, v = (p[0]-gx)/step, (gy-p[1])/step
    i, j = max(0, min(W-2, math.floor(u))), max(0, min(H-2, math.floor(v)))
    a, b = max(0, min(1, u-i)), max(0, min(1, v-j))
    return ((hp[i,j]*(1-a)+hp[i+1,j]*a)*(1-b)+(hp[i,j+1]*(1-a)+hp[i+1,j+1]*a)*b)*.125-1024


check("source file hashes unchanged", all(hashlib.sha256((ROOT/s["path"]).read_bytes()).hexdigest()==s["sha256"] for s in m["source_files"]))
original = next(c for c in world["continents"] if c["id"] == 1)["geometry"]
check("all source coastline vertices exact", coast["source_geometry_normalized"] == original, sum(len(r) for r in original["coordinates"]))
errors = [math.dist(project(uv), p) for ring, out in zip(original["coordinates"], coast["geometry_xy_m"]["coordinates"]) for uv, p in zip(ring, out)]
check("independent coastline projection", max(errors) < .0001, {"maximum_m": max(errors)})
check("all 36 names and anchors exact", len(settlements)==36 and all(all(s[key]==next(t for t in g["settlements"] if t["id"]==s["id"])[key] for key in ("xy_km","lon_lat_deg","name","kind")) for s in settlements))
check("meter conversion and L10 reference", all(s["xy_m"]==[v*1000 for v in s["xy_km"]] and s["world_pixel_L10"]==next(t for t in details["settlements"] if t["id"]==s["id"])["worldPixel"] for s in settlements))
ll_error = max(math.dist(inverse(s["xy_m"]), s["lon_lat_deg"]) for s in settlements)
check("independent inverse projection", ll_error < 1e-8, {"maximum_degrees": ll_error})
check("86 facility candidates preserved", sum(len(s["facilities"]) for s in settlements)==86 and all([(p["id"],p["name"],p["xy_km"]) for p in s["facilities"]]==[(p["id"],p["name"],p["xy_km"]) for p in next(t for t in details["settlements"] if t["id"]==s["id"])["points"]] for s in settlements))
check("all source routes exact and modes separated", len(roads)+len(navigation)==24 and all(r["xy_km"]==next(t for t in g["routes"] if t["id"]==r["id"])["xy_km"] for r in roads+navigation) and all(r["mode"]=="road" for r in roads) and all(r["mode"]!="road" for r in navigation))
check("all source river positions and bed heights exact", len(rivers)==14 and all(all(r[k]==next(t for t in g["rivers"] if t["id"]==r["id"])[k] for k in ("xy_km","bed_elevation_m","outlet")) for r in rivers))
check("authored bed profiles descend", all(all(b<=a for a,b in zip(r["bed_elevation_m"],r["bed_elevation_m"][1:])) for r in rivers))
check("declared confluences have matching XY and height", all(any(math.dist(r["xy_km"][-1],p)<1e-8 and abs(r["bed_elevation_m"][-1]-z)<1e-6 for p,z in zip(next(t for t in rivers if t["id"]==r["outlet"])["xy_km"],next(t for t in rivers if t["id"]==r["outlet"])["bed_elevation_m"])) for r in rivers if not r["outlet"].startswith("sea:")))
check("16-bit height header and complete shape", height.mode in ("I;16","I;16B","I") and (W,H)==(m["grid"]["width"],m["grid"]["height"]) and (ROOT/m["height"]["file"]).read_bytes()[24]==16)
check("land elevations valid and ocean explicit zero", height.getextrema()[0]>=8192 and {i for i,n in enumerate(mask.histogram()) if n}=={0,255} and ImageChops.multiply(height.convert("I").point(lambda v: v-8192).convert("L"),ImageChops.invert(mask)).getbbox() is None)
tile_errors, seam_errors = [], []
tilemap = {}
for t in m["tile_layout"]["tiles"]:
    im = Image.open(ROOT/t["file"])
    x,y = t["sample_origin"]
    if im.size != (513,513) or im.tobytes()!=height.crop((x,y,x+513,y+513)).tobytes():
        tile_errors.append(t["id"])
    if hashlib.sha256((ROOT/t["file"]).read_bytes()).hexdigest()!=t["sha256"]:
        tile_errors.append(t["id"]+":hash")
    tilemap[x,y] = im
for (x,y),im in tilemap.items():
    if (x+512,y) in tilemap and im.crop((512,0,513,513)).tobytes()!=tilemap[x+512,y].crop((0,0,1,513)).tobytes():
        seam_errors.append([x,y,"east"])
    if (x,y+512) in tilemap and im.crop((0,512,513,513)).tobytes()!=tilemap[x,y+512].crop((0,0,513,1)).tobytes():
        seam_errors.append([x,y,"south"])
check("40 tile interiors equal whole master exactly", not tile_errors and len(tilemap)==40, tile_errors)
check("all 67 adjoining tile borders exactly equal", not seam_errors, {"pairs": 7*5+8*4, "failures":seam_errors})
check("all masks share the same grid", all(Image.open(ROOT/v["file"]).size==(W,H) for v in m["layers"].values()))
residual = max(abs(sample(p)-z) for r in rivers for p,z in zip(r["geometry_xy_m"],r["bed_elevation_m"]))
check("macro river control residual within 1m", residual<=1, {"maximum_m": residual,"tolerance_reason":"2km grid interpolation; vector bed profiles govern later channel refinement"})
check("road profiles contain no sea samples", all(r["off_land_sample_count"]==0 for r in roads))
check("river intersections have consistent source beds", not any(x["status"]=="source_profile_conflict" for x in interactions["river_river"]))
check("source geometry within master bounds", all(0<=(p[0]-gx)/step<W and 0<=(gy-p[1])/step<H for ring in coast["geometry_xy_m"]["coordinates"] for p in ring))

# Assess unresolved macro hydrology honestly instead of treating monotonic vectors as a terrain-flow test.
uphill = []
for r in rivers:
    points=[]
    for a,b in zip(r["geometry_xy_m"],r["geometry_xy_m"][1:]):
        count=max(1,math.ceil(math.dist(a,b)/2000))
        points.extend([[a[0]+(b[0]-a[0])*i/count,a[1]+(b[1]-a[1])*i/count] for i in range(count)])
    points.append(r["geometry_xy_m"][-1])
    values=[sample(p) for p in points]
    rises=[b-a for a,b in zip(values,values[1:]) if b>a+.125]
    uphill.append({"id":r["id"],"rising_sample_intervals":len(rises),"maximum_rise_m":max(rises,default=0)})
check("specified river centerlines descend at 2km sampling", all(r["rising_sample_intervals"]==0 for r in uphill), {"uphill_tolerance_m":.125,"scope":"sampled centerlines only, not full basin hydrology"})
report={"status":"PASS" if all(c["pass"] for c in checks) else "FAIL","scope":"coordinate preservation, numeric artifacts, full shared-border reconstruction; not final terrain art or UE validation","checks":checks,"observations":{"river_grid_uphill_intervals":uphill,"road_river_segment_contacts":len(interactions["road_river"]),"road_river_collinear_overlaps":sum(x["type"]=="collinear_overlap" for x in interactions["road_river"]),"unattached_route_vertices":load(ROOT/"data/route-graph.json")["settlements_without_exact_route_vertex"],"lakes":"two symbolic lake centers retained; physical shorelines and bed/surface inconsistency unresolved"}}
(ROOT/"records/verification.json").write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
print(json.dumps({"status":report["status"],"checks":len(checks),"failed":[c for c in checks if not c["pass"]],"max_river_residual_m":residual,"uphill":uphill},ensure_ascii=False))

# Preserve the original world-map display projection for the user-facing overview.
VW,VH=1800,900
ub=[22528/65536,2048/32768,43008/65536,12288/32768]
image=Image.new("RGB",(VW,VH),(19,46,55))
pixels=image.load()
stops=[(0,(68,96,79)),(450,(95,114,81)),(1100,(143,144,107)),(2100,(166,155,130)),(3100,(187,186,173)),(4500,(234,236,230))]
def color(z):
    for (za,a),(zb,b) in zip(stops,stops[1:]):
        if z<=zb:
            t=max(0,min(1,(z-za)/(zb-za)))
            return [a[k]*(1-t)+b[k]*t for k in range(3)]
    return stops[-1][1]
for y in range(VH):
    v=ub[1]+(ub[3]-ub[1])*y/(VH-1)
    for x in range(VW):
        u=ub[0]+(ub[2]-ub[0])*x/(VW-1)
        a,b=project([u,v]);i,j=round((a-gx)/step),round((gy-b)/step)
        if not (1<=i<W-1 and 1<=j<H-1) or not mp[i,j]:
            continue
        z=hp[i,j]*.125-1024
        dx=(hp[i+1,j]-hp[i-1,j])*.125/(2*step)
        dy=(hp[i,j+1]-hp[i,j-1])*.125/(2*step)
        shade=max(.45,min(1.35,.9+(-dx-dy)*5))
        pixels[x,y]=tuple(max(0,min(255,round(c*shade))) for c in color(z))
image.save(ROOT/"previews/geographic-terrain.png")
draw=ImageDraw.Draw(image)
def screen(p):
    lon,lat=inverse(p)
    return (((lon+180)/360-ub[0])/(ub[2]-ub[0])*(VW-1),((90-lat)/180-ub[1])/(ub[3]-ub[1])*(VH-1))
for r in rivers:
    draw.line([screen(p) for p in r["geometry_xy_m"]],fill=(105,194,209),width=2)
for r in roads:
    draw.line([screen(p) for p in r["geometry_xy_m"]],fill=(229,191,119),width=2)
for ring in original["coordinates"]:
    draw.line([((u-ub[0])/(ub[2]-ub[0])*(VW-1),(v-ub[1])/(ub[3]-ub[1])*(VH-1)) for u,v in ring],fill=(136,169,165),width=1)
font_path=Path("C:/Windows/Fonts/malgun.ttf")
font=ImageFont.truetype(str(font_path),19) if font_path.exists() else ImageFont.load_default()
small=ImageFont.truetype(str(font_path),16) if font_path.exists() else font
offsets={"M01":(44,35),"M02":(35,-24),"M03":(20,24),"M04":(-15,-38),"M05":(-48,-30),"M06":(-75,-30),"M07":(20,-30),"M08":(16,-34)}
for s in settlements:
    x,y=screen(s["xy_m"])
    draw.ellipse((x-3,y-3,x+3,y+3),fill=(237,239,230),outline=(28,43,41))
    if s["id"] in offsets:
        dx,dy=offsets[s["id"]];draw.line([(x,y),(x+dx*.7,y+dy),(x+dx,y+dy)],fill=(211,218,212),width=1)
        draw.text((x+dx,y+dy-10),s["id"]+" "+s["name"],font=font,fill="white",stroke_width=2,stroke_fill=(25,39,39))
canvas=Image.new("RGB",(VW,VH+90),(26,30,31))
canvas.paste(image,(0,45));d=ImageDraw.Draw(canvas)
d.text((24,10),"이그니스 공통 지형 원본 · 기존 세계지도 투영 · 배치 데이터 검토",font=font,fill=(232,235,232))
d.text((24,VH+58),"수치 고도 4,097 × 2,561 / 2km 격자   ·   정착지 36   ·   하천 14   ·   육로 회랑 20   ·   청록: 하천 / 황토: 육로",font=small,fill=(211,216,215))
canvas.save(ROOT/"previews/overview.png")
if report["status"]!="PASS":
    raise SystemExit(1)
