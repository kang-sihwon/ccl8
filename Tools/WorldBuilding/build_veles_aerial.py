"""Build the approved full-village composition in a separate playable UE level."""
import json
import math
import random
import runpy
from collections import defaultdict
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
HERE = ROOT / "Tools/WorldBuilding"
kit = runpy.run_path(str(HERE / "refine_veles.py"))
mesh = kit["make_mesh"]
mesh.__globals__["folder"] = "/Game/Level/VelesRiversideAssets"
box, actor_mesh, asset = kit["box"], kit["set_mesh"], kit["asset"]
actors, editor = kit["actors"], kit["editor"]
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
MAP = "/Game/Level/VelesRiverside"
OUT = ROOT / "Saved/Tests/VelesRiverside"
rng = random.Random(251125)

# 남쪽 언덕에서 북쪽(+Y)을 보면 화면 오른쪽은 -X다. 좌표 단위는 m다.
quay = [(15,18),(13,30),(7,42),(5,55),(0,65)]
shop = [(-54,52),(-41,53),(-29,58),(-17,64),(0,65)]
forest = [(0,65),(8,78),(12,88),(12,101),(6,110),(-9,119),
          (-18,135),(-32,151),(-51,166),(-65,180),(-80,205)]
lanes = [quay,shop,forest,
    [(5,55),(23,53),(38,60),(49,74),(55,94),(51,110),(34,124),(12,133),(-18,135)],
    [(12,88),(25,83),(41,82),(49,74)],
    [(6,110),(23,108),(38,102),(55,94)],
    [(0,65),(-15,78),(-21,91),(-14,104),(-9,119)],
    [(-41,53),(-50,76),(-45,96),(-36,113),(-18,135)],
    [(-21,91),(-33,94),(-45,96)],
    [(34,124),(42,146),(28,162),(8,170),(-15,165),(-32,151)],
    [(12,133),(9,149),(8,170)],
    [(8,170),(2,187),(-19,195),(-39,181),(-51,166)]]
tree = (-3,94)
plots = [(85,40,49,6,-8),(84,50,54,6,-6),(84,60,49,6,-3),
         (91,71,48,6,2),(94,82,43,6,6),(89,94,41,6,10)]
footprints = []


def shore(x):
    return 11 + 3.4*math.sin(x*.047) + 2.5*math.sin(x*.096+.5)


def raw_bed(x,y):
    edge = shore(x)
    far = edge-29-3*math.sin(x*.032)
    bank = max(y-edge,far-y)
    if bank < 0:
        return max(-2.6,.02+bank*.29)
    rise = min(bank,8)*.16 + max(0,bank-8)*(.031 if y>edge else .067)
    rough = .28*math.sin(x*.051+y*.022) + .14*math.cos(y*.083)
    upland = max(0,y-180)*.12 + max(0,abs(x)-127)*.085
    return .02+rise+min(bank/10,1)*rough+upland*(1+.45*math.sin(x*.035+y*.018))


def bed(x,y):
    z=raw_bed(x,y)
    for h in footprints:
        if abs(x-h["x"])>22 or abs(y-h["y"])>22:continue
        u,v=kit["local"](h,x,y)
        gap=max(abs(u)-h["w"]/2,abs(v)-h["d"]/2)
        if gap<5:
            blend=min(1,max(0,(5-gap)/2.3))
            z+=blend*(h["z"]-.055-z)
    return z


def group():
    return ([],[],[],[])


def quad(g,vertices,uv=None):
    v,f,t,_ = g
    i=len(v)
    v.extend(vertices)
    a,b,c,d=vertices
    width,depth=math.dist(a,b)/2.2,math.dist(b,c)/2.2
    t.extend(uv or [(0,0),(width,0),(width,depth),(0,depth)])
    f.extend([(i,i+1,i+2),(i,i+2,i+3)])


def line_beam(g,a,b,width,depth):
    # 기울어진 각재의 단면을 유지한다.
    start=len(g[0])
    dx,dy,dz=[q-p for p,q in zip(a,b)]
    length=math.dist(a,b)
    horizontal=math.hypot(dx,dy)
    nx,ny=(-dy/horizontal,dx/horizontal) if horizontal else (1,0)
    side=(nx,ny,0)
    unit=(dx/length,dy/length,dz/length)
    up=(-unit[2]*ny,unit[2]*nx,unit[0]*ny-unit[1]*nx)
    box(g,0,length,-depth/2,depth/2,-width/2,width/2)
    for i in range(start,len(g[0])):
        s,t,h=g[0][i]
        g[0][i]=tuple(a[j]+unit[j]*s+side[j]*t+up[j]*h for j in range(3))


def paint_material():
    folder=mesh.__globals__["folder"]
    name="M_WeatheredTeal"
    if kit["lib"].does_asset_exist(folder+"/"+name):
        return unreal.load_asset(folder+"/"+name)
    m=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    e=unreal.MaterialEditingLibrary
    wood=e.create_material_expression(m,unreal.MaterialExpressionTextureSample)
    wood.texture=asset("Textures/Medieval_Environment/T_CF_01_Wood_01_BC")
    tint=e.create_material_expression(m,unreal.MaterialExpressionConstant3Vector)
    tint.constant=unreal.LinearColor(.32,.82,.73,1)
    multiply=e.create_material_expression(m,unreal.MaterialExpressionMultiply)
    e.connect_material_expressions(wood,"RGB",multiply,"A")
    e.connect_material_expressions(tint,"",multiply,"B")
    e.connect_material_property(multiply,"",unreal.MaterialProperty.MP_BASE_COLOR)
    e.recompile_material(m)
    assert kit["lib"].save_loaded_asset(m,False)
    return m


def surface_material(name,textures,tints,norm=None,water=False):
    folder=mesh.__globals__["folder"]
    if kit["lib"].does_asset_exist(folder+"/"+name):return unreal.load_asset(folder+"/"+name)
    m=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    e=unreal.MaterialEditingLibrary
    def node(kind):return e.create_material_expression(m,kind)
    def link(a,b,pin,port=""):e.connect_material_expressions(a,port,b,pin)
    def product(a,b):
        mul=node(unreal.MaterialExpressionMultiply);link(a,mul,"A");link(b,mul,"B");return mul
    mask=node(unreal.MaterialExpressionVertexColor)
    for i,(path,color) in enumerate(zip(textures,tints)):
        tex=node(unreal.MaterialExpressionTextureSample);tex.texture=asset("Textures/"+path)
        tint=node(unreal.MaterialExpressionConstant3Vector);tint.constant=unreal.LinearColor(*color,1)
        layer=product(tex,tint)
        if i:
            blend=node(unreal.MaterialExpressionLinearInterpolate)
            link(base,blend,"A");link(layer,blend,"B");link(mask,blend,"Alpha","RG"[i-1]);base=blend
        else:base=layer
    e.connect_material_property(base,"",unreal.MaterialProperty.MP_BASE_COLOR)
    if norm:
        tex=node(unreal.MaterialExpressionTextureSample);tex.texture=asset("Textures/"+norm)
        tex.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
        if water:
            flow=node(unreal.MaterialExpressionPanner)
            flow.set_editor_property("speed_x",.015);flow.set_editor_property("speed_y",.008)
            link(flow,tex,"Coordinates")
        e.connect_material_property(tex,"RGB",unreal.MaterialProperty.MP_NORMAL)
    rough=node(unreal.MaterialExpressionConstant);rough.r=.30 if water else .88
    e.connect_material_property(rough,"",unreal.MaterialProperty.MP_ROUGHNESS)
    if water:m.set_editor_property("two_sided",True)
    e.recompile_material(m);assert kit["lib"].save_loaded_asset(m,False)
    return m


def wall_openings(front,half):
    holes=[(0,1.04,0,2.70,"door")] if front else []
    centers=(-half*.58,half*.58) if front or half>5 else (0,)
    holes.extend((p,.84,.84,2.27,"window") for p in centers)
    return sorted(holes)


def make_house(name,w,d,eave,ridge,variant,materials):
    plaster,stone,timber,tile,paint=[group() for _ in range(5)]
    openings=[]
    box(stone,-w/2,w/2,-d/2,d/2,-.72,-.13)
    box(timber,-w/2,w/2,-d/2,d/2,-.13,0)
    for side in range(4):
        front=side<2
        half=(w if front else d)/2
        sign=-1 if side%2==0 else 1
        wall=sign*((d if front else w)/2-.16)
        def at(p,depth,z):
            return (p,wall+sign*depth,z) if front else (wall+sign*depth,p,z)
        def slab(g,left,right,base,top,thick=.30,offset=0):
            if right-left<.005 or top-base<.005:return
            a,b=at(left,offset-thick/2,base),at(right,offset+thick/2,top)
            box(g,min(a[0],b[0]),max(a[0],b[0]),min(a[1],b[1]),max(a[1],b[1]),base,top)
        def brace(a,b,z,t,thick=.16):
            line_beam(timber,at(a,.19,z),at(b,.19,t),thick,.15)
        last=-half
        for p,r,sill,head,kind in wall_openings(front,half):
            slab(plaster,last,p-r,0,eave)
            slab(stone,last,p-r,-.26,.52,.37)
            slab(plaster,p-r,p+r,head,eave)
            slab(plaster,p-r,p+r,0,sill)
            slab(stone,p-r,p+r,-.27,min(.52,sill),.37)
            slab(timber,p-r-.13,p-r,sill,head,.39)
            slab(timber,p+r,p+r+.13,sill,head,.39)
            slab(timber,p-r-.16,p+r+.16,head,head+.16,.41)
            if sill:
                slab(timber,p-r-.1,p+r+.1,sill-.11,sill,.43)
                # 덧문은 개구부 옆에 접는다.
                for flank in (-1,1):
                    for j in range(3):
                        center=p+flank*(r+.18+j*.175)
                        slab(paint,center-.079,center+.079,sill+.03,head-.04,.10,.23)
                    center=p+flank*(r+.35)
                    slab(timber,center-.26,center+.26,sill+.27,sill+.34,.08,.303)
                    slab(timber,center-.26,center+.26,head-.32,head-.25,.08,.303)
            openings.append(dict(kind=kind,side=side,p=p,k=wall,sill=sill,head=head))
            last=p+r
        slab(plaster,last,half,0,eave)
        slab(stone,last,half,-.26,.52,.37)
        for p in (-half+.13,half-.13):
            slab(timber,p-.12,p+.12,-.22,eave,.45)
        slab(timber,-half,half,eave-.22,eave,.43)
        if eave>3.25:
            slab(timber,-half,half,2.90,3.07,.40)
            count=max(2,round(half/1.85))
            for j in range(count):
                a=-half+2*half*j/count
                b=a+2*half/count
                brace(a,a,3.05,eave-.12)
                brace(a,b,3.08 if j%2 else eave-.12,eave-.12 if j%2 else 3.08)
        if not front:
            a,b,c=at(-half,0,eave-.02),at(0,0,ridge),at(half,0,eave-.02)
            # 박공면 양쪽이 보이도록 얇은 외벽에 양면을 만든다.
            v,f,uv,_=plaster
            for points in ((a,b,c),(c,b,a)):
                i=len(v);v.extend(points);f.append((i,i+1,i+2))
                uv.extend(((0,0),(half/2,(ridge-eave)/2),(half,0)))
            brace(-half,0,eave,ridge,.20)
            brace(0,half,ridge,eave,.20)
            brace(0,0,eave,ridge-.1,.20)
            for p in (-half*.53,half*.53):
                brace(p,p,eave,eave+(ridge-eave)*.44,.15)
            # 다락 환기구의 어두운 목재 루버.
            slab(timber,-.49,.49,eave+.18,min(ridge-.25,eave+1.12),.12,.21)
            for j in range(4):
                slab(paint,-.39,.39,eave+.27+j*.17,eave+.33+j*.17,.10,.30)
    # 기와의 줄이 처마를 따라가고 용마루와 풍판이 지붕 단면을 닫는다.
    xl,xr=-w/2-.52,w/2+.52
    for sign in (-1,1):
        edge=sign*(d/2+.62); z=eave-.32
        vs=[(xl,edge,z),(xl,0,ridge),(xr,0,ridge),(xr,edge,z)]
        if sign<0:vs.reverse()
        # UE uses clockwise front faces. Both sides retain an underside in street views.
        for vertices in (vs,list(reversed(vs))):
            quad(tile,vertices,[(p[0]/2.1,math.hypot(p[1],ridge-p[2])/2.1) for p in vertices])
        line_beam(timber,(xl,edge,z-.12),(xr,edge,z-.12),.23,.23)
        for x in (xl,xr):line_beam(timber,(x,edge,z-.10),(x,0,ridge-.10),.23,.18)
    box(tile,xl,xr,-.14,.14,ridge-.06,ridge+.14)
    # 큰 집에는 길 쪽 지붕을 뚫고 내민 박공 다락을 둔다.
    if variant in (0,1,3,4,7):
        offsets=(-w*.24,w*.24) if variant==0 else (w*.13,)
        for mid in offsets:
            half=min(2.1,w*.145);yf=-d/2-.15;yb=-.45
            low=eave+.45;peak=low+half*1.06
            box(plaster,mid-half,mid+half,yf,yf+.26,eave-.16,low)
            v,f,uv,_=plaster;i=len(v)
            v.extend(((mid-half,yf,low),(mid+half,yf,low),(mid,yf,peak)))
            uv.extend(((0,0),(half,0),(half/2,half/2)));f.extend(((i,i+1,i+2),(i+2,i+1,i)))
            for s in (-1,1):
                side=mid+s*(half+.22)
                pts=[(side,yf-.25,low-.13),(mid,yf-.25,peak+.12),(mid,yb,peak+.12),(side,yb,low-.13)]
                for vs in (pts,list(reversed(pts))):quad(tile,vs)
                line_beam(timber,(side,yf-.26,low-.15),(mid,yf-.26,peak+.11),.16,.14)
                box(timber,mid+s*half-.09,mid+s*half+.09,yf-.13,yf+.13,eave-.20,low)
            box(timber,mid-.55,mid+.55,yf-.10,yf,eave-.02,low+.54)
            for j in (-1,0,1):
                box(paint,mid+j*.29-.11,mid+j*.29+.11,yf-.16,yf-.11,eave+.07,low+.43)
    cx,cy=w*(.25 if variant%2 else -.25),d*.19
    roof_at=ridge-(ridge-eave)*.38
    box(stone,cx-.43,cx+.43,cy-.39,cy+.39,roof_at-.45,ridge+.52)
    box(stone,cx-.52,cx+.52,cy-.48,cy+.48,ridge+.43,ridge+.65)
    # 낮은 처마를 기둥으로 받쳐 문 앞에 실제 작업 공간을 만든다.
    span=min(w*.40,6.4) if variant%3==0 else 2.15
    reach=3.2 if variant%3==0 else 1.68
    y0,y1=-d/2+.05,-d/2-reach
    top=min(eave-.12,3.96)
    for x in (-span,span):
        box(timber,x-.12,x+.12,y1-.12,y1+.12,-.26,2.95)
        line_beam(timber,(x,y1,2.91),(x,y0,top),.17,.18)
        line_beam(timber,(x,y1,2.06),(x,y1+.76,3.02),.15,.16)
    vs=[(-span-.22,y1-.19,2.94),(-span-.22,y0,top+.1),(span+.22,y0,top+.1),(span+.22,y1-.19,2.94)]
    for verts in (vs,list(reversed(vs))):quad(tile if variant%2 else timber,verts)
    return mesh("Home_"+name+"_PlainSill",[plaster,stone,timber,tile,paint],materials),openings


def plan():
    for i,path in enumerate(lanes):
        kit["add_road"](path,4.6 if i<3 else 3.1)
    for angle in range(0,360,20):
        a,b=map(math.radians,(angle,angle+20))
        kit["add_road"]([(tree[0]+10*math.cos(t),tree[1]+9*math.sin(t)) for t in (a,b)],4)
    # 독립된 마당마다 집의 장축과 규모를 달리하고 샘물길과 밭의 자리를 비운다.
    types=[("Hall",27,14,5.45,10.6),("Longhouse",19,8.7,3.7,8.4),
           ("Croft",12,8.6,3.32,7.6),("GabledHouse",14.3,9.4,4.75,9.4),
           ("WeaversHouse",16.6,9.5,4.32,8.6),("Cottage",10.6,7.8,3.38,7.3),
           ("Store",15,8.7,3.4,7.8),("UpperHouse",13,10,5.30,10.0)]
    seeds=[("workshop",0,-56,40,-5),("RepairWing",1,-71,59,83),("QuayHouse",2,0,34,18),
           ("Boatwright",4,-21,40,-12),("WaterHouse",5,-9,59,34),
           ("SouthGable",3,26,37,9),("OrchardHouse",1,51,40,8),
           ("Baker",3,28,67,-16),("Dyer",2,3,77,67),("MarketHouse",7,-24,77,26),
           ("WestGable",3,36,91,70),("MarketWeaver",4,-35,81,80),
           ("Leatherworker",1,-58,84,84),("YardHouse",5,-62,106,-9),
           ("CommonHouse",3,22,98,82),("UpperCommon",7,0,119,-23),
           ("Fletcher",2,-28,109,28),("SmithsNeighbor",3,-46,120,63),
           ("GardenGable",5,57,124,-30),("CiderHouse",4,68,108,66),
           ("NorthLodge",1,58,147,72),("Shepherd",2,34,136,-20),("WheelHouse",7,21,144,88),
           ("HighCroft",3,-4,142,-15),("UpperYard",5,-21,153,-35),
           ("PairA",2,-53,147,27),("PairB",4,-69,155,50),
           ("MillerHouse",3,26,178,12),("Herder",5,11,186,72),
           ("BeeYard",2,-4,176,33),("Woodcutter",1,-27,179,4),
           ("NorthCottage",5,-20,207,11),("ForestersLodge",3,-50,195,53),
           ("TwinStoreA",6,-80,175,-33),("TwinStoreB",6,-92,170,-33)]
    for label,t,x,y,yaw in seeds:
        name,w,d,e,h=types[t]
        a=math.radians(yaw)
        for radius in range(0,27):
            ok=False
            n=max(1,6*radius)
            for j in range(n):
                xx=x+radius*math.cos(math.tau*j/n); yy=y+radius*math.sin(math.tau*j/n)
                p=dict(label=label,type=t,x=xx,y=yy,c=math.cos(a),s=math.sin(a),yaw=yaw,w=w,d=d,h=h,z=raw_bed(xx,yy)+.13)
                if any(kit["intersects"](p,old) for old in footprints):continue
                sample=[kit["xy"](p,u*w/8,v*d/8) for u in range(-4,5) for v in range(-4,5)]
                if any(kit["margin"](px,py)<1.2 or py<shore(px)+5 or math.dist((px,py),tree)<13
                       or (px>57 and 30<py<100) for px,py in sample):continue
                footprints.append(p);ok=True;break
            if ok:break
        if not ok:raise ValueError("No buildable lot for "+label)
    for h in footprints:
        for sign in (-1,1):
            p=kit["xy"](h,0,sign*(h["d"]/2+.2))
            _,x,y=min(kit["nearest"](*p,a,b) for path in lanes for a,b in zip(path,path[1:]))
            kit["add_road"]([p,(x,y)],1.8)
    return types


def plot_local(x,y,plot):
    cx,cy,length,width,deg=plot
    angle=math.radians(deg)
    return ((x-cx)*math.cos(angle)+(y-cy)*math.sin(angle),
            -(x-cx)*math.sin(angle)+(y-cy)*math.cos(angle))


spring_path=[(-8,85),(-5.9,80),(-4.8,74),(-4.3,69),(-4.6,64),(-3.1,59),
             (-2.2,54),(-3,50),(-6.5,46),(-8,42),(-10.2,38),(-10.6,35),
             (-10,31),(-9,28),(-6.4,25),(-3.4,22),(-1.1,17),(.4,14),(1,10)]


def spring_sections():
    rows={}
    last=float("inf")
    for (ax,ay),(bx,by) in zip(spring_path,spring_path[1:]):
        for tick in range(round(ay*2),round(by*2)-1,-1):
            y=tick/2
            if y in rows:continue
            x=ax+(bx-ax)*(ay-y)/(ay-by)
            last=max(.09,min(last-.002,bed(x,y)-.16))
            rows[y]=(x,last,.40+.075*math.sin(y*.39)+.035*math.cos(y*.73))
    return rows


def fence_clears_spring(x,y):
    return min(kit["nearest"](x,y,a,b)[0] for a,b in zip(spring_path,spring_path[1:]))>2.8


def clear_spring_fences():
    world=editor.get_editor_world()
    assert world.get_path_name().split(".")[0]==MAP and not levels.is_in_play_in_editor()
    for a in actors.get_all_level_actors():
        if a.get_actor_label().startswith("Garden enclosure"):
            center,_=a.get_actor_bounds(False)
            if not fence_clears_spring(center.x/100,center.y/100):actors.destroy_actor(a)


def terrain(existing=None):
    sections=spring_sections()
    xs=sorted(set(range(-300,401,5))|set(range(-144,149,2))|{i/4 for i in range(-48,13)})
    ys=sorted(set(range(-120,521,5))|set(range(-35,231,2))|set(sections))
    g=group()
    for j,y in enumerate(ys):
        for i,x in enumerate(xs):
            bank=max(y-shore(x),shore(x)-29-3*math.sin(x*.032)-y)
            noise=.43*math.sin(x*.56+y*.34)+.32*math.sin(y*.73-x*.42)
            plaza=math.hypot((x-tree[0])*.89,y-tree[1])-10
            edge=min(kit["margin"](x,y),plaza)
            for h in footprints:
                if abs(x-h["x"])>21 or abs(y-h["y"])>16:continue
                u,v=kit["local"](h,x,y)
                yard=max(abs(u)-h["w"]*.43,abs(v+h["d"]/2+1.5)-(4.6 if h["type"]==0 else 2.6))
                edge=min(edge,yard)
            if 40<x<125 and 25<y<108:
                for plot in plots:
                    u,v=plot_local(x,y,plot)
                    edge=min(edge,max(abs(u)-plot[2]/2,abs(v)-plot[3]/2))
            road=max(0,min(1,.71-(edge+noise)*.44))
            sand=max(0,min(1,.90-(bank+noise)*.20))
            z=bed(x,y)
            if y in sections:
                cx,surface,width=sections[y]
                distance=abs(x-cx)
                if distance<width+.65:
                    bank_t=max(0,(distance-width)/.65)
                    bottom=surface-.18*(1-min(1,distance/width)**2)
                    z=min(z,bottom+(z-bottom)*bank_t)
                    sand=max(sand,1-bank_t)
            g[0].append((x,y,z));g[2].append((x/2.8,y/2.8));g[3].append((road,sand,0,1))
            if i and j:
                n=len(g[0])-1
                g[1].extend(((n-len(xs)-1,n-1,n-len(xs)),(n-len(xs),n-1,n)))
    ground_mat=surface_material("M_MeadowAndLanes",["Medieval_Environment/T_Ground_Forest_Near_01_BC",
        "Medieval_Environment/T_Ground_Dirt_Near_01_BC","Medieval_Environment/T_Ground_Sand_Near_01_BC"],
        [(.57,.52,.31),(.94,.74,.51),(.57,.47,.33)])
    ground_mesh=mesh("Terrain_SpringBed",[g],[ground_mat])
    label="Vale and village paths"
    if existing:
        comp=existing[label].static_mesh_component
        comp.modify()
        comp.set_static_mesh(ground_mesh)
    else:
        actor_mesh(label,ground_mesh,0,0,0,0)
    runpy.run_path(str(HERE/"veles_water.py"))["install"](globals())


def prop(label,path,x,y,yaw=0,size=None,offset=0,folder="Village",collision=True):
    return kit["base"]["place"](label,path,x,y,yaw,size,bed(x,y)-kit["height"](x,y)+offset,folder,collision)


def grove_and_gardens(world):
    batches=defaultdict(list)
    sizes={}
    def scatter(path,x,y,h,yaw=None,burial=0):
        if path not in sizes:
            b=asset(path).get_bounding_box()
            sizes[path]=(b.max.z-b.min.z,b.min.z)
        height,minz=sizes[path]
        s=h*100/height
        batches[path].append(unreal.Transform(location=unreal.Vector(x*100,y*100,(bed(x,y)-burial)*100-minz*s),
            rotation=unreal.Rotator(yaw=rng.uniform(0,360) if yaw is None else yaw),scale=unreal.Vector(s,s,s)))
    oak="Nature/Trees/SM_White_Oak_02"
    young="Nature/Trees/SM_White_Oak_Young_01"
    fir="Nature/Trees/SM_Fir_Tree_02"
    bush="Nature/Shrubs/SM_BushWithoutSnow_02"
    rock="Nature/RocksAndCliffs/Rock_medium_VarB_mdl"
    for i in range(3200):
        x=rng.uniform(-190,185);y=rng.uniform(-77,335)
        fringe=min(abs(x)-117-8*math.sin(y*.077),300)
        north=y-214-8*math.sin(x*.09)
        south=shore(x)-y-53
        if max(fringe,north,south)<0 or abs(y-shore(x)+14)<23:continue
        if kit["margin"](x,y)<5:continue
        scatter(rng.choice((oak,oak,fir,young)),x,y,rng.uniform(8,17),burial=.14)
    for h in footprints:
        for side in (-1,1):
            for j in range(5):
                x,y=kit["xy"](h,rng.uniform(-h["w"]/2,h["w"]/2),side*(h["d"]/2+rng.uniform(3.2,6.8)))
                if kit["margin"](x,y)<1.7 or math.dist((x,y),tree)<11:continue
                if min(max(abs(u)-p["w"]/2,abs(v)-p["d"]/2) for p in footprints for u,v in [kit["local"](p,x,y)])<2.7:continue
                scatter(young if j==0 else bush,x,y,rng.uniform(4.8,8.5) if j==0 else rng.uniform(1.05,2.1),burial=.07)
        if h["type"] in (2,3,4,7):
            x,y=kit["xy"](h,h["w"]*.36,-h["d"]/2-1.4)
            prop("Doorstep planter","Nature/GrassAndFlowers/SM_Flower__Window_002",x,y,h["yaw"],(1.4,None,.7),folder="Yards",collision=False)
    for i in range(900):
        x=rng.uniform(-175,174)
        near=i%3!=0
        bank=shore(x) if near else shore(x)-29-3*math.sin(x*.032)
        y=bank+(1 if near else -1)*rng.uniform(-1.6,4.1)
        if math.dist((x,y),(15,14))<7:continue
        scatter(rock if i%3 else bush,x,y,rng.uniform(.35,1.4),burial=.23)
    for i,p in enumerate(plots):
        x,y,length,width,deg=p;a=math.radians(deg)
        # 조감 거리에서도 농작물의 줄과 높이가 읽히도록 군락 메시를 직접 인스턴싱한다.
        for k in range(int(length/.95)):
            for j in (-2,-1,0,1,2):
                u=-length/2+.6+k*.95;v=j*.98
                px=x+u*math.cos(a)-v*math.sin(a);py=y+u*math.sin(a)+v*math.cos(a)
                scatter("Nature/GrassAndFlowers/SM_WheatGrass01_V2" if i%2 else bush,
                    px,py,rng.uniform(.78,1.08) if i%2 else rng.uniform(.46,.64),burial=.05)
        if i%2:
            prop("Bound harvest","Props/TradeGoods/SM_Bale",x+length*.36,y+width*.68,deg,(2.6,2.0,1.7),folder="StripFields")
    for path,transforms in batches.items():
        folder=mesh.__globals__["folder"];name="FT_"+path.split("/")[-1]
        if kit["lib"].does_asset_exist(folder+"/"+name):ft=unreal.load_asset(folder+"/"+name)
        else:
            ft=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.FoliageType_InstancedStaticMesh,unreal.FoliageType_InstancedStaticMeshFactory())
            ft.set_editor_property("mesh",asset(path))
            ft.set_editor_property("cull_distance",unreal.Int32Interval(0,0))
            assert kit["lib"].save_loaded_asset(ft,False)
        unreal.InstancedFoliageActor.add_instances(world,ft,transforms)
    return {p:len(v) for p,v in batches.items()}


def courts():
    timber,cloth=group(),group()
    for x,y in ((-17,88),(49,109)):
        z=bed(x,y)
        for u in (-3.1,3.1):
            for v in (-2,2):
                box(timber,x+u-.10,x+u+.10,y+v-.10,y+v+.10,z-.15,z+3.0)
        for sign in (-1,1):
            vs=[(x-3.45,y+sign*2.3,z+2.85),(x-3.45,y,z+3.77),(x+3.45,y,z+3.77),(x+3.45,y+sign*2.3,z+2.85)]
            for vv in (vs,list(reversed(vs))):quad(cloth,vv)
    canvas=surface_material("M_OchreCanvas",["OldVillage/T_Stucco_001_C"],[(.99,.51,.12)])
    actor_mesh("Market awnings",mesh("MarketAwnings",[timber,cloth],[asset("Materials/OldVillage/MI_Wood_Planks_003"),canvas]),0,0,0,0)
    for h in footprints:
        if h["type"] not in (2,3,4,7):continue
        for i in range(3):
            x,y=kit["xy"](h,-h["w"]*.5-3.5,(i-1)*3.2)
            if kit["margin"](x,y)>1.9 and fence_clears_spring(x,y):
                prop("Garden enclosure","Structures/FencesAndRailings/SM_Fence_House_3m_1",x,y,h["yaw"]+90,
                     (3.3,None,1.2),-.06,"Yards",False)
    for i in range(4):
        prop("Quay retaining stones","BuildingParts/Walls/SM_MossyStoneWallB",-13-i*3.6,26+i*.7,172,(3.8,1.2,1.5),-.18,"Quay")
    jetty=group()
    for j in range(37):
        y=6+j*.36
        z=max(.64,bed(15,y)+.13)
        box(jetty,13.4,16.6,y,y+.34,z-.14,z)
        if j%6==0:
            for x in (13.55,16.45):box(jetty,x-.12,x+.12,y,y+.23,-2.2,z+.46)
    actor_mesh("Ferry loading walk",mesh("Jetty",[jetty],[asset("Materials/OldVillage/MI_Wood_Planks_001_floor")]),0,0,0,0)
    dress_spring()


def dress_spring():
    scatter=random.Random(8510)
    for y,(cx,z,width) in spring_sections().items():
        if y<15 or y>83 or scatter.random()>.13:continue
        x=cx+scatter.choice((-1,1))*(width+scatter.uniform(.10,.32))
        if kit["margin"](x,y)<.8:continue
        scale=scatter.uniform(.25,.57)
        prop("Spring stones","Nature/RocksAndCliffs/Rock_small_VarA_mdl",x,y,scatter.randrange(360),
             (scale,scale*.8,scale*.49),-.1,"Spring",False)


def dress(world):
    prop("Common oak","Nature/Trees/SM_White_Oak_02",*tree,29,(None,None,24),-.16)
    for i in range(3):
        a=math.radians(25+i*115)
        prop("Common bench","Props/Furniture/SM_OldWoodenBench",tree[0]+5.8*math.cos(a),tree[1]+5.8*math.sin(a),i*115+115,(2.3,None,1))
    prop("Produce stand","Structures/MarketStalls/SM_Stand",-15,88,10,(3.6,1.4,1.1),folder="Market")
    prop("Spring trough","Props/Household/SM_OldWoodenTrough",-8,85,-54,(3,1.2,1.1))
    prop("Ferry pier","Structures/Docks/Dock_C_VarC_mdl",15,11,79,(11,7,2.2),.30,"Quay")
    prop("Ferry boat","Props/BoatsAndFishing/PolyPixel/MedievalDocks/Models/Boat_Small_mdl",21,5,16,(6.7,2.7,1.8),.10-bed(21,5),"Quay")
    prop("Ferry nets","Props/BoatsAndFishing/SM_Fishing_Gear_03",20,22,18,(3.2,2,2),folder="Quay")
    prop("Ferry cargo","Props/Containers/SM_crate_group_01",10,22,-4,(3,2,1.5),folder="Quay")
    for h in footprints:
        if h["label"]=="workshop" or h["label"]=="RepairWing":
            items=[("Anvil","WorkshopTools/SM_BlacksmithAnvilA",.31,1.2),
                   ("Bellows","WorkshopTools/SM_OldBellows",-.33,1.6),
                   ("Repair bench","WorkshopTools/workbench",.22,1.25)]
            for name,path,u,tall in items:
                x,y=kit["xy"](h,h["w"]/2+2.8+(1.8 if name=="Anvil" else 0),u*h["d"]*1.26)
                prop(name,"Props/"+path,x,y,h["yaw"]+5,(None,None,tall),folder="Workshop")
        if h["label"].startswith("TwinStore"):
            for j in range(3):
                x,y=kit["xy"](h,h["w"]/2+3.1+(j%2)*1.7,(j-1)*3.1)
                prop("Stacked timber","BuildingParts/Timber/Lumber_VarA_mdl",x,y,h["yaw"],(4.1,2.3,2),folder="Woodlot")
        if h["type"]%2:
            x,y=kit["xy"](h,-h["w"]*.39,h["d"]/2+.9)
            prop("Yard barrel","Props/Containers/Fishermans_Cabin/SM_Barrel_01",x,y,h["yaw"],(1,1,1.25),folder="Yards")
    prop("Repair handcart","Props/CartsAndWagons/SM_cart_01",-64,49,14,(3.6,2,2.1),folder="Workshop")
    for sign in (-1,1):
        prop("Forest gatepost","BuildingParts/BeamsAndPillars/SM_WoodenPost",-80+sign*3.6,205+sign*1.2,15,(.7,.7,3.2),folder="Woodlot")
    plants=defaultdict(list)
    def plant(kind,x,y,s):
        plants[kind].append(unreal.Transform(location=unreal.Vector(x*100,y*100,bed(x,y)*100-2),
            rotation=unreal.Rotator(yaw=rng.uniform(-180,180)),scale=unreal.Vector(s,s,s)))
    for n,plot in enumerate(plots):
        x,y,length,width,deg=plot;a=math.radians(deg)
        def at(u,v):return (x+u*math.cos(a)-v*math.sin(a),y+u*math.sin(a)+v*math.cos(a))
        for i in range(int(length/.42)):
            for j in range(9):
                px,py=at(-length/2+.3+i*.42+rng.uniform(-.06,.06),(j-4)*.59+rng.uniform(-.08,.08))
                plant("F_Carrot" if n in (0,3,5) else "F_WheatGrass01_01",px,py,rng.uniform(.71,1.12))
        for i in range(0,int(length)-3,4):
            px,py=at(i-length/2+2,-width/2-.65)
            prop("Field hurdle","Structures/FencesAndRailings/SM_FenceModule_03",px,py,deg,(4.1,None,1.27),-.05,"StripFields")
    for row,(x,y) in enumerate(((76,113),(90,116),(79,128),(96,132),(69,153),(85,154))):
        for j in range(3):
            prop("Orchard tree","Nature/Trees/SM_White_Oak_Young_01",x+j*4.8,y+j*1.8,rng.randrange(360),(None,None,rng.uniform(4.7,6.8)),-.10,"Orchard")
    def clear(x,y):
        return all(max(abs(u)-h["w"]/2,abs(v)-h["d"]/2)>3 for h in footprints for u,v in [kit["local"](h,x,y)])
    for path in lanes:
        for a,b in zip(path,path[1:]):
            length=math.dist(a,b)
            for i in range(math.ceil(length*2)):
                f=rng.random();s=rng.choice((-1,1))*rng.uniform(2.2,4.5)
                x=a[0]+(b[0]-a[0])*f-(b[1]-a[1])*s/length
                y=a[1]+(b[1]-a[1])*f+(b[0]-a[0])*s/length
                if kit["margin"](x,y)>.45 and clear(x,y):
                    plant(rng.choice(("F_GroundCover_02","F_Crownbeard_00")),x,y,rng.uniform(.55,.9))
    for i in range(620):
        x=rng.uniform(-200,200);y=shore(x)+rng.choice((rng.uniform(.5,4),-29-3*math.sin(x*.032)-rng.uniform(.9,5)))
        if math.dist((x,y),(15,14))>8:
            plant(rng.choice(("F_ThatchingGrass_02","F_MossyStonesPack_00","F_FernA_00")),x,y,rng.uniform(.45,1))
    # 외곽 수림은 덩어리로 조성하고, 집 뒤의 과수와 마당 관목은 낮게 둔다.
    for i in range(420):
        x=rng.uniform(-215,219);y=rng.uniform(-55,316)
        exterior=(y>212+9*math.sin(x*.09) or x< -112-10*math.sin(y*.05) or x>126+9*math.sin(y*.047) or y<shore(x)-47)
        if not exterior or abs(y-(shore(x)-14))<20 or kit["margin"](x,y)<4.1:continue
        t=("SM_White_Oak_01","SM_White_Oak_02","SM_Tree_Pine_002","SM_Fir_Tree_02")[i%4]
        prop("Woodland tree","Nature/Trees/"+t,x,y,rng.randrange(360),(None,None,rng.uniform(8,18)),-.14,"Woodland")
    for h in footprints:
        for j in range(4):
            x,y=kit["xy"](h,h["w"]*.53+rng.uniform(0,1.4),(j-1.5)*1.25)
            if kit["margin"](x,y)>1.5:
                prop("Yard hedge","Nature/Shrubs/SM_BushWithoutSnow_01",x,y,rng.randrange(360),(None,None,rng.uniform(.85,1.45)),-.08,"Yards",False)
    for kind,instances in plants.items():
        unreal.InstancedFoliageActor.add_instances(world,asset("Foliage/MedievalEnvironment/"+kind),instances)
    courts()
    return {**{k:len(v) for k,v in plants.items()},**grove_and_gardens(world)}


def review_camera():
    editor.set_level_viewport_camera_info(unreal.Vector(450,-4150,12300),unreal.Rotator(yaw=90,pitch=-42))


def walking_route():
    return [*shop,*reversed(quay[:-1]),*quay[1:],*forest[1:],*reversed(forest[:-1]),*reversed(shop[:-1])]


def inspection_layout():
    saved=OUT/"revision.json"
    if saved.exists():
        return json.loads(saved.read_text())
    plan()
    for house in footprints:
        if house["label"]=="CommonHouse":
            house["z"]+=.27
        openings=[]
        for side in range(4):
            front=side<2
            half=(house["w"] if front else house["d"])/2
            wall=(house["d"] if front else house["w"])/2-.16
            wall*=(-1 if side%2==0 else 1)
            for p,_,sill,head,kind in wall_openings(front,half):
                openings.append(dict(kind=kind,side=side,p=p,k=wall,sill=sill,head=head))
        house["openings"]=openings
    return dict(map=MAP,houses=footprints,route=walking_route())


def finish_yards(data):
    """Add distant-readable crop rows, timber hurdles and sheltered working yards."""
    local_rng=random.Random(2506)
    by_name={a.get_actor_label():a for a in actors.get_all_level_actors()}
    if "Cultivated strips and hurdles" in by_name:return
    meadow=surface_material("M_ValleyGrass",["Medieval_Environment/T_Ground_Grass_Near_01_BC",
        "Medieval_Environment/T_Ground_Dirt_Near_01_BC","Medieval_Environment/T_Ground_Sand_Near_01_BC"],
        [(.39,.43,.22),(.78,.54,.36),(.47,.41,.29)])
    by_name["Vale and village paths"].static_mesh_component.set_material(0,meadow)
    timber,soil,green,gold=[group() for _ in range(4)]
    for index,(cx,cy,length,width,deg) in enumerate(plots):
        ang=math.radians(deg)
        def point(u,v,lift):
            x=cx+u*math.cos(ang)-v*math.sin(ang);y=cy+u*math.sin(ang)+v*math.cos(ang)
            return x,y,bed(x,y)+lift
        def ground_quad(g,points):
            quad(g,[point(*p) for p in points])
        for j in range(8):
            center=(j-3.5)*.72
            for k in range(math.ceil(length/2)):
                u=-length/2+2*k+.12;end=min(length/2,u+2)
                wav=.10*math.sin(j+k*.65)
                for a,b,za,zb in ((-.32,0,.045,.23),(0,.32,.23,.045)):
                    ground_quad(soil,[(u,center+a,za),(u,center+b,zb),(end,center+b,zb),(end,center+a,za)])
                for l in range(3):
                    p=u+l*.62;q=center+wav
                    h=local_rng.uniform(.27,.43)+(0 if index%2==0 else .32)
                    leaf=green if index%2==0 else gold
                    for rot in (0,2.1,4.2):
                        dx,dy=.24*math.cos(rot+k),.24*math.sin(rot+k)
                        ground_quad(leaf,[(p-dy*.55,q+dx*.55,.23),(p+dx,q+dy,h),
                                          (p+dy*.55,q-dx*.55,.23),(p-dx,q-dy,h*.80)])
        # 두 줄 가로대에 잔가지를 엮고 밭고랑 한 끝은 열어 둔다.
        for side in (-1,1):
            v=side*(width/2+.4)
            for k in range(math.floor(length/3)):
                u=-length/2+3*k
                x,y,z=point(u,v,0)
                box(timber,x-.095,x+.095,y-.105,y+.105,z-.06,z+1.08+local_rng.uniform(0,.12))
                for ht in (.29,.60,.87):
                    line_beam(timber,point(u,v,ht),point(u+3,v,ht+.035*math.sin(k)),.115,.10)
    wood=asset("Materials/OldVillage/MI_Wood_Planks_003")
    soilmat=surface_material("M_TilledSoil",["Medieval_Environment/T_Ground_Dirt_Near_01_BC"],[(.47,.29,.16)])
    leafmat=surface_material("M_CabbageLeaf",["Medieval_Environment/T_Ground_Clovers_Near_01_BC"],[(.30,.75,.30)])
    grainmat=surface_material("M_HarvestGrain",["OldVillage/T_Stucco_001_C"],[(.90,.60,.19)])
    actor_mesh("Cultivated strips and hurdles",mesh("TilledStrips",[timber,soil,green,gold],
        [wood,soilmat,leafmat,grainmat],False),0,0,0,0)
    garden_trees=[]
    def clearance(x,y):
        return min(max(abs(u)-h["w"]/2,abs(v)-h["d"]/2) for h in footprints for u,v in [kit["local"](h,x,y)])
    for h in footprints:
        for _ in range(18):
            x,y=kit["xy"](h,local_rng.choice((-1,1))*(h["w"]/2+local_rng.uniform(3.5,8)),local_rng.uniform(-h["d"],h["d"]))
            if (clearance(x,y)<3.3 or kit["margin"](x,y)<2.8 or math.dist(tree,(x,y))<12
                or any(math.dist((x,y),p)<6 for p in garden_trees)):continue
            garden_trees.append((x,y))
            prop("Courtyard fruit tree","Nature/Trees/SM_White_Oak_02",x,y,local_rng.randrange(360),
                 (None,None,local_rng.uniform(6.8,10.4)),-.12,"Orchard",False)
            break
    for x,y in ((62,98),(75,105),(85,101),(93,106),(101,111),(76,119),(88,124),(99,126)):
        prop("Orchard crown","Nature/Trees/SM_White_Oak_01",x,y,local_rng.randrange(360),(None,None,8.9),-.09,"Orchard",False)
    work_sheds()
    common=next(h for h in data["houses"] if h["label"]=="CommonHouse")
    common["z"]+=.27
    by_name["CommonHouse"].set_actor_location(unreal.Vector(common["x"]*100,common["y"]*100,common["z"]*100),False,False)
    data["garden_trees"]=len(garden_trees)+8
    data["work_sheds"]=6


def work_sheds():
    shedwood,shedroof=group(),group()
    for cx,cy,deg in ((-39,33,17),(41,25,-13),(-86,91,84),(55,165,27),(-108,173,-35),(75,134,74)):
        angle=math.radians(deg);z=bed(cx,cy)
        def loc(u,v,w):
            return cx+u*math.cos(angle)-v*math.sin(angle),cy+u*math.sin(angle)+v*math.cos(angle),z+w
        for u in (-4,0,4):
            for v in (-2.45,2.45):line_beam(shedwood,loc(u,v,-.12),loc(u,v,2.93),.23,.23)
            for sign in (-1,1):line_beam(shedwood,loc(u,sign*2.6,2.82),loc(u,0,4.77),.22,.18)
        for sign in (-1,1):
            line_beam(shedwood,loc(-4.22,sign*2.5,2.84),loc(4.22,sign*2.5,2.84),.24,.24)
            vs=[loc(-4.55,sign*2.95,2.68),loc(-4.55,0,4.88),loc(4.55,0,4.88),loc(4.55,sign*2.95,2.68)]
            for face in (vs,list(reversed(vs))):quad(shedroof,face)
        prop("Covered yard supplies","Props/Containers/SM_crate_group_01",cx,cy,deg,(4.2,3.0,2.0),folder="Workyards")
    roof=unreal.load_asset(mesh.__globals__["folder"]+"/M_Roof_BrownTile")
    actor_mesh("Open work sheds",mesh("YardShelters",[shedwood,shedroof],[asset("Materials/OldVillage/MI_Wood_Planks_003"),roof]),0,0,0,0)


def revise_open_village():
    world=editor.get_editor_world()
    if world.get_path_name().split(".")[0]!=MAP or levels.is_in_play_in_editor():
        raise RuntimeError("Open VelesRiverside in the editor and finish PIE first")
    types=plan()
    existing={a.get_actor_label():a for a in actors.get_all_level_actors()}
    for h in footprints:
        actual=existing[h["label"]]
        pos=actual.get_actor_location()
        if (math.hypot(pos.x/100-h["x"],pos.y/100-h["y"])>.01
                or abs(actual.get_actor_rotation().yaw-h["yaw"])>.01):
            raise RuntimeError("Changed footprint "+h["label"])
    rows=list(spring_sections().items())
    closest=float("inf")
    for (ay,(ax,az,aw)),(by,(bx,bz,bw)) in zip(rows,rows[1:]):
        assert az>=bz
        for j in range(11):
            t=j/10
            x=ax+(bx-ax)*t
            y=ay+(by-ay)*t
            width=aw+(bw-aw)*t+.09
            for h in footprints:
                u,v=kit["local"](h,x,y)
                clearance=math.hypot(max(0,abs(u)-h["w"]/2),max(0,abs(v)-h["d"]/2))-width
                closest=min(closest,clearance)
    assert closest>1,closest
    shells={}
    for h in footprints:
        comp=existing[h["label"]].static_mesh_component
        n=h["type"]
        if n not in shells:
            shells[n],_=make_house(*types[n],n,[comp.get_material(i) for i in range(5)])
    terrain(existing)
    for h in footprints:
        comp=existing[h["label"]].static_mesh_component
        comp.modify()
        comp.set_static_mesh(shells[h["type"]])
    for a in actors.get_all_level_actors():
        label=a.get_actor_label()
        if label.startswith("Spring stones") and str(a.get_folder_path()).endswith("Spring"):
            actors.destroy_actor(a)
        if label.startswith("Quay retaining stones"):
            p=a.get_actor_location()
            center,_=a.get_actor_bounds(False)
            i=round((center.y/100-26)/.7)
            x=-13-i*3.6
            p.z+=100*(bed(x,center.y/100)-bed(center.x/100,center.y/100))
            p.x+=100*x-center.x
            a.modify()
            a.set_actor_location(p,False,False)
    dress_spring()
    clear_spring_fences()
    assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP)
    OUT.mkdir(parents=True,exist_ok=True)
    info=dict(houses=len(footprints),shells=len(shells),water_to_wall_m=round(closest-.02,2),
              spring_length_m=round(sum(math.dist(a,b) for a,b in zip(spring_path,spring_path[1:])),2))
    (OUT/"spring-sills.json").write_text(json.dumps(info,indent=2))
    unreal.log("VELES_SPRING_SILLS_SAVED "+str(info))


def build():
    if kit["lib"].does_asset_exist(MAP):raise RuntimeError("The review map already exists")
    types=plan()
    world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    terrain()
    materials=[asset("Materials/OldVillage/MI_Stucco_fine_001"),asset("Materials/PolyPixel/INST_StoneWall_A2"),
        asset("Materials/OldVillage/MI_Wood_Planks_003"),asset("Materials/OldVillage/MI_Roof_Tiles_001"),paint_material()]
    roofs=[surface_material("M_Roof_"+name,["OldVillage/T_Roof_tiles_001_C"],[tint],"OldVillage/T_Roof_tiles_001_N")
           for name,tint in (("BurntClay",(1.52,.48,.22)),("Ochre",(1.34,.70,.34)),("BrownTile",(1.06,.46,.23)))]
    shells=[make_house(*spec,n,[*materials[:3],roofs[n%3],materials[4]]) for n,spec in enumerate(types)]
    for h in footprints:
        shell,h["openings"]=shells[h["type"]]
        a=actor_mesh(h["label"],shell,h["x"],h["y"],h["z"],h["yaw"])
        a.set_folder_path("Veles/Houses")
    counts=dress(world)
    start=actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(shop[0][0]*100,shop[0][1]*100,(bed(*shop[0])+1.35)*100),unreal.Rotator(yaw=-95))
    start.set_actor_label("Repair courtyard start")
    bp=unreal.load_asset("/Game/Level/VelesSettlementAssets/BP_VillageGameMode")
    world.get_world_settings().set_editor_property("default_game_mode",bp.generated_class())
    sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,10000),unreal.Rotator(pitch=-39,yaw=124))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property("intensity",5.3)
    sun.light_component.set_editor_property("light_color",unreal.Color(255,239,216,255))
    sun.light_component.set_editor_property("atmosphere_sun_light",True)
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector())
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture",True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
    fog=actors.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,-800))
    fog.get_component_by_class(unreal.ExponentialHeightFogComponent).set_editor_property("fog_density",.009)
    data=dict(houses=footprints)
    finish_yards(data)
    review_camera()
    assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP)
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/"revision.json").write_text(json.dumps(dict(map=MAP,**data,fields=plots,planting=counts,route=walking_route(),
        actor_count=len(actors.get_all_level_actors())),indent=2))
    unreal.log("VELES_AERIAL_SAVED "+MAP)


if __name__=="__main__":
    build()
