import unreal
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for zone in (10, 11):
    asset = assets.load_asset('/Game/Environment/Experiments/DA_Environment_%02d' % zone)
    assert asset
    asset.configure_zone(zone)
    assert assets.save_loaded_asset(asset, False)

def material(name, color=None):
    path = '/Game/Environment/Materials/' + name
    m = assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset(name, '/Game/Environment/Materials', unreal.Material, unreal.MaterialFactoryNew())
    assert m
    unreal.MaterialEditingLibrary.delete_all_material_expressions(m)
    if color is not None:
        m.set_editor_property('used_with_instanced_static_meshes', True)
        node = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionConstant3Vector)
        node.set_editor_property('constant', unreal.LinearColor(*color, 1))
        assert unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        m.set_editor_property('used_with_niagara_sprites', True)
        m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
        m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
        m.set_editor_property('two_sided', True)
        particle = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionParticleColor)
        uv = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate)
        center = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionConstant2Vector)
        center.set_editor_property('r', 0.5)
        center.set_editor_property('g', 0.5)
        radius = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionConstant)
        radius.set_editor_property('r', 0.5)
        mask = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionSphereMask)
        mul = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionMultiply)
        for a,out,b,inp in [(uv,'',mask,'A'),(center,'',mask,'B'),(radius,'',mask,'Radius'),(mask,'',mul,'A'),(particle,'A',mul,'B')]:
            assert unreal.MaterialEditingLibrary.connect_material_expressions(a,out,b,inp), (out,inp)
        assert unreal.MaterialEditingLibrary.connect_material_property(particle,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        assert unreal.MaterialEditingLibrary.connect_material_property(mul,'',unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(m)
    assert assets.save_loaded_asset(m,False)
    return m

sprite = material('M_FireSprite')
fuel = material('M_FireFuel', (0.16,0.045,0.008))
char = material('M_FireChar', (0.009,0.006,0.004))
fx = {}
for name,smoke in [('Flame',False),('RoomSmoke',True)]:
    path = '/Game/Environment/FX/NS_' + name
    system = assets.load_asset(path) if assets.does_asset_exist(path) else assets.duplicate_asset('/Niagara/DefaultAssets/Templates/Systems/FountainLightweight',path)
    assert unreal.CCLFirePresentation.configure_fire_system(system,smoke,sprite)
    assert assets.save_loaded_asset(system,False)
    fx[name] = system
for name in ('EnvironmentPlayground','EnvironmentScenario'):
    assert levels.load_level('/Game/Maps/'+name)
    unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
    existing = [a for a in actors.get_all_level_actors() if isinstance(a,unreal.CCLFirePresentation)]
    assert len(existing)<=1
    view = existing[0] if existing else actors.spawn_actor_from_class(unreal.CCLFirePresentation,unreal.Vector(0,0,0))
    view.set_actor_label('FireAndRoomSmoke')
    view.set_editor_property('flame_system',fx['Flame'])
    view.set_editor_property('smoke_system',fx['RoomSmoke'])
    view.set_editor_property('fuel_material',fuel)
    view.set_editor_property('char_material',char)
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
    unreal.log('CCL_FIRE_ASSETS MAP '+name)
unreal.log('CCL_FIRE_ASSETS PASS')
unreal.SystemLibrary.quit_editor()
