"""Reopen VelesNorthWood alone, compare its terrain seam, then walk the forest loop in PIE."""
from pathlib import Path
import datetime
import json
import math
import runpy
import time
import traceback

import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
tile = runpy.run_path(str(root/"Tools/WorldBuilding/build_veles_north_wood.py"))
editor = tile["art"]["editor"]
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out = root/"Saved/Tests/VelesNorthWood"/datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
out.mkdir(parents=True)
route = tile["route"]
result = {"route_m":0, "visited":0,"waypoints":len(route),"falls":0,"camera_obstructions":0}


def terrain_ray(asset):
    dm=unreal.DynamicMesh()
    unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(unreal.load_asset(asset),dm,
        unreal.GeometryScriptCopyMeshFromAssetOptions(),unreal.GeometryScriptMeshReadLOD())
    spatial=unreal.GeometryScript_MeshSpatial
    _,tree=spatial.build_bvh_for_mesh(dm)
    def trace(x,y):
        _,hit,_=spatial.find_nearest_ray_intersection_with_mesh(dm,tree,unreal.Vector(x*100,y*100,20000),
            unreal.Vector(0,0,-1),unreal.GeometryScriptSpatialQueryOptions())
        assert hit.hit,(x,y)
        return hit.hit_position.z
    return trace


class WoodWalk:
    def __init__(self):
        self.phase="load"
        self.start=time.monotonic()
        self.frame=0
        self.h=unreal.register_slate_post_tick_callback(self.tick)

    def finish(self,error=None):
        result["error"]=error
        result["passed"]=error is None
        (out/"walk.json").write_text(json.dumps(result,indent=2),encoding="utf-8")
        unreal.unregister_slate_post_tick_callback(self.h)
        if levels.is_in_play_in_editor():
            levels.editor_request_end_play()
        unreal.log("VELES_NORTH_WALK "+str(out)+" "+str(result))

    def tick(self,dt):
        try:
            now=time.monotonic()
            if now-self.start>420:
                raise RuntimeError("Timed out in "+self.phase)
            self.frame+=1
            if self.phase=="load":
                world=editor.get_editor_world()
                assert world.get_path_name().split(".")[0]==tile["MAP"]
                loaded=unreal.EditorLevelUtils.get_levels(world)
                result["loaded_levels"]=[level.get_path_name().split(".")[0] for level in loaded]
                assert result["loaded_levels"]==[tile["MAP"]],result["loaded_levels"]
                # 서로 다른 두 메시를 경계 바로 양쪽 0.1cm에서 독립적으로 측정한다.
                a=terrain_ray(tile["VILLAGE"]+"/SM_Terrain_SpringBed")
                b=terrain_ray(tile["ASSETS"]+"/SM_NorthWoodGround")
                xs=[-299.87+j*3.99 for j in range(101)]
                seam=[abs(a(x,519.999)-b(x,520.001)) for x in xs]
                result.update(seam_samples=len(xs),seam_max_cm=max(seam))
                assert max(seam)<.5,max(seam)
                tile["overview"]()
                self.phase="overview"
                self.frame=0
            elif self.phase=="overview":
                if self.frame==35:
                    self.shot=unreal.AutomationLibrary.take_high_res_screenshot(1440,900,str(out/"overview.png"))
                if (out/"overview.png").exists():
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
                self.last=self.progress_pos=self.p.get_actor_location()
                assert math.hypot(self.last.x-route[0][0]*100,self.last.y-route[0][1]*100)<150
                self.last_progress=self.last_probe=now
                self.phase="walk"
                self.index=0
            elif self.phase=="pose":
                if self.frame==15:
                    unreal.SystemLibrary.execute_console_command(self.world,'shot filename="'+(out/(self.photo+".png")).as_posix()+'" -nosuffix')
                if (out/(self.photo+".png")).exists():
                    self.phase="walk"
                    self.last_progress=now
            elif self.phase=="walk":
                pos=self.p.get_actor_location()
                result["route_m"]+=(pos-self.last).length()/100
                self.last=pos
                x,y=route[self.index]
                dx,dy=x*100-pos.x,y*100-pos.y
                if math.hypot(dx,dy)<90:
                    self.index+=1
                    result["visited"]=self.index
                    self.last_progress=now
                    for near,photo,yaw in ((tile["haul"][0],"workyard",90),(tile["return_path"][5],"ridge",210)):
                        if (x,y)==near:
                            self.photo=photo
                            self.ctrl.set_control_rotation(unreal.Rotator(yaw=yaw,pitch=-7))
                            self.phase="pose"
                            self.frame=0
                    if self.index==len(route):
                        assert result["falls"]==0 and result["camera_obstructions"]==0,result
                        self.finish()
                    return
                self.ctrl.set_control_rotation(unreal.Rotator(yaw=math.degrees(math.atan2(dy,dx)),pitch=-9))
                pace=min(1.0,math.hypot(dx,dy)/max(140,dt*1250))
                self.ei.inject_input_vector_for_action(self.move,unreal.Vector(0,pace,0),[],[])
                if (pos-self.progress_pos).length()>40:
                    self.last_progress=now
                    self.progress_pos=pos
                if now-self.last_probe>.45:
                    result["falls"]+=int(self.p.character_movement.is_falling())
                    cam=self.p.get_component_by_class(unreal.CameraComponent).get_world_location()
                    hit=unreal.SystemLibrary.sphere_trace_single(self.world,cam,cam+unreal.Vector(0,0,2),5,
                        unreal.TraceTypeQuery.ECC_CAMERA,False,[self.p],unreal.DrawDebugTrace.NONE)
                    result["camera_obstructions"]+=int(bool(hit))
                    self.last_probe=now
                    (out/"progress.json").write_text(json.dumps({"waypoint":self.index,"metres":round(result["route_m"])}))
                if now-self.last_progress>10:
                    raise RuntimeError("Movement stalled at "+str((self.index,pos)))
        except Exception:
            self.finish(traceback.format_exc())


walk=WoodWalk()
