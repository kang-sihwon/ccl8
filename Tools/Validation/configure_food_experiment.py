from pathlib import Path
import ctypes
import unreal
path = '/Game/Environment/Fonts/F_FoodLabel'
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    font_file = str(Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_content_dir())) / 'Editor/Slate/Fonts/NanumGothic.ttf')
    chars = ''.join(chr(i) for i in range(32, 127)) + ''.join(sorted(set('노출냉각밀폐개신선곰팡이·')))
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
        font = unreal.AssetToolsHelpers.get_asset_tools().create_asset('F_FoodLabel','/Game/Environment/Fonts',unreal.Font,factory)
        assert font and unreal.EditorAssetLibrary.save_loaded_asset(font,False)
    finally:
        ctypes.windll.gdi32.RemoveFontResourceExW(font_file,0x10,0)
assert unreal.EditorAssetLibrary.load_asset(path)


assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
path = '/Game/Environment/Experiments/DA_FoodSample'
asset = assets.load_asset(path) if assets.does_asset_exist(path) else None
if not asset:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.CCLItemDefinition)
    asset = tools.create_asset('DA_FoodSample', '/Game/Environment/Experiments', unreal.CCLItemDefinition, factory)
    assert asset
asset.set_editor_property('item_name', '실험용 빵')
asset.set_editor_property('description', '부패 기준 300초의 보관 조건 비교 표본.')
asset.set_editor_property('max_stack_count', 20)
assert asset.configure_perishable(300., 120., 180.)
assert assets.save_loaded_asset(asset, False)
for name, color in [('Fresh',(0.72,0.34,0.08)), ('Rotten',(0.16,0.1,0.045)), ('Mold',(0.12,0.4,0.12))]:
    path = '/Game/Environment/Materials/M_Food'+name
    material = assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset('M_Food'+name, '/Game/Environment/Materials', unreal.Material, unreal.MaterialFactoryNew())
    assert material
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    node.set_editor_property('constant', unreal.LinearColor(*color,1))
    assert unreal.MaterialEditingLibrary.connect_material_property(node,'',unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    assert assets.save_loaded_asset(material, False)
label_path = '/Game/Environment/Materials/M_FoodLabel'
if not assets.does_asset_exist(label_path):
    label_material = assets.duplicate_asset('/Engine/EngineMaterials/DefaultTextMaterialOpaque', label_path)
    assert label_material
    label_material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    label_material.set_editor_property('two_sided', True)
    color = unreal.MaterialEditingLibrary.create_material_expression(label_material, unreal.MaterialExpressionVertexColor)
    assert unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(label_material)
    assert assets.save_loaded_asset(label_material, False)
experiment = assets.load_asset('/Game/Environment/Experiments/DA_Environment_07')
assert experiment
experiment.configure_zone(7)
assert assets.save_loaded_asset(experiment, False)
unreal.log('CCL_FOOD_EXPERIMENT_ASSET PASS')
unreal.SystemLibrary.quit_editor()
