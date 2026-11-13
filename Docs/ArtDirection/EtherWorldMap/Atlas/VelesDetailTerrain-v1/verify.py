"""Independent artifact, parent-coordinate and placement checks. Requires Pillow."""
import hashlib
import json
import math
from pathlib import Path
from PIL import Image, ImageChops, ImageStat, ImageDraw

ROOT=Path(__file__).resolve().parent
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
m=load(ROOT/'manifest.json')
parent_m=load(ROOT/'../IgnisTerrainMaster-v1/manifest.json')
basis=load(ROOT/'../../Data/worldtree-settlement-basis-v1.json')
ecology=load(ROOT/'../../Data/veles-forest-ecology-v2.json')
placement=load(ROOT/'data/placements.json')
links=load(ROOT/'data/connections.json')
height=Image.open(ROOT/m['height']['file']); hp=height.load()
parent=Image.open(ROOT/'../IgnisTerrainMaster-v1/terrain/height16.png'); pp=parent.load()
B=m['bounds_local_m']; origin=m['origin_regional_xy_m']; checks=[]
def check(name,passed,detail=None): checks.append({'name':name,'pass':bool(passed),'detail':detail})
def parent_value(p):
    u=(p[0]+origin[0]-parent_m['grid']['origin_xy_m'][0])/2000
    v=(parent_m['grid']['origin_xy_m'][1]-p[1]-origin[1])/2000
    i,j=math.floor(u),math.floor(v); a,b=u-i,v-j
    return (pp[i,j]*(1-a)+pp[i+1,j]*a)*(1-b)+(pp[i,j+1]*(1-a)+pp[i+1,j+1]*a)*b
def quantize(v):return math.floor(v+.5)
def distance(p,a,b):
    dx,dy=b[0]-a[0],b[1]-a[1]; d=dx*dx+dy*dy
    t=max(0,min(1,((p[0]-a[0])*dx+(p[1]-a[1])*dy)/d)) if d else 0
    return math.hypot(p[0]-a[0]-t*dx,p[1]-a[1]-t*dy),t
def line_distance(p,points): return min(distance(p,a,b)[0] for a,b in zip(points,points[1:]))
check('parent height source unchanged',hashlib.sha256((ROOT/'../IgnisTerrainMaster-v1/terrain/height16.png').read_bytes()).hexdigest()==m['parent']['height_sha256'])
check('Veles origin preserved',origin==[v*1000 for v in basis['local_frame']['origin_regional_km']])
check('all 15 local reference points preserved',len(placement['pois'])==15 and all(p['point']==basis['local_points_m'][p['id']] for p in placement['pois']))
check('local river and stream anchors preserved',all(p in links['river']['points'] for p in basis['local_river_m']) and links['stream']['points']==basis['spring_water_m'])
check('all existing forest paths preserved',all(next(r for r in links['roads'] if r['id']==p['id'])['points']==[ecology['route_nodes_m'][n] for n in p['nodes']] for p in ecology['paths']))
check('2m 16-bit child terrain',height.size==(2049,3073) and (ROOT/m['height']['file']).read_bytes()[24]==16 and m['height']['spacing_m']==2)
edge_errors=[]
for j in range(height.height):
    for i in [0,height.width-1]:
        p=[B[0]+i*2,B[3]-j*2]
        if hp[i,j]!=quantize(parent_value(p)):edge_errors.append([i,j])
for i in range(height.width):
    for j in [0,height.height-1]:
        p=[B[0]+i*2,B[3]-j*2]
        if hp[i,j]!=quantize(parent_value(p)):edge_errors.append([i,j])
check('entire outer edge equals encoded parent',not edge_errors,{'samples':2*height.width+2*height.height,'mismatches':len(edge_errors)})
anchors=[]
for x in range(math.ceil((B[0]+origin[0])/2000)*2000,math.floor((B[2]+origin[0])/2000)*2000+1,2000):
    for y in range(math.ceil((B[1]+origin[1])/2000)*2000,math.floor((B[3]+origin[1])/2000)*2000+1,2000):
        p=[x-origin[0],y-origin[1]];i=round((p[0]-B[0])/2);j=round((B[3]-p[1])/2)
        anchors.append(hp[i,j]==quantize(parent_value(p)))
check('every inherited parent lattice vertex exact',all(anchors),{'vertices':len(anchors)})
tilemap={};bad=[]
for t in m['tile_layout']['tiles']:
    c,r=int(t['id'][1]),int(t['id'][4]);im=Image.open(ROOT/t['height']);tilemap[c,r]=im
    if im.size!=(513,513) or im.tobytes()!=height.crop((c*512,r*512,c*512+513,r*512+513)).tobytes():bad.append(t['id'])
check('all 24 height tiles reconstruct master exactly',len(tilemap)==24 and not bad,bad)
seams=[]
for (c,r),im in tilemap.items():
    if (c+1,r) in tilemap:seams.append(im.crop((512,0,513,513)).tobytes()==tilemap[c+1,r].crop((0,0,1,513)).tobytes())
    if (c,r+1) in tilemap:seams.append(im.crop((0,512,513,513)).tobytes()==tilemap[c,r+1].crop((0,0,513,1)).tobytes())
check('all 38 interior height borders exact',len(seams)==38 and all(seams))
master_river=next(r for r in load(ROOT/'../IgnisTerrainMaster-v1/data/rivers.json')['rivers'] if r['id']=='H01')
river_edges=[b for b in links['boundaries'] if b['kind']=='river']
check('two river exits match parent H01',len(river_edges)==2 and all(line_distance(b['global_xy_m'],master_river['geometry_xy_m'])<1e-6 and b['width']==master_river['width_m_candidate'] for b in river_edges))
check('three approach exits keep shared coordinates',len([b for b in links['boundaries'] if b['kind']=='road'])==3 and all(b['global_xy_m']==[b['point'][i]+origin[i] for i in [0,1]] for b in links['boundaries']))
g=load(ROOT/'../../Data/ignis-continent-georeference-v1.json')
check('neighbor settlement anchors preserved',all([v*1000 for v in next(s for s in g['settlements'] if s['id']==n['id'])['xy_km']]==[n['point'][i]+origin[i] for i in [0,1]] for n in links['neighbor_anchors']))

def overlaps(a,b):
    for p in (a,b):
        for i in range(4):
            v=[p[(i+1)%4][k]-p[i][k] for k in [0,1]];axis=[-v[1],v[0]]
            av=[q[0]*axis[0]+q[1]*axis[1] for q in a];bv=[q[0]*axis[0]+q[1]*axis[1] for q in b]
            if max(av)<=min(bv) or max(bv)<=min(av):return False
    return True
buildings=placement['buildings'];collisions=[]
for i,b in enumerate(buildings):
    for a in buildings[i+1:]:
        if overlaps(a['footprint'],b['footprint']):collisions.append([a['id'],b['id']])
check('building footprints do not overlap',not collisions,collisions)
def line_hits_building(a,b,building,pad):
    angle=-building['angle'];c,s=math.cos(angle),math.sin(angle)
    def local(p):
        x,y=p[0]-building['center'][0],p[1]-building['center'][1]
        return [x*c-y*s,x*s+y*c]
    a,b=local(a),local(b);low,high=0.,1.
    for k,half in enumerate([building['width']/2+pad,building['depth']/2+pad]):
        d=b[k]-a[k]
        if abs(d)<1e-10:
            if abs(a[k])>=half:return False
        else:
            u,v=(-half-a[k])/d,(half-a[k])/d;low=max(low,min(u,v));high=min(high,max(u,v))
            if low>=high:return False
    return high>0 and low<1
road_hits=[]
for b in buildings:
    for r in links['roads']:
        if r.get('building')==b['id']:continue
        if any(line_hits_building(a,c,b,r['width']/2) for a,c in zip(r['points'],r['points'][1:])):road_hits.append([b['id'],r['id']])
check('roads and other entrances clear every building',not road_hits,road_hits)
doors=[]
for b in buildings:
    r=next(r for r in links['roads'] if r.get('building')==b['id']);target=next(q for q in links['roads'] if q['id']==r['joins'])
    doors.append(r['points'][0]==b['door'] and line_distance(r['points'][-1],target['points'])<1e-6)
check('every building entrance joins an existing lane',all(doors),{'buildings':len(buildings)})
source_bridges=load(ROOT/'../../Data/veles-opening-connections-v1.json')['bridges']
check('four original stream crossings retained',all(any(b.get('reference_id')==s['id'] and math.dist(b['center'],s['xy_m'])<1e-6 for b in placement['bridges']) for s in source_bridges))
bank_hits=[]
for b in buildings:
    for p in b['footprint']:
        candidates=[(distance(p,a,c)[0],i,distance(p,a,c)[1]) for i,(a,c) in enumerate(zip(links['river']['points'],links['river']['points'][1:]))]
        d,i,t=min(candidates);half=(links['river']['widths'][i]*(1-t)+links['river']['widths'][i+1]*t)/2
        if d<half+1 or line_distance(p,links['stream']['points'])<2.25:bank_hits.append(b['id']);break
check('building footprints clear both watercourses',not bank_hits,bank_hits)
check('underground locations not replaced by surface buildings',all(p['surface']=='underground' for p in placement['pois'] if p['id'] in ('boss','old_workshop')) and all(b['id'] not in ('boss','old_workshop') for b in buildings))
texture_report=[]
if all((ROOT/t['texture']).exists() for t in m['tile_layout']['tiles']):
    textures={(int(t['id'][1]),int(t['id'][4])):Image.open(ROOT/t['texture']).convert('RGB') for t in m['tile_layout']['tiles']}
    check('24 native rendered detail tiles exported',len(textures)==24 and all(im.size==(2112,2112) for im in textures.values()))
    for (c,r),im in textures.items():
        for direction,neighbor,a,b in [('east',(c+1,r),(2048,0,2112,2112),(0,0,64,2112)),('south',(c,r+1),(0,2048,2112,2112),(0,0,2112,64))]:
            if neighbor not in textures:continue
            diff=ImageChops.difference(im.crop(a),textures[neighbor].crop(b));stat=ImageStat.Stat(diff)
            texture_report.append({'tile':[c,r],'direction':direction,'maximum_rgb_delta':max(x[1] for x in diff.getextrema()),'mean_rgb_delta':sum(stat.mean)/3})
    check('all 38 independently rendered texture overlaps consistent',len(texture_report)==38 and all(s['mean_rgb_delta']<.1 and s['maximum_rgb_delta']<=12 for s in texture_report),{'maximum':max(s['maximum_rgb_delta'] for s in texture_report),'mean_max':max(s['mean_rgb_delta'] for s in texture_report),'tolerance':'minor rasterization rounding only, not semantic seam repair'})
    # Assemble the actual exported tile cores, rather than rendering an overview.
    assembly=Image.new('RGB',(2048,3072))
    for (c,r),im in textures.items():
        core=im.crop((32,32,2080,2080)).resize((512,512),Image.Resampling.LANCZOS)
        assembly.paste(core,(c*512,r*512))
    assembly.save(ROOT/'previews/assembled.png')
    draw=ImageDraw.Draw(assembly)
    for x in range(0,2048,512):draw.line((x,0,x,3071),fill='#b8c8c6',width=1)
    for y in range(0,3072,512):draw.line((0,y,2047,y),fill='#b8c8c6',width=1)
    for c,r in textures:draw.text((c*512+12,r*512+12),f'C{c}-R{r}',fill='white',stroke_width=2,stroke_fill='#202827')
    assembly.save(ROOT/'previews/assembled-grid.png')
else:check('24 native rendered detail tiles exported',False,'missing texture exports')
report={'status':'PASS' if all(c['pass'] for c in checks) else 'FAIL','scope':'parent inheritance, encoded terrain seams, placement clearance, preserved anchors, exported texture overlaps; not gameplay or structural verification','checks':checks,'texture_overlaps':texture_report}
(ROOT/'records/verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'status':report['status'],'checks':len(checks),'failed':[c for c in checks if not c['pass']]},ensure_ascii=False))
if report['status']!='PASS':raise SystemExit(1)
