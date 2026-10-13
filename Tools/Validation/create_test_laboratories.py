"""Create missing dedicated test maps and copy only the used Fab dependency closure.

Run in the Editor through Python. Existing test maps and source packs are preserved.
The dependency manifest and validation evidence are written to Saved/Laboratories.
"""
from pathlib import Path
import json
import unreal
from editor_toolset.toolsets.object import ObjectTools

A = unreal.EditorAssetLibrary
L = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
E = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
U = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
R = unreal.AssetRegistryHelpers.get_asset_registry()
out = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / 'Laboratories'
out.mkdir(exist_ok=True)

# Distinct colors also render the Mass representation, which has no skeletal mesh.
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
for i, color in enumerate([(0.9, .25, .025), (.015, .6, 1.), (1., .75, .025), (.65, .025, .85)]):
    path = f'/Game/TestResources/Diagnostics/M_Creature{i}'
    if A.does_asset_exist(path):
        continue
    material = asset_tools.create_asset(f'M_Creature{i}', '/Game/TestResources/Diagnostics', unreal.Material, unreal.MaterialFactoryNew())
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -250, 0)
    node.set_editor_property('constant', unreal.LinearColor(*color, 1))
    assert unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    unreal.MaterialEditingLibrary.recompile_material(material)
    assert A.save_loaded_asset(material, False)

roots = [
 '/Game/Fab/Weapon_Pack/Mesh/Weapons/Weapons_Kit/SM_Sword',
 '/Game/Fab/Weapon_Pack/Mesh/Weapons/Weapons_Kit/SM_Shield',
 '/Game/Fab/Weapon_Pack/Mesh/Props/Interior/SM_Barrel',
 '/Game/Fab/M5VFXVOL2/Niagara/Reference/Fireloop/2_Nbonefire_pt',
 '/Game/Fab/M5VFXVOL2/Props/Meshes/BF_mesh_firewood',
 '/Game/Fab/Fishermans_Cabin/Meshes/Wood_Pallet/SM_Wood_Pallet',
] + ['/Game/Fab/NorwayMaple/Geometry/SimpleWind/' + n for n in (
 'SM_NorwayMaple_Seedling_01', 'SM_NorwayMaple_Sapling_01', 'SM_NorwayMaple_Field_01')]
options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
closure = set()
pending = list(roots)
while pending:
    package = pending.pop()
    if package in closure or not package.startswith('/Game/Fab/'):
        continue
    assert A.does_asset_exist(package), package
    closure.add(package)
    pending.extend(str(d) for d in R.get_dependencies(package, options))
mapping = {p: p.replace('/Game/Fab/', '/Game/TestResources/', 1) for p in sorted(closure)}
missing = {a: b for a, b in mapping.items() if not A.does_asset_exist(b)}
if missing:
    # A partial copy needs the complete map to patch cross-package references correctly.
    assert len(missing) == len(mapping), 'Partial resource copy: inspect manifest before continuing.'
    assert unreal.CCLTestResourceLibrary.copy_resources(mapping), 'Advanced dependency copy failed'
R.scan_paths_synchronous(['/Game/TestResources'], True)
assert all(A.does_asset_exist(p) for p in mapping.values())
for destination in mapping.values():
    assert A.save_loaded_asset(A.load_asset(destination), False)
(out / 'resource-manifest.json').write_text(json.dumps({'roots': roots, 'packages': mapping}, indent=2), encoding='utf-8')

# Repoint only the existing test placements and item definitions.
for item in ('DA_TrainingSword', 'DA_TrainingShield'):
    obj = A.load_asset('/Game/Progression/' + item)
    assert unreal.CCLTestResourceLibrary.repoint_resources(obj, mapping)
    assert A.save_loaded_asset(obj, False)
for name in ('EnvironmentScenario', 'EnvironmentPlayground', 'CombatPlayground'):
    assert L.load_level('/Game/Maps/' + name)
    unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
    changed = []
    for actor in E.get_all_level_actors():
        if actor.get_actor_label().startswith('Fab_') or isinstance(actor, unreal.CCLFirePresentation):
            assert unreal.CCLTestResourceLibrary.repoint_resources(actor, mapping)
            changed.append(actor.get_package())
        if isinstance(actor, unreal.CCLVegetationPresentation):
            actor.set_editor_property('living_tree_mesh', A.load_asset(mapping[roots[-2]]))
            actor.refresh_preview()
            changed.append(actor.get_package())
    assert unreal.EditorLoadingAndSavingUtils.save_packages(list(set(changed)), False)
    assert L.save_current_level()

assert L.load_level('/Game/Maps/EnvironmentScenario')
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
loaded = E.get_all_level_actors()
template = next(a for a in loaded if isinstance(a, unreal.CCLWorldEnvironmentConfig))
config_fields = ['celestialDefinition', 'observer', 'surfaces', 'openings', 'probes', 'viewOpeningIds', 'viewSurfaceIds', 'characterPostProcessAnim', 'experimentResidentClass']
# Property JSON owns its values across unloading the template world (StructBase views do not).
ObjectTools.list_properties(template)
config_values = json.loads(ObjectTools.get_properties(template, config_fields))
assert len(config_values) == len(config_fields), config_values
(out / 'template-config.json').write_text(json.dumps(config_values, indent=2), encoding='utf-8')
vegetation = next(a for a in loaded if isinstance(a, unreal.CCLVegetationPresentation))
state_materials = list(vegetation.get_editor_property('state_materials'))
weather = next(a for a in loaded if isinstance(a, unreal.CCLWeatherPresentation))
weather_values = {k: weather.get_editor_property(k) for k in ['rain_system', 'snow_system', 'cloud_material']}
world_view = next(a for a in loaded if isinstance(a, unreal.CCLWorldEnvironmentPresentation))
world_values = {k: world_view.get_editor_property(k) for k in ['solid_mesh', 'glass_mesh', 'solid_material', 'glass_material']}
definitions = [A.load_asset(f'/Game/Environment/Experiments/DA_Environment_{n:02d}') for n in range(12)]
names = ['Clock', 'Celestial', 'Weather', 'Snow', 'Water', 'Terrain', 'Ecology', 'Vegetation', 'Persistence', 'Cycle', 'Fire', 'Shelter']
specs = [(n, z, 0) for z, n in enumerate(names)] + [('Life', 7, 1), ('Decay', 7, 2)]
created = []
for name, zone, subject in specs:
    path = '/Game/Maps/Tests/' + name
    if A.does_asset_exist(path):
        continue
    assert L.new_level(path, False)
    center = unreal.Vector((zone % 4) * 2500, (zone // 4) * 2500, 0)
    floor = E.spawn_actor_from_class(unreal.StaticMeshActor, center + unreal.Vector(0, 0, -50))
    floor.set_actor_label('LaboratoryFloor_BlockAll')
    floor.static_mesh_component.set_static_mesh(A.load_asset('/Engine/BasicShapes/Cube'))
    floor.static_mesh_component.set_material(0, A.load_asset('/Game/Environment/Materials/M_EnvironmentFloor'))
    floor.static_mesh_component.set_collision_profile_name('BlockAll')
    floor.set_actor_scale3d(unreal.Vector(50, 50, 1))
    config = E.spawn_actor_from_class(unreal.CCLWorldEnvironmentConfig, center)
    config.set_actor_label('LaboratorySettings')
    values = json.loads(json.dumps(config_values))
    if zone != 11:
        for key in ('surfaces', 'openings', 'probes', 'viewOpeningIds', 'viewSurfaceIds'):
            values[key] = []
    ObjectTools.list_properties(config)
    assert ObjectTools.set_properties(config, json.dumps(values))
    for key, value in dict(domain=unreal.CCLWorldContentDomain.SCENARIO, laboratory_zone=zone,
                           laboratory_subject=subject, laboratory_variant=0, laboratory_count=3).items():
        config.set_editor_property(key, value)
    director = E.spawn_actor_from_class(unreal.CCLExperimentDirector, center)
    director.set_editor_property('definitions', definitions)
    director.set_actor_label('LaboratoryDirector')
    start = E.spawn_actor_from_class(unreal.PlayerStart, center + unreal.Vector(0, -950, 130))
    start.set_actor_rotation(unreal.Rotator(pitch=0, yaw=90, roll=0), False)
    sun = E.spawn_actor_from_class(unreal.DirectionalLight, center + unreal.Vector(0, 0, 1500))
    sun.set_actor_rotation(unreal.Rotator(pitch=-50, yaw=-40, roll=0), False)
    sun.light_component.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property('intensity', 4.0)
    sky = E.spawn_actor_from_class(unreal.SkyLight, center + unreal.Vector(0, 0, 800))
    sky.light_component.set_editor_property('intensity', 1.0)
    sky.light_component.set_editor_property('real_time_capture', True)
    E.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    if zone in (1, 2, 11):
        view = E.spawn_actor_from_class(unreal.CCLWorldEnvironmentPresentation, unreal.Vector())
        for k, v in world_values.items():
            view.set_editor_property(k, v)
        view.set_editor_property('sun', sun)
        view.refresh_preview()
    if zone == 2:
        w = E.spawn_actor_from_class(unreal.CCLWeatherPresentation, center)
        for k, v in weather_values.items():
            w.set_editor_property(k, v)
    if zone == 7 and subject == 0:
        plants = E.spawn_actor_from_class(unreal.CCLVegetationPresentation, unreal.Vector())
        plants.set_editor_property('state_materials', state_materials)
        plants.set_editor_property('living_tree_mesh', A.load_asset(mapping[roots[-2]]))
        plants.refresh_preview()
    if zone in (7, 10, 11) and subject == 0:
        fire = E.spawn_actor_from_class(unreal.CCLFirePresentation, unreal.Vector())
        fire.set_editor_property('flame_system', A.load_asset(mapping[roots[3]]))
        fire.set_editor_property('smoke_system', A.load_asset('/Game/Environment/FX/NS_RoomSmoke'))
        for k, mat in [('fuel_material', 'M_FireFuel'), ('char_material', 'M_FireChar')]:
            if A.does_asset_exist('/Game/Environment/Materials/' + mat):
                fire.set_editor_property(k, A.load_asset('/Game/Environment/Materials/' + mat))
    settings = U.get_editor_world().get_world_settings()
    settings.set_editor_property('default_game_mode', unreal.CCLExperimentGameMode)
    settings.set_editor_property('kill_z', -1500.)
    assert L.save_current_level()
    created.append(path)
    (out / 'created-maps.json').write_text(json.dumps(created, indent=2), encoding='utf-8')
assert L.load_level('/Game/Maps/Tests/Ecology')
unreal.log('CCL_LAB_CREATION_DONE maps=' + str(len(created)) + ' resources=' + str(len(mapping)))
