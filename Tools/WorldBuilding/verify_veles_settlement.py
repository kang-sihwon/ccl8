"""Check openings against the PIE collision scene, then walk the workshop probes."""
import json
import math
from pathlib import Path
import traceback

import unreal


root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out=Path(globals().get("evidence_dir",root/"Saved/Tests/VelesSettlement"))
layout=json.loads((out/"revision.json").read_text())
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)


def point(h,op,offset,z):
    u,v=(op["p"],op["k"]+offset) if op["side"]<2 else (op["k"]+offset,op["p"])
    return unreal.Vector((h["x"]+u*h["c"]-v*h["s"])*100,(h["y"]+u*h["s"]+v*h["c"])*100,(h["z"]+z)*100)


class Verify:
    def __init__(self):
        self.result={"map":layout["map"],"trace_count":0,"windows":0,"doors":0,"standing_window_blocks":0,
                     "wall_controls":0,"trace_failures":[],"walks":[]}
        self.phase="boot"
        self.frames=0
        self.path_index=0
        self.handle=unreal.register_slate_post_tick_callback(self.tick)

    def finish(self):
        unreal.unregister_slate_post_tick_callback(self.handle)
        ok=(not self.result.get("error") and not self.result["trace_failures"]
            and self.result["wall_controls"]==len(layout["houses"])
            and self.result["windows"]==self.result["standing_window_blocks"]
            and len(self.result["walks"])==4 and all(w["passed"] for w in self.result["walks"]))
        self.result["status"]="PASS" if ok else "FAIL"
        self.result["input"]="CharacterMovement directional input, LeftControl"
        if globals().get("jump_to_sill"):self.result["input"]+=", SpaceBar"
        (out/"opening-check.json").write_text(json.dumps(self.result,indent=2))
        if levels.is_in_play_in_editor():
            levels.editor_request_end_play()

    def sweep(self,h,op,half,z):
        hit=unreal.SystemLibrary.capsule_trace_single(self.world,point(h,op,-.76,z),point(h,op,.76,z),
                42,half,unreal.TraceTypeQuery.ECC_VISIBILITY,False,[self.pawn],unreal.DrawDebugTrace.NONE)
        return hit.to_tuple()[9].get_actor_label() if hit else ""

    def openings(self):
        for h in layout["houses"]:
            for op in h["openings"]:
                isdoor=op["kind"]=="door"
                self.result["doors" if isdoor else "windows"]+=1
                half=96 if isdoor else 42
                hit=self.sweep(h,op,half,op["sill"]+half/100+.07)
                self.result["trace_count"]+=1
                if hit:
                    self.result["trace_failures"].append([h["label"],op,hit])
                if not isdoor:
                    if self.sweep(h,op,96,op["sill"]+1.03):
                        self.result["standing_window_blocks"]+=1
                    else:
                        self.result["trace_failures"].append([h["label"],op,"missing lintel"])
            op=dict(h["openings"][0],p=-(h["w"]-.78)/2+.37)
            if self.sweep(h,op,96,1.03):
                self.result["wall_controls"]+=1

    def tick(self,dt):
        try:
            if self.phase=="boot":
                self.world=editor.get_game_world()
                if not self.world:return
                self.pawn=unreal.GameplayStatics.get_player_pawn(self.world,0)
                if not self.pawn:return
                self.ctrl=unreal.GameplayStatics.get_player_controller(self.world,0)
                self.movement=self.pawn.character_movement
                self.openings()
                self.h=next(h for h in layout["houses"] if h["label"]=="workshop")
                door=next(o for o in self.h["openings"] if o["side"]==0 and o["kind"]=="door")
                window=next(o for o in self.h["openings"] if o["side"]==0 and o["kind"]=="window")
                self.tests=[(door,1),(door,-1),(window,1),(window,-1)]
                self.phase="start"
            elif self.phase=="start":
                self.op,self.sign=self.tests[self.path_index]
                unreal.SystemLibrary.execute_console_command(self.world,"Input.-key LeftControl",self.ctrl)
                self.movement.stop_movement_immediately()
                self.a=point(self.h,self.op,-self.sign*2.15,1.05)
                self.b=point(self.h,self.op,self.sign*2.15,1.05)
                self.pawn.set_actor_location(self.a,False,True)
                self.ctrl.set_control_rotation(unreal.Rotator(yaw=self.h["yaw"]+self.sign*90,pitch=-4))
                self.frames=0
                self.phase="settle"
            elif self.phase=="settle":
                self.frames+=1
                if self.frames==6 and self.op["kind"]=="window" and not globals().get("jump_to_sill"):
                    unreal.SystemLibrary.execute_console_command(self.world,"Input.+key LeftControl",self.ctrl)
                if self.frames>=18:
                    self.takeoff_z=self.pawn.get_actor_location().z
                    self.jumped=self.ducked=False
                    self.frames=0
                    self.phase="walk"
            elif self.phase=="walk":
                self.frames+=1
                pos=self.pawn.get_actor_location()
                direction=self.b-self.a
                length=math.hypot(direction.x,direction.y)
                direction=unreal.Vector(direction.x/length,direction.y/length,0)
                travel=(pos.x-self.a.x)*direction.x+(pos.y-self.a.y)*direction.y
                if globals().get("jump_to_sill") and self.op["kind"]=="window":
                    if not self.jumped and travel>=95:
                        unreal.SystemLibrary.execute_console_command(self.world,"Input.+key SpaceBar",self.ctrl)
                        self.jumped=True
                    if self.jumped and not self.ducked and pos.z-self.takeoff_z>20:
                        unreal.SystemLibrary.execute_console_command(self.world,"Input.+key LeftControl",self.ctrl)
                        unreal.SystemLibrary.execute_console_command(self.world,"Input.-key SpaceBar",self.ctrl)
                        self.ducked=True
                cap=self.pawn.capsule_component.get_scaled_capsule_half_height()
                # 시작점이 벽 중심에서 215cm이므로 320cm 이동하면 캡슐 전체가 반대편을 지난다.
                target=320 if globals().get("jump_to_sill") and self.op["kind"]=="window" else 370
                if travel>=target or self.frames>240:
                    if self.jumped:unreal.SystemLibrary.execute_console_command(self.world,"Input.-key SpaceBar",self.ctrl)
                    self.result["walks"].append(dict(kind=self.op["kind"],direction=self.sign,distance_cm=travel,
                        capsule_half_height=cap,jump_input=self.jumped,crouch_in_air=self.ducked,
                        passed=travel>=target and (self.op["kind"]=="door" or cap<50)))
                    self.path_index+=1
                    if self.path_index==len(self.tests) and globals().get("skip_photos"):
                        self.finish()
                        return
                    self.phase="start" if self.path_index<len(self.tests) else "photo"
                    self.frames=0
                else:
                    self.pawn.add_movement_input(direction,1,True)
            elif self.phase=="photo":
                views=globals().get("photo_views",[("street",self.h["x"]-6,self.h["y"]-30,77),("shore",-43,20,-32),("gardens",18,134,37)])
                idx=self.frames//28
                frame=self.frames%28
                name,x,y,yaw=views[idx]
                if frame==0:
                    unreal.SystemLibrary.execute_console_command(self.world,"Input.-key LeftControl",self.ctrl)
                    self.movement.stop_movement_immediately()
                    floor_z=globals().get("photo_floor",lambda x,y:0)(x,y)
                    self.pawn.set_actor_location(unreal.Vector(x*100,y*100,floor_z*100+190),False,True)
                    self.ctrl.set_control_rotation(unreal.Rotator(yaw=yaw,pitch=-2))
                if frame==20:
                    unreal.SystemLibrary.execute_console_command(self.world,'shot filename="'+(out/(name+".png")).as_posix()+'" -nosuffix')
                self.frames+=1
                if self.frames==len(views)*28:self.finish()
        except Exception:
            self.result["error"]=traceback.format_exc()
            self.finish()


assert editor.get_editor_world().get_path_name().split(".")[0]==layout["map"]
if not levels.is_in_play_in_editor():
    levels.editor_request_begin_play()
run=Verify()
