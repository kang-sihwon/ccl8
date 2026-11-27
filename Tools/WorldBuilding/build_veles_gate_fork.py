"""Build the standalone WP crossing from the two departure-region sources. Units are metres."""
import math
import os
from pathlib import Path
import random
import runpy

import unreal

HERE = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())) / "Tools/WorldBuilding"
BASE = runpy.run_path(str(HERE / "build_veles_departure.py"))
ART = BASE["art"]
FOREST = runpy.run_path(str(HERE / "veles_departure_forest.py"), init_globals=BASE)
ROOT = "/Game/Level/VelesDeparture/GateFork"
MAP = ROOT + "/WP_VelesGateFork_Valley"
GATE = FOREST["gate"]
FORK = (-78, 218)
APPROACH = ART["forest"][-4:] + [FORK]
EAST = [FORK, (-73, 226), *FOREST["routes"][0][1:3]]
WEST = [FORK, (-84, 229), *FOREST["routes"][1][1:3]]
VEC = BASE["vec"]
ACTORS = BASE["actors"]
RNG = random.Random(2611)


def ramp(t):
    t = min(1, max(0, t))
    return t*t*(3-2*t)


def creek(y):
    # 문 동쪽의 둔치를 따라 마을 외곽으로 돌아나간다.
    low = -56 - .72*(y-180) + 1.2*math.sin((y-174)/9)
    t = ramp((y-188)/26)
    return low*(1-t) + FOREST["channel"](y)*t


def width(y):
    return 1.22 + (FOREST["width"](y)-1.22)*ramp((y-166)/56)


def ground(x, y):
    t = ramp((y-194)/38)
    village = ART["bed"](x, y)
    land = village*(1-t) + FOREST["floor"](x, y)*t
    w = width(y)
    d = abs(x-creek(y))
    level = FOREST["surface"](y)
    bed = level - .58 + .58*min(1, d/w)**2 + max(0, d-w)*.25
    blend = ramp((d-w)/4.5)
    return bed*(1-blend) + land*blend


def source_props():
    selection = []
    for region in (0, 1):
        world = BASE["levels"].load_map(BASE["maps"][region])
        for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
            label = actor.get_actor_label()
            mesh = actor.static_mesh_component.static_mesh
            if not mesh:
                continue
            center, extent = actor.get_actor_bounds(False)
            x, y = center.x/100, center.y/100
            if region == 0:
                if not (-148 < x < 20 and 130 < y < 210 and max(extent.x, extent.y) < 1900):
                    continue
                if abs(x-creek(y)) < width(y)+max(extent.x, extent.y)/100+1:
                    continue
            elif not label.startswith(("F01 Coppice", "F01 Worn gate")):
                continue
            if region and ART["kit"]["margin"](x,y) < .5:
                continue
            comp = actor.static_mesh_component
            mats = [m.get_path_name() if m else None for m in comp.get_materials()]
            loc = actor.get_actor_location()
            source_ground = ART["bed"](x,y) if region == 0 else FOREST["floor"](x,y)
            loc.z += (ground(x,y)-source_ground)*100
            selection.append((label, mesh.get_path_name(), loc, actor.get_actor_rotation(),
                              actor.get_actor_scale3d(), mats, actor.get_actor_enable_collision()))
    assert sum(label.startswith("Forest gatepost") for label,*_ in selection) == 2
    return selection


def terrain():
    # 모든 타일 경계는 같은 정수 미터 표본과 월드 UV를 공유한다.
    for x0 in range(-192, 32, 32):
        for y0 in range(128, 320, 32):
            tile = ART["group"]()
            for j in range(33):
                y = y0+j
                for i in range(33):
                    x = x0+i
                    dist = ART["kit"]["margin"](x,y)
                    path = ramp(.62-dist+.24*math.sin(x*.8+y*.3)+.13*math.sin(y*1.4))
                    bank = max(0,1-abs(abs(x-creek(y))-width(y))/2)
                    tile[0].append((i-16,j-16,ground(x,y)))
                    tile[2].append((x/3,y/3))
                    tile[3].append((max(path,.8*bank),.08*(1-path),0,1))
                    k = len(tile[0])-1
                    if i and j:
                        tile[1].extend(((k-34,k-1,k-33),(k-33,k-1,k)))
            key = f"Ground_{x0+192}_{y0}"
            BASE["shape"]("Terrain "+key,key,[tile],["/Game/Level/VelesNorthWoodAssets/M_NorthWoodFloor"],
                          at=(x0+16,y0+16,0))


BROOK_TOP, BROOK_BOTTOM = 354, 94


def water(world, zone=None, body=None):
    zone = zone or ACTORS.spawn_actor_from_class(unreal.WaterZone,VEC((-78,224,8)))
    zone.modify()
    zone.set_editor_property("zone_extent",unreal.Vector2D(31000,31000))
    zone.get_component_by_class(unreal.WaterMeshComponent).set_visibility(False)
    zone.get_component_by_class(unreal.WaterMeshComponent).set_hidden_in_game(True)
    ys = list(range(BROOK_TOP,BROOK_BOTTOM-1,-2))
    body = body or ACTORS.spawn_actor_from_class(unreal.WaterBodyRiver,VEC((creek(ys[0]),ys[0],FOREST["surface"](ys[0]))))
    body.modify()
    body.set_actor_label("Forest gate brook")
    mask = unreal.load_asset(BASE["package"]+"/Geometry/M_WaterQuerySurface")
    comp = body.get_water_body_component()
    comp.set_editor_property("affects_landscape",False)
    comp.set_water_zone_override(zone)
    comp.set_water_material(mask)
    comp.set_water_info_material(unreal.load_asset("/Water/Materials/WaterInfo/DrawWaterInfo"))
    spline = body.get_water_spline()
    spline.modify()
    spline.set_spline_points([VEC((creek(y),y,FOREST["surface"](y))) for y in ys],unreal.SplineCoordinateSpace.WORLD,True)
    for i,y in enumerate(ys):
        slope = (creek(y+.1)-creek(y-.1))/.2
        comp.set_river_width_at_spline_input_key(i,200*width(y)/math.hypot(1,slope))
        comp.set_river_depth_at_spline_input_key(i,58)
        comp.set_water_velocity_at_spline_input_key(i,54)
    spline.call_method("K2_SynchronizeAndBroadcastDataChange")
    for c in comp.get_standard_renderable_components():
        c.set_material(0,mask)
    runpy.run_path(str(HERE/"veles_water.py"))["draw_surface"](ART,body,unreal.load_asset(BASE["watermat"]),"GateBrookToRock",False)


def planting(world, structures):
    types = {}
    for path in unreal.EditorAssetLibrary.list_assets("/Game/Level/VelesNorthWoodAssets",False,False):
        if "/FT_" in path:
            ft = unreal.load_asset(path)
            types[ft.get_name()[3:]] = (ft,ft.get_editor_property("mesh").get_bounding_box(),[])
    def place(name,x,y,h):
        ft,bounds,batch = types[name]
        sc = h*100/(bounds.max.z-bounds.min.z)
        pos = VEC((x,y,ground(x,y)-.04))
        pos.z -= bounds.min.z*sc
        batch.append(unreal.Transform(location=pos,rotation=unreal.Rotator(yaw=RNG.uniform(0,360)),
                                      scale=unreal.Vector(sc,sc,sc)))
    for yy in range(130,318,4):
        for xx in range(-190,30,4):
            x,y = xx+RNG.uniform(-1.5,1.5), yy+RNG.uniform(-1.5,1.5)
            margin = ART["kit"]["margin"](x,y)
            if margin < .7 or any(abs(x-a)<w and abs(y-b)<h for a,b,w,h in structures):
                continue
            bank = abs(x-creek(y))-width(y)
            if bank < -.5:
                continue
            if bank < 1 and RNG.random()<.35:
                place("Rock_medium_VarB_mdl",x,y,RNG.uniform(.25,.65))
            if bank > .7 and margin>2.1 and RNG.random() < (.21 if y>211 else .065):
                place(RNG.choice(("SM_Fir_Tree_01","SM_White_Oak_02","SM_Fir_Tree_02")),x,y,RNG.uniform(9,20))
            place(RNG.choice(("SM_Fern_01","SM_GrassClump_03","SM_BushWithoutSnow_02")),x,y,RNG.uniform(.35,.95))
    for ft,_,batch in types.values():
        if batch:
            unreal.InstancedFoliageActor.add_instances(world,ft,batch)


def finish_banks():
    scatter = random.Random(916)
    for index,y0 in enumerate((161,175,187,197,207,222,235,250,265,283,302)):
        side = 1 if index%3==0 else -1
        for j in range(scatter.randrange(8,16)):
            y = y0+scatter.gauss(0,1.6)
            x = creek(y)+side*(width(y)+scatter.uniform(-.55,.95))
            length = scatter.uniform(.3,1.2)
            path = scatter.choice(("Rock_small_VarA_mdl","Rock_small_VarB_mdl","SM_Pebbles_02"))
            BASE["prop"]("Brook gravel bar","Nature/RocksAndCliffs/"+path,x,y,ground(x,y)-length*.10,
                         length=length,yaw=scatter.uniform(0,360),solid=False)
        for j in range(5):
            y = y0+scatter.uniform(-2.7,2.7)
            x = creek(y)-side*(width(y)+scatter.uniform(.18,1.1))
            BASE["prop"]("Brook bank tuft","Nature/GrassAndFlowers/SM_GrassClump_03",x,y,ground(x,y)-.08,
                         height=scatter.uniform(.28,.67),yaw=scatter.uniform(0,360),solid=False)


def brook_shading():
    # 얕은 민물의 투과와 산란을 전용 재질에서 맞춘다.
    target = ROOT+"/Geometry/M_GateBrook"
    lib = unreal.EditorAssetLibrary
    mat = unreal.load_asset(target) if lib.does_asset_exist(target) else None
    if not mat:
        mat = lib.duplicate_asset("/Game/Level/VelesRiversideAssets/M_VelesFlowingWater",target)
        colors = {(.028,.14,.135):(.012,.027,.022),(.007,.019,.017):(.00065,.0011,.00082),
                  (.052,.012,.017):(.021,.016,.025)}
        edits = 0
        for node in unreal.ObjectIterator(unreal.MaterialExpressionConstant3Vector):
            if node.get_path_name().startswith(mat.get_path_name()+":"):
                v = node.constant
                key = tuple(round(n,4) for n in (v.r,v.g,v.b))
                if key in colors:
                    node.modify()
                    node.constant = unreal.LinearColor(*colors[key],1)
                    edits+=1
        assert edits==3,edits
        unreal.MaterialEditingLibrary.recompile_material(mat)
        assert lib.save_loaded_asset(mat,False)
    sheet = next(a for a in ACTORS.get_all_level_actors() if a.get_actor_label()=="Forest gate brook surface")
    sheet.modify()
    sheet.static_mesh_component.modify()
    sheet.static_mesh_component.set_material(0,mat)


def save_new_world(levels):
    assert levels.save_current_level()
    dirty = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    created = [p for p in dirty if "/gatefork/" in p.get_name().lower()]
    if created:
        assert unreal.EditorLoadingAndSavingUtils.save_packages(created,True)


def valley_rim():
    mesh = unreal.load_asset("/Game/Level/VelesNorthWoodAssets/SM_DistantWoodedRidges")
    a = ART["actor_mesh"]("Valley rim DistantWoodedRidges",mesh,-25,-191.4,-11,0)
    a.set_actor_scale3d(unreal.Vector(.55,.62,.46))
    a.set_actor_enable_collision(False)
    a.static_mesh_component.set_cast_shadow(False)
    a.set_editor_property("is_spatially_loaded",False)
    a.set_folder_path("GateFork/Backdrop")
    perimeter = runpy.run_path(str(HERE/"veles_gate_perimeter.py"),init_globals=globals())
    perimeter["build_perimeter"]()


def repair_perimeter():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert levels.load_level(MAP)
    descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
    unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in descs])
    ART["mesh"].__globals__["folder"] = ROOT+"/Geometry"
    ART["kit"]["road_bins"].clear()
    ART["plan"]()
    for road in (APPROACH,EAST,WEST):
        ART["kit"]["add_road"](road,4)
    world = BASE["editor"].get_editor_world()
    gs = unreal.GameplayStatics
    def single(cls):
        items = gs.get_all_actors_of_class(world,cls)
        assert len(items)==1
        return items[0]
    perimeter = runpy.run_path(str(HERE/"veles_gate_perimeter.py"),init_globals=globals())
    perimeter["build_perimeter"]()
    water(world,single(unreal.WaterZone),single(unreal.WaterBodyRiver))
    brook_shading()
    save_new_world(levels)
    unreal.log("GATE_PERIMETER SAVED")


def finish_saved_banks():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert levels.load_level(MAP)
    descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
    has_banks = any(str(d.label).startswith("Brook gravel bar") for d in descs)
    has_rim = any(str(d.label).startswith("Valley rim") for d in descs)
    unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in descs])
    if not has_banks:
        ART["plan"]()
        finish_banks()
    if not has_rim:
        valley_rim()
    brook_shading()
    save_new_world(levels)
    unreal.log("GATE_BANKS SAVED")


def build():
    lib = unreal.EditorAssetLibrary
    ART["mesh"].__globals__["folder"] = ROOT+"/Geometry"
    ART["kit"]["road_bins"].clear()
    ART["plan"]()
    for road in (APPROACH,EAST,WEST):
        ART["kit"]["add_road"](road,4)
    selection = source_props()
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if lib.does_asset_exist(MAP):
        assert levels.load_level(MAP)
        descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
        if any(str(d.label).startswith(("Terrain ", "Forest gatepost", "Arrival")) for d in descs):
            raise RuntimeError("Destination already populated: "+MAP)
    else:
        assert levels.new_level(MAP,True)
    world = BASE["editor"].get_editor_world()
    grid_class = unreal.load_class(None,"/Script/Engine.RuntimePartitionLHGrid")
    grids = sorted((g for g in unreal.ObjectIterator(grid_class) if g.get_path_name().startswith(MAP+".")),
                   key=lambda g:g.get_path_name())
    grid = grids[0]
    grid.modify()
    grid.set_editor_property("CellSize",3200)
    grid.set_editor_property("LoadingRange",9600)
    structures = []
    for label,mesh,loc,rot,scale,mats,collides in selection:
        actor = ACTORS.spawn_actor_from_class(unreal.StaticMeshActor,loc,rot)
        actor.static_mesh_component.set_static_mesh(unreal.load_asset(mesh))
        actor.set_actor_scale3d(scale)
        actor.set_actor_label(label)
        actor.set_actor_enable_collision(collides)
        for i,path in enumerate(mats):
            if path:
                actor.static_mesh_component.set_material(i,unreal.load_asset(path))
        if "Home_" in mesh:
            p,e = actor.get_actor_bounds(False)
            structures.append((p.x/100,p.y/100,e.x/100+1,e.y/100+1))
    terrain()
    water(world)
    planting(world,structures)
    finish_banks()
    valley_rim()
    brook_shading()
    sun = ACTORS.spawn_actor_from_class(unreal.DirectionalLight,VEC((-80,205,60)),unreal.Rotator(pitch=-38,yaw=18))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property("atmosphere_sun_light",True)
    sun.light_component.set_editor_property("intensity",4.2)
    sun.light_component.set_light_color(unreal.LinearColor(1,.89,.71,1),False)
    ACTORS.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
    sky = ACTORS.spawn_actor_from_class(unreal.SkyLight,unreal.Vector())
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture",True)
    fog = ACTORS.spawn_actor_from_class(unreal.ExponentialHeightFog,VEC((-80,205,8)))
    fog.get_component_by_class(unreal.ExponentialHeightFogComponent).set_editor_property("fog_density",.012)
    x,y = -65,180
    BASE["setup"](world,(x,y,ground(x,y)+1.0),121)
    eye = VEC((-59,173,ground(-59,173)+7))
    BASE["editor"].set_level_viewport_camera_info(eye,unreal.MathLibrary.find_look_at_rotation(eye,VEC((*GATE,ground(*GATE)+1))))
    save_new_world(levels)
    unreal.log(f"GATE_FORK SAVED props={len(selection)} houses={len(structures)} grid={grid.get_path_name()}")


if __name__ == "__main__":
    if os.environ.get("CCL_GATE_FINISH")=="perimeter":
        repair_perimeter()
    elif os.environ.get("CCL_GATE_FINISH")=="banks":
        finish_saved_banks()
    else:
        build()
