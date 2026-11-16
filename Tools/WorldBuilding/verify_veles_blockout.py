"""Read-only editor collision audit; run with the Veles blockout loaded."""
from pathlib import Path
import hashlib
import json
import math
import unreal


def verify():
    root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    data = json.loads((root/'Saved/WorldBuilding/VelesBlockout/layout.json').read_text(encoding='utf-8'))
    out = root/'Saved/Tests/MCP-Veles-20261116'
    out.mkdir(parents=True,exist_ok=True)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_name() == 'VelesBlockout',world.get_name()
    all_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    labels = {a.get_actor_label():a for a in all_actors}
    missing = [s['label'] for s in data['actors'] if s['label'] not in labels]
    blockers = [a for name,a in labels.items() if name.startswith(('VB_Building_','VB_Cover_','VB_Cave_','VB_TreeProxy_'))]
    ignored = [a for a in all_actors if a not in blockers]
    # The project character uses a 42 cm radius, 96 cm half height capsule.
    radius, half = 42.0,96.0
    routes = {r['id']:r['points'] for r in data['roads'] if r['id'] in ('R01','R02','R03','R04','V-WATER')}
    routes['RETURN'] = data['route_out']
    routes['PIER'] = [[-46,20],[0,0]]
    hits, failures, summaries = [],[],{}
    for name, points in routes.items():
        samples=[]
        for a,b in zip(points,points[1:]):
            n=max(1,math.ceil(math.dist(a,b)/2))
            samples.extend([[a[k]+(b[k]-a[k])*j/n for k in range(2)] for j in range(n)])
        samples.append(points[-1])
        route_hits=[]
        for p in samples:
            hit = unreal.SystemLibrary.line_trace_single(world,unreal.Vector(p[0]*100,p[1]*100,20000),
                unreal.Vector(p[0]*100,p[1]*100,-10000),unreal.TraceTypeQuery.ECC_VISIBILITY,
                False,blockers,unreal.DrawDebugTrace.NONE)
            if hit is None:
                failures.append({'route':name,'point':p,'kind':'missing_ground'})
                continue
            values=hit.to_tuple()
            loc, normal, actor = values[5],values[7],values[9]
            route_hits.append({'xy':p,'z_cm':loc.z,'actor':actor.get_actor_label(),'normal_z':normal.z})
            if normal.z < .707:
                failures.append({'route':name,'point':p,'kind':'steep_ground','normal_z':normal.z})
        obstructions=[]
        for a,b in zip(route_hits,route_hits[1:]):
            start=unreal.Vector(a['xy'][0]*100,a['xy'][1]*100,a['z_cm']+half+3)
            end=unreal.Vector(b['xy'][0]*100,b['xy'][1]*100,b['z_cm']+half+3)
            hit=unreal.SystemLibrary.capsule_trace_single(world,start,end,radius,half,
                unreal.TraceTypeQuery.ECC_VISIBILITY,False,ignored,unreal.DrawDebugTrace.NONE)
            if hit is not None:
                actor=hit.to_tuple()[9]
                obstructions.append({'point':a['xy'],'actor':actor.get_actor_label()})
        failures.extend({'route':name,'kind':'capsule_obstruction',**v} for v in obstructions)
        summaries[name]={'samples':len(samples),'ground_hits':len(route_hits),'obstructions':len(obstructions),
            'max_adjacent_height_delta_cm':max((abs(a['z_cm']-b['z_cm']) for a,b in zip(route_hits,route_hits[1:])),default=0)}
        hits.append({'route':name,'samples':route_hits})
    preserved=all(hashlib.sha256((root/p).read_bytes()).hexdigest()==sha for p,sha in data['source_sha256'].items())
    result={'status':'PASS' if not failures and not missing and preserved else 'FAIL','strength':'internal',
        'actor_count':len(all_actors),'expected_data_actors':len(data['actors']),'missing':missing,
        'sources_preserved':preserved,'capsule_cm':{'radius':radius,'half_height':half},
        'routes':summaries,'failures':failures,
        'limits':'Ground rays and obstacle sweeps do not prove CharacterMovement step traversal or full-width clearance.'}
    (out/'collision.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    (out/'ground-samples.json').write_text(json.dumps(hits),encoding='utf-8')
    unreal.log('VELES_COLLISION '+json.dumps({k:v for k,v in result.items() if k!='failures'}))


verify()
