"""Author stage-2 settings and display actors on the two existing experiment maps."""
import json
from pathlib import Path
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)

def guid(value):
    result = unreal.Guid()
    result.import_text(f'43434C322026100900000000{value:08X}')
    return result

def patch(index, center, normal=(0, 0, 1), tangent_u=(1, 0, 0), tangent_v=(0, 1, 0), extent=(4, 3), glass=False):
    result = unreal.CCLSurfacePatch()
    for key, value in dict(surface_id=guid(index), body_id='World', material_id='Glass' if glass else 'Stone',
            center_meters=unreal.Vector(*center), normal=unreal.Vector(*normal),
            tangent_u=unreal.Vector(*tangent_u), tangent_v=unreal.Vector(*tangent_v),
            half_extents_meters=unreal.Vector2D(*extent)).items():
        result.set_editor_property(key, value)
    if glass:
        transmission = unreal.CCLSurfaceTransmission()
        transmission.set_editor_property('sun', 0.7)
        result.set_editor_property('transmission', transmission)
    return result

celestial_path = '/Game/Environment/Experiments/DA_CelestialSystem'
celestial = unreal.load_asset(celestial_path) if assets.does_asset_exist(celestial_path) else None
if not celestial:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.CCLCelestialDefinition)
    celestial = tools.create_asset('DA_CelestialSystem', '/Game/Environment/Experiments', unreal.CCLCelestialDefinition, factory)
    celestial.configure_default(42)
definition_data = celestial.get_editor_property('definition')
definition_data.set_editor_property('version', 2)
celestial.set_editor_property('definition', definition_data)
assert assets.save_loaded_asset(celestial, False)

# Translucent glass cannot use the Nanite cube; the opaque shell uses the existing Nanite asset.
glass_path = '/Game/Environment/Materials/M_EnvironmentGlass'
glass = unreal.load_asset(glass_path) if assets.does_asset_exist(glass_path) else None
if not glass:
    glass = tools.create_asset('M_EnvironmentGlass', '/Game/Environment/Materials', unreal.Material, unreal.MaterialFactoryNew())
    glass.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    glass.set_editor_property('two_sided', True)
    color = unreal.MaterialEditingLibrary.create_material_expression(glass, unreal.MaterialExpressionConstant3Vector, -200, 0)
    color.set_editor_property('constant', unreal.LinearColor(0.25, 0.6, 0.7, 1))
    unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    opacity = unreal.MaterialEditingLibrary.create_material_expression(glass, unreal.MaterialExpressionConstant, -200, 150)
    opacity.set_editor_property('r', 0.22)
    unreal.MaterialEditingLibrary.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(glass)
assert assets.save_loaded_asset(glass, False)

surfaces = [
    patch(1, (75, 50, 0)),
    patch(2, (75, 50, 4.05)),
    patch(3, (75, 50, 4), (0, 0, -1), (1, 0, 0), (0, -1, 0)),
    patch(4, (79, 50, 2), (1, 0, 0), (0, 1, 0), (0, 0, 1), (3, 2)),
    patch(5, (71, 50, 2), (-1, 0, 0), (0, -1, 0), (0, 0, 1), (3, 2)),
    patch(6, (75, 47, 2), (0, -1, 0), (1, 0, 0), (0, 0, 1), (4, 2)),
    patch(7, (75, 53, 2), (0, 1, 0), (-1, 0, 0), (0, 0, 1), (4, 2)),
    patch(8, (66, 49, 4), extent=(2, 2), glass=True)]
opening = unreal.CCLSurfaceOpening()
for key, value in dict(opening_id=guid(100), surface_id=guid(4), center_uv=unreal.Vector2D(0, -0.5),
        half_extents_meters=unreal.Vector2D(1, 1.5), open_fraction=0.0, space_a=guid(200)).items():
    opening.set_editor_property(key, value)
probes = []
for name, position in [('Inside', (77, 49.5, 1.5)), ('Outside', (84, 50, 1.5)), ('Glass', (66, 49, 1.5))]:
    probe = unreal.CCLEnvironmentProbe()
    probe.set_editor_property('probe_id', name)
    probe.set_editor_property('position_meters', unreal.Vector(*position))
    probes.append(probe)
observer = unreal.CCLCelestialObserver()
observer.set_editor_property('body_id', 'World')
observer.set_editor_property('latitude_degrees', 45.0)

for index in (0, 1, 8, 11):
    definition = unreal.load_asset(f'/Game/Environment/Experiments/DA_Environment_{index:02d}')
    assert definition
    definition.configure_zone(index)
    assert assets.save_loaded_asset(definition, False)

for name in ('EnvironmentPlayground', 'EnvironmentScenario'):
    assert levels.load_level('/Game/Maps/' + name)
    unreal.WorldPartitionBlueprintLibrary.load_actors([x.guid for x in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
    loaded = actors.get_all_level_actors()
    config = next((a for a in loaded if isinstance(a, unreal.CCLWorldEnvironmentConfig)), None)
    if not config:
        config = actors.spawn_actor_from_class(unreal.CCLWorldEnvironmentConfig, unreal.Vector(0, 0, 0))
        config.set_actor_label('WorldEnvironmentConfig')
    for key, value in dict(celestial_definition=celestial, observer=observer, surfaces=surfaces, openings=[opening],
            probes=probes, view_opening_ids=[guid(100)], view_surface_ids=[guid(i) for i in range(1, 9)]).items():
        config.set_editor_property(key, value)
    config.set_editor_property('is_spatially_loaded', False)
    presentation = next((a for a in loaded if isinstance(a, unreal.CCLWorldEnvironmentPresentation)), None)
    if not presentation:
        presentation = actors.spawn_actor_from_class(unreal.CCLWorldEnvironmentPresentation, unreal.Vector(0, 0, 0))
        presentation.set_actor_label('WorldEnvironmentPresentation')
    sun = next(a for a in loaded if isinstance(a, unreal.DirectionalLight))
    sun.modify()
    sun.light_component.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property('atmosphere_sun_light', True)
    for key, value in dict(sun=sun, solid_mesh=unreal.load_asset('/Game/Environment/SM_EnvironmentBlock'),
            glass_mesh=unreal.load_asset('/Engine/BasicShapes/Cube'),
            solid_material=unreal.load_asset('/Game/Environment/Materials/M_EnvironmentReady'), glass_material=glass).items():
        presentation.set_editor_property(key, value)
    presentation.set_editor_property('is_spatially_loaded', False)
    presentation.refresh_preview()
    for actor in loaded:
        if isinstance(actor, unreal.CCLExperimentStation):
            actor.refresh_label()
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log('CCL_CELESTIAL_ASSETS saved ' + name)
(root / 'Saved/EnvironmentGoal/celestial-assets.json').write_text(json.dumps(dict(maps=2, surfaces=8, openings=1, probes=3,
    requires_fresh_reload=True)), encoding='utf-8')
