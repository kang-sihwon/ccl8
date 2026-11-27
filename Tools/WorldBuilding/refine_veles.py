"""Create a walkable village revision from the original Veles map in the editor."""
from collections import defaultdict
from pathlib import Path
import json
import math
import random
import runpy

import unreal


ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
base = runpy.run_path(str(ROOT / "Tools/WorldBuilding/build_veles.py"))
asset, height = base["asset"], base["height"]
actors, editor, lib = base["ACTORS"], base["EDITOR"], base["LIB"]
folder = "/Game/Level/VelesSettlementAssets"
target = "/Game/Level/VelesVillage"
report = ROOT / "Saved/Tests/VelesSettlement"
rng = random.Random(6208)
road_bins = defaultdict(list)
houses = []
plots = []
river_line = [(-300,-150), (0,0), (180,160), (500,600)]


def nearest(x,y,a,b):
    dx,dy = b[0]-a[0],b[1]-a[1]
    t = min(1,max(0,((x-a[0])*dx+(y-a[1])*dy)/(dx*dx+dy*dy or 1)))
    px,py = a[0]+t*dx,a[1]+t*dy
    return math.hypot(x-px,y-py),px,py


def add_road(points,width):
    for a,b in zip(points,points[1:]):
        for i in range(math.floor((min(a[0],b[0])-width-8)/16),math.floor((max(a[0],b[0])+width+8)/16)+1):
            for j in range(math.floor((min(a[1],b[1])-width-8)/16),math.floor((max(a[1],b[1])+width+8)/16)+1):
                road_bins[i,j].append((a,b,width/2))


def margin(x,y):
    return min((nearest(x,y,a,b)[0]-w for a,b,w in road_bins[math.floor(x/16),math.floor(y/16)]),default=25)


def river(x,y):
    d,px,py = min(nearest(x,y,a,b) for a,b in zip(river_line,river_line[1:]))
    width = 15.3+2.1*math.sin(px*.043)+1.6*math.sin(py*.092+.7)+.75*math.sin(px*.21)
    return d-width,height(px,py)+.44


def ground(x,y):
    shore,water = river(x,y)
    t = min(1,max(0,(shore+1)/5))
    t = t*t*(3-2*t)
    return (1-t)*(water+max(-2.4,shore*.27))+t*height(x,y)


def xy(h,u,v):
    return h["x"]+h["c"]*u-h["s"]*v,h["y"]+h["s"]*u+h["c"]*v


def local(h,x,y):
    dx,dy = x-h["x"],y-h["y"]
    return dx*h["c"]+dy*h["s"],-dx*h["s"]+dy*h["c"]


def house_gap(x,y):
    return min((max(abs(u)-h["w"]/2,abs(v)-h["d"]/2) for h in houses
                for u,v in [local(h,x,y)]),default=1000)


def intersects(a,b):
    for s in (a,b):
        for nx,ny in ((s["c"],s["s"]),(-s["s"],s["c"])):
            def radius(h):
                return .5*(h["w"]*abs(nx*h["c"]+ny*h["s"])+h["d"]*abs(ny*h["c"]-nx*h["s"]))
            if abs((a["x"]-b["x"])*nx+(a["y"]-b["y"])*ny)>radius(a)+radius(b)+2.8:
                return False
    return True


def position(label,x,y,w,d,h,yaw):
    rot=math.radians(yaw)
    spec=dict(label=label,x=x,y=y,w=w,d=d,h=h,yaw=yaw,c=math.cos(rot),s=math.sin(rot))
    for r in range(0,41,2):
        for i in range(max(1,r*2)):
            angle=i*math.tau/max(1,r*2)
            spec.update(x=x+r*math.cos(angle),y=y+r*math.sin(angle))
            if any(intersects(spec,old) for old in houses):
                continue
            footprint=[xy(spec,u*w/8,v*d/8) for u in range(-4,5) for v in range(-4,5)]
            if min(margin(*p) for p in footprint)<1.25 or min(river(*p)[0] for p in footprint)<2:
                continue
            spec["z"]=ground(spec["x"],spec["y"])+.12
            houses.append(spec)
            return spec
    raise ValueError("No free footprint for "+label)


def make_mesh(name,groups,materials,collision=True):
    path=folder+"/SM_"+name
    if lib.does_asset_exist(path):
        return unreal.load_asset(path)
    mesh=unreal.DynamicMesh()
    for slot,(verts,faces,uv,colors) in enumerate(groups):
        buf=unreal.GeometryScriptSimpleMeshBuffers()
        buf.vertices=[unreal.Vector(x*100,y*100,z*100) for x,y,z in verts]
        buf.triangles=[unreal.IntVector(*face) for face in faces]
        buf.uv0=[unreal.Vector2D(u,v) for u,v in uv]
        if colors:
            buf.vertex_colors=[unreal.LinearColor(*color) for color in colors]
        unreal.GeometryScript_MeshEdits.append_buffers_to_mesh(mesh,buf,slot)
    opt=unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
    opt.enable_collision=collision
    opt.collision_mode=unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
    opt.enable_recompute_normals=opt.enable_recompute_tangents=True
    result,status=unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh,path,opt)
    if not result:
        raise RuntimeError(str(status))
    result.set_editor_property("static_materials",[unreal.StaticMaterial(material_interface=m,material_slot_name="Surface"+str(i)) for i,m in enumerate(materials)])
    assert lib.save_loaded_asset(result,False)
    return result


def box(group,x0,x1,y0,y1,z0,z1):
    verts,faces,uv,_=group
    pts=[(x,y,z) for z in (z0,z1) for y in (y0,y1) for x in (x0,x1)]
    for ids in ((0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)):
        i=len(verts)
        a,b,c,d=[pts[k] for k in ids]
        verts.extend((a,b,c,d))
        u,v=math.dist(a,b)/2,math.dist(b,c)/2
        uv.extend(((0,0),(u,0),(u,v),(0,v)))
        faces.extend(((i,i+1,i+2),(i,i+2,i+3)))


def set_mesh(label,mesh,x,y,z,yaw,original=None):
    a=original or actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector())
    a.static_mesh_component.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(1,1,1))
    a.set_actor_rotation(unreal.Rotator(yaw=yaw),False)
    a.set_actor_location(unreal.Vector(x*100,y*100,z*100),False,False)
    a.set_actor_label(label)
    if not original:
        a.set_folder_path("Veles/Settlement")
    return a


def build_house(h,index,original):
    groups=[([],[],[],[]) for _ in range(3)]
    w,d,total=h["w"],h["d"],h["h"]
    eave=total*.60
    box(groups[2],-w/2+.28,w/2-.28,-d/2+.28,d/2-.28,-.19,0)
    probes=[]
    for side in range(4):
        front=side<2
        length=(w if front else d)-.78
        k=(-1 if side%2==0 else 1)*((d if front else w)/2-.49)
        def slab(slot,lo,hi,bottom,top,thickness=.26):
            if hi-lo<.005 or top-bottom<.005:
                return
            a,b=k-thickness/2,k+thickness/2
            box(groups[slot],lo,hi,a,b,bottom,top) if front else box(groups[slot],a,b,lo,hi,bottom,top)
        holes=[]
        if front:
            holes.append((0,1.04,0,2.70,"door"))
        for j in range(max(1,int(length/5.6))):
            p=(j+.5)*length/max(1,int(length/5.6))-length/2
            if not front or abs(p)>2.45:
                holes.append((p,.83,.24,1.67,"window"))
        start=-length/2
        for p,hw,sill,head,kind in sorted(holes):
            slab(0,start,p-hw,-.32,eave)
            slab(1,start,p-hw,-.32,.48,.32)
            slab(1,p-hw,p+hw,-.32,sill,.30)
            slab(0,p-hw,p+hw,head,eave)
            slab(2,p-hw-.14,p-hw,.0,head,.34)
            slab(2,p+hw,p+hw+.14,.0,head,.34)
            slab(2,p-hw-.16,p+hw+.16,head,head+.18,.35)
            if sill:
                slab(2,p-hw-.07,p+hw+.07,sill-.09,sill,.35)
            probes.append(dict(kind=kind,side=side,p=p,k=k,sill=sill,head=head))
            start=p+hw
        slab(0,start,length/2,-.32,eave)
        slab(1,start,length/2,-.32,.48,.32)
        slab(2,-length/2,length/2,eave-.22,eave,.36)
        for edge in (-length/2+.12,length/2-.12):
            slab(2,edge-.12,edge+.12,-.25,eave,.37)
        slab(2,-length/2,length/2,2.91,3.11,.32)
    mesh=make_mesh("Shell_"+h["label"].replace("-","_"),groups,[
        asset("Materials/Medieval_Environment/MI_CF_01_Plaster_01_BC"),
        asset("Materials/PolyPixel/INST_StoneWall_A2"),asset("Materials/OldVillage/MI_Wood_Planks_001_floor")])
    set_mesh(h["label"],mesh,h["x"],h["y"],h["z"],h["yaw"],original)
    roofmesh=asset("BuildingParts/Roofs/House_"+("ABC"[index%3])+"_VarA_Roof_mdl")
    roof=set_mesh("Roof_"+h["label"],roofmesh,h["x"],h["y"],h["z"],h["yaw"])
    b=roofmesh.get_bounding_box()
    dims=b.max-b.min
    sx,sy,sz=w*100/dims.x,d*100/dims.y,(total-eave)*100/dims.z
    roof.set_actor_scale3d(unreal.Vector(sx,sy,sz))
    x,y=xy(h,-(b.min.x+b.max.x)*sx/200,-(b.min.y+b.max.y)*sy/200)
    roof.set_actor_location(unreal.Vector(x*100,y*100,(h["z"]+eave)*100-b.min.z*sz),False,False)
    h["openings"]=probes


def terrain_material():
    path=folder+"/M_VillageGround"
    if lib.does_asset_exist(path):
        return unreal.load_asset(path)
    m=unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_VillageGround",folder,unreal.Material,unreal.MaterialFactoryNew())
    e=unreal.MaterialEditingLibrary
    color=e.create_material_expression(m,unreal.MaterialExpressionVertexColor)
    result=None
    for texture,channel in (("Grass",""),("Dirt","R"),("Sand","G")):
        sample=e.create_material_expression(m,unreal.MaterialExpressionTextureSample)
        sample.texture=asset("Textures/Medieval_Environment/T_Ground_"+texture+"_Near_01_BC")
        if result is None:
            result=sample
        else:
            blend=e.create_material_expression(m,unreal.MaterialExpressionLinearInterpolate)
            e.connect_material_expressions(result,"",blend,"A")
            e.connect_material_expressions(sample,"RGB",blend,"B")
            e.connect_material_expressions(color,channel,blend,"Alpha")
            result=blend
    e.connect_material_property(result,"",unreal.MaterialProperty.MP_BASE_COLOR)
    e.recompile_material(m)
    assert lib.save_loaded_asset(m,False)
    return m


def landscape(by_label):
    xs=sorted(set(range(-300,401,5))|set(range(-170,146,2)))
    ys=sorted(set(range(-120,521,5))|set(range(-45,301,2)))
    verts,faces,uv,colors=[],[],[],[]
    for j,y in enumerate(ys):
        for i,x in enumerate(xs):
            bank,_=river(x,y)
            noise=.75*math.sin(x*.37+y*.24)+.48*math.sin(y*.59-x*.16)
            edge=margin(x,y)+noise
            road=min(1,max(0,.72-edge*.31))
            sand=min(1,max(0,.85-(bank+noise)*.15))
            verts.append((x,y,ground(x,y)))
            uv.append((x/2.8,y/2.8))
            colors.append((road,sand,0,1))
            if i and j:
                n=len(verts)-1
                faces.extend(((n-len(xs)-1,n-1,n-len(xs)),(n-len(xs),n-1,n)))
    mat=terrain_material()
    terrain=set_mesh("VelesTerrain",make_mesh("VillageGround",[(verts,faces,uv,colors)],[mat]),0,0,0,0,by_label["VelesTerrain"])
    terrain.static_mesh_component.set_material(0,mat)
    actors.destroy_actor(by_label["VelesPaths"])
    verts,faces,uv=[],[],[]
    for a,b in zip(river_line,river_line[1:]):
        length=math.dist(a,b)
        dx,dy=(b[0]-a[0])/length,(b[1]-a[1])/length
        steps=math.ceil(length/2)
        for j in range(steps+1):
            x,y=a[0]+dx*length*j/steps,a[1]+dy*length*j/steps
            if not (-291<x<383 and -111<y<510):
                continue
            width=-river(x,y)[0]+1
            for s in (-width,0,width):
                px,py=x-dy*s,y+dx*s
                verts.append((px,py,height(x,y)+.44))
                uv.append((px/3,py/3))
            n=len(verts)-3
            if n:
                faces.extend(((n-3,n-2,n),(n-2,n+1,n),(n-2,n-1,n+1),(n-1,n+2,n+1)))
    mat=unreal.load_asset("/Game/Level/VelesSurfaces/M_VelesRiver")
    water=set_mesh("VelesRiver",make_mesh("VillageRiver",[(verts,faces,uv,[])],[mat],False),0,0,0,0,by_label["VelesRiver"])
    water.static_mesh_component.set_material(0,mat)


def planting(world):
    clusters=defaultdict(list)
    def plant(name,x,y,scale):
        clusters[name].append(unreal.Transform(location=unreal.Vector(x*100,y*100,ground(x,y)*100-2),
            rotation=unreal.Rotator(yaw=rng.uniform(-180,180)),scale=unreal.Vector(scale,scale,scale)))
    for x,y in [(32,145),(0,164),(-34,185),(-81,191),(69,209),(7,210),(-121,83),(-12,115),(-139,188),(21,250)]:
        for dx in range(-12,13,4):
            if len(plots)>=7:break
            cx=x+dx
            points=[(cx+a,y+b) for a in (-6,-3,0,3,6) for b in (-4,0,4)]
            if min(min(house_gap(*p)-2,margin(*p)-.9,river(*p)[0]-3) for p in points)<0:
                continue
            index=len(plots)
            plots.append((cx,y))
            for row in (-2.4,2.4):
                base["place"]("KitchenGardenBed","Nature/GardenBeds/SM_garden_large",cx,y+row,0,(9,4.4,None),.04,"KitchenGardens",False)
                for i in range(21):
                    for j in range(7):
                        px=cx+(i-10)*.37+rng.uniform(-.09,.09)
                        py=y+row+(j-3)*.49+rng.uniform(-.08,.08)
                        plant("F_Carrot" if index%3==1 else "F_WheatGrass01_01",px,py,rng.uniform(.65,1.05))
            break
    road_list=[r for r in base["LAYOUT"]["roads"] if r["id"] in ("R01","V-WATER") or r["id"].startswith("A")]
    for road in road_list:
        for a,b in zip(road["points"],road["points"][1:]):
            length=math.dist(a,b)
            for _ in range(int(length/1.4)):
                t=rng.random()
                cx,cy=a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t
                if not (-170<cx<125 and -30<cy<300): continue
                side=rng.choice((-1,1))*(road["width"]/2+rng.uniform(.1,3.6))
                cx-=(b[1]-a[1])*side/length
                cy+=(b[0]-a[0])*side/length
                for _ in range(rng.randint(3,9)):
                    x,y=cx+rng.gauss(0,.8),cy+rng.gauss(0,.6)
                    if margin(x,y)>.24 and house_gap(x,y)>1 and river(x,y)[0]>1:
                        plant(rng.choice(("F_GroundCover_02","F_Crownbeard_00","F_GroundCover_02")),x,y,rng.uniform(.42,.92))
    for _ in range(760):
        x=rng.uniform(-186,174)
        y0=x*(.5 if x<0 else 160/180)
        y=y0+rng.choice((-1,1))*rng.uniform(11,28)
        if not (-2<river(x,y)[0]<6) or math.hypot(x+22,y-10)<9:continue
        kind=rng.choice(("F_MossyStonesPack_00","F_GroundCover_02","F_FernA_00"))
        for i in range(rng.randint(2,8)):
            px,py=x+rng.gauss(0,1),y+rng.gauss(0,.8)
            if margin(px,py)>1:
                plant(kind,px,py,rng.uniform(.35,.94))
    for name,instances in clusters.items():
        unreal.InstancedFoliageActor.add_instances(world,asset("Foliage/MedievalEnvironment/"+name),instances)
    return {k:len(v) for k,v in clusters.items()}


def clear_buildings():
    for a in actors.get_all_level_actors():
        if str(a.get_folder_path()).endswith(("/Forest","/Wayside")):
            origin,_=a.get_actor_bounds(False)
            if house_gap(origin.x/100,origin.y/100)<6:
                actors.destroy_actor(a)


def configure_crouch(world):
    def copy(source,name):
        path=folder+"/"+name
        if lib.does_asset_exist(path):
            return unreal.load_asset(path)
        return lib.duplicate_asset("/Game/Level/VelesGameplay/"+source,path)
    traveler=copy("BP_VelesTraveler","BP_VillageTraveler")
    hero=unreal.get_default_object(traveler.generated_class())
    movement=hero.character_movement
    props=movement.get_editor_property("nav_agent_props")
    props.set_editor_property("can_crouch",True)
    movement.set_editor_property("nav_agent_props",props)
    graph=unreal.BlueprintGraphEditor.get_graph_editor(unreal.BlueprintEditorLibrary.find_event_graph(traveler))
    keys=[n for n in graph.list_all_nodes() if n.get_class().get_name()=="K2Node_InputKey"
          and str(n.get_editor_property("input_key").key_name)=="LeftControl"]
    if not keys:
        chord=next(n for n in graph.list_available_nodes([]) if str(n).replace(" ","").lower().endswith(
            ("keyboardevents|leftctrl","keyboardevents|왼쪽ctrl","keyboardevents|leftcontrol"))
            and "|DebugEvents|" not in str(n))
        key=graph.create_node_from_name(chord,unreal.Vector2D(0,420),[])
        for pin,name in (("Pressed","Crouch"),("Released","UnCrouch")):
            call=graph.add_call_function_node("/Script/Engine.Character:"+name)
            assert call and key
            assert unreal.BlueprintEditorLibrary.find_output_pin(key,pin).try_create_connection(
                unreal.BlueprintEditorLibrary.find_execute_pin(call))
    unreal.BlueprintEditorLibrary.compile_blueprint(traveler)
    assert lib.save_loaded_asset(traveler,False)
    mode=copy("BP_VelesGameMode","BP_VillageGameMode")
    unreal.get_default_object(mode.generated_class()).set_editor_property("default_pawn_class",traveler.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(mode)
    assert lib.save_loaded_asset(mode,False)
    world.get_world_settings().set_editor_property("default_game_mode",mode.generated_class())


def finish_frontages():
    timber=([],[],[],[])
    for h in houses:
        for side in range(4):
            front=side<2
            length=(h["w"] if front else h["d"])-.78
            wall=(-1 if side%2==0 else 1)*((h["d"] if front else h["w"])/2-.49)
            sign=-1 if side%2==0 else 1
            outside=wall+sign*.185
            eave=h["h"]*.60
            def beam(a,z,b,top,width):
                axis,up=b-a,top-z
                span=math.hypot(axis,up)
                if span<.01:return
                start=len(timber[0])
                box(timber,0,span,-.08,.08,-width/2,width/2)
                for k in range(start,len(timber[0])):
                    along,depth,lift=timber[0][k]
                    p=a+(axis*along-up*lift)/span
                    wz=z+(up*along+axis*lift)/span
                    u,v=(p,outside+depth) if front else (outside+depth,p)
                    x,y=xy(h,u,v)
                    timber[0][k]=(x,y,h["z"]+wz)
            n=max(2,round(length/3.6))
            for j in range(n):
                left=-length/2+length*j/n
                right=left+length/n
                beam(left,3.05,left,eave-.12,.19)
                beam(left,3.10 if j%2 else eave-.18,right,eave-.18 if j%2 else 3.10,.18)
            holes=sorted([o for o in h["openings"] if o["side"]==side],key=lambda o:o["p"])
            for op in holes:
                if op["kind"]!="window":continue
                for wing in (-1,1):
                    for j in range(3):
                        p=op["p"]+wing*(.99+.19*j)
                        beam(p,.38,p,1.57,.175)
                    p=op["p"]+wing*1.17
                    beam(p-.25,.70,p+.25,.70,.095)
                    beam(p-.25,1.28,p+.25,1.28,.095)
    mat=asset("Materials/OldVillage/MI_Wood_Planks_001_floor")
    set_mesh("VillageTimberwork",make_mesh("VillageTimberwork",[timber],[mat],False),0,0,0,0)


def plant_banks_and_yards():
    seeded=random.Random(482)
    def place(label,path,x,y,size,burial=0):
        return base["place"](label,path,x,y,seeded.uniform(-180,180),(None,None,size),
            ground(x,y)-height(x,y)+burial,"RiverAndYards",False)
    for i in range(125):
        x=seeded.uniform(-196,158)
        sign=seeded.choice((-1,1))
        y=x*(.5 if x<0 else 160/180)+sign*seeded.uniform(15,24)*(1.12 if x<0 else 1.34)
        if abs(river(x,y)[0])>6 or math.hypot(x+20,y-10)<8 or margin(x,y)<2:continue
        place("MossyBank","Nature/GroundCover/SM_MossyEmbankmentA_00",x,y,seeded.uniform(.60,1.25),-.17)
        for j in range(3):
            px,py=x+seeded.gauss(0,2.1),y+seeded.gauss(0,1.4)
            if river(px,py)[0]<-.7 or margin(px,py)<1:continue
            place("RiverbankBush","Nature/Shrubs/SM_BushWithoutSnow_02",px,py,seeded.uniform(.72,1.6),-.10)
    for i in range(90):
        x=seeded.uniform(-170,132)
        y=x*(.5 if x<0 else 160/180)-seeded.uniform(31,56)
        if y < -110 or river(x,y)[0]<4:continue
        tree="Nature/Trees/SM_White_Oak_01" if i%3 else "Nature/Trees/SM_Tree_Pine_002"
        place("FarBankTree",tree,x,y,seeded.uniform(5.5,13),-.12)
    for h in houses:
        for _ in range(8):
            x,y=xy(h,seeded.choice((-1,1))*(h["w"]/2+seeded.uniform(3,7)),seeded.uniform(-h["d"]/2,h["d"]/2))
            if house_gap(x,y)<3 or margin(x,y)<1.5 or river(x,y)[0]<5:continue
            if any(abs(x-cx)<7 and abs(y-cy)<5 for cx,cy in plots):continue
            place("GardenTree","Nature/Trees/SM_White_Oak_01",x,y,seeded.uniform(4.4,7),-.10)
            for j in range(3):
                place("YardShrub","Nature/Shrubs/SM_BushWithoutSnow_01",x+j*.55,y+j*.16,seeded.uniform(.7,1.2),-.07)
            break


def build():
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if levels.is_in_play_in_editor() or unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError("An idle editor with saved maps is required")
    if lib.does_asset_exist(target):
        raise RuntimeError("Revision map already exists: "+target)
    assert levels.new_level_from_template(target,base["MAP"])
    world=editor.get_editor_world()
    assert world.get_path_name().split(".")[0]==target
    by_label={a.get_actor_label():a for a in actors.get_all_level_actors()}
    for r in base["LAYOUT"]["roads"]:
        if r["id"] in ("R01","V-WATER") or r["id"].startswith("A"):
            add_road(r["points"],r["width"])
    add_road([base["ROUTE"][1],[-48,80],[-80,80]],3)
    add_road([[-46,20],[-32,14.6]],3.2)
    originals=[(k,a) for k,a in by_label.items() if k.startswith("house-") or k in ("workshop","water-store")]
    originals.sort(key=lambda v:(v[0]!="workshop",v[0]))
    for label,a in originals:
        b=a.static_mesh_component.static_mesh.get_bounding_box()
        dims=b.max-b.min
        scale=a.get_actor_scale3d()
        center,_=a.get_actor_bounds(False)
        position(label,center.x/100,131 if label=="workshop" else center.y/100,
            dims.x*scale.x*.02,dims.y*scale.y*.02,dims.z*scale.z*.015,a.get_actor_rotation().yaw)
    for i,(x,y,w,d,h,yaw) in enumerate(((31,118,11,9,7,13),(5,104,12,8,7.6,15),(13,151,12,9,7.2,55),(51,176,10,9,6.8,52))):
        position("lane-cottage-"+str(i+1),x,y,w,d,h,yaw)
    for i,h in enumerate(houses):
        build_house(h,i,by_label.get(h["label"]))
    clear_buildings()
    configure_crouch(world)
    # 문턱에서 기존 길까지 흙길을 잇는다.
    streets=[(a,b) for r in base["LAYOUT"]["roads"] if r["id"] in ("R01","V-WATER") or r["id"].startswith("A")
             for a,b in zip(r["points"],r["points"][1:])]
    for h in houses:
        p=xy(h,0,-h["d"]/2)
        _,x,y=min(nearest(*p,a,b) for a,b in streets)
        add_road([p,(x,y)],1.65)
    landscape(by_label)
    counts=planting(world)
    finish_frontages()
    plant_banks_and_yards()
    workshop=houses[0]
    editor.set_level_viewport_camera_info(unreal.Vector((workshop["x"]-12)*100,(workshop["y"]-33)*100,420),
                                        unreal.Rotator(yaw=70,pitch=2))
    assert levels.save_current_level()
    report.mkdir(parents=True,exist_ok=True)
    (report/"revision.json").write_text(json.dumps(dict(map=target,houses=houses,plots=plots,planting=counts),indent=2))
    unreal.log("VELES_SETTLEMENT_SAVED "+target)


if __name__=="__main__":
    build()
