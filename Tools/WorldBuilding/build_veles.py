"""Build /Game/Level/Veles in UE after running prepare_veles_blockout.py."""
import json
import math
import random
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
LAYOUT = json.loads((ROOT / "Saved/WorldBuilding/VelesBlockout/layout.json").read_text(encoding="utf-8"))
MAP = "/Game/Level/Veles"
SURFACES = "/Game/Level/VelesSurfaces"
EXTERNAL = "/Game/ExternalPackage/"
ROUTE = LAYOUT["roads"][0]["points"][:4]
LIB = unreal.EditorAssetLibrary
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
EDITOR = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
RNG = random.Random(124)


def height(x, y):
    fx, fy = (x + 512) / 8, (y + 128) / 8
    ix, iy = int(fx), int(fy)
    u, v = fx - ix, fy - iy
    i = iy * 145 + ix
    z = [LAYOUT["terrain"]["vertices"][i + offset][2] / 100 for offset in (0, 1, 145, 146)]
    if u + v <= 1:
        return z[0] + u * (z[1] - z[0]) + v * (z[2] - z[0])
    return z[3] + (1-u) * (z[2]-z[3]) + (1-v) * (z[1]-z[3])


def asset(path):
    result = unreal.load_asset(EXTERNAL + path)
    if not result:
        raise RuntimeError("Missing organized asset " + path)
    return result


def surface(name, vertices, faces, material, collision):
    path = SURFACES + "/SM_" + name
    mesh = unreal.load_asset(path) if LIB.does_asset_exist(path) else None
    if not mesh:
        buffers = unreal.GeometryScriptSimpleMeshBuffers()
        buffers.vertices = [unreal.Vector(x*100, y*100, z*100) for x,y,z in vertices]
        buffers.uv0 = [unreal.Vector2D(x/3, y/3) for x,y,z in vertices]
        buffers.triangles = [unreal.IntVector(*face) for face in faces]
        dynamic = unreal.DynamicMesh()
        unreal.GeometryScript_MeshEdits.append_buffers_to_mesh(dynamic, buffers)
        options = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
        options.enable_collision = collision
        options.collision_mode = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        options.enable_recompute_normals = options.enable_recompute_tangents = True
        mesh, result = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(dynamic, path, options)
        if not mesh or not LIB.save_loaded_asset(mesh, False):
            raise RuntimeError(str(result))
    actor = ACTORS.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector())
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_material(0, asset(material))
    actor.set_actor_enable_collision(collision)
    actor.set_actor_label(name)
    actor.set_folder_path("Veles/Terrain")


def place(label, path, x, y, yaw=0, size=None, lift=0, folder="Village", collision=True, actor=None):
    mesh = asset(path)
    if actor is None:
        actor = ACTORS.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x*100,y*100,0))
    actor.static_mesh_component.set_static_mesh(mesh)
    bounds = mesh.get_bounding_box()
    extent = bounds.max - bounds.min
    scale = min(w*100/v for w,v in zip(size, (extent.x, extent.y, extent.z)) if w) if size else 1
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    actor.set_actor_rotation(unreal.Rotator(yaw=yaw), False)
    origin, half = actor.get_actor_bounds(False)
    target = unreal.Vector(x*100-origin.x, y*100-origin.y, (height(x,y)+lift)*100-origin.z+half.z)
    actor.set_actor_location(actor.get_actor_location()+target, False, False)
    actor.set_actor_enable_collision(collision)
    actor.set_actor_label(label)
    actor.set_folder_path("Veles/"+folder)
    return actor


def strips(lines, offset):
    vertices, faces = [], []
    for points, width in lines:
        for a,b in zip(points, points[1:]):
            length = math.dist(a,b)
            if not length:
                continue
            dx,dy = (b[0]-a[0])/length, (b[1]-a[1])/length
            n = max(1, math.ceil(length/2))
            previous = None
            for i in range(n+1):
                x,y = a[0]+dx*length*i/n, a[1]+dy*length*i/n
                if not (-290 < x < 385 and -115 < y < 510):
                    previous = None
                    continue
                index = len(vertices)
                for side in (-width/2, 0, width/2):
                    px,py = x-dy*side, y+dx*side
                    vertices.append((px,py,offset(x,y,px,py)))
                if previous is not None:
                    for k in (0,1):
                        faces.extend([(previous+k,previous+k+1,index+k),(previous+k+1,index+k+1,index+k)])
                previous = index
    return vertices, faces


def level_blueprint(name, parent):
    folder = "/Game/Level/VelesGameplay"
    if LIB.does_asset_exist(folder+"/"+name):
        return unreal.load_asset(folder+"/"+name)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class",parent)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Blueprint,factory)
    assert bp
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    return bp


def configure_player(world):
    traveler = level_blueprint("BP_VelesTraveler",unreal.CCLCharacter)
    hero = unreal.get_default_object(traveler.generated_class())
    hero.mesh.set_skeletal_mesh_asset(asset("Characters/Bodies/SK_ElfArden"))
    hero.mesh.set_relative_location(unreal.Vector(0,0,-hero.capsule_component.get_scaled_capsule_half_height()),False,False)
    hero.mesh.set_relative_rotation(unreal.Rotator(yaw=-90),False,False)
    hero.mesh.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    playback = hero.mesh.get_editor_property("animation_data")
    playback.anim_to_play = asset("Characters/Animations/ElfArden/AS_idle")
    hero.mesh.set_editor_property("animation_data",playback)
    unreal.BlueprintEditorLibrary.compile_blueprint(traveler)
    assert LIB.save_loaded_asset(traveler,False)
    mode = level_blueprint("BP_VelesGameMode",unreal.CCLGameModeBase)
    unreal.get_default_object(mode.generated_class()).set_editor_property("default_pawn_class",traveler.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(mode)
    assert LIB.save_loaded_asset(mode,False)
    world.get_world_settings().set_editor_property("default_game_mode",mode.generated_class())


def dress_yards():
    paths = next(a for a in ACTORS.get_all_level_actors() if a.get_actor_label()=="VelesPaths")
    paths.static_mesh_component.set_editor_property("cast_shadow",False)
    paths.set_actor_location(unreal.Vector(0,0,-3.5),False,False)
    place("Bellows","Props/WorkshopTools/SM_OldBellows",58,115,76,(2.6,2,1.7))
    place("ToolCrates","Props/Containers/SM_crate_group_01",64.5,115,4,(2.8,2.3,1.7))
    place("QuayFishCrate","Props/Containers/Market_crate_fish_mdl",-23,11,-24,(1.4,1,1),folder="WorkAndLanding")
    seeded=random.Random(8101)
    for a,b in zip(ROUTE,ROUTE[1:]):
        length=math.dist(a,b)
        dx,dy=(b[0]-a[0])/length,(b[1]-a[1])/length
        for i in range(int(length/2)):
            along=seeded.uniform(0,length)
            side=seeded.choice((-1,1))*seeded.uniform(2.3,9)
            x,y=a[0]+dx*along-dy*side,a[1]+dy*along+dx*side
            path="Nature/GrassAndFlowers/SM_GrassClump_01" if i%3 else "Nature/GrassAndFlowers/SM_GrassClump_03"
            place("WaysideGrass",path,x,y,seeded.uniform(0,360),(None,None,seeded.uniform(.35,.8)),-.04,"Wayside",False)
    for i in range(64):
        x=seeded.uniform(-203,180)
        y=seeded.uniform(310,390) if i%2 else seeded.uniform(-76,-52)
        path="Nature/Trees/SM_Tree_Pine_002" if i%3 else "Nature/Trees/SM_White_Oak_01"
        place("DistantTreeline",path,x,y,seeded.uniform(0,360),(None,None,seeded.uniform(10,19)),-.1,"Treeline")


def finish_quay():
    by_label={a.get_actor_label():a for a in ACTORS.get_all_level_actors()}
    place("Landing","Structures/Docks/Dock_C_VarC_mdl",-20,10,-24,(13,8,2.1),
          -.45,"WorkAndLanding",actor=by_label.get("Landing"))
    place("MooredBoat","Props/BoatsAndFishing/PolyPixel/MedievalDocks/Models/Boat_Small_mdl",
          -13,3,38,(7.5,3,2.2),.3,"WorkAndLanding",actor=by_label.get("MooredBoat"))
    path=SURFACES+"/M_VelesRiver"
    if LIB.does_asset_exist(path):
        water=unreal.load_asset(path)
    else:
        water=unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_VelesRiver",SURFACES,unreal.Material,unreal.MaterialFactoryNew())
        editor=unreal.MaterialEditingLibrary
        color=editor.create_material_expression(water,unreal.MaterialExpressionConstant3Vector)
        color.set_editor_property("constant",unreal.LinearColor(.025,.125,.14,1))
        editor.connect_material_property(color,"",unreal.MaterialProperty.MP_BASE_COLOR)
        rough=editor.create_material_expression(water,unreal.MaterialExpressionConstant)
        rough.set_editor_property("r",.24)
        editor.connect_material_property(rough,"",unreal.MaterialProperty.MP_ROUGHNESS)
        waves=editor.create_material_expression(water,unreal.MaterialExpressionTextureSample)
        waves.set_editor_property("texture",asset("Textures/Medieval_Environment/T_Medieval_Water_01_N"))
        waves.set_editor_property("sampler_type",unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        flow=editor.create_material_expression(water,unreal.MaterialExpressionPanner)
        flow.set_editor_property("speed_x",.025)
        flow.set_editor_property("speed_y",.013)
        editor.connect_material_expressions(flow,"",waves,"Coordinates")
        editor.connect_material_property(waves,"RGB",unreal.MaterialProperty.MP_NORMAL)
        water.set_editor_property("two_sided",True)
        editor.recompile_material(water)
        assert LIB.save_loaded_asset(water,False)
    river=by_label["VelesRiver"]
    river.static_mesh_component.set_material(0,water)
    river.static_mesh_component.set_editor_property("cast_shadow",False)
    river.set_actor_location(unreal.Vector(0,0,30),False,False)


def build():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if LIB.does_asset_exist(MAP) or levels.is_in_play_in_editor():
        raise RuntimeError("A new level path and an idle editor are required.")
    if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages() or unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages():
        raise RuntimeError("Save existing editor changes before switching levels.")
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    vertices = [(x,y,height(x,y)) for y in range(-120,521,4) for x in range(-300,401,4)]
    faces = []
    for row in range(160):
        for col in range(175):
            i = row*176+col
            faces.extend([(i,i+176,i+1),(i+1,i+176,i+177)])
    surface("VelesTerrain", vertices, faces, "Materials/GothicTemple/MI_GrassGround_01", True)
    roads = [(r["points"],r["width"]) for r in LAYOUT["roads"] if r["id"] in ("R01","V-WATER") or r["id"].startswith("A")]
    roads.append(([ROUTE[1], [-48,80], [-80,80]],3))
    verts, tris = strips(roads, lambda x,y,px,py: height(px,py)+.065)
    surface("VelesPaths",verts,tris,"Materials/GothicTemple/MI_SoilGround_01",False)
    river = [[-300,-150],[0,0],[180,160],[500,600]]
    verts,tris = strips([(river,32)],lambda x,y,px,py: height(x,y)+.24)
    surface("VelesRiver",verts,tris,"Materials/Medieval_Environment/MI_Water_River_01",False)
    houses = ["Buildings/Other/SM_Building_01","Buildings/Houses/SM_Hovel","Buildings/Other/SM_Building_02"]
    clearings = []
    for i,spec in enumerate(LAYOUT["actors"]):
        if not spec["label"].startswith("VB_Building_"):
            continue
        x,y = [n/100 for n in spec["location"][:2]]
        if not (-150<x<130 and 15<y<285):
            continue
        w,d,h = spec["scale"]
        label = spec["label"][12:]
        path = houses[i % len(houses)]
        if label == "workshop":
            path = houses[0]
        place(label,path,x,y,spec["yaw"],(w,d,h*1.35),-.12)
        clearings.append((x,y,max(w,d)*.65))
    props = [
        ("Anvil","Props/WorkshopTools/SM_BlacksmithAnvilA",56,114,15,(2,2,1.1)),
        ("Handcart","Props/CartsAndWagons/SM_cart_01",65,113,-18,(3,2,2)),
        ("WorkshopBarrel","Props/Containers/Fishermans_Cabin/SM_Barrel_01",63,115,15,(1,1,1.3)),
        ("FishingGear","Props/BoatsAndFishing/SM_Fishing_Gear_03",-23,85,-12,(3,2,2)),
        ("WaterBarrel","Props/Containers/Fishermans_Cabin/SM_Barrel_01",-24,86,12,(1,1,1.3)),
    ]
    for label,path,x,y,yaw,size in props:
        place(label,path,x,y,yaw,size,folder="WorkAndLanding")
    # 나루 길 끝에서 물가까지 이어지는 목재 상판.
    quay = [(-46,20),(-20,10)]
    verts,tris = strips([(quay,3.2)],lambda x,y,px,py: height(x,y)+.16)
    surface("QuayBoardwalk",verts,tris,"Materials/OldVillage/MI_Wood_Planks_001_floor",True)
    for side in (-1,1):
        place("ForestGatePost","BuildingParts/BeamsAndPillars/SM_WoodenPost",90+side*4.6,260-side*2.8,
              57,(.9,.9,4.5),folder="ForestGate")
        place("ForestFence","Structures/FencesAndRailings/SM_FenceModule_03",90+side*7.2,260-side*4.6,
              -31,(5,5,1.7),folder="ForestGate")
    trees = ["Nature/Trees/SM_Fir_Tree_02","Nature/Trees/SM_Tree_Pine_002","Nature/Trees/SM_White_Oak_01"]
    for i in range(260):
        t = RNG.uniform(0,1.6)
        side = RNG.choice((-1,1))*RNG.uniform(6,100)
        x,y = 42+48*t+side*.84,187+74*t-side*.54
        if any(math.hypot(x-a,y-b)<r for a,b,r in clearings):
            continue
        small = i % 3 == 0
        path = "Nature/GrassAndFlowers/SM_Fern_01" if small else trees[i%len(trees)]
        if i%7 == 0:
            path = "Nature/Shrubs/SM_Shrub_Pine_001"
            small = True
        h = RNG.uniform(.6,1.5) if small else RNG.uniform(8,19)
        place("ForestUnderstory" if small else "ForestTree",path,x,y,RNG.uniform(0,360),(None,None,h),-.09,"Forest",not small)
    start_xy = ROUTE[0]
    start_cm = unreal.Vector(start_xy[0]*100,start_xy[1]*100,(height(*start_xy)+1.2)*100)
    start = ACTORS.spawn_actor_from_class(unreal.PlayerStart,start_cm,unreal.Rotator(yaw=194))
    start.set_actor_label("WorkshopStart")
    sun = ACTORS.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,10000),unreal.Rotator(pitch=-35,yaw=-48))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property("intensity",8)
    sun.light_component.set_editor_property("atmosphere_sun_light",True)
    sky = ACTORS.spawn_actor_from_class(unreal.SkyLight,unreal.Vector())
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture",True)
    ACTORS.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
    fog = ACTORS.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,-500))
    fog.get_component_by_class(unreal.ExponentialHeightFogComponent).set_editor_property("fog_density",.012)
    configure_player(world)
    dress_yards()
    finish_quay()
    EDITOR.set_level_viewport_camera_info(unreal.Vector(4850,10200,540),unreal.Rotator(pitch=-9,yaw=64))
    count = len(ACTORS.get_all_level_actors())
    assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP)
    assert levels.load_level(MAP)
    assert len(ACTORS.get_all_level_actors()) == count
    unreal.log(f"VELES_REBUILD SAVED {count} actors, {len(clearings)} buildings")


if __name__ == "__main__":
    build()
