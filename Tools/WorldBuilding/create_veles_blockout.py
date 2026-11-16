"""Run in the UE editor Python console after prepare_veles_blockout.py.

Only creates/updates this task's labelled actors in its dedicated map. Other maps,
original atlas sources and existing shared materials are never changed.
"""
from pathlib import Path
import json
import math
import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
OUT = ROOT / 'Saved/WorldBuilding/VelesBlockout'
DATA = json.loads((OUT / 'layout.json').read_text(encoding='utf-8'))
MAP = '/Game/Maps/VelesBlockout'
ASSET_ROOT = '/Game/WorldBuilding/VelesBlockout'
assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
OWNER = 'VelesBlockout_20261116'


def material(name, color):
    path = ASSET_ROOT + '/M_' + name
    if assets.does_asset_exist(path):
        return unreal.load_asset(path)
    value = tools.create_asset('M_' + name, ASSET_ROOT, unreal.Material, unreal.MaterialFactoryNew())
    node = unreal.MaterialEditingLibrary.create_material_expression(value, unreal.MaterialExpressionConstant3Vector, -300, 0)
    node.set_editor_property('constant', unreal.LinearColor(*color, 1.0))
    assert unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(value, unreal.MaterialExpressionConstant, -300, 140)
    rough.set_editor_property('r', .9)
    assert unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    value.set_editor_property('two_sided', True)
    unreal.MaterialEditingLibrary.recompile_material(value)
    assert assets.save_loaded_asset(value, False)
    return value


def build():
    if assets.does_asset_exist(MAP):
        # Loading a World as an asset and retaining its Python wrapper across
        # LoadLevel pins the package during Map_Load garbage collection.
        assert levels.load_level(MAP)
        current = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        if assets.get_metadata_tag(current, 'BlockoutOwner') != OWNER:
            raise RuntimeError('Existing map is not owned by this task; refusing replacement.')
    else:
        assert levels.new_level(MAP, False)
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        assets.set_metadata_tag(world, 'BlockoutOwner', OWNER)
        assert levels.save_current_level()
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    by_label = {a.get_actor_label():a for a in actors.get_all_level_actors()}
    colors = {'Terrain':(.12,.19,.13),'Road':(.48,.35,.19),'Building':(.46,.47,.44),
              'Cover':(.18,.22,.25),'Marker':(.9,.51,.06),'Pad':(.28,.43,.38),
              'Bridge':(.31,.20,.11),'Water':(.035,.22,.30)}
    materials = {name:material(name,c) for name,c in colors.items()}
    mesh_path = ASSET_ROOT + '/SM_Terrain'
    if assets.does_asset_exist(mesh_path):
        terrain = unreal.load_asset(mesh_path)
    else:
        dynamic = unreal.DynamicMesh()
        buffers = unreal.GeometryScriptSimpleMeshBuffers()
        buffers.set_editor_property('vertices', [unreal.Vector(*v) for v in DATA['terrain']['vertices']])
        buffers.set_editor_property('normals', [unreal.Vector(0,0,1) for v in DATA['terrain']['vertices']])
        buffers.set_editor_property('uv0', [unreal.Vector2D(v[0]/1000,v[1]/1000) for v in DATA['terrain']['vertices']])
        buffers.set_editor_property('triangles', [unreal.IntVector(*v) for v in DATA['terrain']['triangles']])
        unreal.GeometryScript_MeshEdits.append_buffers_to_mesh(dynamic, buffers)
        opts = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
        opts.set_editor_property('enable_recompute_normals', True)
        opts.set_editor_property('enable_recompute_tangents', True)
        opts.set_editor_property('enable_collision', True)
        opts.set_editor_property('collision_mode', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        terrain, outcome = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(dynamic,mesh_path,opts)
        assert terrain, str(outcome)
        assert assets.save_loaded_asset(terrain,False)
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')

    def get_actor(label, cls, location):
        value = by_label.get(label)
        if value is None:
            value = actors.spawn_actor_from_class(cls, unreal.Vector(*location))
            assert value, label
            value.set_actor_label(label)
            by_label[label] = value
        return value

    ground = get_actor('VB_Terrain',unreal.StaticMeshActor,[0,0,0])
    ground.static_mesh_component.set_static_mesh(terrain)
    ground.static_mesh_component.set_material(0,materials['Terrain'])
    ground.set_folder_path('Veles/Terrain')
    for i, spec in enumerate(DATA['actors']):
        actor = get_actor(spec['label'],unreal.StaticMeshActor,spec['location'])
        actor.set_actor_location(unreal.Vector(*spec['location']),False,False)
        actor.set_actor_rotation(unreal.Rotator(pitch=spec['pitch'],yaw=spec['yaw'],roll=spec.get('roll',0)),False)
        actor.set_actor_scale3d(unreal.Vector(*spec['scale']))
        actor.static_mesh_component.set_static_mesh(cube)
        actor.static_mesh_component.set_material(0,materials[spec['surface']])
        actor.set_actor_enable_collision(spec['collision'])
        actor.set_folder_path('Veles/'+spec['folder'])
        if i % 300 == 0:
            unreal.log('VELES_BUILD progress '+str(i))
    start = get_actor('VB_PlayerStart',unreal.PlayerStart,DATA['player_start_cm'])
    start.set_folder_path('Veles/Gameplay')
    sun = get_actor('VB_Sun',unreal.DirectionalLight,[0,0,10000])
    sun.set_actor_rotation(unreal.Rotator(pitch=-48,yaw=-35,roll=0),False)
    sun.light_component.set_editor_property('intensity',6.0)
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky = get_actor('VB_SkyLight',unreal.SkyLight,[0,0,0])
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property('real_time_capture',True)
    get_actor('VB_SkyAtmosphere',unreal.SkyAtmosphere,[0,0,0])
    exposure = get_actor('VB_Exposure',unreal.PostProcessVolume,[0,0,0])
    exposure.set_editor_property('unbound',True)
    post = exposure.get_editor_property('settings')
    post.set_editor_property('override_auto_exposure_min_brightness',True)
    post.set_editor_property('override_auto_exposure_max_brightness',True)
    post.set_editor_property('override_auto_exposure_bias',True)
    post.set_editor_property('auto_exposure_min_brightness',1.0)
    post.set_editor_property('auto_exposure_max_brightness',1.0)
    post.set_editor_property('auto_exposure_bias',0.0)
    exposure.set_editor_property('settings',post)
    exposure.set_folder_path('Veles/Lighting')
    for a in (sun,sky,by_label['VB_SkyAtmosphere']):
        a.set_folder_path('Veles/Lighting')
    for poi in DATA['pois']:
        if not poi['visible'] or poi['surface'] == 'underground':
            continue
        # Labels identify reference locations, not finished buildings or gameplay triggers.
        label = get_actor('VB_POI_'+poi['id'],unreal.TextRenderActor,
                          [poi['point'][0]*100,poi['point'][1]*100,(poi['z_m']+4)*100])
        label.text_render.set_text(poi['id'].upper().replace('_',' '))
        label.text_render.set_editor_property('world_size',150.0)
        label.set_actor_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0),False)
        label.set_folder_path('Veles/ReferenceLabels')
    # Five-metre threshold with a clearly provisional opening, no underground room.
    cave = next(p for p in DATA['pois'] if p['id']=='cave')
    approach=next(r for r in DATA['roads'] if r['id']=='R03')['points'][-2]
    east=next(r for r in DATA['roads'] if r['id']=='R01')['points'][-2]
    angles=[math.atan2(cave['point'][1]-p[1],cave['point'][0]-p[0]) for p in (approach,east)]
    gate_yaw=sum(angles)/2+math.pi/2
    for name,offset,scale in [('Left',[-4,0,2],[3,4,4]),('Right',[4,0,2],[3,4,4]),('Lintel',[0,0,4.5],[11,4,1])]:
        location=[(cave['point'][0]+offset[0]*math.cos(gate_yaw))*100,
                  (cave['point'][1]+offset[0]*math.sin(gate_yaw))*100,(cave['z_m']+offset[2])*100]
        actor=get_actor('VB_Cave_'+name,unreal.StaticMeshActor,
                        location)
        actor.set_actor_location(unreal.Vector(*location),False,False)
        actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=math.degrees(gate_yaw),roll=0),False)
        actor.static_mesh_component.set_static_mesh(cube)
        actor.static_mesh_component.set_material(0,materials['Cover'])
        actor.set_actor_scale3d(unreal.Vector(*scale))
        actor.set_folder_path('Veles/CaveEntrance')
    settings=world.get_world_settings()
    settings.set_editor_property('default_game_mode',unreal.CCLGameModeBase)
    settings.set_editor_property('kill_z',-100000.0)
    assert levels.save_current_level()
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
        unreal.Vector(-48000,-65000,85000),unreal.Rotator(pitch=-34,yaw=53,roll=0))
    report={'map':MAP,'actor_count':len(actors.get_all_level_actors()),'data_actor_count':len(DATA['actors']),
            'terrain_vertices':len(DATA['terrain']['vertices']),'terrain_triangles':len(DATA['terrain']['triangles']),
            'limitations':DATA['limitations'],'status':'saved_not_yet_play_verified'}
    (OUT/'build.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log('VELES_BUILD SAVED '+json.dumps(report))


build()
