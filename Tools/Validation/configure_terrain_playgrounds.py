"""Refresh the terrain experiment definition and its two existing map signs."""
import unreal
assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
definition = unreal.load_asset('/Game/Environment/Experiments/DA_Environment_05')
assert definition
definition.configure_zone(5)
assert assets.save_loaded_asset(definition, False)
for name in ('EnvironmentPlayground', 'EnvironmentScenario'):
    assert levels.load_level('/Game/Maps/' + name)
    unreal.WorldPartitionBlueprintLibrary.load_actors([a.guid for a in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.CCLExperimentStation) and actor.get_editor_property('definition') == definition:
            actor.modify()
            actor.refresh_label()
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log('CCL_TERRAIN_ASSETS saved ' + name)
