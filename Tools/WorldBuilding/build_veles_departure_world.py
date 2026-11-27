"""Assemble three saved StoryRegions into a separate World Partition map in a full editor."""
from pathlib import Path
import runpy
import traceback

import unreal

here = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))/"Tools/WorldBuilding"
land = runpy.run_path(str(here/"veles_departure_world_land.py"))
base, art, forest = (land[k] for k in ("BASE","ART","FOREST"))
root, target = land["ROOT"], land["MAP"]
lib = unreal.EditorAssetLibrary
actors = base["actors"]
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
vec = base["vec"]
space = unreal.SplineCoordinateSpace.WORLD
skip = {"Vale and village paths","V00 Valley ridgelines","F00 Forest watershed","F00 Outer forest slopes",
        "F06 Recessed spring adit","C00 Cut rock strata","C00 Source cavern bedrock",
        "C00 Root hill geological cap","C00 Oak above the spring","Review Former section"}


def read_instances():
    vegetation=[]
    rivers=[]
    for i,path in enumerate(base["maps"][:2]):
        world=base["levels"].load_map(path)
        for holder in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.InstancedFoliageActor):
            for comp in holder.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
                if not comp.static_mesh or not comp.get_instance_count():
                    continue
                name=comp.static_mesh.get_name()
                paths=([base["package"]+"/Geometry/FT_Valley_"+name,"/Game/Level/VelesNorthWoodAssets/FT_"+name] if i
                       else ["/Game/Level/VelesRiversideAssets/FT_"+name])
                ft=next((p for p in paths if lib.does_asset_exist(p)),None)
                if not ft:
                    candidates=[f for f in unreal.ObjectIterator(unreal.FoliageType_InstancedStaticMesh)
                                if f.get_editor_property("mesh")==comp.static_mesh and not f.get_name().startswith("Default__")]
                    assert candidates,(i,name)
                    asset=next((f for f in candidates if ":" not in f.get_path_name()),None)
                    if not asset:
                        asset=unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset("FT_"+name,root+"/Geometry",candidates[0])
                        assert asset and lib.save_loaded_asset(asset,False)
                    ft=asset.get_path_name()
                assert ft,(i,name)
                rows=[]
                for k in range(comp.get_instance_count()):
                    t=comp.get_instance_transform(k,True)
                    pos=t.translation
                    x,y=pos.x/100,pos.y/100
                    if (i==0 and y>=205) or (i==1 and y<205):
                        continue
                    if y<232 and y>6 and any(abs(x-land["stream"](y,d))<land["channel_width"](y,d)+.1 for d in (False,True)):
                        continue
                    if y>1246 and y<1262 and abs(x-272)<7:
                        continue
                    old=(forest["floor"] if i else art["bed"])(x,y)
                    t.translation=pos+unreal.Vector(0,0,(land["surface_height"](x,y)-old)*100)
                    rows.append(t)
                if rows:
                    vegetation.append((ft,rows))
        if not i:
            for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.WaterBodyRiver):
                s=a.get_water_spline(); c=a.get_water_body_component()
                points=[(s.get_location_at_spline_point(j,space),c.get_river_width_at_spline_input_key(j),
                         c.get_river_depth_at_spline_input_key(j),c.get_water_velocity_at_spline_input_key(j))
                        for j in range(s.get_number_of_spline_points())]
                rivers.append((a.get_actor_label(),a.get_class().get_path_name(),points))
    return vegetation,rivers


def copy_region(world, i):
    path=base["maps"][i]
    src=unreal.load_object(None,path+"."+path.rsplit("/",1)[1])
    prefix=src.get_path_name()+":PersistentLevel."
    types=(unreal.StaticMeshActor,unreal.SkeletalMeshActor,unreal.PointLight,unreal.CameraActor)
    if not i:
        types+=(unreal.DirectionalLight,unreal.SkyLight,unreal.SkyAtmosphere,unreal.ExponentialHeightFog)
    items=[a for a in unreal.ObjectIterator(unreal.Actor) if a.get_path_name().startswith(prefix)
           and isinstance(a,types) and a.get_actor_label() not in skip
           and not a.get_actor_label().startswith("C00 Exterior skylight")
           and not (i==1 and a.get_actor_label().endswith(" surface"))]
    for start in range(0,len(items),100):
        batch=items[start:start+100]
        copies=actors.duplicate_actors(batch,world)
        assert len(copies)==len(batch),(path,start,len(copies),len(batch))
        for a in copies:
            a.set_folder_path("Departure/"+("Village","GoldenMistForest","FrostrootCave")[i])
            if i==2:
                p=a.get_actor_location()
                rotation=a.get_actor_rotation()
                rotation.yaw+=90
                a.set_actor_location_and_rotation(vec(land["cave_point"](p.x/100,p.y/100,p.z/100)),rotation,False,True)
    unreal.log(f"DEPARTURE_REGION {path} {len(items)}")


def make_water(world,rivers):
    zone=actors.spawn_actor_from_class(unreal.WaterZone,vec((260,640,40)))
    zone.set_editor_property("zone_extent",unreal.Vector2D(188000,189000))
    zone.set_editor_property("is_spatially_loaded",False)
    zone.get_component_by_class(unreal.WaterMeshComponent).set_visibility(False)
    zone.get_component_by_class(unreal.WaterMeshComponent).set_hidden_in_game(True)
    mask=unreal.load_asset(base["package"]+"/Geometry/M_WaterQuerySurface")
    draw=runpy.run_path(str(here/"veles_water.py"))["draw_surface"]
    for deep in (False,True):
        points=[]
        for y in range(1320 if deep else 1248,-1,-2):
            x=land["stream"](y,deep); z=land["water_z"](y,deep)
            slope=(land["stream"](y+.1,deep)-land["stream"](y-.1,deep))/.2
            w=200*land["channel_width"](y,deep)/(1+slope*slope)**.5
            points.append((vec((x,y,z)),w,220 if deep else 64,80 if deep else 54))
        rivers.append(("Eastern river" if deep else "Spring to Veles river",
                       "/Script/Water.WaterBodyRiver" if deep else "/Game/Level/VelesRiversideAssets/BP_VelesBrook.BP_VelesBrook_C",points))
    for index,(label,cls,points) in enumerate(rivers):
        a=actors.spawn_actor_from_class(unreal.load_class(None,cls),points[0][0])
        a.set_actor_label(label);a.set_folder_path("Departure/Water")
        a.set_editor_property("is_spatially_loaded",False)
        comp=a.get_water_body_component()
        comp.set_editor_property("affects_landscape",False)
        comp.set_water_zone_override(zone);comp.set_water_material(mask)
        comp.set_water_info_material(unreal.load_asset("/Water/Materials/WaterInfo/DrawWaterInfo"))
        s=a.get_water_spline()
        s.set_spline_points([row[0] for row in points],space,True)
        for j,(_,w,d,v) in enumerate(points):
            comp.set_river_width_at_spline_input_key(j,w)
            comp.set_river_depth_at_spline_input_key(j,d)
            comp.set_water_velocity_at_spline_input_key(j,v)
        s.call_method("K2_SynchronizeAndBroadcastDataChange")
        for c in comp.get_standard_renderable_components():
            c.set_material(0,mask)
        if index>=2:
            draw(art,a,unreal.load_asset(base["watermat"]),"DepartureRiver"+str(index),index==3)


def collar():
    import math
    arch=art["group"]()
    for i in range(8):
        ends=[]
        for d in (i,i+1):
            t=d/8
            cx,cy,cz=land["cave_point"](-12+d,land["CAVE"]["center"](-12+d),land["CAVE"]["base"](-12+d))
            row=[(cx+(1.46+2.24*t)*math.cos(math.pi*j/20),cy,
                  cz-.16+1.62*(1-t)+(1.08+2.98*t)*math.sin(math.pi*j/20)) for j in range(21)]
            ends.append([(row[0][0],cy,cz-.4)]+row+[(row[-1][0],cy,cz-.4)])
        for j in range(len(ends[0])-1):
            face=[ends[0][j],ends[0][j+1],ends[1][j+1],ends[1][j]]
            art["quad"](arch,face);art["quad"](arch,face[::-1])
    base["shape"]("Open spring intake throat","OpenIntake",[arch],[base["package"]+"/Geometry/M_CaveWetStrata"])


def add_editor_region(world):
    import veles_departure_editor as preview

    assert world.get_package().get_name()==preview.MAP
    existing=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.LocationVolume)
    region=next((v for v in existing if v.get_actor_label()==preview.VOLUME),None)
    if region is None:
        mesh=unreal.DynamicMesh()
        opts=unreal.GeometryScriptPrimitiveOptions()
        mesh.append_box(opts,unreal.Transform(),185000,202000,100000,origin=unreal.GeometryScriptPrimitiveOriginMode.CENTER)
        transform=unreal.Transform()
        transform.translation=unreal.Vector(27500,46000,30000)
        options=unreal.GeometryScriptCreateNewVolumeFromMeshOptions()
        options.volume_type=unreal.LocationVolume
        region,_=unreal.GeometryScript_NewAssetUtils.create_new_volume_from_mesh_world_context(world,mesh,transform,preview.VOLUME,options)
        assert region and isinstance(region,unreal.LocationVolume)
        region.set_actor_label(preview.VOLUME)
        region.set_folder_path("Departure/Editor")
    p,e=region.get_actor_bounds(False)
    assert (p-unreal.Vector(27500,46000,30000)).length()<10 and e.x>=92499 and e.y>=100999,(p,e)
    region.load()
    return region


def build():
    vegetation, rivers=read_instances()
    if lib.does_asset_exist(target):
        assert levels.load_level(target)
        assert not any("Departure ground" in str(d.label) or "FrostrootCave" in str(d.label)
                       or str(d.label).startswith("V01 ") for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()),"Destination already built"
    else:
        assert levels.new_level(target,True)
    world=base["editor"].get_editor_world()
    grid_cls=unreal.load_class(None,"/Script/Engine.RuntimePartitionLHGrid")
    grid=next(o for o in unreal.ObjectIterator(grid_cls) if o.get_path_name().startswith(target+"."))
    grid.set_editor_property("CellSize",12800)
    grid.set_editor_property("LoadingRange",36000)
    for i in range(3):
        copy_region(world,i)
    land["build_ground"]()
    land["build_surroundings"]()
    collar()
    make_water(world,rivers)
    for ft,rows in vegetation:
        unreal.InstancedFoliageActor.add_instances(world,unreal.load_asset(ft),rows)
    base["setup"](world,(-48,53,art["bed"](-48,53)+1.05),8)
    add_editor_region(world)
    eye=vec((-355,-445,665)); aim=vec((60,585,27))
    base["camera"]("Departure overview",(-355,-445,665),(60,585,27),74)
    base["editor"].set_level_viewport_camera_info(eye,unreal.MathLibrary.find_look_at_rotation(eye,aim))
    assert levels.save_current_level()
    dirty=[p for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
           if "/connected/" in p.get_name().lower()]
    if dirty:
        assert unreal.EditorLoadingAndSavingUtils.save_packages(dirty,True)
    runpy.run_path(str(here/"finish_departure_landscape.py"))["finish"](world)
    unreal.log("DEPARTURE_CONNECTED_SAVED "+target)


if __name__=="__main__":
    try:
        build()
    except Exception:
        unreal.log_error(traceback.format_exc())
    finally:
        unreal.SystemLibrary.quit_editor()
