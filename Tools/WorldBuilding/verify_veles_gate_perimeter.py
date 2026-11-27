"""Measure the saved perimeter's collision and walk across each former edge in PIE."""
import math
import time

import unreal


class Edges:
    def __init__(self,walk,scene,out):
        self.walk,self.scene,self.out = walk,scene,out
        self.p,self.world,self.ctrl = walk.p,walk.world,walk.ctrl
        self.to_vec = scene["VEC"]
        creek = scene["creek"]
        self.paths = [
            ("south",[(creek(y)-5,y) for y in (134,128,122,117)]),
            ("north",[(creek(y)-6,y) for y in (314,320,326,331)]),
            ("west",[(x,233) for x in (-186,-192,-198,-203)]),
            ("east",[(x,270) for x in (26,32,38,43)])]
        self.data = {"walks":[]}
        walk.report["perimeter"] = self.data
        self.section = 0
        self.mode = "place"
        self.probe_perimeter()

    def terrain_ignore(self):
        return [a for a in unreal.GameplayStatics.get_all_actors_of_class(self.world,unreal.Actor)
                if a.get_actor_label()!="Continuous valley hills" and not a.get_actor_label().startswith("Terrain Ground_")]

    def hit_z(self,x,y,ignore):
        hit = unreal.SystemLibrary.line_trace_single(self.world,self.to_vec((x,y,140)),self.to_vec((x,y,-80)),
            unreal.TraceTypeQuery.ECC_VISIBILITY,True,ignore,unreal.DrawDebugTrace.NONE)
        return hit.to_tuple()[4].z/100 if hit else None

    def probe_perimeter(self):
        ignore = self.terrain_ignore()
        sample = [(x,y) for x in range(-192,33,2) for y in (127.75,320.25)]
        sample += [(x,y) for y in range(128,321,2) for x in (-192.25,32.25)]
        gaps = [(x,y) for x,y in sample if self.hit_z(x,y,ignore) is None]
        self.data.update(perimeter_samples=len(sample),missing_ground=gaps)
        assert not gaps,gaps[:5]
        body = unreal.GameplayStatics.get_all_actors_of_class(self.world,unreal.WaterBodyRiver)[0]
        spline = body.get_water_spline()
        depths = []
        for index,y in ((0,self.scene["BROOK_TOP"]),(-1,self.scene["BROOK_BOTTOM"])):
            key = index if index==0 else spline.get_number_of_spline_points()-1
            p = spline.get_location_at_spline_point(key,unreal.SplineCoordinateSpace.WORLD)
            assert abs(p.y/100-y)<.02
            # 수면 끝의 좌우 폭 전체가 지형 아래에 있는지 확인한다.
            width = self.scene["width"](y)
            for offset in (-width,0,width):
                z = self.hit_z(p.x/100+offset,y,ignore)
                assert z is not None
                depths.append(round(z-p.z/100,2))
        self.data["spline_ends_ground_above_water_m"] = depths
        assert min(depths)>1,depths
        self.walk.save()

    def change(self,mode):
        self.mode = mode
        self.walk.reset()

    def step(self):
        name,path = self.paths[self.section]
        if self.mode=="place":
            x,y = path[0]
            # 공간 이동은 검증 구역 간 준비에만 쓴다. 각 경계의 양방향 통과는 이동 입력으로 한다.
            self.p.character_movement.set_movement_mode(unreal.MovementMode.MOVE_FLYING)
            self.p.character_movement.stop_movement_immediately()
            self.p.set_actor_location(self.to_vec((x,y,self.scene["ground"](x,y)+2)),False,True)
            self.change("stream")
        elif self.mode=="stream":
            if time.monotonic()-self.walk.since<1.4:
                return
            x,y = path[0]
            z = self.hit_z(x,y,self.terrain_ignore())
            if z is None:
                return
            p = self.to_vec((x,y,z))
            p.z += self.p.capsule_component.get_scaled_capsule_half_height()+2
            self.p.set_actor_location(p,False,True)
            self.p.character_movement.set_movement_mode(unreal.MovementMode.MOVE_WALKING)
            self.change("settle")
        elif self.mode=="settle":
            if self.p.character_movement.is_falling() or time.monotonic()-self.walk.since<.35:
                return
            self.seq = path[1:]+path[-2::-1]
            self.i,self.falls,self.distance = 0,0,0.
            self.last = self.p.get_actor_location()
            self.change("walk")
        elif self.mode=="walk":
            p = self.p.get_actor_location()
            self.falls += int(self.p.character_movement.is_falling())
            self.distance += (p-self.last).length()/100
            self.last = p
            x,y = self.seq[self.i]
            dx,dy = x*100-p.x,y*100-p.y
            if math.hypot(dx,dy)<52:
                self.i+=1
                self.walk.reset()
                if self.i==2:
                    if name in ("north","south"):
                        tx = self.scene["creek"](path[-1][1])
                        dx,dy = tx*100-p.x,path[-1][1]*100-p.y
                    else:
                        dx,dy = path[-1][0]*100-p.x,0
                    self.ctrl.set_control_rotation(unreal.Rotator(yaw=math.degrees(math.atan2(dy,dx)),pitch=-10))
                    self.change("aim")
                elif self.i==len(self.seq):
                    self.data["walks"].append({"side":name,"metres":round(self.distance,2),
                        "falling_frames":self.falls,"returned_to":path[0]})
                    self.walk.save()
                    assert self.falls==0 and self.distance>28,(name,self.falls,self.distance)
                    self.section+=1
                    if self.section==len(self.paths):
                        self.walk.report["status"]="PASS"
                        unreal.log("GATE_FORK_PIE PASS "+str(self.out))
                        self.walk.finish()
                    else:
                        self.change("place")
                return
            self.ctrl.set_control_rotation(unreal.Rotator(yaw=math.degrees(math.atan2(dy,dx)),pitch=-9))
            self.walk.ei.inject_input_vector_for_action(self.walk.action,unreal.Vector(0,min(1,math.hypot(dx,dy)/115),0),[],[])
        elif self.mode=="aim" and time.monotonic()-self.walk.since>1.0:
            self.file = self.out/("edge-"+name+".png")
            unreal.SystemLibrary.execute_console_command(self.world,'Shot filename="'+self.file.as_posix()+'" -nosuffix',self.ctrl)
            self.change("capture")
        elif self.mode=="capture" and self.file.is_file() and self.file.stat().st_size>8000:
            self.walk.report["screens"].append(self.file.name)
            self.change("walk")
