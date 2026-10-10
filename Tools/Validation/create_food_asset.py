import unreal
path = '/Game/Progression/DA_Bread'
asset = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if not asset:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.CCLItemDefinition)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_Bread','/Game/Progression',unreal.CCLItemDefinition,factory)
    assert asset
    asset.set_editor_property('item_name', '빵')
    asset.set_editor_property('description', '보관 온도와 습도에 따라 신선도와 곰팡이가 변하는 음식.')
    asset.set_editor_property('max_stack_count', 20)
    assert asset.configure_perishable(86400., 43200., 43200.)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
unreal.log('CCL_FOOD_ASSET PASS')
unreal.SystemLibrary.quit_editor()
