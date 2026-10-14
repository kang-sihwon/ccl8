"""Import the selected CC0 animal prototypes from Saved/Laboratories/Animals."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_saved_dir()) / 'Laboratories/Animals'
tasks = []
for name, source in [('FishRig', 'FishRig.fbx'), ('SheepRig', 'SheepRig.fbx'), ('Pigeon', 'Pigeon.fbx'), ('AntRig', 'AntRig.fbx')]:
    if unreal.EditorAssetLibrary.does_asset_exist('/Game/TestResources/Animals/' + name + '/SK_' + name):
        continue
    task = unreal.AssetImportTask()
    task.filename = str(root / source)
    task.destination_path = '/Game/TestResources/Animals/' + name
    task.destination_name = 'SK_' + name
    task.automated = True
    task.save = True
    task.replace_existing = False
    options = unreal.FbxImportUI()
    options.set_editor_property('automated_import_should_detect_type', False)
    options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property('import_as_skeletal', True)
    options.set_editor_property('import_mesh', True)
    options.set_editor_property('import_animations', True)
    options.set_editor_property('create_physics_asset', False)
    options.set_editor_property('import_materials', True)
    options.set_editor_property('import_textures', True)
    task.options = options
    tasks.append(task)
# The legacy factory honors the explicit skeletal/animation options above.
unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
report = []
for task in tasks:
    for path in task.imported_object_paths:
        asset = unreal.load_asset(path)
        report.append({'path': path, 'class': asset.get_class().get_name()})
(root.parent / 'animal-import.json').write_text(json.dumps(report, indent=2), encoding='utf8')
for path in unreal.EditorAssetLibrary.list_assets('/Game/TestResources/Animals', True, False):
    unreal.EditorAssetLibrary.save_asset(path, False)
unreal.log('CCL_ANIMAL_IMPORT_DONE')
