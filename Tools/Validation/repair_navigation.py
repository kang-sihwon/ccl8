"""Repair generated navigation brush geometry without rebuilding edited maps."""
import unreal
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for path in ('/Game/Maps/CombatPlayground', '/Game/Maps/Campaign'):
    if not levels.load_level(path):
        raise RuntimeError('Cannot open ' + path)
    if not unreal.CCLCombatAssetLibrary.configure_combat_world():
        raise RuntimeError('Cannot configure navigation in ' + path)
    bounds = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.NavMeshBoundsVolume)]
    if len(bounds) != 1:
        raise RuntimeError('Expected exactly one navigation volume in ' + path)
    extent = bounds[0].get_actor_bounds(False)[1]
    if extent.x < 1900 or extent.y < 1900 or extent.z < 250:
        raise RuntimeError('Navigation bounds missing geometry in ' + path)
    if not levels.save_current_level() or not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
        raise RuntimeError('Cannot save repaired navigation in ' + path)
    unreal.log('CCL_NAVIGATION_ASSETS PASS ' + path + ' extent=' + str(extent))
