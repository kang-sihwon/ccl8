"""Walk both arms of the first fork and return through the gate in one PIE world."""
from pathlib import Path
import datetime
import json
import math
import runpy
import time
import traceback

import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
SCENE = runpy.run_path(str(ROOT/"Tools/WorldBuilding/build_veles_gate_fork.py"))
OUT = ROOT/"Saved/Tests/VelesGateFork"/datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
OUT.mkdir(parents=True)
EDITOR = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
GS = unreal.GameplayStatics
GATE, FORK = SCENE["GATE"], SCENE["FORK"]
ROUTE = [(-65,180),(-72,192),GATE,FORK,(-73,226),(-70,234),(-67,244),
         (-70,234),(-73,226),FORK,(-84,229),(-89,239),(-84,229),FORK,GATE,(-72,192),(-65,180)]
VIEWS = {2:("outward",(-75,222)),5:("upstream",(-59,240)),9:("fork",(-86,237)),
         13:("downstream",(-69,201)),14:("homeward",(-63,180))}


class Walk:
    def __init__(self):
        self.phase = "load"
        self.index = 0
        self.distance = 0
        self.falling = 0
        self.max_step = 0
        self.in_water = 0
        self.report = {"map":SCENE["MAP"],"status":"RUNNING","input":"Enhanced Input","waypoints":[],"screens":[]}
        self.handle = unreal.register_slate_post_tick_callback(self.tick)
        self.reset()

    def reset(self):
        self.since = time.monotonic()

    def save(self):
        (OUT/"result.json").write_text(json.dumps(self.report,indent=2),encoding="utf-8")

    def finish(self):
        self.save()
        unreal.unregister_slate_post_tick_callback(self.handle)
        if LEVELS.is_in_play_in_editor():
            LEVELS.editor_request_end_play()
        unreal.SystemLibrary.quit_editor()

    def sample_water(self):
        bodies = GS.get_all_actors_of_class(self.world,unreal.WaterBodyRiver)
        assert len(bodies)==1, len(bodies)
        body = bodies[0]
        sheets = [a for a in GS.get_all_actors_of_class(self.world,unreal.StaticMeshActor)
                  if a.get_actor_label()=="Forest gate brook surface"]
        assert len(sheets)==1
        mat = sheets[0].static_mesh_component.get_material(0)
        self.report["visible_water_material"] = mat.get_path_name()
        assert mat.get_name()=="M_GateBrook"
        spline = body.get_water_spline()
        space = unreal.SplineCoordinateSpace.WORLD
        comp = body.get_water_body_component()
        depths, freeboards, levels = [], [], []
        # 수면 위 수관과 울타리의 충돌이 하상 높이로 집계되지 않도록 지형만 추적한다.
        ignore = [a for a in GS.get_all_actors_of_class(self.world,unreal.Actor)
                  if not a.get_actor_label().startswith("Terrain Ground_")]
        for y in range(182,243,2):
            key = (SCENE["BROOK_TOP"]-y)/2
            pos = spline.get_location_at_spline_point(int(key),space)
            w = comp.get_river_width_at_spline_input_key(key)/2
            assert abs(pos.y/100-y)<.02 and w>90
            def bed(dx):
                xy = pos+unreal.Vector(dx,0,0)
                hit = unreal.SystemLibrary.line_trace_single(self.world,xy+unreal.Vector(0,0,2500),
                    xy-unreal.Vector(0,0,2500),unreal.TraceTypeQuery.ECC_VISIBILITY,True,
                    ignore,unreal.DrawDebugTrace.NONE)
                assert hit, (y,dx)
                return hit.to_tuple()[4].z
            depths.append(pos.z-bed(0))
            freeboards.extend(bed(sign*(SCENE["width"](y)*100+150))-pos.z for sign in (-1,1))
            levels.append(pos.z)
            assert comp.get_water_velocity_at_spline_input_key(key)>0
        assert all(a<b for a,b in zip(levels,levels[1:]))
        self.report["water"] = {"sections":len(levels),"bed_min_depth_cm":round(min(depths),2),
            "min_bank_above_water_cm":round(min(freeboards),2),"fall_upstream_to_downstream_m":round((levels[-1]-levels[0])/100,2)}
        self.save()
        assert min(depths)>20 and min(freeboards)>0, self.report["water"]

    def record_point(self,pos):
        tiles = [a.get_actor_label() for a in GS.get_all_actors_of_class(self.world,unreal.StaticMeshActor)
                 if a.get_actor_label().startswith("Terrain Ground_")]
        self.report["waypoints"].append({"index":self.index,"target_m":ROUTE[self.index],
            "position_m":[round(n/100,2) for n in (pos.x,pos.y,pos.z)],"loaded_tiles":sorted(tiles)})
        self.save()

    def step(self):
        if self.phase=="edges":
            self.edges.step()
            return
        if self.phase=="load":
            assert LEVELS.load_level(SCENE["MAP"])
            descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
            terrain = [d for d in descs if str(d.label).startswith("Terrain Ground_")]
            self.report["partitioned_terrain_actors"] = len(terrain)
            assert len(terrain)==42 and all(d.is_spatially_loaded for d in terrain),len(terrain)
            LEVELS.editor_request_begin_play()
            self.phase="spawn"
            self.reset()
        elif self.phase=="spawn":
            self.world = EDITOR.get_game_world()
            if not self.world:
                return
            self.ctrl = GS.get_player_controller(self.world,0)
            self.p = GS.get_player_pawn(self.world,0)
            if not self.p or not self.ctrl or self.p.character_movement.is_falling():
                return
            self.ei = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                           if s.get_outer().get_class().get_name()=="CCLLocalPlayer")
            self.action = next(a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==self.ctrl and
                a.value_type==unreal.InputActionValueType.AXIS2D and a.accumulation_behavior==unreal.InputActionAccumulationBehavior.CUMULATIVE)
            self.last = self.p.get_actor_location()
            assert math.hypot(self.last.x/100+65,self.last.y/100-180)<1.2,self.last
            self.phase="walk"
            self.reset()
        elif self.phase=="walk":
            pos = self.p.get_actor_location()
            x,y = ROUTE[self.index]
            dx,dy = x*100-pos.x,y*100-pos.y
            if math.hypot(dx,dy)<65:
                self.record_point(pos)
                if self.index==3:
                    self.sample_water()
                if self.index in VIEWS:
                    name,target = VIEWS[self.index]
                    self.ctrl.set_control_rotation(unreal.Rotator(yaw=math.degrees(math.atan2(target[1]-y,target[0]-x)),pitch=-14))
                    self.file = OUT/(name+".png")
                    self.phase="aim"
                self.index+=1
                self.reset()
                if self.index==len(ROUTE):
                    self.report.update(metres=round(self.distance,2),falling_frames=self.falling,
                                       water_overlap_frames=self.in_water,max_observed_vertical_step_cm=round(self.max_step,2))
                    tiles = [set(w["loaded_tiles"]) for w in self.report["waypoints"]]
                    self.report["terrain_tiles_loaded_range"] = [min(map(len,tiles)),max(map(len,tiles))]
                    assert self.falling==0 and self.in_water==0 and self.distance>140
                    assert any(t!=tiles[0] for t in tiles)
                    edge_test = runpy.run_path(str(ROOT/"Tools/WorldBuilding/verify_veles_gate_perimeter.py"))
                    self.edges = edge_test["Edges"](self,SCENE,OUT)
                    self.phase="edges"
                    self.reset()
                return
            self.ctrl.set_control_rotation(unreal.Rotator(yaw=math.degrees(math.atan2(dy,dx)),pitch=-9))
            self.ei.inject_input_vector_for_action(self.action,unreal.Vector(0,min(1,math.hypot(dx,dy)/120),0),[],[])
        elif self.phase=="aim" and time.monotonic()-self.since>1.4:
            unreal.SystemLibrary.execute_console_command(self.world,'Shot filename="'+self.file.as_posix()+'" -nosuffix',self.ctrl)
            self.phase="capture"
            self.reset()
        elif self.phase=="capture" and self.file.is_file() and self.file.stat().st_size>8000:
            self.report["screens"].append(self.file.name)
            self.phase="walk"
            self.reset()

    def tick(self,dt):
        try:
            if self.phase in ("walk","aim","capture"):
                pos = self.p.get_actor_location()
                self.distance += (pos-self.last).length()/100
                self.max_step = max(self.max_step,abs(pos.z-self.last.z))
                self.falling += int(self.p.character_movement.is_falling())
                self.in_water += int(bool(self.p.get_overlapping_actors(unreal.WaterBodyRiver)))
                self.last = pos
            if time.monotonic()-self.since>50:
                if self.phase=="edges":
                    raise RuntimeError(f"Boundary {self.edges.section} {self.edges.mode}, at {self.p.get_actor_location()}")
                raise RuntimeError(f"Timeout in {self.phase}, waypoint {self.index}")
            self.step()
        except Exception:
            self.report.update(status="FAIL",error=traceback.format_exc())
            unreal.log_error(self.report["error"])
            self.finish()


walk = Walk()
