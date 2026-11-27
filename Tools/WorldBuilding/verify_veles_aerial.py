"""Reopen the aerial level, capture it, test openings, and walk the full route in PIE."""
import datetime
import json
import math
from pathlib import Path
import runpy
import time
import traceback

import unreal


root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
scripts=root/"Tools/WorldBuilding"
art=runpy.run_path(str(scripts/"build_veles_aerial.py"))
editor,levels=art["editor"],art["levels"]
layout=art["inspection_layout"]()
out=art["OUT"]/datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
out.mkdir(parents=True)
(out/"revision.json").write_text(json.dumps(layout,indent=2))
route=layout["route"]


def inspect_spring_and_sills(world,all_actors):
    if not art["footprints"]:art["plan"]()
    by_label={a.get_actor_label():a for a in all_actors}
    assert all(by_label[h["label"]].static_mesh_component.static_mesh.get_name().endswith("_PlainSill")
               for h in layout["houses"])
    ground=by_label["Vale and village paths"]
    assert ground.static_mesh_component.static_mesh.get_name()=="SM_Terrain_SpringBed"
    assert ground.get_component_by_class(unreal.WaterTerrainComponent)
    assert isinstance(by_label["Veles main river"],unreal.WaterBodyRiver)
    brook=by_label["Veles village brook"]
    assert isinstance(brook,unreal.WaterBodyRiver)
    spline=brook.get_water_spline()
    terrain=unreal.DynamicMesh()
    unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(ground.static_mesh_component.static_mesh,
            terrain,unreal.GeometryScriptCopyMeshFromAssetOptions(),unreal.GeometryScriptMeshReadLOD())
    spatial=unreal.GeometryScript_MeshSpatial
    _,tree=spatial.build_bvh_for_mesh(terrain)
    depths=[]
    for y,(x,z,width) in art["spring_sections"]().items():
        if not y.is_integer() or y<14:continue
        _,hit,_=spatial.find_nearest_ray_intersection_with_mesh(terrain,tree,unreal.Vector(x*100,y*100,(z+2)*100),
                unreal.Vector(0,0,-1),unreal.GeometryScriptSpatialQueryOptions())
        assert hit.hit,(x,y)
        pos=unreal.Vector(x*100,y*100,z*100)
        surface=spline.find_location_closest_to_world_location(pos,unreal.SplineCoordinateSpace.WORLD)
        assert (surface-pos).length()<4,(x,y,surface,pos)
        depths.append(surface.z-hit.hit_position.z)
    (out/"spring.json").write_text(json.dumps(dict(samples=len(depths),min_depth_cm=min(depths),
            max_depth_cm=max(depths),shells_without_steps=len(layout["houses"])),indent=2))
    assert min(depths)>5,min(depths)


class Check:
    def __init__(self):
        self.phase="load"
        self.frame=0
        self.index=0
        self.travel=0
        self.falls=0
        self.overlaps=0
        self.last_probe=0
        self.started=time.monotonic()
        self.handle=unreal.register_slate_post_tick_callback(self.tick)

    def finish(self,passed,reason):
        unreal.unregister_slate_post_tick_callback(self.handle)
        if levels.is_in_play_in_editor():levels.editor_request_end_play()
        result=dict(status="PASS" if passed else "FAIL",reason=reason,phase=self.phase,
            waypoints=self.index,total_waypoints=len(route),distance_m=round(self.travel,2),
            fall_samples=self.falls,camera_overlap_samples=self.overlaps)
        (out/"route.json").write_text(json.dumps(result,indent=2))
        art["review_camera"]()
        unreal.log("VELES_AERIAL_REVIEW "+str(out)+" "+str(result))

    def tick(self,dt):
        try:
            self.frame+=1
            now=time.monotonic()
            if self.phase=="load":
                if levels.is_in_play_in_editor():raise RuntimeError("Finish current PIE before running")
                current=editor.get_editor_world().get_path_name().split(".")[0]
                if current!=layout["map"]:
                    if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages():raise RuntimeError("Current map has unsaved edits")
                    assert levels.load_level(layout["map"])
                self.world=editor.get_editor_world()
                all_actors=art["actors"].get_all_level_actors()
                inspect_spring_and_sills(self.world,all_actors)
                names={a.get_actor_label() for a in all_actors}
                assert all(h["label"] in names for h in layout["houses"])
                assert isinstance(next(a for a in all_actors if a.get_actor_label()=="Repair courtyard start"),unreal.PlayerStart)
                (out/"reload.json").write_text(json.dumps(dict(map=self.world.get_path_name(),actors=len(all_actors),
                    houses=len(layout["houses"]),gamemode=self.world.get_world_settings().get_editor_property("default_game_mode").get_name())))
                art["review_camera"]()
                self.phase="aerial";self.frame=0
            elif self.phase=="aerial":
                if self.frame==28:
                    self.shot=unreal.AutomationLibrary.take_high_res_screenshot(1600,900,str(out/"aerial.png"))
                if self.frame>55 and (out/"aerial.png").exists():
                    self.phase="openings"
                    runpy.run_path(str(scripts/"verify_veles_settlement.py"),init_globals=dict(evidence_dir=out,
                        photo_floor=art["bed"],jump_to_sill=True,photo_views=[("workshop",-49,23,119),
                        ("spring",-13,32,68),("common",10,79,130),("fields",62,36,49)]))
                elif now-self.started>150:raise RuntimeError("Aerial capture did not complete")
            elif self.phase=="openings":
                if (out/"opening-check.json").exists() and not levels.is_in_play_in_editor():
                    levels.editor_request_begin_play()
                    self.phase="spawn"
            elif self.phase=="spawn":
                self.world=editor.get_game_world()
                if not self.world:return
                self.p=unreal.GameplayStatics.get_player_pawn(self.world,0)
                if not self.p:return
                self.ctrl=unreal.GameplayStatics.get_player_controller(self.world,0)
                self.ei=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                    if s.get_outer().get_class().get_name()=="CCLLocalPlayer")
                self.move=next(a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==self.ctrl and
                    a.value_type==unreal.InputActionValueType.AXIS2D and a.accumulation_behavior==unreal.InputActionAccumulationBehavior.CUMULATIVE)
                self.prev=self.progress_pos=self.p.get_actor_location()
                self.progress=now;self.phase="walk";self.frame=0
            elif self.phase=="walk":
                pos=self.p.get_actor_location()
                self.travel+=math.dist((pos.x,pos.y,pos.z),(self.prev.x,self.prev.y,self.prev.z))/100
                self.prev=pos
                x,y=route[self.index];dx,dy=x*100-pos.x,y*100-pos.y;d=math.hypot(dx,dy)
                if d<75:
                    self.index+=1
                    self.progress=now
                    if self.index==len(route):
                        openings=json.loads((out/"opening-check.json").read_text())
                        self.finish(not self.falls and not self.overlaps and openings["status"]=="PASS",
                                "Completed round trip; opening checks "+openings["status"])
                    return
                yaw=math.atan2(dy,dx)
                self.ctrl.set_control_rotation(unreal.Rotator(yaw=math.degrees(yaw),pitch=-9))
                self.ei.inject_input_vector_for_action(self.move,unreal.Vector(0,1,0),[],[])
                if (pos-self.progress_pos).length()>35:
                    self.progress_pos=pos;self.progress=now
                if now-self.last_probe>.5:
                    cam=self.p.get_component_by_class(unreal.CameraComponent).get_world_location()
                    hit=unreal.SystemLibrary.sphere_trace_single(self.world,cam,cam+unreal.Vector(0,0,1),5,
                        unreal.TraceTypeQuery.ECC_CAMERA,False,[self.p],unreal.DrawDebugTrace.NONE)
                    self.overlaps+=int(bool(hit));self.falls+=int(self.p.character_movement.is_falling() and self.frame>20)
                    (out/"progress.json").write_text(json.dumps(dict(waypoint=self.index,x=pos.x/100,y=pos.y/100,
                        z=pos.z/100,camera_hit=hit.to_tuple()[9].get_actor_label() if hit else None)))
                    self.last_probe=now
                if now-self.progress>18:raise RuntimeError("Stalled at waypoint "+str(self.index))
        except Exception:
            self.finish(False,traceback.format_exc())


if globals().get("geometry_only"):
    inspect_spring_and_sills(editor.get_editor_world(),art["actors"].get_all_level_actors())
else:
    run=Check()
