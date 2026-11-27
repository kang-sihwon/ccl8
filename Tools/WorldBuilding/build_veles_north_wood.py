"""Build the independent northern Veles woodland tile in village world coordinates."""
import bisect
import json
import math
from collections import defaultdict
from pathlib import Path
import random
import runpy

import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
MAP = "/Game/Level/VelesNorthWood"
ASSETS = "/Game/Level/VelesNorthWoodAssets"
VILLAGE = "/Game/Level/VelesRiversideAssets"
DETAIL = ROOT / "Docs/ArtDirection/EtherWorldMap/Data/veles-forest-detail-v1.json"
workroad = json.loads(DETAIL.read_text(encoding="utf-8"))["areas"][0]
haul = [tuple(p) for p in workroad["detailed_centerline_m"]]
# 서쪽 바위숲길의 두 기준점 사이에서만 지형을 따라 굽힌다.
west = [(-110,520),(-110,530),(-127,551),(-142,568),(-144,584),(-161,608),
        (-181,624),(-191,644),(-213,661),(-222,681),(-228,701),(-250,730)]
return_path = [(39.8669,738.7838),(33,720),(20,697),(-6,682),(-15,661),
               (-24,635),(-46,623),(-66,599),(-69,583),(-86,563),(-110,530)]
route = west[1:] + haul[1:9] + return_path[1:]
seam_xs = sorted(set(range(-300,401,5)) | set(range(-144,149,2)) | {k/4 for k in range(-48,13)})
rng = random.Random(112526)
art = runpy.run_path(str(ROOT / "Tools/WorldBuilding/build_veles_aerial.py"))
kit = art["kit"]


def edge_height(x):
    i = min(len(seam_xs)-2,max(0,bisect.bisect_right(seam_xs,x)-1))
    a,b = seam_xs[i:i+2]
    za,zb = art["raw_bed"](a,520),art["raw_bed"](b,520)
    return za+(zb-za)*(x-a)/(b-a)


def elevation(x,y):
    north = y-520
    t = max(0,min(1,north/132))
    blend = t*t*(3-2*t)
    seam = edge_height(x)+.085*north
    shelf = (63 + north*.058 + 4.7*math.sin(x*.028+y*.011) + 2.7*math.cos(y*.049)
             + 14*math.exp(-((x+28)/61)**2-((y-648)/49)**2)
             + 12*math.exp(-((x+220)/75)**2-((y-796)/38)**2))
    return seam+(shelf-seam)*blend


def pad_distance(x,y):
    return min(math.hypot(x-p["center_m"][0],(y-p["center_m"][1])*1.3)-p["size_m"][0]*.56
               for p in workroad["pads"])


def meadow(x,y):
    return ((x+48)/59)**2+((y-641)/49)**2


def forest_floor_material():
    art["mesh"].__globals__["folder"] = ASSETS
    return art["surface_material"]("M_NorthWoodFloor",[
        "Medieval_Environment/T_Ground_Grass_Near_01_BC",
        "Medieval_Environment/T_Ground_Dirt_Near_01_BC",
        "Medieval_Environment/T_Ground_Clovers_Near_01_BC"],
        [(.54,.62,.43),(.83,.68,.50),(.75,.84,.54)])


def ground():
    make = art["mesh"]
    make.__globals__["folder"] = ASSETS
    xs = sorted(set(range(-300,101,2)) | {x for x in seam_xs if x<=100})
    ys = range(520,821,2)
    v,f,uv,colors = art["group"]()
    for j,y in enumerate(ys):
        for i,x in enumerate(xs):
            z = elevation(x,y)
            edge = min(kit["margin"](x,y),pad_distance(x,y))
            fringe = .36*math.sin(.46*x+.23*y)+.19*math.sin(.79*y-.3*x)
            soil = max(0,min(1,.63-(edge+fringe)*.48))
            # 들판과 솔숲의 바닥색이 길 가장자리까지 서서히 바뀐다.
            straw = max(0,1-meadow(x,y))*.24*(1-soil)
            v.append((x,y,z))
            uv.append((x/2.8,y/2.8))
            colors.append((soil,straw,0,1))
            if i and j:
                k=len(v)-1
                f.extend(((k-len(xs)-1,k-1,k-len(xs)),(k-len(xs),k-1,k)))
    material = forest_floor_material()
    mesh = make("NorthWoodGround",[(v,f,uv,colors)],[material])
    actor = art["actor_mesh"]("Northern forest floor",mesh,0,0,0,0)
    actor.set_folder_path("NorthWood/Terrain")


def prop(label,path,x,y,size,yaw=0,burial=0):
    # 기본 배치 함수가 메시 피벗과 실제 회전 바운드로 밑면을 지면에 맞춘다.
    return kit["base"]["place"](label,path,x,y,yaw,size,
          elevation(x,y)-kit["height"](x,y)-burial,"NorthWood/Landmarks")


def saw_logs():
    sides,ends = art["group"](),art["group"]()
    for row,n in enumerate((5,4,3)):
        for j in range(n):
            cy=(j-(n-1)/2)*.43
            z=.23+row*.37
            half=2.2+.19*math.cos(j*1.8+row)
            rings=[]
            for x,radius in ((-half,.225),(half,.19)):
                rings.append([(x,cy+radius*math.cos(k*math.tau/10),z+radius*math.sin(k*math.tau/10)) for k in range(10)])
            for k in range(10):
                m=(k+1)%10
                art["quad"](sides,[rings[0][k],rings[1][k],rings[1][m],rings[0][m]])
            for i,ring in enumerate(rings):
                idx=len(ends[0])
                ends[0].extend(ring)
                ends[2].extend([((p[1]-cy)*2+.5,(p[2]-z)*2+.5) for p in ring])
                for k in range(1,9):
                    tri=(idx,idx+k,idx+k+1)
                    ends[1].append(tri if i==0 else tri[::-1])
    mats=[art["asset"]("Materials/OldVillage/MI_Bark_Pine_001"),
          art["asset"]("Materials/OldVillage/MI_Wood_Planks_003")]
    mesh=art["mesh"]("SawLogs",[sides,ends],mats)
    actor=art["actor_mesh"]("Sorted saw logs",mesh,-253,734,elevation(-253,734)-.07,12)
    actor.set_folder_path("NorthWood/Landmarks")


def backdrop():
    # 원경 능선은 접합 경계에서 떨어진 단일 비충돌 액터로 둔다.
    land = art["group"]()
    sectors = 180
    radii = (490,585,680,755,895,1030,1170)
    heights = (-120,8,73,133,64,168,60)
    for row,(radius,base) in enumerate(zip(radii,heights)):
        for i in range(sectors+1):
            a = math.tau*i/sectors
            r = radius + 23*math.sin(6*a+.4*row) + 21*math.cos(3*a+row*.1)
            z = base + (.24 if row<2 else 1)*(38*math.sin(3*a+.8*row)+22*math.cos(8*a+row*.4)
                +14*math.cos(15*a-row*.19)+7*math.sin(31*a+.5*row))
            x,y = -100+r*math.cos(a),670+r*math.sin(a)
            k=len(land[0])
            land[0].append((x,y,z))
            land[2].append((x/45,y/45))
            land[3].append((max(0,min(.80,(z-105)/135)),0,0,1))
            if row and i:
                land[1].extend(((k-sectors-2,k-sectors-1,k-1),(k-sectors-1,k,k-1)))
    mat=art["surface_material"]("M_DistantWood",[
        "Medieval_Environment/T_Ground_Forest_Far_01_BC","Medieval_Environment/T_Ground_Rock_Near_01_BC"],
        [(.26,.40,.36),(.53,.59,.61)])
    mesh=art["mesh"]("DistantWoodedRidges",[land],[mat],False)
    actor=art["actor_mesh"]("Distant wooded ridges",mesh,0,0,0,0)
    actor.set_actor_enable_collision(False)
    actor.static_mesh_component.set_cast_shadow(False)
    actor.set_folder_path("NorthWood/Backdrop")


def foothills():
    # 직사각형 타일의 외곽에서 먼 산까지 보이는 지면만 잇는다.
    land=art["group"]()
    segments=240
    for row in range(19):
        t=row/18
        for i in range(segments+1):
            a=math.tau*i/segments
            dx,dy=math.cos(a),math.sin(a)
            edge=min(200/max(abs(dx),1e-6),150/max(abs(dy),1e-6))
            bx,by=-100+edge*dx,670+edge*dy
            r=edge-7+(680-edge+7)*t
            x,y=-100+r*dx,670+r*dy
            end=46+22*math.sin(3*a)+13*math.cos(7*a)
            z=(1-t)*elevation(bx,by)+t*end
            z+=math.sin(math.pi*t)*(6*math.sin(x*.023)+3*math.cos(y*.052))-3*(1-t)**5
            k=len(land[0])
            land[0].append((x,y,z))
            land[2].append((x/28,y/28))
            land[3].append((0,0,0,1))
            if row and i:
                land[1].extend(((k-segments-2,k-segments-1,k-1),(k-segments-1,k,k-1)))
    mat=unreal.load_asset(ASSETS+"/M_DistantWood")
    mesh=art["mesh"]("WoodedFoothills",[land],[mat],False)
    actor=art["actor_mesh"]("Distant forest floor",mesh,0,0,0,0)
    actor.set_actor_enable_collision(False)
    actor.static_mesh_component.set_cast_shadow(False)
    actor.set_folder_path("NorthWood/Backdrop")


def landmarks():
    wood = art["asset"]("Materials/OldVillage/MI_Wood_Planks_003")
    boards,posts = art["group"](),art["group"]()
    beam = art["line_beam"]
    for x in (-4,0,4):
        for y,h in ((-2,3.0),(2,4.0)):
            beam(posts,(x,y,-.8),(x,y,h),.26,.26)
        beam(posts,(x,-2.5,2.90),(x,2.5,4.15),.21,.23)
    for y,h in ((-2,3),(2,4)):
        beam(posts,(-4.3,y,h),(4.3,y,h),.26,.25)
        beam(posts,(-4,y,h-1.4),(-2.5,y,h-.15),.16,.17)
    for k in range(27):
        x = -4.55+k*.35
        beam(boards,(x,-2.6,2.98),(x,2.6,4.28),.12,.37)
    shed = art["mesh"]("SawyersShelter",[posts,boards],[wood,wood])
    a = art["actor_mesh"]("Sawyers open shelter",shed,-251,739,elevation(-251,739),7)
    a.set_folder_path("NorthWood/Landmarks")
    prop("Sawyers trestle","Props/Furniture/SM_OldWoodenTable",-251,739,(2.6,None,1.05),7)
    prop("Hauling cart","Props/CartsAndWagons/SM_cart_03",-243,737,(3.1,2.5,2.1),24)
    prop("Water barrel","Props/Containers/SM_WoodenBarrelA",-254,737,(None,None,1.2))
    prop("Sawyers cutting block","Nature/BranchesAndStumps/SM_TreeStumpA_00",-247,736,(None,None,.86))
    saw_logs()
    for k in range(7):
        x,y=-258+k*.43,729
        prop("Stacked haul timber","Nature/BranchesAndStumps/SM_TreeLog",x,y,(1.4,5.1,1.0),93,.06)
    for i,(x,y,h) in enumerate(((-141,745,4.7),(-127,751,3.4),(-65,755,4.2),(-55,761,3.3),
                                (-218,650,4.7),(-195,624,3.1),(-27,650,4.0),(-32,647,2.7))):
        prop("Granite shoulder","Nature/RocksAndCliffs/SM_Cliff_Rock_0"+str(1+i%3),x,y,(13,None,h),33*i,.52)
    for i,(x,y) in enumerate(((-112,536),(-242,719),(33,735),(-76,581))):
        # 낮은 돌무더기와 그 위의 좁은 돌로 사람이 쓰는 길의 분기를 표시한다.
        prop("Trail cairn base","Nature/RocksAndCliffs/Rock_medium_VarB_mdl",x,y,(1.6,1.5,.82),41*i,.09)
        prop("Trail cairn cap","Nature/RocksAndCliffs/Rock_small_VarA_mdl",x,y,(.8,.76,.47),90-i*13,-.49)


def foliage_settings(ft, path):
    tree = "/Trees/" in path
    solid = tree or "Rock_medium" in path or "Stump" in path
    end = 76000 if tree else (12700 if solid else 6200)
    ft.set_editor_property("cull_distance",unreal.Int32Interval(int(end*.8),end))
    ft.set_editor_property("cast_shadow",solid)
    if solid:
        body = ft.get_editor_property("body_instance")
        body.set_editor_property("collision_profile_name","BlockAll")
        ft.set_editor_property("body_instance",body)


def planting(world):
    groups = defaultdict(list)
    cache = {}
    def put(path,x,y,h,sink=.06):
        if path not in cache:
            mesh = art["asset"](path)
            b = mesh.get_bounding_box()
            cache[path] = mesh, b.min.z, b.max.z-b.min.z
        mesh,bottom,height = cache[path]
        s=h*100/height
        groups[path].append(unreal.Transform(location=unreal.Vector(x*100,y*100,100*(elevation(x,y)-sink)-s*bottom),
            rotation=unreal.Rotator(yaw=rng.uniform(0,360)),scale=unreal.Vector(s,s,s)))
    # 넓은 처마와 수레를 담을 작업장 바깥 여유 공간.
    def clearance(x,y):
        yard = math.hypot(x+250,(y-735)*1.1)-16
        return min(kit["margin"](x,y),pad_distance(x,y),yard)
    for gx in range(-296,99,6):
        for gy in range(523,820,6):
            x,y = gx+rng.uniform(-2.4,2.4),gy+rng.uniform(-2.4,2.4)
            edge=clearance(x,y)
            if edge<4.2 or meadow(x,y)<.95:
                continue
            density=.58+.20*math.sin(x*.055+y*.044)+.14*math.cos(y*.075)
            if rng.random()>density:
                continue
            fir = rng.random()<(.7 if x < -145 or y>748 else .29)
            species = rng.choice(("SM_Fir_Tree_01","SM_Fir_Tree_02")) if fir else rng.choice(("SM_White_Oak_02","SM_White_Oak_Young_01"))
            h = rng.uniform(7,12) if "Young" in species else rng.uniform(11,21)
            put("Nature/Trees/"+species,x,y,h,.17)
    for i in range(7100):
        x,y=rng.uniform(-298,98),rng.uniform(522,818)
        e=clearance(x,y)
        if e<.75:
            continue
        field=meadow(x,y)<1.2
        if i%13==0 and e>2.5 and not field:
            put("Nature/RocksAndCliffs/Rock_medium_VarB_mdl",x,y,rng.uniform(.6,2.4),.23)
        elif i%11==0 and e>2.5 and not field:
            put("Nature/BranchesAndStumps/SM_FirTreeStump",x,y,rng.uniform(.6,1.3),.18)
        elif field or e<3.1:
            path="Nature/GrassAndFlowers/"+("SM_flower_yellow" if i%6==0 else "SM_GrassClump_03")
            put(path,x,y,rng.uniform(.25,.75))
        else:
            path="Nature/GrassAndFlowers/SM_Fern_01" if i%4 else "Nature/Shrubs/SM_BushWithoutSnow_02"
            put(path,x,y,rng.uniform(.55,1.35))
    for path,transforms in groups.items():
        name="FT_"+path.split("/")[-1]
        ft=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,ASSETS,unreal.FoliageType_InstancedStaticMesh,unreal.FoliageType_InstancedStaticMeshFactory())
        ft.set_editor_property("mesh",cache[path][0])
        foliage_settings(ft,path)
        assert unreal.EditorAssetLibrary.save_loaded_asset(ft,False)
        unreal.InstancedFoliageActor.add_instances(world,ft,transforms)
    return {p:len(t) for p,t in groups.items()}


def overview():
    art["editor"].set_level_viewport_camera_info(unreal.Vector(-11500,52600,42600),unreal.Rotator(yaw=87,pitch=-67))


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        raise RuntimeError("Outskirts map already exists")
    world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    for pts,width in ((west,4.5),(haul,4.2),(return_path,3.5),([haul[0],(-245,762),(-238,793),(-236,820)],3.2)):
        kit["add_road"](pts,width)
    ground()
    backdrop()
    foothills()
    landmarks()
    counts=planting(world)
    actors=art["actors"]
    x,y=route[0]
    p=actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(x*100,y*100,(elevation(x,y)+1.05)*100),unreal.Rotator(yaw=129))
    p.set_actor_label("South trailhead")
    gm=unreal.load_asset("/Game/Level/VelesSettlementAssets/BP_VillageGameMode")
    world.get_world_settings().set_editor_property("default_game_mode",gm.generated_class())
    sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,16000),unreal.Rotator(pitch=-35,yaw=130))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property("atmosphere_sun_light",True)
    sun.light_component.set_editor_property("intensity",4.7)
    sun.light_component.set_editor_property("light_color",unreal.Color(255,236,204,255))
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector())
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture",True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
    fog=actors.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,4000))
    fog.get_component_by_class(unreal.ExponentialHeightFogComponent).set_editor_property("fog_density",.013)
    overview()
    assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP)
    unreal.log("VELES_NORTH_SAVED "+json.dumps(counts))


if __name__=="__main__":
    build()
