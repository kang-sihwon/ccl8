"""Apply the review corrections to the three StoryRegions maps and their own art assets."""
from pathlib import Path
import math
import os
import runpy
import unreal

root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
here=root/"Tools/WorldBuilding"
base=runpy.run_path(str(here/"build_veles_departure.py"))
globals().update({k:v for k,v in base.items() if not k.startswith("__")})
detail=runpy.run_path(str(here/"veles_departure_finishes.py"),init_globals=base)
tube=detail["tube"]
pose=detail["pose_camera"]
find=detail["find"]

def comp_of(cls):
    return next(a for a in actors.get_all_level_actors() if isinstance(a,cls))
def variant_lighting():
    # 속성 경로는 에디터의 표시 언어로 현지화된다.
    unreal.InternationalizationLibrary.set_current_language("en",False)
    lib=unreal.VariantManagerLibrary
    path=package+"/VelesVillageTimes"
    lib_asset=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else lib.create_level_variant_sets_asset("VelesVillageTimes",package)
    tag(lib.create_level_variant_sets_actor(lib_asset),"V00 Village times")
    group=unreal.VariantSet();group.set_display_text("Departure");lib_asset.add_variant_set(group)
    sun=comp_of(unreal.DirectionalLight)
    sky=comp_of(unreal.SkyLight)
    fog=comp_of(unreal.ExponentialHeightFog)
    c=fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    lamps=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.PointLight) and a.get_actor_label().startswith("V0")]
    guide={"Day":(-32,124,4.4,.85,.012,(1,.97,.92)),
           "Evening":(2.8,215,.035,.24,.028,(.40,.53,.77)),
           "Dawn":(-4,31,.85,.54,.038,(1,.81,.68))}
    for name in ("Evening","Dawn","Day"):
        pitch,yaw,power,ambient,mist,col=guide[name]
        sun.set_actor_rotation(unreal.Rotator(pitch=pitch,yaw=yaw),False)
        sun.light_component.set_intensity(power)
        sun.light_component.set_light_color(unreal.LinearColor(*col,1),False)
        sky.light_component.set_intensity(ambient)
        c.set_editor_property("fog_density",mist)
        v=unreal.Variant();v.set_display_text(name);group.add_variant(v)
        def bind(actor,props):
            v.add_actor_binding(actor)
            paths=lib.get_capturable_properties(actor)
            candidates={str(q).split("/")[-1].replace(" ","").lower():q for q in paths}
            for p in props:
                assert p in candidates,(p,list(paths))
                cap=v.capture_property(actor,candidates[p]);assert cap
                cap.record()
        bind(sun,["relativerotation","intensity","lightcolor"])
        bind(sky,["intensity"]);bind(fog,["fogdensity"])
        for l in lamps:
            l.light_component.set_intensity((65 if name=="Evening" else 6) if "Embers" not in l.get_actor_label() else (140 if name=="Evening" else 3))
            bind(l,["intensity"])
    assert unreal.EditorAssetLibrary.save_loaded_asset(lib_asset,False)
def civilians():
    clay=art["surface_material"]("M_VelesWarmSkin",["OldVillage/T_Stucco_001_C"],[(.45,.24,.16)])
    cloth=detail["tint"]("M_WorkingWool",(.21,.15,.083))
    caps=group()
    for a in actors.get_all_level_actors():
        if not isinstance(a,unreal.SkeletalMeshActor):continue
        label=a.get_actor_label().lower()
        if not (label.endswith("_head") or label.endswith("_hands")):continue
        sk=a.skeletal_mesh_component
        for j in range(sk.get_num_materials()):sk.set_material(j,clay)
        if not label.endswith("_head"):continue
        # 두상 메시 자체의 로컬 바운드로 두건을 맞춘다. 뼈대 공통 바운드는 쓰지 않는다.

        p=sk.get_socket_location("head")
        ax,ay,az=p.x/100,p.y/100,p.z/100
        if az-a.get_actor_location().z/100<.95:az=a.get_actor_location().z/100+1.53
        tube(caps,[(ax,ay,az+.055),(ax,ay,az+.11),(ax-.008,ay,az+.225),(ax-.035,ay,az+.26)],[.107,.128,.102,.014],16)
    shape("V00 Work caps and linen coifs","VillageCoifs",[caps],[cloth],False)
def valley_skirt(f):
    floor=f["floor"];river=f["main_river"];g=group()
    ring=[]
    for i in range(161):
        t=i/160;ring.append((-380+(river(180)+195+380)*t,180))
    for i in range(1,161):
        y=180+1140*i/160;ring.append((river(y)+195,y))
    for i in range(1,161):ring.append((river(1320)+195+(-380-river(1320)-195)*i/160,1320))
    for i in range(1,161):ring.append((-380,1320-1140*i/160))
    for j in range(13):
        t=j/12
        for i,(ex,ey) in enumerate(ring):
            dx,dy=ex-225,ey-750;length=math.hypot(dx,dy);a=math.atan2(dy,dx)
            r=length-.65+t*(1800-length)
            x=225+dx/length*r;y=750+dy/length*r
            z=(1-t)*floor(ex,ey)+t*(50+29*math.cos(a*6))+math.sin(math.pi*t)*(24+6*math.cos(a*13))-.42
            k=len(g[0]);g[0].append((x,y,z));g[2].append((x/22,y/22));g[3].append((0,0,0,1))
            if j and i:
                n=len(ring);g[1].extend(((k-n-1,k-n,k-1),(k-n,k,k-1)))
    shape("F00 Outer forest slopes","WatershedFoothills",[g],["/Game/Level/VelesNorthWoodAssets/M_DistantWood"],False)
def forest(world):
    f=runpy.run_path(str(here/"veles_departure_forest.py"),init_globals=base)
    # 원래 북쪽 숲이 공유하는 식생 타입은 이 지역 전용으로 복제한다.
    holder=comp_of(unreal.InstancedFoliageActor)
    batches=[]
    for c in holder.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
        m=c.get_editor_property("static_mesh")
        if not m or "/Trees/" not in m.get_path_name():continue
        rows=[c.get_instance_transform(i,True) for i in range(c.get_instance_count())]
        if rows:batches.append((m.get_name(),rows))
    assert sum(len(t) for _,t in batches)>2000
    for name,rows in batches:
        old="/Game/Level/VelesNorthWoodAssets/FT_"+name
        dest=package+"/Geometry/FT_Valley_"+name
        new=unreal.load_asset(dest) if unreal.EditorAssetLibrary.does_asset_exist(dest) else unreal.EditorAssetLibrary.duplicate_asset(old,dest)
        new.set_editor_property("cull_distance",unreal.Int32Interval(240000,310000))
        assert unreal.EditorAssetLibrary.save_loaded_asset(new,False)
        unreal.InstancedFoliageActor.remove_all_instances(world,unreal.load_asset(old))
        unreal.InstancedFoliageActor.add_instances(world,new,rows)
    unreal.log("DEPARTURE_CANOPY "+str([(n,len(t)) for n,t in batches]))
    valley_skirt(f)
    sun=comp_of(unreal.DirectionalLight);sun.light_component.set_light_color(unreal.LinearColor(1,.98,.95,1),False)
    floor=f["floor"];x,y=f["root"];z=floor(x,y)
    # 언덕 옆면을 보는 카메라를 동쪽 둔덕으로 옮기고 어두운 점검 통로를 액자 안에 둔다.
    pose("F00",(146,102,381),(125,810,49))
    pose("F06",(267.7,1242.8,floor(267.7,1242.8)+2.05),(260,1250,z+1.56))
    g=group();inner=group()
    for j in range(12):
        a=j*math.pi/12;b=(j+1)*math.pi/12
        yy=y+.15
        p=lambda t,d:(x+1.43*math.cos(t),yy+d,z+1.62+1.12*math.sin(t))
        face=[p(a,0),p(a,5),p(b,5),p(b,0)];quad(g,face);quad(g,face[::-1])
    box(inner,x-1.48,x+1.48,y+4.5,y+4.8,z-.35,z+2.81)
    for s in (-1,1):
        box(g,x+s*1.44-.1,x+s*1.44+.1,y,y+4.5,z-.45,z+1.68)
    shape("F06 Recessed spring adit","AditInterior",[g,inner],[wood,detail["tint"]("M_AditShadow",(.027,.023,.018))])
    for a in actors.get_all_level_actors():
        if a.get_actor_label().startswith("F06 Exposed roots"):
            a.set_actor_location(a.get_actor_location()-unreal.Vector(0,0,38),False,True)
    # 흙 위에 선 수지 채취 잔은 나무 기둥에 붙여 놓는다.
    for i in range(3):
        xx,yy=221+i*2.8,600-i;zz=floor(xx,yy)
        g=group();tube(g,[(xx,yy,zz-.2),(xx,yy,zz+.85),(xx+.08,yy+.04,zz+2.6),(xx+.12,yy+.1,zz+4.1)],[.36,.32,.25,.17])
        shape("F02 Tapped trunk","TappedTrunk"+str(i),[g],[bark])
def cave(world):
    c=runpy.run_path(str(here/"veles_departure_cave.py"),init_globals=base)
    # 단면 검토 방향은 입구가 왼쪽, 수원 공동이 오른쪽으로 읽히게 맞춘다.
    pose("C00",(37,74,46),(36,0,2.3))
    pose("C03",(65.2,-7.3,c["base"](65.2)+2.85),(61.5,1,c["base"](61.5)+1.12))
    m=find("C03 Bartul brown bear").static_mesh_component
    coat=art["surface_material"]("M_BartulFur",["ROG_Modular_Assets/T_Fur_Root1"],[(.31,.18,.098)],"ROG_Modular_Assets/T_Fur_N")
    m.set_material(0,coat)
    mat=art["surface_material"]("M_CaveWetStrata",["Medieval_Environment/T_Ground_Rock_Near_01_BC"],[(.33,.39,.43)],"Medieval_Environment/T_Ground_Rock_Near_01_N")
    find("C00 Rock vault and walls").static_mesh_component.set_material(0,mat)
    for a in actors.get_all_level_actors():
        if isinstance(a,unreal.PointLight):
            name=a.get_actor_label()
            if "Amber" in name:a.light_component.set_intensity(55)
            if "Reflected" in name:a.light_component.set_light_color(unreal.LinearColor(.44,.66,1,1),False)
            if "through roots" in name:a.light_component.set_intensity(1400)
    shelf=group()
    for i in range(95):
        x=-11+i;yy=c["center"](x);r=c["radius"](x)
        a=(x,yy+r*.97,c["base"](x)-.52);b=(x+1,c["center"](x+1)+c["radius"](x+1)*.97,c["base"](x+1)-.52)
        face=[a,b,(b[0],b[1],b[2]-3.45),(a[0],a[1],a[2]-3.45)]
        quad(shelf,face);quad(shelf,face[::-1])
    shape("C00 Source cavern bedrock","SpringCutLip",[shelf],[mat.get_path_name()],False)
def align_village_water(world):
    if any(a.get_actor_label()=="V02 Intake lamp bracket" for a in actors.get_all_level_actors()):return
    # 수위 질의와 충돌은 WaterBody에 두고 수면 렌더는 동일 스플라인에서 만든 메시로 한정한다.
    name="M_WaterQuerySurface";path=package+"/Geometry/"+name
    m=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,package+"/Geometry",unreal.Material,unreal.MaterialFactoryNew())
    lib=unreal.MaterialEditingLibrary
    m.set_editor_property("blend_mode",unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("used_with_spline_meshes",True)
    m.set_editor_property("used_with_water",True)
    zero=lib.create_material_expression(m,unreal.MaterialExpressionConstant);zero.r=0
    lib.connect_material_property(zero,"",unreal.MaterialProperty.MP_OPACITY_MASK)
    lib.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m,False)
    for body in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.WaterBodyRiver):
        c=body.get_water_body_component();c.set_water_material(m)
        for component in c.get_standard_renderable_components():component.set_material(0,m)
    zone=comp_of(unreal.WaterZone)
    wm=zone.get_component_by_class(unreal.WaterMeshComponent)
    wm.set_visibility(False);wm.set_hidden_in_game(True)
    l=find("V02 Lit intake lamp")
    o,h=l.get_actor_bounds(False)
    x,y,z=o.x/100,o.y/100,(o.z-h.z)/100
    g=group()
    box(g,x-.13,x+.13,y-.14,y+.14,z-1.16,z-.05)
    box(g,x-.37,x+.37,y-.33,y+.33,z-.09,z)
    shape("V02 Intake lamp bracket","IntakeLampBracket",[g],[wood])
    assert levels.save_map(world,maps[0])

def adjust(i):
    world=levels.load_map(maps[i])
    assert world.get_path_name().split(".")[0]==maps[i]
    marker=("V00 Village times","F06 Recessed spring adit","C00 Source cavern bedrock")[i]
    if any(a.get_actor_label()==marker for a in actors.get_all_level_actors()):
        if i==0:align_village_water(world)
        return
    if i==0:
        civilians();variant_lighting();align_village_water(world)
    elif i==1:forest(world)
    else:cave(world)
    c=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.CameraActor) and a.get_actor_label()[1:3]=="00")
    editor.set_level_viewport_camera_info(c.get_actor_location(),c.get_actor_rotation())
    assert levels.save_map(world,maps[i])
    unreal.log("DEPARTURE_FINAL "+maps[i])
if __name__=="__main__":
    adjust(int(os.environ["CCL_DEPARTURE_REGION"])-1)
