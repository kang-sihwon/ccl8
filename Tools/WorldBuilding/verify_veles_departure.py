"""Capture all numbered game-world cameras and exercise the playable sets in PIE."""
from pathlib import Path
import datetime
import json
import math
import os
import runpy
import time
import traceback
import unreal

root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out=root/"Saved/Tests/VelesDeparture"/datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
out.mkdir(parents=True)
maps=["L01_VelesVillage","L02_GoldenMistForest","L03_FrostrootCave"]
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
package="/Game/Level/VelesDeparture/StoryRegions/"
gs=unreal.GameplayStatics
# 독립적인 진입 표본에서 실제 이동 입력으로 장소에 접근한다.
walks=[
    [[(-43,53),(-48,52)],[(-13.3,89),(-11.9,87.9)],[(-8,104),(-5,102)],[(13,22),(15,18)]],
    [[(-70,232),(-72,228)],[(223,598),(226,602)],[(0,0),(0,0)],[(-241,721),(-246,729)],[(0,0),(0,0)],[(273,1241),(272,1246),(272,1251)]],
    [[(-7,0),(-1,0),(5,.8),(12,1),(17,1),(23,1),(29,1),(34,.5),(40,0),(45,0),(51,0),(57,-1),(61,-2),(65,-1)]]
]
def cx(y):return -74+.325*(y-205)+13*math.sin((y-215)/58)+5*math.sin(y/25)
walks[1][2]=[(cx(y)-6,y) for y in (750,755)]
walks[1][4]=[(cx(y)-6,y) for y in (1047,1041)]
class Audit:
    def __init__(self):
        self.region=int(os.environ.get("CCL_DEPARTURE_REGION","1"))-1
        self.begin=self.region
        self.phase="load"; self.frame=0
        self.report={};self.reset()
        self.tick_handle=unreal.register_slate_post_tick_callback(self.tick)
    def reset(self):
        self.frame=0;self.clock=time.monotonic()
    def record(self):
        (out/"report.json").write_text(json.dumps(self.report,indent=2),encoding="utf-8")
    def finish(self):
        self.record()
        unreal.unregister_slate_post_tick_callback(self.tick_handle)
        unreal.SystemLibrary.quit_editor()
    def tick(self,dt):
        try:
            self.frame+=1
            now=time.monotonic()
            if now-self.clock>135:raise RuntimeError("Timeout "+self.phase)
            if self.phase=="load":
                world=unreal.EditorLoadingAndSavingUtils.load_map(package+maps[self.region])
                assert len(unreal.EditorLevelUtils.get_levels(world))==1
                self.row={"cameras":[],"walks":[],"issues":[]}
                self.report[maps[self.region]]=self.row
                self.cams=sorted([a for a in gs.get_all_actors_of_class(world,unreal.CameraActor)
                    if a.get_actor_label()[:1] in "VFC" and a.get_actor_label()[1:3].isdigit()],key=lambda a:a.get_actor_label())
                assert len(self.cams)==[5,7,4][self.region],len(self.cams)
                if self.region==1:
                    helpers=runpy.run_path(str(root/"Tools/WorldBuilding/build_veles_departure.py"))
                    forest=runpy.run_path(str(root/"Tools/WorldBuilding/veles_departure_forest.py"),init_globals=helpers)
                    self.floor=forest["floor"]
                    if os.environ.get("CCL_DEPARTURE_LONG")=="1":
                        routes=forest["routes"];side=forest["side"];branch=routes[1][6:]
                        east=routes[0]+[side(747,-7),side(756,-6)]
                        west=branch[::-1]+branch[1:]
                        upper=[side(y,-6) for y in range(765,1212,12)]
                        end=[side(1212,-7),side(1212,7)]+[side(y,13) for y in range(1219,1248,7)]+[(272,1248),(272,1251)]
                        walks[1]=[*walks[1],east+west+upper+end]
                    foliage=gs.get_all_actors_of_class(world,unreal.InstancedFoliageActor)[0]
                    trees=[c for c in foliage.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent) if c.static_mesh and "/Trees/" in c.static_mesh.get_path_name() and c.get_instance_count()]
                    self.row["trees"]=sum(c.get_instance_count() for c in trees)
                    self.row["tree_cull_end_cm"]=[c.get_editor_property("instance_end_cull_distance") for c in trees]
                    assert self.row["trees"]>2000 and min(self.row["tree_cull_end_cm"])>200000
                    self.row["water_bodies"]=len(gs.get_all_actors_of_class(world,unreal.WaterBodyRiver))
                    assert self.row["water_bodies"]==2
                self.cam=0;self.phase="aim";self.reset()
            elif self.phase=="aim":
                c=self.cams[self.cam]
                if self.region==0:
                    sets=next(a for a in gs.get_all_actors_of_class(editor.get_editor_world(),unreal.LevelVariantSetsActor) if a.get_actor_label()=="V00 Village times")
                    asset=unreal.load_asset(package+"VelesVillageTimes")
                    assert sets.get_level_variant_sets(True)==asset,(sets.get_actor_label(),sets.get_level_variant_sets(True))
                    group=asset.get_variant_set(0)
                    self.row["time_variants"]=[str(group.get_variant(i).get_display_text()) for i in range(group.get_num_variants())]
                    state={"V03":"Evening","V04":"Dawn"}.get(c.get_actor_label()[:3],"Day")
                    assert sets.switch_on_variant_by_index(0,self.row["time_variants"].index(state)),self.row["time_variants"]
                    sun=gs.get_all_actors_of_class(editor.get_editor_world(),unreal.DirectionalLight)[0]
                    self.row.setdefault("sun_lux",{})[c.get_actor_label()[:3]]=round(sun.light_component.intensity,3)
                editor.set_level_viewport_camera_info(c.get_actor_location(),c.get_actor_rotation())
                self.phase="expose";self.reset()
            elif self.phase=="expose" and self.frame>=26 and now-self.clock>1.7:
                c=self.cams[self.cam];self.file=out/(c.get_actor_label()[:3]+".png")
                self.shot=unreal.AutomationLibrary.take_high_res_screenshot(1280,720,str(self.file),camera=c,delay=.35)
                self.phase="photo";self.reset()
            elif self.phase=="photo" and self.file.exists() and self.file.stat().st_size>3000:
                self.row["cameras"].append(self.file.stem);self.record();self.cam+=1
                self.phase="aim" if self.cam<len(self.cams) else "play";self.reset()
            elif self.phase=="play" and self.frame>=8:
                if self.region==0:
                    sets=next(a for a in gs.get_all_actors_of_class(editor.get_editor_world(),unreal.LevelVariantSetsActor) if a.get_actor_label()=="V00 Village times")
                    asset=unreal.load_asset(package+"VelesVillageTimes")
                    assert sets.get_level_variant_sets(True)==asset,(sets.get_actor_label(),sets.get_level_variant_sets(True))
                    group=asset.get_variant_set(0)
                    self.row["time_variants"]=[str(group.get_variant(i).get_display_text()) for i in range(group.get_num_variants())]
                    assert sets.switch_on_variant_by_index(0,self.row["time_variants"].index("Day"))
                    values=self.row["sun_lux"]
                    assert values["V03"]<values["V04"]<values["V01"]
                levels.editor_request_begin_play(); self.phase="spawn";self.reset()
            elif self.phase=="spawn":
                self.world=editor.get_game_world()
                if not self.world:return
                self.ctrl=gs.get_player_controller(self.world,0);self.p=gs.get_player_pawn(self.world,0)
                if not self.ctrl or not self.p:return
                players=[s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                         if s.get_outer().get_class().get_name()=="CCLLocalPlayer"]
                assert len(players)==1, [s.get_path_name() for s in players]
                self.ei=players[0]
                self.move=next(a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==self.ctrl and
                    a.value_type==unreal.InputActionValueType.AXIS2D and a.accumulation_behavior==unreal.InputActionAccumulationBehavior.CUMULATIVE)
                self.row["start"]=str(self.p.get_actor_location())
                self.seg=0;self.phase="position";self.reset()
            elif self.phase=="position":
                self.journey=self.region==1 and self.seg==6
                if self.journey:
                    gs.set_global_time_dilation(self.world,4.0)
                    self.row["journey_time_dilation"]=4
                    self.row["journey_waypoints"]=len(walks[1][self.seg])-1
                x,y=walks[self.region][self.seg][0]
                low=-10 if self.region==2 else -30
                high=(.8-.063*x+2.4 if self.region==2 else self.floor(x,y)+1.05 if self.region==1 else 160)
                hit=unreal.SystemLibrary.line_trace_single(self.world,unreal.Vector(x*100,y*100,high*100),unreal.Vector(x*100,y*100,low*100),unreal.TraceTypeQuery.ECC_VISIBILITY,True,[self.p],unreal.DrawDebugTrace.NONE)
                if not hit:raise RuntimeError("No ground at "+str((x,y)))
                p=hit.to_tuple()[4]+unreal.Vector(0,0,self.p.capsule_component.get_scaled_capsule_half_height()+4)
                self.p.set_actor_location(p,False,True);self.p.character_movement.stop_movement_immediately()
                self.way=1;self.meters=0;self.last=p;self.falls=0;self.phase="settle";self.reset()
            elif self.phase=="settle" and self.frame>=8:
                self.last=self.p.get_actor_location()
                self.progress_position=self.last;self.progress_time=now
                self.phase="walk";self.reset()
            elif self.phase=="walk":
                pos=self.p.get_actor_location(); self.meters+=(pos-self.last).length()/100;self.last=pos
                self.falls+=int(self.p.character_movement.is_falling())
                seq=walks[self.region][self.seg]
                x,y=seq[self.way];dx=x*100-pos.x;dy=y*100-pos.y
                if math.hypot(dx,dy)<(100 if self.journey else 65):
                    self.way+=1
                    if self.way==len(seq):self.next_walk(True)
                    else:
                        self.row["waypoints_reached"]=self.way-1
                        self.row["last_position_m"]=[round(v/100,2) for v in (pos.x,pos.y,pos.z)]
                        self.record();self.reset()
                    return
                if (pos-self.progress_position).length()>45:
                    self.progress_position=pos;self.progress_time=now
                if now-self.progress_time>8 or now-self.clock>(90 if self.journey else 14):
                    self.row["issues"].append("Walk stalled "+str((self.seg,self.way,pos)))
                    self.next_walk(False);return
                self.ctrl.set_control_rotation(unreal.Rotator(yaw=math.degrees(math.atan2(dy,dx)),pitch=-8))
                self.ei.inject_input_vector_for_action(self.move,unreal.Vector(0,min(1,math.hypot(dx,dy)/120),0),[],[])
            elif self.phase=="stop" and not levels.is_in_play_in_editor() and self.frame>=12:
                self.finish()
        except Exception:
            self.report["error"]=traceback.format_exc();unreal.log_error(self.report["error"]);self.finish()
    def next_walk(self,ok):
        self.row["walks"].append(dict(segment=self.seg,passed=ok,metres=round(self.meters,2),falling_frames=self.falls))
        self.record();self.seg+=1;self.reset()
        if self.seg<len(walks[self.region]):self.phase="position"
        else:
            levels.editor_request_end_play();self.phase="stop"
audit=Audit()
