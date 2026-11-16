"""Drive a real PIE character through the candidate loop using Enhanced Input.

No teleportation, speed changes, health edits or direct movement calls are used.
Start PIE at PlayerStart before running. Stop automatically on completion/stall.
"""
from pathlib import Path
import datetime
import json
import math
import time
import unreal

VELES_WALK = {}
VELES_WALK_HANDLE = None


def finish(status, reason):
    global VELES_WALK_HANDLE
    s=VELES_WALK
    report={k:v for k,v in s.items() if k in ('start_cm','waypoint','distance_m','capsule_cm','max_walk_speed_cm_s','samples','route_m','start_utc')}
    report.update(status=status,reason=reason,strength='MCP/EI',elapsed_seconds=time.monotonic()-s['started'],
                  route_waypoints=len(s['route']),end_cm=s.get('last_cm'),
                  limits='Continuous Enhanced Input loop; not a manual keyboard, combat, full-width or multiplayer test.')
    (s['out']/'walk.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(VELES_WALK_HANDLE)
    VELES_WALK_HANDLE=None
    s.clear()
    unreal.log('VELES_WALK '+status+' '+reason)


def tick(delta):
    s=VELES_WALK
    try:
        if not s:
            return
        now=time.monotonic()
        pos=s['pawn'].get_actor_location()
        current=[pos.x,pos.y,pos.z]
        s['distance_m']+=math.dist(current,s['last_cm'])/100
        s['last_cm']=current
        if math.dist(current,s['progress_cm'])>30:
            s['progress_cm']=current
            s['progress_at']=now
        target=s['route'][s['waypoint']]
        dx,dy=target[0]*100-pos.x,target[1]*100-pos.y
        distance=math.hypot(dx,dy)
        if distance<70:
            s['waypoint']+=1
            if s['waypoint']==len(s['route']):
                finish('PASS','Completed workshop, east approach, cave threshold, west return and workshop loop.')
                return
            target=s['route'][s['waypoint']]
            dx,dy=target[0]*100-pos.x,target[1]*100-pos.y
            distance=math.hypot(dx,dy)
        yaw=math.radians(s['controller'].get_control_rotation().yaw)
        forward=(dx*math.cos(yaw)+dy*math.sin(yaw))/max(1,distance)
        right=(-dx*math.sin(yaw)+dy*math.cos(yaw))/max(1,distance)
        s['input'].inject_input_vector_for_action(s['move'],unreal.Vector(right,forward,0),[],[])
        heading=math.degrees(math.atan2(dy,dx))
        turn=(heading-math.degrees(yaw)+180)%360-180
        s['input'].inject_input_vector_for_action(s['look'],unreal.Vector(max(-2,min(2,turn)),0,0),[],[])
        if now-s['sample_at']>=1:
            s['sample_at']=now
            s['samples']+=1
            probe={'seconds':now-s['started'],'waypoint':s['waypoint'],'position_cm':current,
                   'target_m':target,'distance_m':s['distance_m'],'falling':s['pawn'].character_movement.is_falling()}
            (s['out']/'walk-progress.json').write_text(json.dumps(probe),encoding='utf-8')
            with (s['out']/'probes.jsonl').open('a',encoding='utf-8') as f:
                f.write(json.dumps(probe)+'\n')
            with (s['out']/'inputs.jsonl').open('a',encoding='utf-8') as f:
                f.write(json.dumps({'seconds':probe['seconds'],'move':[right,forward],'look_yaw':max(-2,min(2,turn))})+'\n')
        if now-s['progress_at']>12:
            finish('FAIL','No position progress for 12 seconds; inspect walk-progress.json.')
        elif now-s['started']>1200:
            finish('FAIL','Twenty-minute wall-time limit reached.')
    except Exception as exc:
        finish('FAIL',str(exc))


def start():
    global VELES_WALK_HANDLE
    root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    data=json.loads((root/'Saved/WorldBuilding/VelesBlockout/layout.json').read_text(encoding='utf-8'))
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    assert world and world.get_name()=='VelesBlockout'
    controller=unreal.GameplayStatics.get_player_controller(world,0)
    pawn=unreal.GameplayStatics.get_player_pawn(world,0)
    assert pawn and pawn.get_class().get_name()=='CCLCharacter'
    actions=[o for o in unreal.ObjectIterator(unreal.InputAction) if o.get_outer()==controller and o.value_type==unreal.InputActionValueType.AXIS2D]
    move=next(o for o in actions if o.accumulation_behavior==unreal.InputActionAccumulationBehavior.CUMULATIVE)
    look=next(o for o in actions if o!=move)
    systems=[o for o in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if o.get_outer().get_class().get_name()=='CCLLocalPlayer']
    assert len(systems)==1,'Run this check with exactly one local PIE player.'
    main=next(r['points'] for r in data['roads'] if r['id']=='R01')
    route=main[:3]+data['route_in']+data['route_out'][1:]+list(reversed(main[:3]))
    current=pawn.get_actor_location()
    assert math.hypot(current.x-route[0][0]*100,current.y-route[0][1]*100)<200,'Start at the authored PlayerStart.'
    now=time.monotonic()
    out=root/'Saved/Tests/MCP-Veles-20261116'
    out.mkdir(parents=True,exist_ok=True)
    VELES_WALK.update(pawn=pawn,controller=controller,input=systems[0],move=move,look=look,
        route=route,waypoint=1,started=now,progress_at=now,sample_at=now,
        start_cm=[current.x,current.y,current.z],last_cm=[current.x,current.y,current.z],
        progress_cm=[current.x,current.y,current.z],distance_m=0,samples=0,out=out,
        capsule_cm=list(pawn.capsule_component.get_scaled_capsule_size()),
        max_walk_speed_cm_s=pawn.character_movement.max_walk_speed,
        route_m=sum(math.dist(a,b) for a,b in zip(route,route[1:])),
        start_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
    VELES_WALK_HANDLE=unreal.register_slate_post_tick_callback(tick)
    unreal.log('VELES_WALK START '+str(VELES_WALK['route_m'])+' metres')


start()
