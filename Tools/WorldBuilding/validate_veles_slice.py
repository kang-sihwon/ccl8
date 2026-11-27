"""Measure a single-player PIE route and camera orbits using Enhanced Input.

Run after StartPIE, once per run. No pawn teleportation or speed changes.
Wall frame intervals include editor overhead. The parallel engine CSV capture
provides actual frame/thread/GPU timings; do not infer FPS from Slate callbacks.
"""
from pathlib import Path
import builtins
import datetime
import json
import math
import time
import unreal


class VelesSliceRun:
    def __init__(self):
        root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
        self.world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        assert self.world and self.world.get_name() in ('VelesBlockout','VelesArtSlice','Veles','VelesVillage')
        self.name=self.world.get_name()
        self.with_quay=self.name in ('Veles','VelesVillage')
        parent=root/'Saved/Tests' if self.name!='VelesVillage' else root/'Saved/Tests/VelesSettlement'
        self.out=parent/('MCP-VelesSlice-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'-'+self.name)
        self.out.mkdir(parents=True,exist_ok=False)
        self.pc=unreal.GameplayStatics.get_player_controller(self.world,0)
        self.pawn=unreal.GameplayStatics.get_player_pawn(self.world,0)
        assert isinstance(self.pawn,unreal.CCLCharacter)
        if self.with_quay:
            unreal.log('VELES_PAWN '+self.pawn.get_class().get_name()+' mesh='+str(self.pawn.mesh.get_skeletal_mesh_asset()))
        actions=[a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==self.pc and a.value_type==unreal.InputActionValueType.AXIS2D]
        self.move=next(a for a in actions if a.accumulation_behavior==unreal.InputActionAccumulationBehavior.CUMULATIVE)
        self.look=next(a for a in actions if a!=self.move)
        systems=[s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_outer().get_class().get_name()=='CCLLocalPlayer']
        assert len(systems)==1
        self.input=systems[0]
        data=json.loads((root/'Saved/WorldBuilding/VelesBlockout/layout.json').read_text(encoding='utf-8'))
        route=next(r['points'] for r in data['roads'] if r['id']=='R01')[:4]
        self.route=route+list(reversed(route[:-1]))
        self.orbit_stops={0:'workshop',1:'waterpoint',3:'forestgate',6:'return'}
        if self.with_quay:
            quay_road=next(r['points'] for r in data['roads'] if r['id']=='V-WATER')
            branch=[[-48,80],*quay_road,[-32,14.6]]
            self.route=[*route[:2],*branch,*reversed(branch[:-1]),*route[1:],*reversed(route[:-1])]
            self.orbit_stops={0:'workshop',1:'waterpoint',1+len(branch):'quay',
                              len(self.route)-4:'forestgate',len(self.route)-1:'return'}
        p=self.pawn.get_actor_location()
        assert math.hypot(p.x-route[0][0]*100,p.y-route[0][1]*100)<200
        self.started=self.last=self.progress_time=time.monotonic()
        self.last_pos=self.progress_pos=[p.x,p.y,p.z]
        self.index=0
        self.phase='warmup'
        self.phase_start=self.started
        self.frames=[]
        self.probes=[]
        self.distance=0
        self.sample_at=0
        self.orbits=set()
        self.orbit_yaw=0
        self.orbit_previous_yaw=0
        self.orbit_turns={}
        self.captured=set()
        self.fall_samples=0
        self.csv_started=False
        self.active_seconds=0
        self.still_frames=0
        self.gpu_csv_original=unreal.SystemLibrary.get_console_variable_int_value('r.GPUCsvStatsEnabled')
        self.settings={n:unreal.SystemLibrary.get_console_variable_float_value(n) for n in ('t.MaxFPS','r.VSync','r.ScreenPercentage','sg.ViewDistanceQuality','sg.ShadowQuality','sg.FoliageQuality','r.DynamicRes.OperationMode')}
        self.handle=unreal.register_slate_post_tick_callback(self.tick)
        (root/'Saved/WorldBuilding/VelesBlockout/active-validation.txt').write_text(str(self.out),encoding='utf-8')
        unreal.log('VELES_SLICE_RUN START '+str(self.out))

    def tick(self,delta):
        try:
            now=time.monotonic()
            dt=now-self.last
            self.last=now
            self.active_seconds+=min(dt,.25)
            if self.with_quay and dt>.5:
                self.progress_time+=dt
                self.phase_start+=dt
            p=self.pawn.get_actor_location()
            pos=[p.x,p.y,p.z]
            self.distance+=math.dist(pos,self.last_pos)/100
            self.last_pos=pos
            if p.z < -2000:
                self.finish('FAIL','Pawn left the authored ground vertically.')
                return
            if math.dist(pos,self.progress_pos)>30:
                self.progress_pos=pos
                self.progress_time=now
            if self.phase=='warmup':
                if now-self.phase_start>15:
                    if not self.with_quay:
                        unreal.SystemLibrary.execute_console_command(self.world,'r.GPUCsvStatsEnabled 1')
                        unreal.SystemLibrary.execute_console_command(self.world,'csvprofile start')
                        self.csv_started=True
                    self.phase='orbit'
                    self.phase_start=now
                    self.orbit_yaw=self.pc.get_control_rotation().yaw
                    self.orbit_previous_yaw=self.orbit_yaw
                return
            self.frames.append({'wall_ms':dt*1000,'slate_ms':delta*1000,'phase':self.phase,'waypoint':self.index})
            yaw=self.pc.get_control_rotation().yaw
            if self.phase=='orbit':
                self.orbit_turns[self.index]=self.orbit_turns.get(self.index,0)+(yaw-self.orbit_previous_yaw+180)%360-180
                self.orbit_previous_yaw=yaw
                elapsed=now-self.phase_start
                if self.with_quay and self.index not in self.captured:
                    if elapsed>90:
                        self.finish('FAIL','Camera did not settle for the landmark screenshot.')
                        return
                    location=self.orbit_stops[self.index]
                    headings={'workshop':90,'waterpoint':325,'quay':338,'forestgate':58,'return':80}
                    error=(headings[location]-yaw+180)%360-180
                    if abs(error)>1:
                        self.still_frames=0
                        self.input.inject_input_vector_for_action(self.look,unreal.Vector(max(-4,min(4,error*.25)),0,0),[],[])
                    else:
                        self.still_frames+=1
                    if self.still_frames==16:
                        screenshot=(self.out/(location+'.png')).as_posix()
                        unreal.SystemLibrary.execute_console_command(self.world,'shot filename="'+screenshot+'" -nosuffix')
                    if self.still_frames>=24:
                        self.captured.add(self.index)
                        self.still_frames=0
                        self.phase_start=now
                        self.orbit_yaw=self.orbit_previous_yaw=yaw
                        self.orbit_turns[self.index]=0
                    return
                rotated=self.orbit_turns[self.index]
                target=self.orbit_yaw+min(360,rotated+12) if self.with_quay else self.orbit_yaw+min(360,elapsed*30)
                turn=(target-yaw+180)%360-180
                self.input.inject_input_vector_for_action(self.look,unreal.Vector(max(-3,min(3,turn*.1)),0,0),[],[])
                orbit_finished=elapsed>=14
                if self.with_quay:
                    orbit_finished=abs(rotated-360)<=3 or elapsed>90
                if orbit_finished:
                    if abs(self.orbit_turns[self.index]-360)>3:
                        self.finish('FAIL','Camera did not complete the requested 360 degree orbit.')
                        return
                    self.orbits.add(self.index)
                    self.index+=1
                    if self.index>=len(self.route):
                        self.finish('PASS','Completed authored out-and-back route and camera orbits.')
                        return
                    self.phase='walk'
                    self.progress_time=now
            else:
                target=self.route[self.index]
                dx,dy=target[0]*100-p.x,target[1]*100-p.y
                dist=math.hypot(dx,dy)
                if dist<70:
                    if self.index in self.orbit_stops:
                        self.phase='orbit'
                        self.phase_start=now
                        self.orbit_yaw=yaw
                        self.orbit_previous_yaw=yaw
                    else:
                        self.index+=1
                else:
                    r=math.radians(yaw)
                    f=(dx*math.cos(r)+dy*math.sin(r))/dist
                    side=(-dx*math.sin(r)+dy*math.cos(r))/dist
                    self.input.inject_input_vector_for_action(self.move,unreal.Vector(side,f,0),[],[])
                    heading=math.degrees(math.atan2(dy,dx))
                    turn=(heading-yaw+180)%360-180
                    self.input.inject_input_vector_for_action(self.look,unreal.Vector(max(-2,min(2,turn*.1)),0,0),[],[])
                    if now-self.progress_time>15:
                        self.finish('FAIL','No movement progress for 15 seconds.')
                        return
            if now-self.sample_at>=.25:
                self.sample_at=now
                cam=self.pawn.get_component_by_class(unreal.CameraComponent)
                cp=cam.get_world_location()
                falling=self.pawn.character_movement.is_falling()
                self.fall_samples+=int(falling)
                # A read-only overlap catches a camera origin inside blocking geometry.
                overlaps=unreal.SystemLibrary.sphere_trace_single(self.world,cp,cp+unreal.Vector(0,0,.1),5,
                    unreal.TraceTypeQuery.ECC_CAMERA,False,[self.pawn],unreal.DrawDebugTrace.NONE)
                probe={'seconds':now-self.started,'phase':self.phase,'waypoint':self.index,'pawn_cm':pos,
                    'camera_cm':[cp.x,cp.y,cp.z],'yaw':yaw,'falling':falling,
                    'camera_overlap':str(overlaps.to_tuple()[9].get_name()) if overlaps else None}
                self.probes.append(probe)
                (self.out/'progress.json').write_text(json.dumps(probe),encoding='utf-8')
            if (self.active_seconds if self.with_quay else now-self.started)>600:
                self.finish('FAIL','Ten-minute limit.')
        except Exception as exc:
            self.finish('FAIL',repr(exc))

    def finish(self,status,reason):
        unreal.unregister_slate_post_tick_callback(self.handle)
        if self.csv_started:
            unreal.SystemLibrary.execute_console_command(self.world,'csvprofile stop')
        unreal.SystemLibrary.execute_console_command(self.world,'r.GPUCsvStatsEnabled '+str(self.gpu_csv_original))
        def stats(values):
            v=sorted(values)
            if not v:return {}
            return {'count':len(v),'mean_ms':sum(v)/len(v),'p50_ms':v[len(v)//2],
                'p95_ms':v[min(len(v)-1,int(len(v)*.95))],'p99_ms':v[min(len(v)-1,int(len(v)*.99))],
                'max_ms':v[-1],'over_50ms':sum(x>50 for x in v)}
        result={'status':status,'reason':reason,'map':self.name,'strength':'PIE/EI' if self.with_quay else 'MCP/EI',
            'elapsed_s':time.monotonic()-self.started,'distance_m':self.distance,'waypoint':self.index,
            'route_m':sum(math.dist(a,b) for a,b in zip(self.route,self.route[1:])),
            'orbits':sorted(self.orbits),'fall_samples':self.fall_samples,
            'orbit_turn_degrees':self.orbit_turns,
            'camera_overlap_samples':sum(bool(p['camera_overlap']) for p in self.probes),
            'settings':self.settings,'wall_frame':stats([f['wall_ms'] for f in self.frames]),
            'engine_csv_started':self.csv_started,
            'walk_wall_frame':stats([f['wall_ms'] for f in self.frames if f['phase']=='walk']),
            'limits':'Editor Slate wall-frame intervals, includes tool/editor overhead; not packaged GPU timing, manual play, combat or full-width clearance.'}
        (self.out/'result.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        (self.out/'frames.json').write_text(json.dumps(self.frames),encoding='utf-8')
        (self.out/'probes.json').write_text(json.dumps(self.probes),encoding='utf-8')
        unreal.log('VELES_SLICE_RUN '+json.dumps(result))
        builtins._veles_slice_run=None
        self.world=self.pc=self.pawn=self.input=self.move=self.look=None


assert not getattr(builtins,'_veles_slice_run',None),'A slice run is already active.'
builtins._veles_slice_run=VelesSliceRun()
