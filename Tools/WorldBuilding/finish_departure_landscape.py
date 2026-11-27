"""Dress the shared valley edge and seat plants on the saved connected terrain triangles."""
from collections import defaultdict
import json
import math
import random
import runpy
from pathlib import Path

import unreal


HERE=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))/"Tools/WorldBuilding"
LAND=runpy.run_path(str(HERE/"veles_departure_world_land.py"))
ART=LAND["ART"]
VEC=unreal.Vector
IFA=unreal.InstancedFoliageActor
HISM=unreal.HierarchicalInstancedStaticMeshComponent


class Terrain:
    def __init__(self, actors):
        self.tiles={tuple(map(int,a.get_actor_label().split()[-2:])):a for a in actors
                    if a.get_actor_label().startswith("Departure ground ")}
        assert len(self.tiles)==144
        self.cache={}
        self.options=unreal.GeometryScriptSpatialQueryOptions()
        self.cap=next(a for a in actors if a.get_actor_label()=="F06 Earth bank above the adit")

    def z(self,x,y):
        # x/y/z는 cm다. 편집 월드의 물리 상태와 무관하게 저장된 지면 삼각형을 찍는다.
        key=(math.floor(x/12800)*128,math.floor(y/12800)*128)
        if key not in self.tiles:
            raise ValueError(("Outside detailed terrain",x,y))
        z=self.hit(self.tiles[key],x,y)
        if 26400<x<27900 and 124900<y<126500:
            cap=self.hit(self.cap,x,y)
            if cap is not None:
                return cap if z is None else max(z,cap)
        assert z is not None,(key,x,y)
        return z

    def hit(self,actor,x,y):
        if actor not in self.cache:
            dm=unreal.DynamicMesh()
            unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(actor.static_mesh_component.static_mesh,
                dm,unreal.GeometryScriptCopyMeshFromAssetOptions(),unreal.GeometryScriptMeshReadLOD())
            _,bvh=unreal.GeometryScript_MeshSpatial.build_bvh_for_mesh(dm)
            self.cache[actor]=(dm,bvh,actor.get_actor_location())
        dm,bvh,p=self.cache[actor]
        _,hit,_=unreal.GeometryScript_MeshSpatial.find_nearest_ray_intersection_with_mesh(
            dm,bvh,VEC(x-p.x,y-p.y,90000),VEC(0,0,-1),self.options)
        return hit.hit_position.z+p.z if hit.hit else None


def tree_type(mesh,end):
    name=mesh.get_name()
    if end==310000:
        path="/Game/Level/VelesDeparture/StoryRegions/Geometry/FT_Valley_"+name
    elif end==0:
        path="/Game/Level/VelesRiversideAssets/FT_"+name
    else:
        raise ValueError(("Unknown tree foliage type",name,end))
    ft=unreal.load_asset(path)
    assert ft and ft.get_editor_property("mesh")==mesh and ft.get_editor_property("cull_distance").max==end
    return ft


def plant_base(terrain,p):
    # 전나무 뿌리 메시 일부는 줄기 원점보다 수 m 아래로 뻗는다. 줄기가 흙을 만나는 원점을 사용한다.
    return min(terrain.z(p.x+dx,p.y+dy) for dx,dy in ((0,0),(24,0),(-24,0),(0,24),(0,-24)))-9


def trunk_position(terrain,mesh,p):
    # 서쪽 원목길 한복판의 소나무는 굵은 밑동 충돌까지 길 바깥에 두도록 둔덕 쪽으로 옮긴다.
    x=p.x
    if mesh.get_name()=="SM_Tree_Pine_002" and abs(x+9247)<15 and abs(p.y-24008)<15:
        x-=950
    point=VEC(x,p.y,0)
    point.z=plant_base(terrain,point)
    return point


def seat_trees(world,actors,terrain,apply):
    gaps=[]; batches=defaultdict(list)
    for actor in actors:
        if isinstance(actor,IFA):
            for comp in actor.get_components_by_class(HISM):
                mesh=comp.static_mesh
                if not mesh or "/Trees/" not in mesh.get_path_name() or not comp.get_instance_count():
                    continue
                end=comp.get_editor_property("instance_end_cull_distance")
                if end==48000: # 이 맵의 숲 가장자리 묘목
                    ft=None
                else:
                    ft=tree_type(mesh,end)
                for i in range(comp.get_instance_count()):
                    t=comp.get_instance_transform(i,True)
                    pos=t.translation
                    z=plant_base(terrain,pos)
                    gaps.append(pos.z-terrain.z(pos.x,pos.y))
                    if ft and apply:
                        t.translation=VEC(pos.x,pos.y,z)
                        batches[ft].append(t)
        elif isinstance(actor,unreal.StaticMeshActor):
            c=actor.static_mesh_component
            if not c.static_mesh or "/Trees/" not in c.static_mesh.get_path_name():
                continue
            p=c.get_world_transform().translation
            gaps.append(p.z-terrain.z(p.x,p.y))
            if apply:
                target=trunk_position(terrain,c.static_mesh,p)
                if (target-p).length()>.05:
                    actor.modify()
                    actor.set_actor_location(actor.get_actor_location()+(target-p),False,True)
    if apply:
        for ft,rows in batches.items():
            IFA.remove_all_instances(world,ft)
            IFA.add_instances(world,ft,rows)
    return dict(total=len(gaps),over_20cm=sum(g>20 for g in gaps),max_cm=round(max(gaps),2),
                lowest_cm=round(min(gaps),2))


def inside_yard(x,y):
    for h in ART["footprints"]:
        if abs(h["x"]-x)<30 and abs(h["y"]-y)<30:
            u,v=ART["kit"]["local"](h,x,y)
            if abs(u)<h["w"]/2+4 and abs(v)<h["d"]/2+4:return True
    return 54<x<127 and 32<y<103


def seat_understory(world,actors,terrain,apply):
    instances=defaultdict(list)
    changed=set()
    gaps=[]
    groups={}
    if apply:
        registry=unreal.AssetRegistryHelpers.get_asset_registry()
        options=unreal.AssetRegistryDependencyOptions(include_hard_package_references=True)
        for desc in unreal.WorldPartitionBlueprintLibrary.get_actor_descs():
            if not str(desc.label).startswith("InstancedFoliageActor"):continue
            types={}
            for path in registry.get_dependencies(desc.actor_package,options) or []:
                assets=registry.get_assets_by_package_name(path) or []
                for asset in assets:
                    if str(asset.asset_class_path.asset_name)!="FoliageType_InstancedStaticMesh":
                        continue
                    ft=asset.get_asset()
                    key=(ft.get_editor_property("mesh"),ft.get_editor_property("cull_distance").max)
                    assert key not in types,(path,key)
                    types[key]=ft
            groups[str(desc.label)]=types
    for a in actors:
        if not isinstance(a,IFA):continue
        for c in a.get_components_by_class(HISM):
            m=c.static_mesh
            if not m or "/Trees/" in m.get_path_name() or not c.get_instance_count():continue
            key=(m,c.get_editor_property("instance_end_cull_distance"))
            ft=groups[a.get_actor_label()][key] if apply else None
            bottom=m.get_bounding_box().min.z
            for i in range(c.get_instance_count()):
                t=c.get_instance_transform(i,True)
                p=t.translation
                gap=p.z+bottom*t.scale3d.z-terrain.z(p.x,p.y)
                gaps.append(gap)
                if apply:
                    if gap>2:
                        t.translation=VEC(p.x,p.y,p.z-gap-4)
                        changed.add(ft)
                    instances[ft].append(t)
    for ft in changed:
        IFA.remove_all_instances(world,ft)
        IFA.add_instances(world,ft,instances[ft])
    return dict(total=len(gaps),above_2cm=sum(g>2 for g in gaps),max_cm=round(max(gaps),2))


def plant_edges(world,terrain):
    rng=random.Random(2608)
    groups=defaultdict(list)
    margin=ART["kit"]["margin"]
    def clump(x,y,tree=False):
        for i in range(rng.randint(5,13)):
            xx=x+rng.gauss(0,2.4); yy=y+rng.gauss(0,2.0)
            edge=margin(xx,yy)
            distance=min(abs(xx-LAND["stream"](yy,deep))-LAND["channel_width"](yy,deep) for deep in (False,True))
            if edge<.8 or distance<.20 or inside_yard(xx,yy):continue
            name=rng.choice(("SM_Fern_01","SM_GrassClump_03","SM_GrassClump_03","SM_BushWithoutSnow_02"))
            if tree and i==0 and edge>3.3 and distance>2.7:name="SM_White_Oak_Young_01"
            if distance<1.1:name="SM_GrassClump_03"
            groups[name].append((xx,yy))
    for line in [ART["forest"],*LAND["FOREST"]["routes"][:2]]:
        for (ax,ay),(bx,by) in zip(line,line[1:]):
            length=math.hypot(bx-ax,by-ay)
            for i in range(max(1,int(length/8))):
                t=rng.random();y=ay+(by-ay)*t
                if not 117<y<375:continue
                off=rng.choice((-1,1))*rng.uniform(4.2,10)
                clump(ax+(bx-ax)*t-off*(by-ay)/length,y+off*(bx-ax)/length,tree=y>150)
    for deep in (False,True):
        for sign in (-1,1):
            for j in range(10,430,4):
                y=j+rng.uniform(-2,2)
                if rng.random()<.30:continue
                x=LAND["stream"](y,deep)+sign*(LAND["channel_width"](y,deep)+rng.uniform(1,4.8))
                clump(x,y,tree=y>120 and j%5==0)
    for j in range(200):
        y=rng.uniform(150,305);x=rng.uniform(-190,150)
        if math.sin(x*.051+y*.037)+math.sin(y*.073-x*.034)>.3:
            clump(x,y,tree=True)
    for name,spots in groups.items():
        folder=LAND["ROOT"]+"/Geometry"
        path=folder+"/FT_Verge_"+name
        lib=unreal.EditorAssetLibrary
        ft=unreal.load_asset(path) if lib.does_asset_exist(path) else lib.duplicate_asset("/Game/Level/VelesNorthWoodAssets/FT_"+name,path)
        assert ft
        is_tree="Oak" in name
        ft.set_editor_property("cull_distance",unreal.Int32Interval(36000,48000) if is_tree else unreal.Int32Interval(9000,15500))
        assert lib.save_loaded_asset(ft,False)
        bb=ft.get_editor_property("mesh").get_bounding_box();rows=[]
        for x,y in spots:
            height=rng.uniform(2.7,5.1) if is_tree else rng.uniform(.4,1.15) if "Bush" in name else rng.uniform(.27,.76)
            sc=100*height/(bb.max.z-bb.min.z)
            p=VEC(x*100,y*100,0)
            p.z=plant_base(terrain,p) if is_tree else terrain.z(p.x,p.y)-bb.min.z*sc-3
            rows.append(unreal.Transform(location=p,rotation=unreal.Rotator(yaw=rng.uniform(0,360)),scale=VEC(sc,sc,sc)))
        IFA.remove_all_instances(world,ft)
        IFA.add_instances(world,ft,rows)
    return {name:len(spots) for name,spots in groups.items()}


def finish(world):
    assert world.get_package().get_name()==LAND["MAP"]
    api=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors=api.get_all_level_actors()
    terrain=Terrain(actors)
    out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/"Tests/DepartureLandscape"
    out.mkdir(exist_ok=True)
    mat=runpy.run_path(str(HERE/"veles_departure_ground_material.py"))["make_material"](LAND)
    for a in actors:
        label=a.get_actor_label()
        if label.startswith("Departure ground ") or label in ("Departure surrounding ridges","Departure far terrain"):
            a.modify();a.static_mesh_component.modify()
            if label=="Departure far terrain":
                a.static_mesh_component.set_static_mesh(LAND["far_terrain"]())
            a.static_mesh_component.set_material(0,mat)
        elif label=="F00 Worn forest tracks":
            a.modify();a.static_mesh_component.modify()
            a.static_mesh_component.set_visibility(False);a.set_actor_hidden_in_game(True)
        elif isinstance(a,IFA):
            a.modify()
    before=seat_trees(world,actors,terrain,True)
    low=seat_understory(world,api.get_all_level_actors(),terrain,True)
    extra=plant_edges(world,terrain)
    after=seat_trees(world,api.get_all_level_actors(),terrain,False)
    assert after["max_cm"]<=0 and after["total"]>=before["total"],after
    dirty=unreal.EditorLoadingAndSavingUtils
    packages=list(dirty.get_dirty_map_packages())+list(dirty.get_dirty_content_packages())
    mine=[p for p in packages if "/velesdeparture/connected/" in p.get_name().lower()]
    assert mine and dirty.save_packages(mine,True)
    report=dict(before=before,after=after,edge_plantings=extra,understory_before=low,packages=len(mine))
    (out/"author.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
    unreal.log("DEPARTURE_LANDSCAPE_SAVED "+str(report))
