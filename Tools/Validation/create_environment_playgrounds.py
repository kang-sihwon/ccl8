"""Create two independent World Partition experiment maps without replacing user maps.

Run with UnrealEditor-Cmd CCL.uproject -run=pythonscript -script=<this file>.
Existing maps are preserved. Definitions and shared assets are created only when absent.
Run verify_environment_playgrounds.py in a fresh editor process after generation.
"""
from pathlib import Path
import ctypes
import json
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

font_path = '/Game/Environment/Fonts/F_EnvironmentLabel'
font = unreal.load_asset(font_path) if assets.does_asset_exist(font_path) else None
if not font:
    font_file = str(Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_content_dir())) / 'Editor/Slate/Fonts/NanumGothic.ttf')
    source = ''.join(p.read_text(encoding='utf-8-sig') for p in (root / 'Source/CCL/Environment').glob('*.cpp'))
    chars = ''.join(chr(i) for i in range(32, 127)) + ''.join(sorted({c for c in source if '\uac00' <= c <= '\ud7a3'})) + '→↑·'
    assert ctypes.windll.gdi32.AddFontResourceExW(font_file, 0x10, 0), 'Bundled font registration failed'
    try:
        factory = unreal.TrueTypeFontFactory()
        options = factory.get_editor_property('import_options')
        data = options.get_editor_property('data')
        for key, value in dict(font_name='NanumGothic', height=24.0, chars=chars, include_ascii_range=True,
                               texture_page_width=2048, texture_page_max_height=2048,
                               use_distance_field_alpha=True, distance_field_scale_factor=4).items():
            data.set_editor_property(key, value)
        options.set_editor_property('data', data)
        font = tools.create_asset('F_EnvironmentLabel', '/Game/Environment/Fonts', unreal.Font, factory)
        assert font and assets.save_loaded_asset(font, False), 'Environment font save failed'
    finally:
        ctypes.windll.gdi32.RemoveFontResourceExW(font_file, 0x10, 0)

def material(name, color):
    path = '/Game/Environment/Materials/' + name
    if assets.does_asset_exist(path):
        return unreal.load_asset(path)
    value = tools.create_asset(name, '/Game/Environment/Materials', unreal.Material, unreal.MaterialFactoryNew())
    node = unreal.MaterialEditingLibrary.create_material_expression(value, unreal.MaterialExpressionConstant3Vector, -300, 0)
    node.set_editor_property('constant', unreal.LinearColor(*color, 1.0))
    assert unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(value, unreal.MaterialExpressionConstant, -300, 160)
    roughness.set_editor_property('r', 0.85)
    assert unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(value)
    assert assets.save_loaded_asset(value, False)
    return value

floor_mat = material('M_EnvironmentFloor', (0.075, 0.105, 0.15))
ready_mat = material('M_EnvironmentReady', (0.08, 0.28, 0.38))
reserved_mat = material('M_EnvironmentReserved', (0.18, 0.19, 0.22))
line_mat = material('M_EnvironmentPath', (0.48, 0.62, 0.64))
for surface in (floor_mat, ready_mat, reserved_mat, line_mat):
    unreal.MaterialEditingLibrary.set_material_usage(surface, unreal.MaterialUsage.MATUSAGE_NANITE)
    unreal.MaterialEditingLibrary.recompile_material(surface)
    assert assets.save_loaded_asset(surface, False)

mesh_path = '/Game/Environment/SM_EnvironmentBlock'
mesh = unreal.load_asset(mesh_path) if assets.does_asset_exist(mesh_path) else None
if not mesh:
    mesh = assets.duplicate_asset('/Engine/BasicShapes/Cube', mesh_path)
    settings = mesh.get_editor_property('nanite_settings')
    settings.set_editor_property('enabled', True)
    mesh.set_editor_property('nanite_settings', settings)
    assert assets.save_loaded_asset(mesh, False)

definitions = []
for index in range(12):
    name = f'DA_Environment_{index:02d}'
    path = '/Game/Environment/Experiments/' + name
    definition = unreal.load_asset(path) if assets.does_asset_exist(path) else None
    if not definition:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', unreal.CCLExperimentDefinition)
        definition = tools.create_asset(name, '/Game/Environment/Experiments', unreal.CCLExperimentDefinition, factory)
        assert definition, path
        definition.configure_zone(index)
        assert assets.save_loaded_asset(definition, False)
    definitions.append(definition)

def block(label, location, scale, surface):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    assert actor, label
    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_material(0, surface)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor

reports = []
for name in ('EnvironmentPlayground', 'EnvironmentScenario'):
    path = '/Game/Maps/' + name
    if not assets.does_asset_exist(path):
        assert levels.new_level(path, True), path
        block('EnvironmentBase', (3750, 2500, -70), (100, 75, 1), floor_mat)
        for index, definition in enumerate(definitions):
            x, y = (index % 4) * 2500, (index // 4) * 2500
            surface = ready_mat if index in (0, 1, 8) else reserved_mat
            block(f'Zone_{index:02d}_Platform', (x, y, -10), (16, 16, 0.2), surface)
            block(f'Zone_{index:02d}_SignStand', (x + 720, y, 100), (0.5, 14, 2), floor_mat)
            station = actors.spawn_actor_from_class(unreal.CCLExperimentStation, unreal.Vector(x + 680, y, 0))
            assert station
            station.set_actor_label(f'{index:02d}_EnvironmentStation')
            station.set_editor_property('definition', definition)
            station.set_actor_rotation(unreal.Rotator(pitch=0, yaw=180, roll=0), False)
            station.refresh_label()
        for y in (1250, 3750):
            block('EastWestConnection', (3750, y, -16), (92, 2, 0.1), line_mat)
        for x in (1250, 3750, 6250):
            block('NorthSouthConnection', (x, 2500, -16), (2, 66, 0.1), line_mat)
        for index in range(4):
            start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-550, -450 + index * 180, 130))
            start.set_actor_label(f'EnvironmentStart_{index}')
        director = actors.spawn_actor_from_class(unreal.CCLExperimentDirector, unreal.Vector(0, 0, 0))
        director.set_editor_property('definitions', definitions)
        director.set_actor_label('EnvironmentExperimentDirector')
        sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1200))
        sun.set_actor_rotation(unreal.Rotator(pitch=-55, yaw=-25, roll=0), False)
        sun.light_component.set_editor_property('intensity', 4.0)
        sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 800))
        sky.light_component.set_editor_property('intensity', 1.0)
        actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property('default_game_mode', unreal.CCLExperimentGameMode)
        world.get_world_settings().set_editor_property('kill_z', -1500.0)
        assert unreal.CCLCombatAssetLibrary.configure_combat_world(), 'World Partition setup failed'
        assert levels.save_current_level()
        assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    reports.append(dict(map=path, saved=assets.does_asset_exist(path)))
result_path = root / 'Saved/EnvironmentGoal/experiment-creation.json'
result_path.write_text(json.dumps(dict(maps=reports, new_maps_preserve_existing=True, reload_verification='verify_environment_playgrounds.py'), indent=2), encoding='utf-8')
unreal.log('CCL_EXPERIMENT_CREATED maps=2; verify saved actor content in a fresh editor')
