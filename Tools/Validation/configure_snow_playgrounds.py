"""Install stage 5 diagnostic materials and refresh existing water/terrain stations."""
import unreal
assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for name, color, roughness in (
    ('Water', (0.01, 0.23, 0.34), 0.2),
    ('Ice', (0.65, 0.85, 0.95), 0.3),
    ('Mud', (0.08, 0.035, 0.012), 0.45),
    ('Bed', (0.32, 0.22, 0.1), 0.9),
    ('Snow', (0.86, 0.91, 0.98), 0.95),
):
    path = '/Game/Environment/Materials/M_Surface' + name
    material = unreal.load_asset(path) if assets.does_asset_exist(path) else None
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_Surface' + name, '/Game/Environment/Materials', unreal.Material, unreal.MaterialFactoryNew())
        tint = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -300, 0)
        tint.set_editor_property('constant', unreal.LinearColor(*color, 1.0))
        unreal.MaterialEditingLibrary.connect_material_property(tint, '', unreal.MaterialProperty.MP_BASE_COLOR)
        rough = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 160)
        rough.set_editor_property('r', roughness)
        unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(material)
    assert material
    material.set_editor_property("used_with_instanced_static_meshes", True)
    unreal.MaterialEditingLibrary.recompile_material(material)
    assert assets.save_loaded_asset(material, False)
assert unreal.CCLSnowAuthoring.build_post_process_asset()
assert assets.save_asset('/Game/Environment/Animation/ABP_SnowPostProcess', False)
definitions = []
for index in (3, 4, 5):
    definition = unreal.load_asset('/Game/Environment/Experiments/DA_Environment_%02d' % index)
    assert definition
    definition.configure_zone(index)
    assert assets.save_loaded_asset(definition, False)
    definitions.append(definition)
for name in ('EnvironmentPlayground', 'EnvironmentScenario'):
    assert levels.load_level('/Game/Maps/' + name)
    unreal.WorldPartitionBlueprintLibrary.load_actors([a.guid for a in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.CCLExperimentStation) and actor.get_editor_property('definition') in definitions:
            actor.modify()
            actor.refresh_label()
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log('CCL_SNOW_ASSETS saved ' + name)
