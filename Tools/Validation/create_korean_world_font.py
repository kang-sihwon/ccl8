"""Create the offline world-label atlas from UE's bundled NanumGothic (Windows).

For regeneration, preserve the previous .uasset outside Content before running.
The atlas embeds glyphs, so the game does not require an installed Windows font.
"""
import ctypes
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
font_file = str(Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_content_dir())) / 'Editor/Slate/Fonts/NanumGothic.ttf')
package = '/Game/UI/Fonts/F_KoreanLabel'
assert not unreal.EditorAssetLibrary.does_asset_exist(package), 'Preserve the existing atlas outside Content before regeneration.'
source = ''.join(p.read_text(encoding='utf-8-sig') for p in (root / 'Source/CCL').rglob('*') if p.suffix in ('.h', '.cpp'))
labels = json.loads((root / 'Tools/Validation/korean_ui_text.json').read_text(encoding='utf-8'))
source += ''.join(labels.values())
# Chars filters ASCII rasterization too, even when IncludeASCIIRange is true.
chars = ''.join(chr(i) for i in range(32, 127)) + ''.join(sorted({c for c in source if '\uac00' <= c <= '\ud7a3'})) + '→↑·'
assert ctypes.windll.gdi32.AddFontResourceExW(font_file, 0x10, 0), 'Cannot register bundled font for import.'
try:
    factory = unreal.TrueTypeFontFactory()
    options = factory.get_editor_property('import_options')
    data = options.get_editor_property('data')
    values = {
        'font_name': 'NanumGothic', 'height': 24.0, 'chars': chars,
        'include_ascii_range': True, 'texture_page_width': 2048, 'texture_page_max_height': 2048,
        'use_distance_field_alpha': True, 'distance_field_scale_factor': 4,
    }
    for key, value in values.items():
        data.set_editor_property(key, value)
    options.set_editor_property('data', data)
    font = unreal.AssetToolsHelpers.get_asset_tools().create_asset('F_KoreanLabel', '/Game/UI/Fonts', unreal.Font, factory)
    assert font and unreal.EditorAssetLibrary.save_loaded_asset(font, False)
    unreal.log('CCL_KOREAN_FONT PASS chars=%d' % len(chars))
finally:
    ctypes.windll.gdi32.RemoveFontResourceExW(font_file, 0x10, 0)
