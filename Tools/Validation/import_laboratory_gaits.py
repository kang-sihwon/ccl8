"""Import separately baked sheep gaits onto the preserved SheepRig root."""
import unreal
for name in ['Walk', 'Idle']:
    task = unreal.AssetImportTask()
    task.filename = unreal.Paths.project_saved_dir() + 'Laboratories/Animals/Sheep' + name + 'Rig.fbx'
    task.destination_path = '/Game/TestResources/Animals/SheepRig'
    task.destination_name = 'A_Sheep' + name
    task.automated = True
    task.save = True
    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    options.import_mesh = False
    options.import_animations = True
    options.skeleton = unreal.load_asset('/Game/TestResources/Animals/SheepRig/SK_SheepRig_Skeleton')
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
unreal.EditorAssetLibrary.save_directory('/Game/TestResources/Animals/SheepRig', False, True)
