"""The managed woodland, branch trails and watershed upstream of Veles."""
from collections import defaultdict
import random

rng=random.Random(42516)
gate=(-80,205)
berries=(228,603)
yard=(-250,730)
root=(260,1250)
def channel(y):
    return -74+.325*(y-205)+13*math.sin((y-215)/58)+5*math.sin(y/25)
def main_river(y):
    return 320+.46*(y-205)+22*math.sin(y/95)
def surface(y):
    return 7.7+.044*(y-200)+.30*math.sin(y/62)
def width(y):
    return 2.35+.65*math.cos(y/38)
def floor(x,y):
    dx=abs(x-channel(y))
    t=max(0,min(1,(dx-width(y))/6))
    d=abs(x-main_river(y))
    near=min(1,dx/120)
    inland=(surface(y)+.48+min(8,dx*.055)+near*(4*math.sin(x/63+y/80)+3*math.cos(y/36))
            +10*math.exp(-((x-255)/54)**2-((y-1262)/31)**2))
    bank=surface(y)-.65+.63*min(1,(dx/width(y))**2)
    h=bank+(inland-bank)*t
    riverz=5+.015*(y-200)
    rt=min(1,max(0,(d-16)/47))
    return riverz-2.2+min(2.2,(d/16)**2*2.2)+(h-riverz)*rt*rt*(3-2*rt)
def side(y,d):
    return (channel(y)+d,y)
maint=side(756,-5)
upper=side(1038,-5)
routes=[
    [gate,(-70,234),(-48,310),(-10,376),(16,441),(88,476),(135,525),berries,(200,676),side(747,7)],
    [gate,(-99,258),(-144,379),(-153,496),(-184,570),(-217,660),yard,(-195,779),(-112,814),(-40,809),maint],
    [side(y,-6) for y in range(230,1231,6)]+[root],
    [berries,(258,636),(270,651)], [side(747,7),side(747,-6)],
    [side(1030,-6),side(1030,7),side(1075,6)],
    [root,(254,1255)]]
for r in routes: art["kit"]["add_road"](r,4.0)
def terrain():
    g=group()
    # 물길과 같은 횡단면에서 수면 가장자리와 강바닥을 함께 샘플링한다.
    for row,y in enumerate(range(180,1321,4)):
        cx,mx=channel(y),main_river(y)
        w=width(y)
        anchors=[(-380,0),(cx-14,64),(cx-w-1,3),(cx-w,1),(cx,2),(cx+w,2),(cx+w+1,1),
                 (cx+14,3),(mx-52,86),(mx-16,9),(mx,4),(mx+16,4),(mx+52,9),(mx+195,26)]
        xs=[anchors[0][0]]
        for (left,_),(right,n) in zip(anchors,anchors[1:]):
            xs.extend(left+(right-left)*i/n for i in range(1,n+1))
        for j,x in enumerate(xs):
            k=len(g[0]); edge=art["kit"]["margin"](x,y)
            wet=max(0,1-abs(abs(x-cx)-w)/3)
            dirt=max(wet,.99*max(0,min(1,1-(edge+.24*math.sin(x*.7+y))/2)))
            g[0].append((x,y,floor(x,y)))
            g[2].append((x/3.2,y/3.2))
            g[3].append((dirt,.12*(1-dirt),0,1))
            if row and j: g[1].extend(((k-len(xs)-1,k-1,k-len(xs)),(k-len(xs),k-1,k)))
    shape("F00 Forest watershed","Watershed",[g],["/Game/Level/VelesNorthWoodAssets/M_NorthWoodFloor"])

def water(world):
    zone=actors.spawn_actor_from_class(unreal.WaterZone,vec((230,750,50)))
    zone.set_editor_property("zone_extent",unreal.Vector2D(175000,145000))
    zone.set_editor_property("render_target_resolution",unreal.IntPoint(1024,1024))
    draw=runpy.run_path(str(HERE/"veles_water.py"))["draw_surface"]
    for deep in (False,True):
        rows=[]
        for y in range(1320 if deep else 1248,179,-4):
            x=main_river(y) if deep else channel(y)
            z=5+.015*(y-200) if deep else surface(y)
            rows.append((x,y,z))
        cls=unreal.WaterBodyRiver if deep else unreal.load_asset("/Game/Level/VelesRiversideAssets/BP_VelesBrook").generated_class()
        a=actors.spawn_actor_from_class(cls,vec(rows[0]))
        tag(a,"F00 Eastern river" if deep else "F03 Spring maintenance stream")
        c=a.get_water_body_component()
        c.set_editor_property("affects_landscape",False)
        c.set_water_zone_override(zone)
        c.set_water_material(material("/Game/Level/VelesRiversideAssets/M_VelesFlowingWater"))
        c.set_water_info_material(material("/Water/Materials/WaterInfo/DrawWaterInfo"))
        spline=a.get_water_spline()
        spline.set_spline_points([vec(p) for p in rows],unreal.SplineCoordinateSpace.WORLD,True)
        for i,(_,y,z) in enumerate(rows):
            slope=(channel(y+1)-channel(y-1))/2 if not deep else (main_river(y+1)-main_river(y-1))/2
            c.set_river_width_at_spline_input_key(i,(32 if deep else 2*width(y))*100/math.hypot(1,slope))
            c.set_river_depth_at_spline_input_key(i,220 if deep else 64)
            c.set_water_velocity_at_spline_input_key(i,80 if deep else 54)
        spline.call_method("K2_SynchronizeAndBroadcastDataChange")
        draw(art,a,material(watermat),"ForestMainWater" if deep else "ForestSpringWater",deep)
def at(label,path,x,y,**kw):
    return prop(label,path,x,y,floor(x,y),**kw)
def footbridge(code,y):
    x=channel(y); z=max(floor(x+i*6,y) for i in (-1,1))+.09
    g=group()
    for j in range(39):
        xx=x-6.35+j*.33
        box(g,xx,xx+.322,y-.99,y+.99,z-.14,z)
    for yy in (y-1.02,y+1.02):
        beam(g,(x-6.35,yy,z-.26),(x+6.35,yy,z-.26),.24,.26)
        for d in (-5,-2,2,5):beam(g,(x+d,yy,z-2),(x+d,yy,z+.94),.18,.16)
        beam(g,(x-5,yy,z+.91),(x+5,yy,z+.91),.12,.14)
    shape(code+" Walkable bridge","Bridge"+code,[g],[wood])
def plants(world):
    groups=defaultdict(list)
    types={}
    for path in unreal.EditorAssetLibrary.list_assets("/Game/Level/VelesNorthWoodAssets",False,False):
        if "/FT_" in path:
            ft=unreal.load_asset(path)
            m=ft.get_editor_property("mesh")
            types[m.get_name()]=(ft,m.get_bounding_box())
    def add(name,x,y,h):
        ft,b=types[name]; sc=h*100/(b.max.z-b.min.z)
        pos=vec((x,y,floor(x,y)-.06)); pos.z-=b.min.z*sc
        groups[name].append(unreal.Transform(location=pos,rotation=unreal.Rotator(yaw=rng.uniform(0,360)),scale=unreal.Vector(sc,sc,sc)))
    for y0 in range(185,1316,8):
        for x0 in range(-370,int(main_river(y0)+170),8):
            x=x0+rng.uniform(-3,3); y=y0+rng.uniform(-3,3)
            if abs(x-main_river(y))<26 or abs(x-channel(y))<width(y)+1 or art["kit"]["margin"](x,y)<2.5:continue
            if min(math.dist((x,y),p) for p in [gate,berries,yard,maint,upper,root])<13:continue
            if rng.random()<.52+.18*math.sin(x/27+y/68):
                name=rng.choice(("SM_Fir_Tree_01","SM_Fir_Tree_02")) if x<-90 else rng.choice(("SM_White_Oak_02","SM_White_Oak_Young_01"))
                add(name,x,y,rng.uniform(8,18))
            for j in range(4):
                add(rng.choice(("SM_Fern_01","SM_GrassClump_03","SM_BushWithoutSnow_02")),x+j*.9,y+.6,rng.uniform(.36,1.12))
    for name,transforms in groups.items():
        unreal.InstancedFoliageActor.add_instances(world,types[name][0],transforms)

def build_region():
    world=levels.new_blank_map(False)
    terrain(); water(world)
    # 시안 순서대로 관리 흔적, 채집지, 수문, 벌목장, 상류, 샘 문턱을 배치한다.
    for i in range(9):
        x,y=-77+i%3*1.4,219+i//3*1.7
        at("F01 Coppice stool",stump,x,y,height=.5+.12*(i%3),yaw=44*i)
        for j in range(3):
            at("F01 Coppice regrowth","Nature/Trees/SM_White_Oak_Young_01",x+.32*math.cos(j*2),y+.32*math.sin(j*2),height=3.2+.3*j,solid=False)
    for d in (-1,1):
        at("F01 Worn gate wing","Structures/FencesAndRailings/SM_Fence_House_3m_1",-70+3.1*d,234,length=3.5,yaw=22)
    footbridge("F01",234)
    berry=group()
    for i in range(16):
        x=berries[0]+rng.uniform(-8,6); y=berries[1]+rng.uniform(2,9)
        at("F02 Gathering patch","Nature/Shrubs/SM_BushWithoutSnow_02",x,y,height=1.12,solid=False)
        for j in range(17):
            bx=x+rng.uniform(-.66,.66);by=y+rng.uniform(-.66,.66);z=floor(x,y)+rng.uniform(.58,.88)
            beam(berry,(bx,by,z),(bx+.014,by,z+.033),.038,.041)
    red=art["surface_material"]("M_ForestBerry",["OldVillage/T_Stucco_001_C"],[(.49,.019,.033)]).get_path_name()
    shape("F02 Clusters of ripe berries","SmallBerryClusters",[berry],[red],False)
    at("F02 Gatherer's pail",bucket,226,603,height=.42)
    for i in range(3):
        x,y=221+i*2.8,600-i
        at("F02 Tapped fir","Nature/Trees/SM_Fir_Tree_01",x,y,height=12)
        prop("F02 Resin cup","Props/Kitchenware/SM_Wooden_cup_1",x+.38,y,floor(x,y)+.87,height=.22)
    x,y=maint; table=at("F03 Sluice repair bench",bench,x-1.7,y,height=.98)
    tabletop("F03 Worn waterworks hammer",table,x-1.7,y,"Props/WorkshopTools/SM_RustyHammer",.13)
    footbridge("F03",747)
    sluice=group(); x=channel(y);z=surface(y)
    for d in (-1,1):
        box(sluice,x+d*width(y)-.18,x+d*width(y)+.18,y-.33,y+.33,z-.7,z+1.3)
    for j in range(3):box(sluice,x-width(y),x+width(y),y-.13,y+.13,z+.04+j*.21,z+.23+j*.21)
    shape("F03 Stopboard sluice","Sluice",[sluice],[wood])
    for l,path,dx,dy,ang in (("Sawyers open shelter","SawyersShelter",-1,9,7),("Sorted logs","SawLogs",-3,4,12)):
        a=art["actor_mesh"]("F04 "+l,material("/Game/Level/VelesNorthWoodAssets/SM_"+path),yard[0]+dx,yard[1]+dy,floor(yard[0]+dx,yard[1]+dy),ang)
        tag(a,"F04 "+l)
    at("F04 Haul cart","Props/CartsAndWagons/SM_cart_03",-244,743,length=3.6,yaw=25)
    at("F04 Saw trestle",bench,-251,738,height=1)
    for i in range(18):
        x=-223-i*1.44+(-1)**i*6;y=670+i*2.9
        if art["kit"]["margin"](x,y)>4:at("F04 Granite shoulder",cliff,x,y,height=rng.uniform(2,6),yaw=i*71)
    footbridge("F05",1030)
    for j in range(7):
        x=channel(1039+j*.47)-width(1039+j*.47)-.38-(j%2)*.32; y=1039+j*.47
        at("F05 Impressed paw pad","Nature/RocksAndCliffs/SM_MossyRock",x,y,length=.28,solid=False)
        for k in range(4):
            at("F05 Toe impression","Nature/RocksAndCliffs/SM_MossyRock",x+(k-1.5)*.051,y+.14,length=.055,solid=False)
    x,y=root; z=floor(x,y)
    at("F06 Old watershed oak","Nature/Trees/SM_White_Oak_02",x,y+6,height=35)
    for i in range(9):
        dx=(-1 if i%2 else 1)*(2.7+(i//2)*1.25)
        at("F06 Rootbound granite",cliff,x+dx,y+3+i*.3,length=4.2+i*.40,yaw=i*79)
        at("F06 Exposed roots",roots,x+dx,y+1+i*.5,length=6.4,yaw=i*61)
    prop("F06 Entrance crown",cliff,x,y+2,z+2.65,length=6.8)
    frame=group()
    for dx in (-1.43,1.43):beam(frame,(x+dx,y,z-.2),(x+dx*.94,y,z+2.7),.28,.32)
    beam(frame,(x-1.8,y,z+2.6),(x+1.8,y,z+2.7),.38,.36)
    shape("F06 Timber intake frame","RootEntrance",[frame],[wood])
    plants(world)
    sun=actors.spawn_actor_from_class(unreal.DirectionalLight,vec((0,0,120)),unreal.Rotator(pitch=-28,yaw=120))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property("atmosphere_sun_light",True)
    sun.light_component.set_editor_property("intensity",4.2)
    sun.light_component.set_light_color(unreal.LinearColor(1,.80,.52,1),False)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector())
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture",True)
    fog=actors.spawn_actor_from_class(unreal.ExponentialHeightFog,vec((0,0,19)))
    fog.get_component_by_class(unreal.ExponentialHeightFogComponent).set_editor_property("fog_density",.018)
    setup(world,(*gate,floor(*gate)+1.05),74)
    overview=camera("F00 Golden mist watershed",(-382,-272,612),(131,838,32),62)
    for label,p,t in (("F01 Managed woodland",(-72,243),(-77,219)),("F02 Berry terrace",(222,595),(231,607)),
                      ("F03 Waterworks",side(750,-10),maint),("F04 Timber yard",(-239,722),(-251,739)),
                      ("F05 Upper bank",side(1041,-5),side(1030,0)),("F06 Root hill",(274,1237),root)):
        camera(label,(*p,floor(*p)+2.15),(*t,floor(*t)+1.1))
    editor.set_level_viewport_camera_info(overview.get_actor_location(),overview.get_actor_rotation())
    return world
