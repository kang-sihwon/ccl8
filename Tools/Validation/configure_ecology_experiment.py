from pathlib import Path
import ctypes
import unreal
path = '/Game/Environment/Fonts/F_EcologyLabel'
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    font_file = str(Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_content_dir())) / 'Editor/Slate/Fonts/NanumGothic.ttf')
    chars = ''.join(chr(i) for i in range(32, 127)) + ''.join(sorted(set('지상생물고기새곤충마리먹이에너지·')))
    assert ctypes.windll.gdi32.AddFontResourceExW(font_file, 0x10, 0)
    try:
        factory = unreal.TrueTypeFontFactory()
        options = factory.get_editor_property('import_options')
        data = options.get_editor_property('data')
        for key, value in dict(font_name='NanumGothic',height=24.0,chars=chars,include_ascii_range=True,
                               texture_page_width=2048,texture_page_max_height=2048,
                               use_distance_field_alpha=True,distance_field_scale_factor=4).items():
            data.set_editor_property(key,value)
        options.set_editor_property('data',data)
        font = unreal.AssetToolsHelpers.get_asset_tools().create_asset('F_EcologyLabel','/Game/Environment/Fonts',unreal.Font,factory)
        assert font and unreal.EditorAssetLibrary.save_loaded_asset(font,False)
    finally:
        ctypes.windll.gdi32.RemoveFontResourceExW(font_file,0x10,0)
assert unreal.EditorAssetLibrary.load_asset(path)


import unreal
asset = unreal.EditorAssetLibrary.load_asset('/Game/Environment/Experiments/DA_Environment_06')
assert asset
asset.configure_zone(6)
assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
unreal.log('CCL_ECOLOGY_ASSET PASS')
unreal.SystemLibrary.quit_editor()
