"""Check a cold editor open, then walk from the repair yard to the cavern and back in one PIE session."""
import datetime
import json
import math
from pathlib import Path
import runpy
import time
import traceback

import unreal

root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
land=runpy.run_path(str(root/"Tools/WorldBuilding/veles_departure_world_land.py"))
forest,art,cave=land["FOREST"],land["ART"],land["CAVE"]
vec=land["BASE"]["vec"]
editor=land["BASE"]["editor"]
actors=land["BASE"]["actors"]
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
gs=unreal.GameplayStatics
out=root/"Saved/Tests/VelesDepartureWorld"/datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
out.mkdir(parents=True)
village=[(-48,53),*art["shop"][1:],*art["forest"][1:]]
side=forest["side"]
east=forest["routes"][0][1:]+[side(747,-7),side(756,-6)]
upper=[side(y,-6) for y in range(765,1212,12)]+[side(1212,-7),side(1212,7)]
upper += [side(y,13) for y in range(1219,1248,7)]+[(272,1248),(272,1250)]
cave_local=[(-11,cave["center"](-11)),(-8,cave["center"](-8)),(-4,cave["center"](-4)),
            (0,0),(5,.8),(12,1),(17,1),(23,1),(29,1),(35,0),(41,-1),(47,-1),(53,-1),(59,-2),(65,-1)]
inside=[land["cave_point"](x,y,0)[:2] for x,y in cave_local]
west=list(reversed(forest["routes"][1][1:-1]))
stages=[("village to gate",village),("eastern trail",east),("upstream to intake",upper),
        ("intake to source chamber",inside),("source chamber to intake",inside[-2::-1]),
        ("return downstream",upper[::-1]),("western timber trail",[side(756,-6),*west,forest["gate"]]),
        ("gate to repair yard",village[-2::-1])]


class Audit:
    def __init__(self):
        self.report={"map":land["MAP"],"status":"RUNNING","editor_views":[],"walks":[],"input":"Enhanced Input","time_dilation":1}
        self.phase="editor";self.since=time.monotonic()
        self.shot=0;self.stage=0;self.way=0;self.total=0.;self.falls=0
        self.track=[];self.max_step=0.
        self.handle=unreal.register_slate_post_tick_callback(self.tick)

    def save(self):
        (out/"result.json").write_text(json.dumps(self.report,indent=2),encoding="utf-8")

    def finish(self,status):
        self.report["status"]=status
        self.report["max_sample_step_m"]=round(self.max_step,2)
        (out/"track.json").write_text(json.dumps(self.track),encoding="utf-8")
        self.save()
        unreal.log("DEPARTURE_WORLD_"+status+" "+str(out))
        unreal.unregister_slate_post_tick_callback(self.handle)
        if levels.is_in_play_in_editor():
            levels.editor_request_end_play()
        unreal.SystemLibrary.quit_editor()

    def editor_check(self):
        world=editor.get_editor_world()
        assert world.get_package().get_name()==land["MAP"]
        assert not editor.get_game_world()
        present=actors.get_all_level_actors()
        labels=[a.get_actor_label() for a in present]
        desc=unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
        terrain=[d for d in desc if str(d.label).startswith("Departure ground ")]
        assert len(terrain)>80 and all(d.is_spatially_loaded for d in terrain)
        assert sum(s.startswith("Departure ground ") for s in labels)==len(terrain)
        for marker in ("V01 Repair bench","F02 Clusters","F04 Haul cart","F06 Timber intake","C01 Frozen intake","C03 Bartul"):
            assert any(s.startswith(marker) for s in labels),marker
        self.tiles=len(terrain)
        self.report["pre_play"]=dict(loaded_actors=len(present),terrain_tiles=self.tiles,
            foliage_instances=sum(c.get_instance_count() for a in present if isinstance(a,unreal.InstancedFoliageActor)
                for c in a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)))
        cams=[a for a in present if isinstance(a,unreal.CameraActor)]
        self.cameras=[next(a for a in cams if a.get_actor_label().startswith(prefix))
                      for prefix in ("Departure overview","V01 Workshop","F00 Golden","F06 Root","C02 Workshop","C03 Bartul")]
        self.poses=[(a.get_actor_label(),a.get_actor_location(),a.get_actor_rotation()) for a in self.cameras]
        self.phase="view";self.save()

    def drive(self):
        p=self.pawn.get_actor_location()
        name,seq=stages[self.stage]
        dt=(p-self.last).length()/100
        assert self.pawn==gs.get_player_pawn(self.world,0) and dt<12 and p.z>-1000,(name,p,dt)
        self.max_step=max(self.max_step,dt)
        if not self.track or math.hypot(p.x/100-self.track[-1][1],p.y/100-self.track[-1][2])>6:
            self.track.append([self.stage,round(p.x/100,2),round(p.y/100,2),round(p.z/100,2)])
        self.total+=dt;self.last=p
        self.falls+=int(self.pawn.character_movement.is_falling())
        x,y=seq[self.way];dx,dy=x*100-p.x,y*100-p.y
        if math.hypot(dx,dy)<(48 if "intake" in name or "chamber" in name else 85):
            self.way+=1
            if self.way==len(seq):
                count=sum(a.get_actor_label().startswith("Departure ground ") for a in
                          gs.get_all_actors_of_class(self.world,unreal.StaticMeshActor))
                self.report["walks"].append(dict(stage=name,waypoints=len(seq),position_m=[round(v/100,2) for v in (p.x,p.y,p.z)],
                    cumulative_metres=round(self.total,2),falling_samples=self.falls,runtime_tiles=count))
                self.stage+=1;self.way=0
            self.progress=p;self.since=time.monotonic()
            self.report["progress"]=[self.stage,self.way]
            self.save()
            if self.stage==len(stages):
                self.report["metres"]=round(self.total,2)
                self.report["return_error_m"]=round((p-self.start).length()/100,2)
                assert self.total>2400 and self.report["return_error_m"]<2 and self.pawn==gs.get_player_pawn(self.world,0)
                assert min(w["runtime_tiles"] for w in self.report["walks"])<self.tiles
                self.finish("PASS")
            return
        if (p-self.progress).length()>50:
            self.progress=p;self.since=time.monotonic()
        assert time.monotonic()-self.since<12,(name,self.way,(x,y),p)
        self.ctrl.set_control_rotation(unreal.Rotator(yaw=math.degrees(math.atan2(dy,dx)),pitch=-8))
        self.ei.inject_input_vector_for_action(self.move,unreal.Vector(0,min(1,math.hypot(dx,dy)/160),0),[],[])

    def tick(self,delta):
        try:
            now=time.monotonic()
            if self.phase=="editor":
                self.editor_check()
            elif self.phase=="view":
                _,eye,rotation=self.poses[self.shot]
                editor.set_level_viewport_camera_info(eye,rotation)
                self.phase="settle";self.since=now
            elif self.phase=="settle" and now-self.since>2:
                self.path=out/f"editor-{self.shot}.png"
                unreal.AutomationLibrary.take_high_res_screenshot(1280,720,str(self.path),delay=.25)
                self.phase="photo";self.since=now
            elif self.phase=="photo":
                if self.path.exists() and self.path.stat().st_size>8000:
                    self.report["editor_views"].append(self.poses[self.shot][0]);self.save()
                    self.shot+=1
                    if self.shot<len(self.poses):
                        self.phase="view"
                    else:
                        levels.editor_request_begin_play();self.phase="spawn";self.since=now
                elif now-self.since>180:
                    raise RuntimeError("Editor viewport capture timed out: "+str(self.path))
            elif self.phase=="spawn":
                assert now-self.since<120,"PIE spawn timeout"
                self.world=editor.get_game_world()
                if not self.world:return
                self.ctrl=gs.get_player_controller(self.world,0)
                self.pawn=gs.get_player_pawn(self.world,0)
                if not self.pawn or not self.ctrl or self.pawn.character_movement.is_falling():return
                p=self.pawn.get_actor_location()
                assert math.hypot(p.x/100+48,p.y/100-53)<1.5,p
                self.start=self.last=self.progress=p
                self.ei=next(e for e in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                             if e.get_outer().get_class().get_name()=="CCLLocalPlayer")
                self.move=next(a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==self.ctrl and
                               a.value_type==unreal.InputActionValueType.AXIS2D and a.accumulation_behavior==unreal.InputActionAccumulationBehavior.CUMULATIVE)
                gs.set_global_time_dilation(self.world,1)
                self.world.get_world_settings().set_editor_property("max_undilated_frame_time",.05)
                self.report["pie_max_frame_time_s"]=.05
                self.report["pawn_class"]=self.pawn.get_class().get_name()
                self.report["max_walk_speed_cm_s"]=self.pawn.character_movement.max_walk_speed
                self.phase="walking";self.since=now
            elif self.phase=="walking":
                self.drive()
        except Exception:
            self.report["error"]=traceback.format_exc()
            unreal.log_error(self.report["error"])
            self.finish("FAIL")


if __name__=="__main__":
    audit=Audit()
