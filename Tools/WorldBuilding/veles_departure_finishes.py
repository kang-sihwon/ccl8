"""Hand-set near-field details, landscape silhouettes and settlement inhabitants."""
import random
rng=random.Random(1689)

def find(label):
    return next(a for a in actors.get_all_level_actors() if a.get_actor_label()==label)
def tint(name,color,texture="OldVillage/T_Stucco_001_C"):
    return art["surface_material"](name,[texture],[color]).get_path_name()
def tube(g,points,radii,n=9):
    for i,p in enumerate(points):
        aa=points[max(0,i-1)];bb=points[min(len(points)-1,i+1)]
        dx,dy,dz=(b-a for a,b in zip(aa,bb));length=math.dist(aa,bb)
        dx,dy,dz=dx/length,dy/length,dz/length
        h=math.hypot(dx,dy)
        sx,sy=(-dy/h,dx/h) if h>.0001 else (1,0)
        ux,uy,uz=-dz*sy,dz*sx,dx*sy-dy*sx
        for j in range(n+1):
            a=j/n*math.tau;r=radii[i];c,s=math.cos(a)*r,math.sin(a)*r
            k=len(g[0]);g[0].append((p[0]+sx*c+ux*s,p[1]+sy*c+uy*s,p[2]+uz*s))
            g[2].append((j/n,i*.74))
            if i and j:g[1].extend(((k-n-2,k-1,k-n-1),(k-n-1,k-1,k)))
def ridges(index):
    cx,cy,inner=(0,120,310) if index==0 else (220,760,835)
    g=group();n=220
    for row,(distance,mean,rough) in enumerate(((inner,-20,3),(inner*1.45,25,22),(inner*1.85,105,49),(inner*2.2,70,29),(inner*2.7,195,80),(inner*3.05,127,51))):
        for i in range(n+1):
            a=math.tau*i/n
            r=distance*(1+.075*math.cos(5*a+.25*row)+.05*math.sin(12*a-.4))
            x=cx+r*math.cos(a);y=cy+r*math.sin(a)
            z=mean+rough*(.8*math.sin(4*a+row*.1)+.5*math.cos(9*a)+.3*math.sin(18*a+.3*row)+.22*math.cos(31*a))
            k=len(g[0]);g[0].append((x,y,z));g[2].append((x/24,y/24));g[3].append((max(0,(z-112)/185),0,0,1))
            if row and i:g[1].extend(((k-n-2,k-n-1,k-1),(k-n-1,k,k-1)))
    shape(("V00" if index==0 else "F00")+" Valley ridgelines","Ridges"+str(index),[g],["/Game/Level/VelesNorthWoodAssets/M_DistantWood"],False)
def pose_camera(code,eye,look):
    c=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.CameraActor) and a.get_actor_label().startswith(code))
    c.set_actor_location(vec(eye),False,True)
    c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(vec(eye),vec(look)),False)
def people(code,x,y,z,yaw,female,scale=1):
    gender="Female" if female else "Male"
    body="ROG_"+gender
    # 함께 제작된 체형과 의복에 동일한 대기 동작을 직렬화한다.
    animation=art["asset"]("Characters/Animations/ROG_Modular_Assets/A_"+body+("_ORIGIN" if female else "")+"_Idle")
    pieces=["Bodies/"+body+"/SK_"+body+"_Head","Bodies/"+body+"/SK_"+body+"_Hands",
            "Equipment/"+body+"/SK_Shirt_Slim","Equipment/"+body+"/SK_Pants_04_Light_T2",
            "Equipment/"+body+"/SK_Boots_08_Type5_Light_T6_"+("m1" if female else "m2_set_Forester")]
    for part in pieces:
        a=actors.spawn_actor_from_class(unreal.SkeletalMeshActor,vec((x,y,z)),unreal.Rotator(yaw=yaw))
        a.set_actor_scale3d(unreal.Vector(scale,scale,scale))
        c=a.skeletal_mesh_component
        c.set_skeletal_mesh_asset(art["asset"]("Characters/"+part))
        c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        data=c.get_editor_property("animation_data")
        data.set_editor_property("anim_to_play",animation)
        data.set_editor_property("saved_looping",True)
        data.set_editor_property("saved_playing",True)
        c.set_editor_property("animation_data",data)
        a.set_actor_enable_collision(False)
        tag(a,code+" "+part.split("/")[-1])
def cargo_boat():
    boat=find("Ferry boat");boat.set_is_temporarily_hidden_in_editor(True)
    boat.set_actor_hidden_in_game(True);boat.set_actor_enable_collision(False)
    hull,frame,rig=group(),group(),group()
    # 배의 장축은 잔교와 나란하고 열린 갑판의 가운데가 승선판과 만난다.
    x,y=18.25,5.7
    def station(t):
        return 1.39*(.05+.95*math.sin(math.pi*t)**.68),.94+.65*abs(t*2-1)**2
    for j in range(28):
        a=j/28;b=(j+1)/28
        for sign in (-1,1):
            for k in range(5):
                face=[]
                for t,h in ((a,k/5),(b,k/5),(b,(k+1)/5),(a,(k+1)/5)):
                    w,z=station(t);face.append((x+sign*w*(.18+.82*h**.56),y+9.4*(t-.5),-.32+z*h))
                quad(hull,face);quad(hull,face[::-1])
        if j%3==0:
            w,z=station(a)
            for s in (-1,1):beam(frame,(x,y+9.4*(a-.5),.01),(x+s*w,y+9.4*(a-.5),z-.26),.10,.10)
        w,z=station((a+b)/2)
        box(frame,x-w*.83,x+w*.83,y+9.4*(a-.5),y+9.4*(b-.5)-.008,.34,.42)
    beam(frame,(x,y-2,.4),(x+.06,y-2,4.4),.16,.17)
    for sign in (-1,1):
        tube(rig,[(x,y-2,4.37),(x+sign*.47,y-1.2,2.55),(x+sign*1.2,y+.35,.77)],[.026]*3,6)
    for yy in (y+2.5,y+3.6):box(frame,x-.85,x+.85,yy,yy+.23,.67,.79)
    shape("V04 Loaded inland cargo boat","ClinkerBoat",[hull,frame,rig],[wood,"Materials/OldVillage/MI_Wood_Planks_001_floor",bark])
    for i in range(4):prop("V04 Freight aboard","Props/TradeGoods/SM_Bale",x+(i%2-.5)*.64,y-2.6+i//2*.8,.42,length=.91,yaw=72+i*9)
    gang=group()
    for i in range(5):
        yy=y-.6+i*.19
        quad(gang,[(16.3,yy,.78),(16.3,yy+.18,.78),(18.05,yy+.18,.48),(18.05,yy,.48)])
    shape("V04 Boarding plank","FerryGangplank",[gang],[wood])
    pose_camera("V04",(13,14.8,2.65),(18.1,5.5,1.25))
    people("V04 Ferryman on shore",15.7,8.6,.71,35,False,1.02)
    people("V04 Traveler aboard",18.15,5.45,.43,210,True,.96)
def village_detail():
    z=art["bed"]
    wheel_actor=find("V01 Replacement wheel")
    # 원본 바퀴는 XY에 누워 있다.
    wheel_actor.set_actor_scale3d(unreal.Vector(1,1,1))
    wheel_actor.set_actor_rotation(unreal.Rotator(roll=78,yaw=-15),False)
    o,h=wheel_actor.get_actor_bounds(False)
    scale=64/h.z
    wheel_actor.set_actor_scale3d(unreal.Vector(scale,scale,scale))
    o,h=wheel_actor.get_actor_bounds(False)
    wheel_actor.set_actor_location(wheel_actor.get_actor_location()+vec((-52.7,49.1,z(-52.7,49.1)))-o+unreal.Vector(0,0,h.z),False,True)
    people("V01 Wheelwright",-52.2,47.9,z(-52.2,47.9),35,False)
    people("V02 Water carrier",-12.1,86.5,z(-12.1,86.5),-35,True)
    for i,(x,y,yaw) in enumerate(((-5.25,98.5,18),(.37,99.5,182),(-2.4,100.8,-85),(-1.2,96.95,90))):
        people("V03 Neighbour",x,y,z(x,y),yaw,i%2==1,.94+.035*i)
    cargo_boat()
    g=group()
    for j in range(21):
        a=math.tau*j/21;b=math.tau*(j+.95)/21
        for ring in range(2):
            if j==4 and ring:continue
            x,y=-9.8,86;zz=z(x,y)
            pts=[(x+1.46*math.cos(t),y+1.08*math.sin(t),zz+.13+ring*.21) for t in (a,b)]
            beam(g,*pts,.23,.33)
    shape("V02 Weathered masonry rim","WellRim",[g],["Materials/OldVillage/MI_Decorative_Stones_001_dirty"])
    old=find("V02 Dry stone collection basin");old.static_mesh_component.set_visibility(False);old.set_actor_hidden_in_game(True)
    for code,temp,gain in (("V01",5850,.89),("V02",5600,.92),("V03",4400,.53),("V04",6550,.85)):
        cam=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.CameraActor) and a.get_actor_label().startswith(code))
        pp=cam.camera_component.get_editor_property("post_process_settings")
        for name,val in (("white_temp",temp),("color_gain",unreal.Vector4(gain,gain*.97,gain*.93,1))):
            pp.set_editor_property("override_"+name,True);pp.set_editor_property(name,val)
        cam.camera_component.set_editor_property("post_process_settings",pp)
    floor=group()
    for i in range(19):
        x,y=-52+i%5*.84,48.9+i//5*.58;zz=z(x,y)
        box(floor,x-.41,x+.38,y-.27,y+.25,zz-.13,zz+.035+.015*math.sin(i))
    shape("V01 Setts at repair yard","WorkshopSetts",[floor],["Materials/OldVillage/MI_Decorative_Stones_001_ground"])
    ridges(0)
def leafspray(g,x,y,z,angle,size):
    d=(math.cos(angle),math.sin(angle));s=(-d[1],d[0]); w=size*.27
    pts=[(x,y,z),(x+d[0]*size*.41+s[0]*w,y+d[1]*size*.41+s[1]*w,z+.04),
         (x+d[0]*size,y+d[1]*size,z-.035),(x+d[0]*size*.41-s[0]*w,y+d[1]*size*.41-s[1]*w,z+.014)]
    quad(g,pts);quad(g,pts[::-1])
def forest_detail(world):
    f=runpy.run_path(str(HERE/"veles_departure_forest.py"),init_globals=globals())
    floor=f["floor"];cx=f["channel"]
    path=group()
    for route in f["routes"]:
        points=[]
        for a,b in zip(route,route[1:]):
            n=max(1,math.ceil(math.dist(a,b)/1.2))
            points.extend([(a[0]+(b[0]-a[0])*i/n,a[1]+(b[1]-a[1])*i/n) for i in range(n)])
        points.append(route[-1])
        prev=None
        for idx,(x,y) in enumerate(points):
            a=points[max(0,idx-1)];b=points[min(idx+1,len(points)-1)]
            d=math.dist(a,b) or 1;nx=-(b[1]-a[1])/d;ny=(b[0]-a[0])/d
            w=1.42+.27*math.sin(x*.32+y*.24)
            now=[]
            for offset in (-w,0,w):
                xx=x+nx*offset;yy=y+ny*offset;now.append((xx,yy,floor(xx,yy)+.068))
            if prev and abs(x-cx(y))>f["width"](y)+1:
                for j in range(2):
                    face=[prev[j],now[j],now[j+1],prev[j+1]]
                    quad(path,face[::-1],[(q[0]/2,q[1]/2) for q in face[::-1]])
            prev=now
    mat=tint("M_WoodlandTrack",(.70,.52,.35),"Medieval_Environment/T_Ground_Forest_Near_02_BC")
    shape("F00 Worn forest tracks","BranchTrails",[path],[mat],False)
    land=find("F00 Forest watershed").static_mesh_component
    land.set_material(0,art["surface_material"]("M_DeepWoodFloor",["Medieval_Environment/T_Ground_Forest_Near_01_BC","Medieval_Environment/T_Ground_Dirt_Near_01_BC","Medieval_Environment/T_Ground_Clovers_Near_01_BC"],[(.60,.59,.42),(.64,.49,.33),(.64,.68,.43)],"Medieval_Environment/T_Ground_Forest_Near_01_N"))
    specs=(("Nature/GrassAndFlowers/SM_Fern_01",.66),("Nature/Shrubs/SM_Bush_04",1.32),
           ("Nature/GrassAndFlowers/SM_GrassClump_03",.5),("Nature/RocksAndCliffs/Rock_medium_VarB_mdl",.73),
           ("Nature/BranchesAndStumps/SM_TreeStumpA_00",.76))
    spots=[(-75,224),(228,603),f["maint"],(-250,730),f["upper"],(260,1250)]
    for path,height in specs:
        m=material(path);b=m.get_bounding_box();tf=[]
        for sx,sy in spots:
            for j in range(65 if "Grass" in path or "Bush" in path else 20):
                a=rng.random()*math.tau;r=rng.uniform(2.4,22)
                x=sx+r*math.cos(a);y=sy+r*math.sin(a);sc=height*rng.uniform(.72,1.5)*100/(b.max.z-b.min.z)
                if art["kit"]["margin"](x,y)<.7 or abs(x-cx(y))<f["width"](y) or (abs(x+251)<6 and abs(y-739)<4):continue
                p=vec((x,y,floor(x,y)-.04));p.z-=b.min.z*sc
                tf.append(unreal.Transform(location=p,rotation=unreal.Rotator(yaw=rng.randrange(360)),scale=unreal.Vector(sc,sc,sc)))
        name="FT_Detail_"+m.get_name();dest=package+"/Geometry"
        ft=unreal.load_asset(dest+"/"+name) if unreal.EditorAssetLibrary.does_asset_exist(dest+"/"+name) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,dest,unreal.FoliageType_InstancedStaticMesh,unreal.FoliageType_InstancedStaticMeshFactory())
        ft.set_editor_property("mesh",m);ft.set_editor_property("cull_distance",unreal.Int32Interval(15000,21000))
        assert unreal.EditorAssetLibrary.save_loaded_asset(ft,False)
        unreal.InstancedFoliageActor.add_instances(world,ft,tf)
    stems,green,pale=group(),group(),group()
    for a in actors.get_all_level_actors():
        if a.get_actor_label().startswith("F02 Gathering patch"):
            loc,_=a.get_actor_bounds(False);x,y=loc.x/100,loc.y/100;z=floor(x,y)
            for i in range(15):
                angle=rng.uniform(0,math.tau);dx,dy=math.cos(angle),math.sin(angle)
                pts=[(x+dx*t*.66,y+dy*t*.66,z+.08+t*.91) for t in (0,.32,.65,1)]
                tube(stems,pts,[.018,.014,.009,.003],5)
                for k in range(2,10):
                    t=k/10;leafspray(green if k%3 else pale,x+dx*.66*t,y+dy*.66*t,z+.08+t*.91,angle+(-1)**k*.91,.18)
    shape("F02 Berry shrub leaves and canes","ForestBerryLeaves",[stems,green,pale],[bark,tint("M_BerryLeaf",(.13,.21,.049)),tint("M_BerryLeafSun",(.23,.30,.075))],False)
    # 상류를 건너 오른쪽 둔덕에서 뿌리 언덕에 접근한다.
    f["footbridge"]("F06",1212)
    x,y=260,1250;z=floor(x,y);g=group()
    for k in range(18):
        sign=(-1)**k;d=1.6+(k//2)*.46
        pts=[(x+sign*d*.27,y+5.8,z+5.4),(x+sign*d*.65,y+4,z+3.7),(x+sign*d,y+1.3,z+1.3),
             (x+sign*(d+1.3),y-1.1,floor(x+sign*(d+1.3),y-1.1)+.12)]
        tube(g,pts,[.36,.33,.23,.035])
    shape("F06 Spreading oak buttress roots","RootHillButtress",[g],[bark])
    pose_camera("F06",(274,1238,floor(274,1238)+2.5),(260,1251,z+2.1))
    ridges(1)
def cave_detail():
    c=runpy.run_path(str(HERE/"veles_departure_cave.py"),init_globals=globals())
    center,base,radius=c["center"],c["base"],c["radius"]
    timber,pipes,hunks=group(),group(),group()
    for i,x in enumerate((1,7,14,19,25,30,38,46,71)):
        yy=center(x);zz=base(x);span=min(radius(x)*.77,4.6);h=3 if span<3.5 else 3.8
        for s in (-1,1):
            beam(timber,(x,yy+s*span,zz-.23),(x,yy+s*(span-.3),zz+h),.28,.33)
            beam(timber,(x,yy+s*(span-.15),zz+h-1),(x,yy+s*(span-1.4),zz+h),.22,.25)
        beam(timber,(x,yy-span,zz+h),(x,yy+span,zz+h+.12),.36,.32)
    for i in range(125):
        x=rng.uniform(-3,77);y0=center(x);zz=base(x);r=radius(x);s=rng.choice((-1,1))
        a=rng.uniform(.48,1.4)
        yy=y0+s*r*math.cos(a);h=zz+(r+1)*math.sin(a)
        pts=[(x,yy,h+.4),(x+.45,yy-s*.30,h-.35),(x+.8,yy-s*.5,h-1.0),
             (x+1.3,yy-s*.82,h-1.7-rng.random())]
        tube(hunks,pts,[.20,.15,.089,.018],7)
    shape("C00 Timbers and descending roots","CaveFrames",[timber,hunks],[wood,bark])
    for h in (.80,1.78):
        pts=[(x,center(x)+4.8,base(x)+h) for x in (19,22,25,27.3)]
        tube(pipes,pts,[.24]*4,16)
        for x in (20,22.5,25):
            y=center(x)+4.8;z=base(x)+h
            tube(pipes,[(x-.09,y,z),(x+.09,y,z)],[.30,.30],16)
    shape("C02 Copper conduit collars","IntakePipes",[pipes],[c["bronze"]])
    # 땅속 단면의 하단과 입구 밖 둔덕이 검토 카메라에서도 이어진다.
    cross=group()
    for i in range(90):
        x=i-9
        if x>79:break
        def edge(xx):
            yy=center(xx)+radius(xx)*.88
            return xx,yy,base(xx)-.19
        a,b=edge(x),edge(x+1);face=[a,b,(b[0],b[1],b[2]-4.2),(a[0],a[1],a[2]-4.2)]
        quad(cross,face);quad(cross,face[::-1])
    shape("C00 Cut rock strata","CutawayStrata",[cross],[package+"/Geometry/M_FrostrootStone"],False)
    slab=find("C03 Governor wheel")
    # 수평으로 만든 바퀴의 손잡이 면을 조작자 방향으로 세운다.
    slab.set_actor_rotation(unreal.Rotator(roll=70),False)
def finish(index,world):
    if index==0:village_detail()
    elif index==1:forest_detail(world)
    else:cave_detail()
