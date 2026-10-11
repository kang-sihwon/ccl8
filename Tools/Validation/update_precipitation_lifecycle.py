import unreal
for suffix, snow in [('Rain', False), ('Snow', True)]:
    system = unreal.EditorAssetLibrary.load_asset('/Game/Environment/FX/NS_Weather' + suffix)
    assert system and unreal.CCLEnvironmentFXLibrary.configure_precipitation_system(system, snow)
    assert unreal.EditorAssetLibrary.save_loaded_asset(system, False)
    unreal.log('CCL_PRECIPITATION_LIFECYCLE PASS ' + suffix)
unreal.SystemLibrary.quit_editor()
