"""PIE check of native water spline flow and a bank-to-bank crossing."""
from pathlib import Path
import datetime
import json
import time
import traceback
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out = root / "Saved/Tests/VelesWater" / datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
out.mkdir(parents=True)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


class Crossing:
    def __init__(self):
        self.state = "begin"
        self.started = time.monotonic()
        self.result = {"wet_frames": 0, "active_frames": 0, "splash_instances": 0}
        self.handle = unreal.register_slate_post_tick_callback(self.tick)

    def finish(self, error=None):
        self.result["error"] = error
        (out/"crossing.json").write_text(json.dumps(self.result, indent=2))
        unreal.unregister_slate_post_tick_callback(self.handle)
        levels.editor_request_end_play()
        unreal.log("VELES_CROSSING "+str(out)+" "+str(self.result))

    def tick(self, dt):
        try:
            if time.monotonic()-self.started > 210:raise RuntimeError("Timeout at "+self.state)
            if self.state == "begin":
                levels.editor_request_begin_play()
                self.state = "spawn"
                return
            world = editor.get_game_world()
            if not world:return
            p = unreal.GameplayStatics.get_player_pawn(world, 0)
            if not p:return
            if self.state == "spawn":
                bodies = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WaterBodyRiver)
                assert len(bodies) == 2
                self.creek = next(b for b in bodies if b.get_actor_label()=="Veles village brook")
                self.result["bodies"] = {}
                for b in bodies:
                    c = b.get_water_body_component()
                    self.result["bodies"][b.get_actor_label()] = [b.get_water_spline().get_number_of_spline_points(),
                            c.get_river_width_at_spline_input_key(4),c.get_water_velocity_at_spline_input_key(4)]
                self.c = self.creek.get_water_body_component()
                self.collection = unreal.load_asset("/Game/Level/VelesRiversideAssets/MPC_VelesFootfall")
                assert self.collection
                self.point = self.creek.get_water_spline().get_location_at_spline_point(51,unreal.SplineCoordinateSpace.WORLD)
                self.upstream = self.creek.get_water_spline().get_location_at_spline_point(48,unreal.SplineCoordinateSpace.WORLD)
                self.leg=0
                p.set_actor_location(self.point+unreal.Vector(-175,0,145),False,True)
                self.ctrl = unreal.GameplayStatics.get_player_controller(world,0)
                self.ctrl.set_control_rotation(unreal.Rotator(yaw=69,pitch=-26))
                self.elapsed = 0
                self.state = "ready"
            self.elapsed += dt
            subsystem = next((s for s in unreal.ObjectIterator(unreal.BasicShallowWaterSubsystem)
                              if s.get_outer()==world),None)
            sim = subsystem.shallow_water_niagara_simulation if subsystem else None
            if self.state=="ready":
                if self.elapsed < 2 or not sim:return
                self.state = "cross"
                self.result["dry_before"] = self.creek not in p.get_overlapping_actors(unreal.WaterBodyRiver)
            if self.state=="cross":
                pos = p.get_actor_location()
                wet = self.creek in p.get_overlapping_actors(unreal.WaterBodyRiver)
                if wet:
                    self.result["wet_frames"] += 1
                    self.result["active_frames"] += int(bool(sim and sim.is_active()))
                    stamp = unreal.MaterialLibrary.get_scalar_parameter_value(world, self.collection, "FootTime")
                    if stamp > 0 and stamp != self.result.get("last_impact_s"):
                        self.result["last_impact_s"] = stamp
                        self.result["ripple_impacts"] = self.result.get("ripple_impacts", 0) + 1
                    self.result["collider_tags"] = [str(t) for t in p.mesh.component_tags]
                    bursts=[c for c in unreal.ObjectIterator(unreal.NiagaraComponent) if c.get_asset()
                            and c.get_asset().get_name()=="NS_BrookFootSplash" and c.get_owner()
                            and c.get_owner().get_world()==world and c.is_active()]
                    self.result["splash_instances"]=max(self.result["splash_instances"],len(bursts))
                    if self.result["wet_frames"] in (3, 16) or (bursts and "splash_cm" not in self.result):
                        self.result["movement"] = str(p.character_movement.movement_mode)
                        self.result["niagara"] = sim.get_asset().get_path_name() if sim else None
                        if bursts:self.result["splash_cm"]=round((bursts[0].get_world_location()-self.point).length(),1)
                        unreal.SystemLibrary.execute_console_command(world,'shot filename="'+(out/"crossing.png").as_posix()+'" -nosuffix')
                    if self.result["wet_frames"] in (4,8,12,18,24,32,44,56):
                        rt=subsystem.get_editor_property("NormalRT")
                        patch=unreal.RenderingLibrary.read_render_target_raw_pixel_area(world,rt,110,110,146,146,False)
                        self.result.setdefault("normal_rt_span",[]).append([round(max(getattr(c,k) for c in patch)-min(getattr(c,k) for c in patch),5) for k in ("r","g","b","a")])
                target=(self.point,self.upstream,self.upstream+unreal.Vector(180,0,0))[self.leg]
                delta=target-pos
                delta.z=0
                if delta.length()>20:
                    p.add_movement_input(delta/delta.length(),.24,True)
                elif self.leg<2:
                    self.leg+=1
                else:
                    self.result["dry_after"] = not wet
                    self.result["distance_cm"] = pos.x-(self.point.x-175)
                    assert self.result["dry_before"] and not wet and self.result["active_frames"]>0 and self.result["splash_instances"]>0,self.result
                    assert self.result.get("ripple_impacts", 0) > 0, self.result
                    self.state="quay"
                    self.elapsed=0
                    p.set_actor_location(unreal.Vector(1500,2050,440),False,True)
                    self.ctrl.set_control_rotation(unreal.Rotator(yaw=-90,pitch=-20))
            elif self.state=="quay" and self.elapsed>1.5:
                unreal.SystemLibrary.execute_console_command(world,'shot filename="'+(out/"main-river.png").as_posix()+'" -nosuffix')
                self.state="finish"
            elif self.state=="finish" and (out/"main-river.png").exists():self.finish()
        except Exception:
            self.finish(traceback.format_exc())


check = Crossing()
