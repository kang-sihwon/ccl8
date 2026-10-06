"""Create encounter definitions and the village NPC."""
import unreal
if not unreal.CCLProgressionAssetLibrary.create_encounter_assets():
    raise RuntimeError('Encounter definitions failed')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level('/Game/Maps/Campaign'):
    raise RuntimeError('Campaign unavailable')
if not any(a.get_actor_label()=='VillageSteward' for a in actors.get_all_level_actors()):
    actor=actors.spawn_actor_from_class(unreal.CCLVillageSteward,unreal.Vector(-1750,250,96),unreal.Rotator(yaw=-90))
    if not actor:
        raise RuntimeError('NPC spawn failed')
    actor.modify()
    actor.set_actor_label('VillageSteward')
if not levels.save_current_level() or not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True):
    raise RuntimeError('NPC save failed')
unreal.log('CCL_CONTENT_ASSETS PASS')
