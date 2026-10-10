import unreal
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
font = assets.load_asset('/Game/Environment/Fonts/F_VegetationLabel')
assert font, 'Run create_vegetation_font.py first'
asset = assets.load_asset('/Game/Environment/Experiments/DA_Environment_07')
assert asset
asset.configure_zone(7)
assert assets.save_loaded_asset(asset, False)
materials = []
for name, color in [('Living', (0.08, 0.3, 0.035)), ('Dormant', (0.32, 0.16, 0.035)), ('Char', (0.015, 0.008, 0.004)), ('Flower', (0.65, 0.12, 0.22))]:
    path = '/Game/Environment/Materials/M_Vegetation' + name
    material = assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset('M_Vegetation' + name, '/Game/Environment/Materials', unreal.Material, unreal.MaterialFactoryNew())
    assert material
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property('used_with_instanced_static_meshes', True)
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    node.set_editor_property('constant', unreal.LinearColor(*color, 1))
    assert unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant)
    roughness.set_editor_property('r', 0.85)
    assert unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    assert assets.save_loaded_asset(material, False)
    materials.append(material)
for name in ('EnvironmentPlayground', 'EnvironmentScenario'):
    assert levels.load_level('/Game/Maps/' + name)
    unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
    existing = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.CCLVegetationPresentation)]
    assert len(existing) <= 1
    view = existing[0] if existing else actors.spawn_actor_from_class(unreal.CCLVegetationPresentation, unreal.Vector(0, 0, 0))
    view.set_actor_label('VegetationLifecycle')
    view.set_editor_property('is_spatially_loaded', False)
    view.set_editor_property('state_materials', materials)
    labels = view.get_components_by_class(unreal.TextRenderComponent)
    assert len(labels) == 9
    for label in labels:
        label.set_font(font)
    view.refresh_preview()
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log('CCL_VEGETATION_ASSET PASS ' + name)
unreal.SystemLibrary.quit_editor()
