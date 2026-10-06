"""Create a lightweight start map for the Slate menu."""
import unreal
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
path='/Game/Maps/FrontEnd'
if unreal.EditorAssetLibrary.does_asset_exist(path):
    if not levels.load_level(path): raise RuntimeError('FrontEnd load failed')
else:
    if not levels.new_level(path,False): raise RuntimeError('FrontEnd creation failed')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().modify()
world.get_world_settings().set_editor_property('default_game_mode',unreal.CCLMenuGameMode)
if not levels.save_current_level() or not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True):
    raise RuntimeError('FrontEnd save failed')
unreal.log('CCL_FRONTEND_ASSETS PASS')
