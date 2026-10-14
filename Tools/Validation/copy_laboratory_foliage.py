"""Copy only the three owned foliage meshes used by the vegetation laboratory."""
import unreal
A = unreal.EditorAssetLibrary
R = unreal.AssetRegistryHelpers.get_asset_registry()
roots = ['/Game/Fab/Fishermans_Cabin/Meshes/Foliage/' + p for p in ['Grass/SM_Grass_01', 'Flower/SM_Flower_01', 'Bush/SM_Bush_01']]
options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
closure, pending = set(), roots[:]
while pending:
    p = pending.pop()
    if p in closure or not p.startswith('/Game/Fab/'): continue
    assert A.does_asset_exist(p), p
    closure.add(p)
    pending.extend(str(d) for d in R.get_dependencies(p, options))
mapping = {p: p.replace('/Game/Fab/', '/Game/TestResources/', 1) for p in sorted(closure)}
missing = {p: d for p, d in mapping.items() if not A.does_asset_exist(d)}
if missing:
    assert unreal.CCLTestResourceLibrary.copy_resources(missing)
    for p in missing.values():
        a = A.load_asset(p)
        assert unreal.CCLTestResourceLibrary.repoint_resources(a, mapping)
        assert A.save_loaded_asset(a, False)
unreal.log('CCL_FOLIAGE_COPIED')
