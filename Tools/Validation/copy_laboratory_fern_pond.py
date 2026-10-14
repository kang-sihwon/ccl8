"""Run in Editor: copy the selected fern and fish with their package dependencies."""
import json
from pathlib import Path
import unreal

A = unreal.EditorAssetLibrary
R = unreal.AssetRegistryHelpers.get_asset_registry()
roots = [
    '/Game/Fab/PN_Fern/Meshes/fern_01_01',
    '/Game/Fab/AnimatedPondFish/Fish/Meshes/SKM_Fish',
    '/Game/Fab/AnimatedPondFish/Fish/Meshes/SKM_Fish_Animation',
]
options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
pending, closure = roots[:], set()
while pending:
    p = pending.pop()
    if p in closure or not p.startswith('/Game/Fab/'):
        continue
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
R.scan_paths_synchronous(['/Game/TestResources/PN_Fern', '/Game/TestResources/AnimatedPondFish'], True)
for p in mapping.values():
    assert not any(str(d).startswith('/Game/Fab/') for d in R.get_dependencies(p, options)), p
out = Path(unreal.Paths.project_saved_dir()) / 'Laboratories/fern-pond-copy.json'
out.write_text(json.dumps({'roots': roots, 'mapping': mapping}, indent=2), encoding='utf8')
unreal.log('CCL_FERN_POND_COPIED ' + str(len(mapping)))
