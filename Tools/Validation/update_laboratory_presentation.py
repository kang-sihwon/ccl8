"""Update dedicated laboratory presentation assets in an idle Unreal Editor."""
from pathlib import Path
import json
import unreal

A = unreal.EditorAssetLibrary
L = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
E = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
out = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / 'Laboratories'
assert not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world(), 'Stop PIE before editing maps'

assert L.load_level('/Game/Maps/Tests/Weather')
weather = next(a for a in E.get_all_level_actors() if isinstance(a, unreal.CCLWeatherPresentation))
weather_paths = {k: weather.get_editor_property(k).get_path_name() for k in ('rain_system', 'snow_system', 'cloud_material') if weather.get_editor_property(k)}
for name in ('Snow', 'Water', 'Terrain', 'Vegetation', 'Cycle', 'Fire', 'Shelter'):
    assert L.load_level('/Game/Maps/Tests/' + name)
    actors = E.get_all_level_actors()
    config = next(a for a in actors if isinstance(a, unreal.CCLWorldEnvironmentConfig))
    actor = next((a for a in actors if isinstance(a, unreal.CCLWeatherPresentation)), None)
    if actor is None:
        actor = E.spawn_actor_from_class(unreal.CCLWeatherPresentation, config.get_actor_location())
        actor.set_actor_label('LaboratoryWeather')
    for key, path in weather_paths.items():
        actor.set_editor_property(key, A.load_asset(path))
    assert L.save_current_level()

path = '/Game/TestResources/Diagnostics/M_SurfaceWaterPond'
material = A.load_asset(path)
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_SurfaceWaterPond', '/Game/TestResources/Diagnostics', unreal.Material, unreal.MaterialFactoryNew())
edit = unreal.MaterialEditingLibrary
edit.delete_all_material_expressions(material)
material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property('two_sided', True)
material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
color = edit.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
color.set_editor_property('constant', unreal.LinearColor(0.025, 0.3, 0.42, 1.))
opacity = edit.create_material_expression(material, unreal.MaterialExpressionConstant)
opacity.set_editor_property('r', 0.22)
assert edit.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
assert edit.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
edit.recompile_material(material)
assert A.save_loaded_asset(material, False)

assert L.load_level('/Game/Maps/Tests/Celestial')
clock = '/Game/Maps/Tests/Clock'
if A.does_asset_exist(clock):
    references = A.find_package_referencers_for_asset(clock, True)
    assert not references, f'Clock still referenced by {references}'
    assert A.delete_asset(clock)
report = {'weather_maps': list(('Snow', 'Water', 'Terrain', 'Vegetation', 'Cycle', 'Fire', 'Shelter')), 'pond_material': path, 'clock_deleted': not A.does_asset_exist(clock)}
(out / 'presentation-update.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('CCL_LAB_PRESENTATION_UPDATE ' + json.dumps(report))
