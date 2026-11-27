"""Read-only ground, route clearance, save-state and input preservation checks."""
from pathlib import Path
import hashlib
import json
import math
import unreal


def audit():
    root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    data=json.loads((root/'Saved/WorldBuilding/VelesBlockout/layout.json').read_text(encoding='utf-8'))
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_name() in ('VelesArtSlice','Veles','VelesVillage')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    route=next(r['points'] for r in data['roads'] if r['id']=='R01')[:4]
    segments=list(zip(route,route[1:]))
    if world.get_name() in ('Veles','VelesVillage'):
        ground=next(a for a in actors if a.get_actor_label()=='VelesTerrain')
        floors=[a for a in actors if a==ground or a.get_actor_label() in ('QuayBoardwalk','Landing')]
        blockers=[a for a in actors if isinstance(a,unreal.StaticMeshActor) and a.get_actor_enable_collision() and a not in floors]
        quay_road=next(r['points'] for r in data['roads'] if r['id']=='V-WATER')
        branch=[route[1],[-48,80],*quay_road,[-32,14.6]]
        segments.extend(zip(branch,branch[1:]))
    else:
        ground=next(a for a in actors if a.get_actor_label()=='VB_Terrain')
        floors=[a for a in actors if a==ground or a.get_actor_label().startswith(('VB_Bridge','VA_Apron_'))]
        blockers=[a for a in actors if a.get_actor_label().startswith('VA_') and a.get_actor_enable_collision() and a not in floors]
    ignore_ground=[a for a in actors if a not in floors]
    ignore_blockers=[a for a in actors if a not in blockers]
    failures=[];samples=[]
    for a,b in segments:
        length=math.dist(a,b);steps=max(1,math.ceil(length/2))
        nx=-(b[1]-a[1])/length;ny=(b[0]-a[0])/length
        for i in range(steps+1):
            for offset in (-1,0,1):
                x=a[0]+(b[0]-a[0])*i/steps+nx*offset
                y=a[1]+(b[1]-a[1])*i/steps+ny*offset
                hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x*100,y*100,20000),unreal.Vector(x*100,y*100,-10000),
                    unreal.TraceTypeQuery.ECC_VISIBILITY,False,ignore_ground,unreal.DrawDebugTrace.NONE)
                if not hit:
                    failures.append({'kind':'missing_upward_ground','xy_m':[x,y]});continue
                v=hit.to_tuple();z=v[5].z
                if v[7].z<.7:failures.append({'kind':'ground_normal','xy_m':[x,y],'normal_z':v[7].z})
                p=unreal.Vector(x*100,y*100,z+99)
                obstruction=unreal.SystemLibrary.capsule_trace_single(world,p,p+unreal.Vector(0,0,.1),42,96,
                    unreal.TraceTypeQuery.ECC_VISIBILITY,False,ignore_blockers,unreal.DrawDebugTrace.NONE)
                if obstruction:
                    failures.append({'kind':'capsule_obstruction','xy_m':[x,y],'actor':obstruction.to_tuple()[9].get_actor_label()})
                samples.append({'xy_m':[x,y],'z_cm':z,'normal_z':v[7].z})
    preserved=all(hashlib.sha256((root/p).read_bytes()).hexdigest()==sha for p,sha in data['source_sha256'].items())
    result={'status':'PASS' if not failures and preserved else 'FAIL','strength':'internal',
        'ground_samples':len(samples),'corridor_offsets_m':[-1,0,1],'capsule_cm':[42,96],
        'sources_preserved':preserved,'failures':failures,'actor_count':len(actors),
        'terrain_mesh':ground.static_mesh_component.static_mesh.get_path_name(),
        'limits':'Three sampled lines, not every point of the corridor or combat navigation.'}
    filename={'Veles':'veles-audit.json','VelesVillage':'village-audit.json','VelesArtSlice':'art-audit.json'}[world.get_name()]
    out=root/'Saved/WorldBuilding/VelesBlockout'/filename
    out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    unreal.log('VELES_ART_AUDIT '+json.dumps(result))


audit()
