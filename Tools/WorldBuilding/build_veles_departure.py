"""Playable sets for the sixteen Veles departure plates. Distances are metres."""
from pathlib import Path
import math
import runpy

import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
HERE = ROOT / "Tools/WorldBuilding"
art = runpy.run_path(str(HERE / "build_veles_aerial.py"))
actors = art["actors"]
editor = art["editor"]
levels = unreal.EditorLoadingAndSavingUtils
package = "/Game/Level/VelesDeparture/StoryRegions"
maps = [package + "/" + name for name in ("L01_VelesVillage", "L02_GoldenMistForest", "L03_FrostrootCave")]
art["mesh"].__globals__["folder"] = package + "/Geometry"
wood = "Materials/OldVillage/MI_Wood_Planks_003"
stone = "Materials/MedievalEnvironment/MI_GraniteRockAssembly"
bark = "Materials/OldVillage/MI_Bark_Pine_001"
lamp = "Props/Lighting/SM_ManMade_Lantern_01"
bench = "Props/Furniture/SM_OldWoodenTable"
roots = "Nature/BranchesAndStumps/SM_ForestRootsA"
cliff = "Nature/RocksAndCliffs/SM_Cliff_Rock_01"
stump = "Nature/BranchesAndStumps/SM_TreeStumpA_00"
bucket = "Props/Containers/SM_OldWoodenBucketA"
wheel = "Props/CartsAndWagons/SM_WoodenWheelB"
watermat = "/Game/Level/VelesRiversideAssets/M_VelesCurrent"
group, quad, beam, box = [art[n] for n in ("group","quad","line_beam","box")]


def vec(p):
    return unreal.Vector(*(100*v for v in p))


def tag(actor, label):
    actor.set_actor_label(label)
    actor.set_folder_path("Departure/" + label.split(" ")[0].upper())
    return actor


def material(path):
    return unreal.load_asset(path) if path.startswith("/") else art["asset"](path)


def shape(label, name, parts, materials, collision=True, at=(0,0,0)):
    mesh = art["mesh"](name,parts,[material(m) for m in materials],collision)
    actor = art["actor_mesh"](label,mesh,*at,0)
    actor.set_actor_enable_collision(collision)
    return tag(actor,label)


def prop(label,path,x,y,z,*,height=None,length=None,yaw=0,solid=True):
    mesh=material(path)
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,vec((x,y,z)))
    a.static_mesh_component.set_static_mesh(mesh)
    bounds=mesh.get_bounding_box()
    extent=bounds.max-bounds.min
    scale=height*100/extent.z if height else length*100/max(extent.x,extent.y)
    a.set_actor_scale3d(unreal.Vector(scale,scale,scale))
    a.set_actor_rotation(unreal.Rotator(yaw=yaw),False)
    center,half=a.get_actor_bounds(False)
    a.set_actor_location(a.get_actor_location()+vec((x,y,z))-center+unreal.Vector(0,0,half.z),False,False)
    a.set_actor_enable_collision(solid)
    return tag(a,label)


def top(a):
    p,h=a.get_actor_bounds(False)
    return (p.z+h.z)/100


def glow(label,p,tint=(255,175,92,255),power=300,radius=6):
    a=actors.spawn_actor_from_class(unreal.PointLight,vec(p))
    c=a.light_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("intensity_units",unreal.LightUnits.LUMENS)
    c.set_editor_property("intensity",power)
    c.set_light_color(unreal.LinearColor(*(v/255 for v in tint)),False)
    c.set_editor_property("attenuation_radius",100*radius)
    return tag(a,label)


def camera(label,p,look,fov=64):
    direction=unreal.MathLibrary.find_look_at_rotation(vec(p),vec(look))
    a=actors.spawn_actor_from_class(unreal.CameraActor,vec(p),direction)
    a.camera_component.set_editor_property("field_of_view",fov)
    return tag(a,label)


def setup(world,p,yaw):
    a=actors.spawn_actor_from_class(unreal.PlayerStart,vec(p),unreal.Rotator(yaw=yaw))
    a.set_actor_label("Arrival")
    gm=unreal.load_asset("/Game/Level/VelesSettlementAssets/BP_VillageGameMode")
    world.get_world_settings().set_editor_property("default_game_mode",gm.generated_class())


def tabletop(label,a,x,y,path,h,angle=0):
    return prop(label,path,x,y,top(a),height=h,yaw=angle,solid=False)


def village():
    world=levels.load_map(art["MAP"])
    art["plan"]()
    ground=art["bed"]
    def place(label,path,x,y,**kw):
        return prop(label,path,x,y,ground(x,y),**kw)
    table=place("V01 Repair bench at courtyard",bench,-51,49,height=.92,yaw=-6)
    lantern=tabletop("V01 Repaired ether lantern",table,-51.45,49,lamp,.47)
    glow("V01 Lantern glow",(-51.45,49,top(lantern)-.2))
    tabletop("V01 Work hammer",table,-50.7,49,"Props/WorkshopTools/SM_OldHammer",.12,30)
    place("V01 Replacement wheel",wheel,-52.5,49,height=1.22,yaw=-8)
    place("V01 Chopping block",stump,-53.2,48.2,height=.67)
    # 마른 취수구와 양동이를 물이 흐르는 본류 옆에 따로 둔다.
    basin=group()
    x,y=-9.8,86.0
    z=ground(x,y)
    for j in range(24):
        a,b=math.tau*j/24,math.tau*(j+.92)/24
        p=lambda t:(x+1.43*math.cos(t),y+1.03*math.sin(t),z+.21)
        if j!=4: beam(basin,p(a),p(b),.46,.31)
    shape("V02 Dry stone collection basin","CatchBasin",[basin],[stone])
    place("V02 Empty pail",bucket,x+1.8,y,height=.46)
    place("V02 Wash stone","Nature/RocksAndCliffs/SM_MossyRock",x-1.4,y-.55,length=1.1)
    stand=place("V02 Intake lamp pedestal",stump,x+.9,y+1.35,height=.92)
    l=tabletop("V02 Lit intake lamp",stand,x+.9,y+1.35,lamp,.43)
    glow("V02 Service lamp",(x+.9,y+1.35,top(l)-.12))
    for j in range(2):
        tx,ty=-3.7+2.42*j,99
        table=place("V03 Shared supper table "+str(j),bench,tx,ty,length=2.48)
        for k,path in enumerate(("Props/Food/SM_Bread_01","Props/Kitchenware/SM_WoodenBowls_03","Props/Kitchenware/SM_Wooden_cup_1")):
            tabletop("V03 Evening place setting",table,tx+(k-1)*.61,ty+.09*(-1)**k,path,(.17,.14,.13)[k],k*32)
        for dy in (-1,1):
            place("V03 Long supper bench","Props/Furniture/SM_OldWoodenBench",tx,ty+dy,length=2.2)
    place("V03 Supper hearth","Structures/FirePits/SM_Fire_Pit",-.1,101.3,length=1.2)
    glow("V03 Embers",(-.1,101.3,ground(-.1,101.3)+.38),power=720)
    # 나룻배와 접하는 잔교 끝의 뱃짐은 승선로 옆에 모은다.
    for i in range(3):
        prop("V04 Cargo bale","Props/TradeGoods/SM_Bale",14.0+(i%2)*.85,8.5+i//2,.66,height=.53,yaw=12*i)
    prop("V04 Mooring line","Props/BoatsAndFishing/SM_Knot_Connection_Boat",16,7.8,.68,length=1.2,solid=False)
    c=camera("V00 Village across the river",(54,-84,67),(-12,92,5),67)
    for code,eye,look in (
        ("V01 Workshop",(-49,52,1.75),(-51.8,48.7,1.05)),
        ("V02 Water point",(-6.3,89.5,1.7),(-9.8,86,.47)),
        ("V03 Oak supper",(-8,102,1.7),(-1.9,98.4,1.0)),
        ("V04 Ferry",(12.3,15.3,2.2),(21,5,1.2))):
        camera(code,(eye[0],eye[1],ground(*eye[:2])+eye[2]),(look[0],look[1],ground(*look[:2])+look[2]))
    editor.set_level_viewport_camera_info(c.get_actor_location(),c.get_actor_rotation())
    return world


def build(index):
    if unreal.EditorAssetLibrary.does_asset_exist(maps[index]):
        raise RuntimeError("Destination already exists: "+maps[index])
    if index==0:
        world=village()
    else:
        source=HERE/("veles_departure_forest.py" if index==1 else "veles_departure_cave.py")
        world=runpy.run_path(str(source),init_globals=globals())["build_region"]()
    finish=runpy.run_path(str(HERE/"veles_departure_finishes.py"),init_globals=globals())
    finish["finish"](index,world)
    assert levels.save_map(world,maps[index])
    unreal.log("DEPARTURE_SAVED "+maps[index])


if __name__=="__main__":
    import os
    build(int(os.environ["CCL_DEPARTURE_REGION"])-1)
