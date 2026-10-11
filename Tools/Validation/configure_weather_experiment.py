import unreal
asset = unreal.EditorAssetLibrary.load_asset('/Game/Environment/Experiments/DA_Environment_02')
if not asset:
    raise RuntimeError('Missing weather experiment definition')
asset.configure_zone(2)
if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
    raise RuntimeError('Could not save weather experiment definition')
unreal.log('CCL_WEATHER_ASSET PASS')

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
roof_id = unreal.Guid()
roof_id.import_text('000000CA000000CA000000CA000000CA')
fx = {}
for suffix, snow in [('Rain', False), ('Snow', True)]:
    path = '/Game/Environment/FX/NS_Weather' + suffix
    system = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.EditorAssetLibrary.duplicate_asset('/Niagara/DefaultAssets/Templates/Systems/FountainLightweight', path)
    assert system and unreal.CCLEnvironmentFXLibrary.configure_precipitation_system(system, snow), 'Niagara configuration failed'
    assert unreal.EditorAssetLibrary.save_loaded_asset(system, False)
    fx[suffix] = system

for name in ('EnvironmentPlayground', 'EnvironmentScenario'):
    assert levels.load_level('/Game/Maps/' + name)
    descriptors = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
    unreal.WorldPartitionBlueprintLibrary.load_actors([item.guid for item in descriptors])
    loaded = actors.get_all_level_actors()
    config = next(a for a in loaded if isinstance(a, unreal.CCLWorldEnvironmentConfig))
    patches = [p for p in config.get_editor_property('surfaces') if p.surface_id.export_text() != roof_id.export_text()]
    roof = unreal.KSISurfacePatch()
    roof.surface_id = roof_id
    roof.body_id = config.observer.body_id
    roof.material_id = 'WeatherRoof'
    roof.center_meters = unreal.Vector(48, 0, 3)
    roof.half_extents_meters = unreal.Vector2D(2, 4)
    patches.append(roof)
    assert len({p.surface_id.export_text() for p in patches}) == len(patches)
    config.set_editor_property('surfaces', patches)
    ids = [i for i in config.get_editor_property('view_surface_ids') if i.export_text() != roof_id.export_text()]
    ids.append(roof_id)
    config.set_editor_property('view_surface_ids', ids)
    for actor in loaded:
        if isinstance(actor, unreal.CCLWorldEnvironmentPresentation):
            actor.refresh_preview()
    weather = next((a for a in loaded if isinstance(a, unreal.CCLWeatherPresentation)), None)
    if weather is None:
        weather = actors.spawn_actor_from_class(unreal.CCLWeatherPresentation, unreal.Vector(0, 0, 0))
        weather.set_actor_label('WeatherPrecipitation')
    weather.set_editor_property('rain_system', fx['Rain'])
    weather.set_editor_property('snow_system', fx['Snow'])
    weather.set_editor_property('cloud_material', unreal.EditorAssetLibrary.load_asset('/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud'))
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log('CCL_WEATHER_ROOF saved ' + name)

cloud = unreal.EditorAssetLibrary.load_asset('/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud')
unreal.log('WX_CLOUD_PARAMS ' + str([(str(p), unreal.MaterialEditingLibrary.get_material_default_scalar_parameter_value(cloud, p)) for p in unreal.MaterialEditingLibrary.get_scalar_parameter_names(cloud)]))
unreal.SystemLibrary.quit_editor()
