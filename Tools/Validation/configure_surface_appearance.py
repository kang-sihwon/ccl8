"""Build the lab's procedural surface materials and refresh its existing instructions.

Run in the editor with -ExecutePythonScript. These five materials are owned by
this script. No texture downloads or collision changes are involved.
"""
import unreal

assets = unreal.EditorAssetLibrary
edit = unreal.MaterialEditingLibrary

# Detail is world aligned in centimetres; it never displaces the collision surface.
noise = """
float2 q = P.xy;
float broad = 0.5 + 0.25*sin(q.x*0.031 + sin(q.y*0.023)) + 0.25*sin(q.y*0.047 + q.x*0.011);
float grain = frac(sin(dot(floor(q*2.0), float2(12.9898,78.233))) * 43758.5453);
"""
settings = {
    'Water': {
        'color': 'return lerp(float3(0.012,0.075,0.105), float3(0.035,0.21,0.25), broad);',
        'rough': 'return 0.12 + broad*0.1;',
        'normal': 'float2 d=float2(cos(q.x*0.09+q.y*0.035+Phase*0.9),sin(q.y*0.11-q.x*0.025+Phase*1.1)); return normalize(N+float3(d*0.09,0));',
    },
    'Ice': {
        'color': 'float veins=pow(saturate(1-abs(sin(q.x*0.021+sin(q.y*0.038)))*12),4); return lerp(float3(0.16,0.3,0.36),float3(0.65,0.8,0.86),veins*0.6+broad*0.22);',
        'rough': 'return 0.22 + broad*0.18;',
        'normal': 'return normalize(N+float3(sin(q.x*0.05),cos(q.y*0.06),0)*0.035);',
    },
    'Mud': {
        'color': 'return lerp(float3(0.035,0.018,0.008),float3(0.105,0.064,0.029),broad)*(0.9+grain*0.1);',
        'rough': 'return 0.22 + broad*0.35;',
        'normal': 'return normalize(N+float3(sin(q.x*0.5+sin(q.y*0.7)),cos(q.y*0.45+sin(q.x*0.6)),0)*0.16);',
    },
    'Bed': {
        'color': 'return lerp(float3(0.11,0.071,0.035),float3(0.28,0.19,0.1),broad)*(0.8+grain*0.2);',
        'rough': 'return 0.88;',
        'normal': 'return normalize(N+float3(sin(q.x*0.23+q.y*0.17),cos(q.y*0.29),0)*0.12);',
    },
    'Snow': {
        'color': 'return lerp(float3(0.66,0.76,0.86),float3(0.92,0.95,0.99),0.65+broad*0.3)*(0.97+grain*0.03);',
        'rough': 'return 0.78 + grain*0.2;',
        'normal': 'float2 d=float2(sin(q.x*1.5+q.y*0.7),cos(q.y*1.7-q.x*0.6)); return normalize(N+float3(d*0.065,0));',
    },
}

for name, code in settings.items():
    path = '/Game/Environment/Materials/M_Surface' + name
    material = unreal.load_asset(path)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            'M_Surface' + name, '/Game/Environment/Materials', unreal.Material, unreal.MaterialFactoryNew())
    assert material is not None
    edit.delete_all_material_expressions(material)
    material.set_editor_property('two_sided', True)
    material.set_editor_property('tangent_space_normal', False)
    material.set_editor_property('used_with_instanced_static_meshes', True)
    position = edit.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -800, 0)
    normal = edit.create_material_expression(material, unreal.MaterialExpressionVertexNormalWS, -800, 180)
    time = edit.create_material_expression(material, unreal.MaterialExpressionTime, -800, 360)
    for index, (key, prop, output) in enumerate((
        ('color', unreal.MaterialProperty.MP_BASE_COLOR, unreal.CustomMaterialOutputType.CMOT_FLOAT3),
        ('rough', unreal.MaterialProperty.MP_ROUGHNESS, unreal.CustomMaterialOutputType.CMOT_FLOAT1),
        ('normal', unreal.MaterialProperty.MP_NORMAL, unreal.CustomMaterialOutputType.CMOT_FLOAT3),
    )):
        expression = edit.create_material_expression(material, unreal.MaterialExpressionCustom, -360, index*200)
        expression.set_editor_property('description', 'World aligned ' + name + ' ' + key)
        expression.set_editor_property('code', noise + code[key])
        expression.set_editor_property('output_type', output)
        inputs = []
        for input_name in ('P', 'N', 'Phase'):
            pin = unreal.CustomInput()
            pin.set_editor_property('input_name', input_name)
            inputs.append(pin)
        expression.set_editor_property('inputs', inputs)
        assert edit.connect_material_expressions(position, '', expression, 'P')
        assert edit.connect_material_expressions(normal, '', expression, 'N')
        assert edit.connect_material_expressions(time, '', expression, 'Phase')
        assert edit.connect_material_property(expression, '', prop)
    edit.recompile_material(material)
    assert assets.save_loaded_asset(material, False)
    unreal.log('CCL_SURFACE_APPEARANCE saved ' + path)

for index in (0, 1, 3, 4, 5, 8, 11):
    definition = unreal.load_asset('/Game/Environment/Experiments/DA_Environment_%02d' % index)
    assert definition is not None
    definition.configure_zone(index)
    assert assets.save_loaded_asset(definition, False)
unreal.log('CCL_SURFACE_APPEARANCE PASS')
