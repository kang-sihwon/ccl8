"""Copy the owned bread dependency closure and build gradual decay presentation in an idle Editor."""
import unreal
A = unreal.EditorAssetLibrary
R = unreal.AssetRegistryHelpers.get_asset_registry()
root = '/Game/Fab/Fishermans_Cabin/Meshes/Bread/SM_Bread_01'
options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
closure, pending = set(), [root]
while pending:
    package = pending.pop()
    if package in closure or not package.startswith('/Game/Fab/'):
        continue
    assert A.does_asset_exist(package), package
    closure.add(package)
    pending.extend(str(p) for p in R.get_dependencies(package, options))
mapping = {p: p.replace('/Game/Fab/', '/Game/TestResources/', 1) for p in sorted(closure)}
missing = {p: d for p, d in mapping.items() if not A.does_asset_exist(d)}
if missing:
    assert unreal.CCLTestResourceLibrary.copy_resources(missing)
    for dest in missing.values():
        asset = A.load_asset(dest)
        assert unreal.CCLTestResourceLibrary.repoint_resources(asset, mapping)
        assert A.save_loaded_asset(asset, False)
path = '/Game/TestResources/Diagnostics/M_FoodProgress'
material = A.load_asset(path)
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_FoodProgress', '/Game/TestResources/Diagnostics', unreal.Material, unreal.MaterialFactoryNew())
edit = unreal.MaterialEditingLibrary
edit.delete_all_material_expressions(material)
def node(cls): return edit.create_material_expression(material, cls)
def link(a, output, b, name): assert edit.connect_material_expressions(a, output, b, name)
color = node(unreal.MaterialExpressionVectorParameter)
color.set_editor_property('parameter_name', 'FoodColor')
color.set_editor_property('default_value', unreal.LinearColor(1., 1., 1., 1.))
def texture(suffix, normal=False):
    t = node(unreal.MaterialExpressionTextureSample)
    t.set_editor_property('texture', A.load_asset('/Game/TestResources/Fishermans_Cabin/Textures/Bread/T_Bread_' + suffix))
    if normal: t.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    elif suffix == 'ORM': t.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    return t
albedo, normal, orm = texture('DO'), texture('N', True), texture('ORM')
tint = node(unreal.MaterialExpressionMultiply)
link(albedo, 'RGB', tint, 'A'); link(color, '', tint, 'B')
mold = node(unreal.MaterialExpressionScalarParameter)
mold.set_editor_property('parameter_name', 'Mold')
# The bread's own texture provides a stable surface pattern; the threshold spreads with mold exposure.
coverage = node(unreal.MaterialExpressionMultiply); coverage.set_editor_property('const_b', 1.2); link(mold, '', coverage, 'A')
threshold = node(unreal.MaterialExpressionSubtract); link(coverage, '', threshold, 'A'); link(albedo, 'R', threshold, 'B')
edge = node(unreal.MaterialExpressionMultiply); edge.set_editor_property('const_b', 4.); link(threshold, '', edge, 'A')
mask = node(unreal.MaterialExpressionSaturate); link(edge, '', mask, '')
mold_color = node(unreal.MaterialExpressionConstant3Vector); mold_color.set_editor_property('constant', unreal.LinearColor(0.07, 0.30, 0.055, 1.))
blend = node(unreal.MaterialExpressionLinearInterpolate)
link(tint, '', blend, 'A'); link(mold_color, '', blend, 'B'); link(mask, '', blend, 'Alpha')
assert edit.connect_material_property(blend, '', unreal.MaterialProperty.MP_BASE_COLOR)
assert edit.connect_material_property(normal, 'RGB', unreal.MaterialProperty.MP_NORMAL)
assert edit.connect_material_property(orm, 'R', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
assert edit.connect_material_property(orm, 'G', unreal.MaterialProperty.MP_ROUGHNESS)
edit.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE)
edit.recompile_material(material)
assert A.save_loaded_asset(material, False)
unreal.log('CCL_FOOD_PROGRESS_MATERIAL_READY')
