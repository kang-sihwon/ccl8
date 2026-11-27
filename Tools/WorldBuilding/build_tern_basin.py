"""테른 유역 좌표 아틀라스의 오프라인 데이터와 지형 음영을 다시 만든다."""
from pathlib import Path
import bisect
import json
import math
import re
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2] / 'Docs/ArtDirection/EtherWorldMap'
ATLAS = ROOT / 'Atlas/VelesTarnelRoden-v1'
DEST = ATLAS / 'TernBasin377'
def read(path):
    return json.loads(path.read_text(encoding='utf8'))
grid = read(ATLAS/'VelesCardinalTiles/tiles.json')
data = read(ATLAS/'coordinates.json')
world = read(ROOT/'Data/ignis-continent-georeference-v1.json')
s2 = read(ROOT/'Data/tarnel-roden-exploration-v1.json')
eco = read(ROOT/'Data/veles-forest-ecology-v2.json')
connections = read(ROOT/'Atlas/VelesDetailTerrain-v1/data/connections.json')
w,h = grid['tile_size_m']
base = grid['tiles']['center']['bounds_xyxy']
limits = [base[0]-188*w,base[1]-188*h,base[2]+188*w,base[3]+188*h]
origin = data['frames']['V']['origin_km']
def local(p):
    return [(p[i]-origin[i])*1000 for i in range(2)]
def shift(ps,off):
    return [[x+off[0],y+off[1]] for x,y in ps]
def cuts(a,b,box):
    lo,hi = 0,1
    for i in range(2):
        delta=b[i]-a[i]
        if abs(delta)<1e-12:
            if not box[i]<=a[i]<=box[i+2]: return False
        else:
            t,u=sorted([(box[i]-a[i])/delta,(box[i+2]-a[i])/delta])
            lo,hi=max(lo,t),min(hi,u)
    return lo<=hi
def inside(p):
    return all(limits[i]<=p[i]<=limits[i+2] for i in range(2))
inventory={}
for k in ['settlements','rivers','routes','ridges','passes','lakes']:
    ids=[]
    for obj in world[k]:
        points=obj.get('xy_km',obj.get('center_xy_km'))
        if not points: continue
        if isinstance(points[0],list):
            ps=list(map(local,points))
            hit=any(cuts(a,b,limits) for a,b in zip(ps,ps[1:]))
        else:
            p=local(points);rad=obj.get('symbol_radius_km',0)*1000
            hit=math.hypot(*(max(limits[i]-p[i],0,p[i]-limits[i+2]) for i in range(2)))<=rad
        if hit:ids.append(obj['id'])
    inventory[k]=ids

# 기존 도판 경계를 통과하는 베지어는 기존 핸들을 포함해 표본을 낸다.
def cubic_rows(d):
    points=np.array([float(v) for v in re.findall(r'[-+]?(?:\d*\.\d+|\d+)',d)]).reshape(-1,2)
    return [points[i:i+4] for i in range(0,len(points)-1,3)]
def sample(rows,step):
    result=[]
    for a,b,c,d in rows:
        n=max(4,math.ceil((np.linalg.norm(b-a)+np.linalg.norm(c-b)+np.linalg.norm(d-c))/step))
        t=np.linspace(0,1,n,endpoint=False)[:,None]
        result.extend(((1-t)**3*a+3*t*(1-t)**2*b+3*t*t*(1-t)*c+t**3*d).tolist())
    return result+[d.tolist()]
def smooth(ps,step=24):
    ps=np.asarray(ps,dtype=float)
    rows=[]
    for i,(a,b) in enumerate(zip(ps,ps[1:])):
        # 급격한 축척 변화에서 짧은 구간 너머로 제어점이 뻗지 않게 한다.
        v=b-ps[max(0,i-1)];u=ps[min(len(ps)-1,i+2)]-a
        size=np.linalg.norm(b-a)/3
        rows.append([a,a+v*size/max(1,np.linalg.norm(v)),b-u*size/max(1,np.linalg.norm(u)),b])
    return sample(rows,step)
def legacy(sheet,line):
    svg=ET.parse(ATLAS/'VelesCardinalTiles'/(sheet+'.svg'))
    return [cubic_rows(el.get('d')) for el in svg.findall(f".//*[@data-line='{line}']")]
def lower_paths():
    svg=ET.parse(ATLAS/'05-tarnel-lower.svg')
    banks=[p for p in svg.getroot() if p.get('stroke')=='#9a9671'][:2]
    road=svg.find(".//*[@id='A07-to-A08']")[0]
    def convert(el):
        return [[-13650+x*.612,-21018-y*.612] for x,y in sample(cubic_rows(el.get('d')),0.8)]
    return [convert(el) for el in banks+[road]]
low_w,low_e,low_road=lower_paths()
def chan(y):
    ps=sorted((py,px) for px,py in map(local,world['rivers'][0]['xy_km']))
    ys,xs=zip(*ps)
    j=max(0,min(len(ps)-2,bisect.bisect_right(ys,y)-1))
    u=(y-ys[j])/(ys[j+1]-ys[j])
    span=ys[j+1]-ys[j]
    # 각 광역 절점을 지나며 양쪽 기울기를 약 500 m 안에서 잇는다.
    slope=(xs[j+1]-xs[j])/span
    x=xs[j]+(y-ys[j])*slope
    for k,delta in [(j,y-ys[j]),(j+1,y-ys[j+1])]:
        if 0<k<len(ys)-1 and abs(delta)<550:
            left=(xs[k]-xs[k-1])/(ys[k]-ys[k-1])
            right=(xs[k+1]-xs[k])/(ys[k+1]-ys[k])
            x+=((left+right)/2-slope)*delta*(1-abs(delta)/550)**2
    return x-470*math.exp(-((y-6000)/690)**2)+44*math.sin(y/480)*math.sin(math.pi*u)**2
def by_y(ps,y):
    ys,xs=zip(*sorted((p[1],p[0]) for p in ps))
    return float(np.interp(y,ys,xs))
def transition(ps,curve):
    # 접점의 기존 미분 방향으로 다음 제어점을 둔다.
    a=np.array(ps[-1]);b=np.array(curve[0]);delta=b-a
    va=a-np.array(ps[-2]);vb=np.array(curve[1])-b
    scale=min(float(np.linalg.norm(delta))*.32,220)
    return sample([[a,a+va*scale/max(.0001,np.linalg.norm(va)),b-vb*scale/max(.0001,np.linalg.norm(vb)),b]],min(8,max(1,scale/12)))
def join(a,b):
    return a+transition(a,b)[1:-1]+b
def regional_banks():
    banks=[]
    for i,lower in enumerate([low_w,low_e]):
        side=(-1,1)[i]
        def reach(ys):
            pts=[]
            for y in sorted(set(ys)):
                wave=math.sin(y/(407+i*81)+i)*11+math.sin(y/819+1)*9
                width=80+wave
                pts.append([chan(y)+side*width,y])
            # 하천 폭과 강안은 수면을 따라 남북순으로 보간한다.
            return pts
        ys=list(range(-54000,-22099,105))+[-50000,-49911,-49441,-44000,-35000,-33000,-22060]
        down=sorted(lower,key=lambda p:p[1])
        trunk=join(reach(ys),down)
        north_of_low=reach(list(range(-20780,-1300,105))+[-20000,-19920,-15000,-5000,-1310])
        # 실제 접안 후보는 주거·하역 마당에서 떨어진 물가에 있다.
        for points,quays in [(trunk,[(-49911,-39903),(-49441,-39270)]),(north_of_low,[(-19920,-12841)])]:
            for row in points:
                for yq,xq in quays:
                    weight=max(0,1-abs(row[1]-yq)/260)
                    quay_side=1 if yq==-19920 else 0
                    if i==quay_side and weight:
                        row[0]+=weight*(xq-(chan(yq)+side*80))
                    if i==quay_side and row[1]==yq:row[0]=xq
        trunk=join(trunk,north_of_low)
        foot=sample(legacy('outer-sw','bank'+str(i))[0],.65)
        head=sample(legacy('outer-ne','bank'+str(i))[0],.65)
        trunk=join(trunk,foot)
        # 중앙 다섯 장은 원 도판이 덮는다.
        trunk=join(trunk,head)
        trunk=join(trunk,reach(list(range(1400,55000,118))+[12000,55000]))
        banks.append(trunk)
    return banks
banks=regional_banks()
water_y=sorted(set(round(p[1],5) for bank in banks for p in bank))
levels=[(np.array([p[1] for p in bank]),np.array([p[0] for p in bank])) for bank in banks]
water=np.column_stack([water_y,*[np.interp(water_y,y,x) for y,x in levels]]).round(4).tolist()
assert all(left<right for y,left,right in water)
lines=[]
def line(id,name,kind,width,ps,source='신규 후보',curved=True):
    points=smooth(ps,max(2,min(60,width*5))) if curved else ps
    lines.append(dict(id=id,name=name,kind=kind,width=width,points=np.round(points,4).tolist(),source=source))
def outerlink(sheet,id,name,kind,width,ps,reverse=False):
    rows=legacy(sheet,id)[0]
    arr=sample(rows,.65)
    if reverse:arr.reverse()
    line(id,name,kind,width,join(arr,smooth(ps)), '5×5 접선과 외곽 시안',False)
f=data['features']
outerlink('outer-w','westpass','숲곁 판재길','road',3.3,[[-790,38],[-1150,-180],[-2048,-454.2909],[-3750,-910],[-5200,-1260],[-6600,-1770],[-7000,-2000]],True)
outerlink('outer-ese','ridge','동녘 능선길','road',1.3,[[815,-196],[1180,-450],[2130,-235],[3040,515],[4700,1700],[6900,1040],[8800,-400],[13000,-1500],[18100,-3300],[24700,-6100]])
outerlink('outer-n','upstream','상류 물가와 취수구','road',2.5,[p for p in f['f-up']['geometry'] if p[1]>=801])
outerlink('outer-n','creek','수원 개울','stream',4.6,[p for p in f['f-water0']['geometry'] if p[1]>=800])
old=legacy('outer-n','workyard')
line('workyard','원목 작업장 귀환길','road',3.35,join(sample(legacy('outer-nnw','workyard')[0],.6),sample(old[1],.6)),'5×5 접선과 서쪽 귀환 좌표',False)
middle_x=np.mean(np.array(water)[:,1:],axis=1)
def river_x(y):
    return float(np.interp(y,water_y,middle_x))
branches=[
 ('ash','숯등 물골',22,[[-35000,45500],[-30300,36400],[-27000,27800],[-22200,21100],[-19700,15900],[-17250,13500],[-13700,8600],[-10000,6800],[-5700,4100],[-2900,2270],[river_x(1750),1750]]),
 ('deer','사슴골 샛물',28,[[45100,49500],[33900,41400],[24280,29970],[20000,25300],[18700,22600],[river_x(20700),20700]]),
 ('clay','황토천',24,[[50500,8050],[40800,2500],[33100,-3300],[24780,-6190],[20000,-7120],[13700,-8050],[7800,-9500],[river_x(-11400),-11400]]),
 ('reed','섶늪 물길',38,[[40500,-22500],[27400,-27000],[16600,-28500],[11800,-32600],[4200,-35500],[-5600,-36700],[-17500,-37500],[river_x(-38700),-38700]]),
 ('mill','모래턱 샛물',18,[[-48200,-12500],[-41700,-16500],[-38100,-23700],[-31800,-26900],[-28200,-30100],[-25900,-32240],[river_x(-33300),-33300]])]
for id,name,width,ps in branches:line(id,name,'stream',width,ps)
roads=[
 ('pine','거목 돌봄길',5,[[-1150,-180],[-1440,700],[-720,2050],[300,2800],[1280.769,3584],[2400,4900],[4000,6000]]),
 ('grain','남서 곡물길',6,[[-1150,-180],[-1500,-850],[-2048,-1233.6],[-4200,-2850],[-6900,-5370],[-8700,-7000],[-10000,-8000]]),
 ('market','타르넬 곡물 운반로',5,[[-10000,-8000],[-11800,-9600],[-14200,-12700],[-15400,-15300],[-15200,-18100],[-14500,-19300],[-13300,-19700],[-12640,-19920]]),
 ('pass','서쪽 돌짐길',3.5,[[-7000,-2000],[-11000,-1350],[-15500,-3510],[-20300,-4200],[-28700,-3200],[-33600,-3800]]),
 ('coal','숯막 짐길',3,[[-11000,-1350],[-12800,4600],[-15400,9700],[-17100,13500],[-20100,18400],[-24000,22900],[-26700,27600]]),
 ('upper','상류 표지길',3,[[4000,6000],[6300,8400],[10500,13700],[14200,17300],[18000,20900],[20500,26300],[24000,30000]]),
 ('marsh','갈대 운반 둑길',3,[[24700,-6100],[20100,-10900],[15500,-17400],[13500,-24500],[8500,-30200],[4100,-35400]]),
 ('bell','바람등 돌표길',2,[[4100,-35400],[10200,-36300],[19300,-33800],[24200,-35800],[31700,-38600]]),
 ('millgate','수차 점검길',3,[[-12640,-19920],[-15700,-23000],[-18900,-26200],[-22200,-28100],[-25800,-32200],[-29600,-36800],[-32000,-40400],[-36600,-45700],[-40000+40,-50000+90]])]
for id,name,width,ps in roads:line(id,name,'road',width,ps)
line('weir','어살에서 쉼나루','road',7*.612,low_road,'타르넬 하류 상세도',False)
details=[]
for key,v in f.items():
    frame=v['frame']
    if frame not in ['V','T','R']:continue
    off=local(data['frames'][frame]['origin_km'])
    geom=v['geometry']
    point=isinstance(geom[0],(int,float))
    coords=shift([geom] if point else geom,off)
    item=dict(id=key,name=v['name'],kind=v['kind'],frame=frame,p=coords)
    if key.startswith('draft-') or key.startswith('opening-'):continue
    if key.startswith('t-') and key in ['t-out','t-short']:
        pts=item['p']
        if key=='t-out':item['p']=pts[:-2]
        else:item['p']=pts[2:]
    if key=='t-back':item['p']=item['p'][1:]
    if key in ('f-water0','f-water1','f-up','f-west','T-river','R-river'):continue
    details.append(item)
for group in s2['groups']:
    off=local(data['frames'][group['area']]['origin_km'])
    group['home']=shift([group['home_xy_m']],off)[0]
    group['route']=shift(group['patrol_xy_m'],off)
for z in eco['zones']:
    details.append(dict(id='eco-'+z['id'],frame='V',name=z['name'],kind='ecozone',p=z['polygon_m']))
# 2 km 고도의 부드러운 음영. 큰 그림이 사용하는 범례와 높이 단위를 유지한다.
terrain=ROOT/'Atlas/IgnisTerrainMaster-v1'
m=read(terrain/'manifest.json')
grid2=m['grid']; x0,y0=grid2['origin_xy_m']; sp=grid2['spacing_m']
elev=np.asarray(Image.open(terrain/m['height']['file']),dtype=float)
size=960
xs=(np.linspace(limits[0],limits[2],size)+origin[0]*1000-x0)/sp
ys=(y0-origin[1]*1000-np.linspace(limits[3],limits[1],size))/sp
ix=xs.astype(int);iy=ys.astype(int);dx=xs-ix;dy=(ys-iy)[:,None]
def row(j):
    return elev[np.ix_(iy+j,ix)]*(1-dx)+elev[np.ix_(iy+j,ix+1)]*dx
z=(row(0)*(1-dy)+row(1)*dy)*.125-1024
gy,gx=np.gradient(z)
light=np.clip(.52+(gx-gy)*.32,.18,.82)
bands=(z/12)%1
shade=light*82+112-np.exp(-(np.minimum(bands,1-bands)/.065)**2)*17
rx,ry=np.meshgrid(np.linspace(limits[0],limits[2],size),np.linspace(limits[3],limits[1],size))
wx=rx+1000*np.sin(ry/6800)+490*np.cos((rx+ry)/3600)
wy=ry+850*np.sin(rx/5100)
weights=[];colors=[]
for x,y,col in re.findall(r'^ \["[^"]+",(-?\d+),(-?\d+),"(#[0-9a-f]{6})"',(DEST/'lore.js').read_text(encoding='utf8'),re.M):
    weights.append(((wx-int(x))**2+(wy-int(y))**2)/20e6)
    colors.append([int(col[i:i+2],16) for i in (1,3,5)])
dist=np.stack(weights);mix=np.exp(np.min(dist,axis=0)-dist)
rgb=np.einsum('ijk,ic->jkc',mix,np.array(colors))/np.sum(mix,axis=0)[:,:,None]
relief=shade-154+3*np.sin(rx/537+np.sin(ry/301))+2*np.cos(ry/780+rx/412)
Image.fromarray(np.uint8(np.clip(rgb+relief[:,:,None],0,255))).save(DEST/'relief.webp',quality=90)

ns='http://www.w3.org/2000/svg'
ET.register_namespace('',ns)
sheet=ET.parse(ATLAS/'VelesCardinalTiles/outer-n.svg')
trees=ET.Element('{'+ns+'}svg',width='768',height='64',viewBox='0 0 768 64')
for j in range(12):
    group=ET.SubElement(trees,'{'+ns+'}g',transform=f'translate({64*j+32} 32) scale(20 -20)')
    group.append(sheet.find(f".//*[@id='tree{j}']"))
(DEST/'crowns.svg').write_text(ET.tostring(trees,encoding='unicode'),encoding='utf8')
lower=ET.parse(ATLAS/'05-tarnel-lower.svg').getroot()
marks=[]
active=False
for el in lower:
    key=el.get('id')
    if key=='notes':break
    if key in ['from-A10','toward-A09']:
        for child in el:marks.append(child.attrib)
    if active and el.tag.endswith('path'):marks.append(el.attrib)
    if key=='toward-A09':active=True
inner=ET.parse(ATLAS/'VelesCardinalTiles/southwest.svg')
old_banks=[sample(cubic_rows(p.get('d')),2) for p in inner.findall(".//*[@stroke='#8e9268']")[:2]]
def respace(ps):
    ps=np.asarray(ps);length=np.r_[0,np.cumsum(np.linalg.norm(np.diff(ps,axis=0),axis=1))]
    return np.column_stack([np.interp(np.linspace(0,length[-1],600),length,ps[:,i]) for i in (0,1)])
middle=(respace(old_banks[0])+respace(old_banks[1]))/2
stop=int(np.argmin(np.linalg.norm(middle-[16.3,-9],axis=1)))
voyage=[[16.3,5.5]]+middle[:stop+1][::-1].round(3).tolist()
for y in range(math.floor(voyage[-1][1])-1,-49911,-12):
    voyage.append([round(river_x(y),3),y])
voyage.append([-39903,-49911])
result=dict(grid=grid['tiles'],base=base,size=[w,h],limits=limits,inventory=inventory,water=water,lines=lines,details=details,groups=s2['groups'],states=data['states'],weirMarks=marks,voyage=voyage)
(DEST/'geography.js').write_text('const geo = '+json.dumps(result,ensure_ascii=False,separators=(',',':'))+';\n',encoding='utf8')
print(inventory, len(water),len(lines),len(details))

