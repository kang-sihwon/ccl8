"""Create progression definitions and place village supplies without replacing maps."""
import unreal
if not unreal.CCLProgressionAssetLibrary.create_progression_assets():
    raise RuntimeError('Progression asset creation failed')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level('/Game/Maps/Campaign'):
    raise RuntimeError('Campaign map unavailable')
labels={a.get_actor_label() for a in actors.get_all_level_actors()}
for label, asset, quantity, y in (
    ('Supply_Gauntlets', '/Game/Progression/DA_IronGauntlets', 1, 0),
    ('Supply_Potions', '/Game/Progression/DA_RecoveryPotion', 3, 200),
):
    if label in labels:
        continue
    definition=unreal.load_asset(asset)
    if not definition:
        raise RuntimeError('Definition unavailable: '+asset)
    pickup=actors.spawn_actor_from_class(unreal.CCLWorldPickup, unreal.Vector(-1300,y,55))
    if not pickup:
        raise RuntimeError('Cannot create '+label)
    pickup.modify()
    pickup.set_actor_label(label)
    pickup.set_editor_property('definition',definition)
    pickup.set_editor_property('quantity',quantity)
if not levels.save_current_level() or not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True):
    raise RuntimeError('Cannot save village supplies')
unreal.log('CCL_PROGRESSION_ASSETS PASS')
