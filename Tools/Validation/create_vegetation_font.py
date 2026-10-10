from pathlib import Path
import ctypes
import unreal
path = '/Game/Environment/Fonts/F_VegetationLabel'
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    font_file = str(Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_content_dir())) / 'Editor/Slate/Fonts/NanumGothic.ttf')
    chars = ''.join(chr(i) for i in range(32, 127)) + ''.join(sorted(set('나무풀꽃관목이끼양치조류수생버섯채광그늘분부족')))
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
        font = unreal.AssetToolsHelpers.get_asset_tools().create_asset('F_VegetationLabel','/Game/Environment/Fonts',unreal.Font,factory)
        assert font and unreal.EditorAssetLibrary.save_loaded_asset(font,False)
    finally:
        ctypes.windll.gdi32.RemoveFontResourceExW(font_file,0x10,0)
assert unreal.EditorAssetLibrary.load_asset(path)
unreal.log('CCL_VEGETATION_FONT PASS')
unreal.SystemLibrary.quit_editor()
