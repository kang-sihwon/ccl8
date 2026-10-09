"""Apply stable exposure to the two environment experiment maps only."""
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
for name in ('EnvironmentPlayground', 'EnvironmentScenario'):
    assert levels.load_level('/Game/Maps/' + name)
    descriptors = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
    unreal.WorldPartitionBlueprintLibrary.load_actors([item.guid for item in descriptors])
    loaded = actors.get_all_level_actors()
    assert sum(isinstance(a, unreal.CCLExperimentDirector) for a in loaded) == 1
    exposure = next((a for a in loaded if a.get_actor_label() == 'EnvironmentExposure'), None)
    if not exposure:
        exposure = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
        exposure.set_actor_label('EnvironmentExposure')
    for actor in loaded:
        if isinstance(actor, unreal.DirectionalLight):
            actor.light_component.set_editor_property('intensity', 4.0)
            actor.light_component.set_editor_property('atmosphere_sun_light', True)
        elif isinstance(actor, unreal.SkyLight):
            actor.light_component.set_editor_property('real_time_capture', True)
    exposure.set_editor_property('unbound', True)
    settings = exposure.get_editor_property('settings')
    for key, value in dict(override_auto_exposure_min_brightness=True, override_auto_exposure_max_brightness=True,
                           auto_exposure_min_brightness=0.0, auto_exposure_max_brightness=0.0,
                           override_auto_exposure_bias=True, auto_exposure_bias=0.0,
                           override_bloom_intensity=True, bloom_intensity=0.15).items():
        settings.set_editor_property(key, value)
    exposure.set_editor_property('settings', settings)
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log('CCL_EXPERIMENT_VISUALS saved ' + name)
