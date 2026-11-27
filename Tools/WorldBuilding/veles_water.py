"""Install the river splines and register the existing streambed with UE Water."""
import math

import unreal


def stream_surface(folder):
    name="M_VelesFlowingWater"
    if unreal.EditorAssetLibrary.does_asset_exist(folder+"/"+name):
        return unreal.load_asset(folder+"/"+name)
    lib=unreal.MaterialEditingLibrary
    mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property("shading_model",unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
    mat.set_editor_property("two_sided",True)
    mat.set_editor_property("used_with_spline_meshes",True)
    def node(cls):return lib.create_material_expression(mat,cls)
    def rgb(r,g,b):
        n=node(unreal.MaterialExpressionConstant3Vector)
        n.constant=unreal.LinearColor(r,g,b,1)
        return n
    def scalar(v):
        n=node(unreal.MaterialExpressionConstant);n.r=v
        return n
    def link(a,b,pin,port=""):
        pins=[str(n) for n in lib.get_material_expression_input_names(b)]
        if pin=="Coordinates":pin="UVs"
        actual=next((str(p) for p in pins if str(p).replace(" ","").lower()==pin.lower()),pin)
        if not lib.connect_material_expressions(a,port,b,actual):
            if len(pins)!=1 or not lib.connect_material_expressions(a,port,b,""):raise ValueError((pin,pins,port))
    def output(n,prop):assert lib.connect_material_property(n,"",getattr(unreal.MaterialProperty,prop))
    output(rgb(.028,.14,.135),"MP_BASE_COLOR")
    output(scalar(.36),"MP_OPACITY")
    output(scalar(.17),"MP_ROUGHNESS")
    output(scalar(.70),"MP_SPECULAR")
    volume=node(unreal.MaterialExpressionSingleLayerWaterMaterialOutput)
    # 단위는 1/cm. 얕은 물에서 모래가 보일 만큼만 빛을 흡수한다.
    link(rgb(.007,.019,.017),volume,"ScatteringCoefficients")
    link(rgb(.052,.012,.017),volume,"AbsorptionCoefficients")
    uv=node(unreal.MaterialExpressionTextureCoordinate)
    uv.u_tiling=1.8;uv.v_tiling=.85
    pan=node(unreal.MaterialExpressionPanner)
    pan.set_editor_property("speed_x",-.34);pan.set_editor_property("speed_y",.012)
    link(uv,pan,"Coordinate")
    tex=node(unreal.MaterialExpressionTextureSample)
    tex.texture=unreal.load_asset("/Game/ExternalPackage/Textures/Medieval_Environment/T_Medieval_Water_01_N")
    tex.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
    link(pan,tex,"Coordinates")
    # WaterAdvanced가 바인딩하는 플레이어 주변 수면 노멀을 흐름 노멀에 합친다.
    mpc=unreal.load_asset("/Water/Materials/MPC/MPC_Water")
    def collection(param):
        n=node(unreal.MaterialExpressionCollectionParameter)
        n.set_editor_property("collection",mpc);n.set_editor_property("parameter_name",param)
        return n
    world=node(unreal.MaterialExpressionWorldPosition)
    def mask_xy(source):
        mask=node(unreal.MaterialExpressionComponentMask)
        mask.set_editor_property("R",True);mask.set_editor_property("G",True)
        link(source,mask,"Input")
        return mask
    offset=node(unreal.MaterialExpressionSubtract)
    link(mask_xy(world),offset,"A");link(mask_xy(collection("SimLocation")),offset,"B")
    divide=node(unreal.MaterialExpressionDivide)
    link(offset,divide,"A");link(collection("FluidSimSize"),divide,"B")
    center=node(unreal.MaterialExpressionAdd);center.set_editor_property("ConstB",.5)
    link(divide,center,"A")
    rt=node(unreal.MaterialExpressionTextureSampleParameter2D)
    rt.set_editor_property("parameter_name","NormalAndHeight");rt.texture=unreal.load_asset("/Engine/EngineResources/Black")
    link(center,rt,"Coordinates")
    xy=mask_xy(rt)
    perturb=node(unreal.MaterialExpressionAppendVector)
    link(xy,perturb,"A");link(scalar(0),perturb,"B")
    strength=node(unreal.MaterialExpressionScalarParameter);strength.set_editor_property("parameter_name","DEV_UseNewShallowWaterSubsystem")
    scale=node(unreal.MaterialExpressionMultiply);link(perturb,scale,"A");link(strength,scale,"B")
    normal=node(unreal.MaterialExpressionAdd);link(tex,normal,"A","RGB");link(scale,normal,"B")
    output(normal,"MP_NORMAL")
    lib.recompile_material(mat)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
    return mat


def impact_collection(folder):
    assets = unreal.EditorAssetLibrary
    name = "MPC_VelesFootfall"
    if assets.does_asset_exist(folder + "/" + name):
        return unreal.load_asset(folder + "/" + name)
    collection = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    point = unreal.CollectionVectorParameter()
    point.set_editor_property("ParameterName", "FootPosition")
    stamp = unreal.CollectionScalarParameter()
    stamp.set_editor_property("ParameterName", "FootTime")
    stamp.set_editor_property("DefaultValue", -10.0)
    collection.set_editor_property("VectorParameters", [point])
    collection.set_editor_property("ScalarParameters", [stamp])
    assert assets.save_loaded_asset(collection, False)
    return collection


def visible_water(folder, collection):
    name = "M_VelesCurrent"
    path = folder + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("two_sided", True)
    lib = unreal.MaterialEditingLibrary
    def node(cls):
        return lib.create_material_expression(mat, cls)
    def link(a, b, pin, output=""):
        assert lib.connect_material_expressions(a, output, b, pin), pin
    def output(a, prop, channel=""):
        assert lib.connect_material_property(a, channel, getattr(unreal.MaterialProperty, prop))
    def uniform(param):
        n = node(unreal.MaterialExpressionCollectionParameter)
        n.set_editor_property("collection", collection)
        n.set_editor_property("parameter_name", param)
        return n
    flow = node(unreal.MaterialExpressionPanner)
    flow.set_editor_property("speed_x", -.24)
    flow.set_editor_property("speed_y", .014)
    tex = node(unreal.MaterialExpressionTextureSample)
    tex.texture = unreal.load_asset("/Game/ExternalPackage/Textures/Medieval_Environment/T_Medieval_Water_01_N")
    tex.sampler_type = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
    link(flow, tex, "UVs")
    color = node(unreal.MaterialExpressionCustom)
    sources = {"SurfacePos": node(unreal.MaterialExpressionWorldPosition),
               "FootPos": uniform("FootPosition"), "StartTime": uniform("FootTime"),
               "Now": node(unreal.MaterialExpressionTime), "Wave": tex,
               "Edge": node(unreal.MaterialExpressionVertexColor)}
    ports = []
    for param in sources:
        port = unreal.CustomInput()
        port.set_editor_property("InputName", param)
        ports.append(port)
    color.set_editor_property("inputs", ports)
    color.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    color.set_editor_property("code", """
float age = Now - StartTime;
float radius = length(SurfacePos.xy - FootPos.xy);
float front = (radius - age * 95.0) / 2.8;
float ring = exp(-front * front) * saturate(1.0 - age / 0.68) * step(0.0, age);
float crest = saturate(0.38 + Wave.x * 0.55 + Wave.y * 0.38);
float3 current = lerp(float3(.018,.10,.105), float3(.13,.33,.32), crest);
return current + float3(.42,.52,.48) * ring;
""")
    for param, src in sources.items():
        link(src, color, param, "RGB" if param == "Wave" else "")
    output(color, "MP_BASE_COLOR")
    opacity = node(unreal.MaterialExpressionVertexColor)
    output(opacity, "MP_OPACITY", "R")
    rough = node(unreal.MaterialExpressionConstant)
    rough.r = .2
    output(rough, "MP_ROUGHNESS")
    lib.recompile_material(mat)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
    return mat


def draw_surface(art, body, material, name, deep):
    # 렌더 표면과 물 판정이 같은 스플라인 폭·위치·높이를 사용한다.
    spline = body.get_water_spline()
    water = body.get_water_body_component()
    space = unreal.SplineCoordinateSpace.WORLD
    length = spline.get_spline_length()
    count = math.ceil(length / (150 if deep else 24))
    verts, faces, uvs, colors = art["group"]()
    for i in range(count + 1):
        distance = length * i / count
        pos = spline.get_location_at_distance_along_spline(distance, space)
        tangent = spline.get_direction_at_distance_along_spline(distance, space)
        norm = math.hypot(tangent.x, tangent.y)
        key = spline.get_input_key_value_at_distance_along_spline(distance)
        width = water.get_river_width_at_spline_input_key(key)
        for j, side in enumerate((-1, -.80, 0, .80, 1)):
            off = side * width * .5
            verts.append(((pos.x - tangent.y * off / norm) / 100,
                          (pos.y + tangent.x * off / norm) / 100, (pos.z + .7) / 100))
            uvs.append((distance / 160, off / 160))
            alpha = (.88 if deep else .62) * (1 if j in (1, 2, 3) else .12)
            colors.append((alpha, 0, 0, 1))
            k = len(verts) - 1
            if i and j:
                faces.extend(((k-6, k-5, k-1), (k-5, k, k-1)))
    mesh = art["mesh"](name, [(verts, faces, uvs, colors)], [material], False)
    label = body.get_actor_label() + " surface"
    existing = next((a for a in art["actors"].get_all_level_actors() if a.get_actor_label()==label), None)
    actor = art["actor_mesh"](label, mesh, 0, 0, 0, 0, existing)
    actor.set_folder_path("Veles/Water")
    actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    actor.static_mesh_component.set_cast_shadow(False)


def brook_class(folder):
    assets = unreal.EditorAssetLibrary
    name = "NS_BrookFootSplash"
    path = folder+"/"+name
    fx = unreal.load_asset(path) if assets.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
            name,folder,unreal.load_asset("/Niagara/DefaultAssets/Templates/Systems/DirectionalBurstLightweight"))
    assert fx

    def module(name):
        cls = unreal.load_class(None, "/Script/Niagara."+name)
        return next(o for o in unreal.ObjectIterator(cls) if o.get_path_name().startswith(fx.get_path_name()+":"))

    def distribution(obj, prop, text):
        value = obj.get_editor_property(prop)
        value.import_text(text)
        obj.set_editor_property(prop,value)

    particle = module("NiagaraStatelessModule_InitializeParticle")
    distribution(particle,"LifetimeDistribution","(Mode=UniformRange,Min=0.18,Max=0.32,ChannelConstantsAndRanges=(0.18,0.32))")
    distribution(particle,"SpriteSizeDistribution","(Mode=NonUniformRange,Min=(X=0.7,Y=1.2),Max=(X=1.8,Y=3.4),ChannelConstantsAndRanges=(0.7,1.2,1.8,3.4))")
    velocity = module("NiagaraStatelessModule_AddVelocity")
    distribution(velocity,"ConeVelocityDistribution","(Mode=UniformRange,Min=55,Max=120,ChannelConstantsAndRanges=(55,120))")
    velocity.set_editor_property("ConeAngle",95)
    emitter = module("NiagaraStatelessEmitter")
    bursts = emitter.get_editor_property("SpawnInfos")
    distribution(bursts[0],"Amount","(Mode=UniformRange,Min=5,Max=9,ChannelConstantsAndRanges=(5,9))")
    emitter.set_editor_property("SpawnInfos",bursts)
    assert assets.save_loaded_asset(fx,False)

    name = "BP_VelesBrook"
    if assets.does_asset_exist(folder+"/"+name):
        bp=assets.load_asset(folder+"/"+name)
        if assets.get_metadata_tag(bp,"VelesFootImpact")=="2":
            return bp.generated_class()
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class",unreal.WaterBodyRiver)
        bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Blueprint,factory)
    lib = unreal.BlueprintEditorLibrary
    graph = unreal.BlueprintGraphEditor.get_graph_editor(lib.find_event_graph(bp))
    graph.remove_nodes([n for n in graph.list_all_nodes() if n.get_class().get_name() in ("K2Node_IfThenElse","K2Node_CallFunction")])
    def call(fn):
        n = graph.add_call_function_node("/Script/"+fn)
        if not n:raise ValueError(fn)
        return n
    def wire(a,output,b,input_name):
        assert lib.find_output_pin(a,output).try_create_connection(lib.find_input_pin(b,input_name)),(output,input_name)
    def value(n,p,v):
        assert lib.find_input_pin(n,p).set_pin_value(v),(p,v)
    tick = graph.find_event_node("ReceiveTick")
    pawn = call("Engine.GameplayStatics:GetPlayerPawn")
    overlap = call("Engine.Actor:IsOverlappingActor")
    wire(pawn,"ReturnValue",overlap,"Other")
    wet = graph.add_branch_node()
    wire(tick,"then",wet,"execute")
    wire(overlap,"ReturnValue",wet,"Condition")
    speed = call("Engine.Actor:GetVelocity")
    wire(pawn,"ReturnValue",speed,"self")
    length = call("Engine.KismetMathLibrary:VSizeXY")
    wire(speed,"ReturnValue",length,"A")
    moving = call("Engine.KismetMathLibrary:Greater_DoubleDouble")
    wire(length,"ReturnValue",moving,"A")
    value(moving,"B","12")
    step = graph.add_branch_node()
    wire(wet,"then",step,"execute")
    wire(moving,"ReturnValue",step,"Condition")
    location = call("Engine.Actor:K2_GetActorLocation")
    wire(pawn,"ReturnValue",location,"self")
    spline = call("Water.WaterBody:GetWaterSpline")
    surface = call("Engine.SplineComponent:FindLocationClosestToWorldLocation")
    wire(spline,"ReturnValue",surface,"self")
    wire(location,"ReturnValue",surface,"WorldLocation")
    value(surface,"CoordinateSpace","World")
    spawn = call("Niagara.NiagaraFunctionLibrary:SpawnSystemAtLocation")
    wire(step,"then",spawn,"execute")
    wire(surface,"ReturnValue",spawn,"Location")
    value(spawn,"SystemTemplate",fx.get_path_name())
    channel=unreal.load_asset("/WaterAdvanced/Niagara/DataChannels/NDC_ShallowWater")
    writer=call("Niagara.NiagaraDataChannelLibrary:WriteToNiagaraDataChannel")
    wire(spawn,"then",writer,"execute")
    value(writer,"Channel",channel.get_path_name())
    value(writer,"Count","1")
    value(writer,"bVisibleToCPU","true");value(writer,"bVisibleToGPU","true")
    value(writer,"DebugSource","Veles brook footfall")
    last=writer
    for method,param,default in (("WritePosition","Position",None),("WriteVector","Velocity","0,0,-75"),("WriteFloat","Radius","12")):
        write=call("Niagara.NiagaraDataChannelWriter:"+method)
        wire(last,"then",write,"execute");wire(writer,"ReturnValue",write,"self")
        value(write,"VarName",param)
        if default is None:wire(surface,"ReturnValue",write,"InData")
        else:value(write,"InData",default)
        last=write
    collection = impact_collection(folder)
    location_color = call("Engine.KismetMathLibrary:Conv_VectorToLinearColor")
    wire(surface, "ReturnValue", location_color, "InVec")
    clock = call("Engine.KismetSystemLibrary:GetGameTimeInSeconds")
    for kind, param, src in (("Vector","FootPosition",location_color),("Scalar","FootTime",clock)):
        setter = call("Engine.KismetMaterialLibrary:Set" + kind + "ParameterValue")
        wire(last,"then",setter,"execute")
        value(setter,"Collection",collection.get_path_name())
        value(setter,"ParameterName",param)
        wire(src,"ReturnValue",setter,"ParameterValue")
        last = setter
    lib.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    t = cdo.get_editor_property("primary_actor_tick")
    t.set_editor_property("tick_interval",.20)
    cdo.set_editor_property("primary_actor_tick",t)
    assets.set_metadata_tag(bp,"VelesFootImpact","2")
    assert assets.save_loaded_asset(bp,False)
    return bp.generated_class()


def install(art):
    actors = art["actors"]
    by_label = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
    ground = by_label["Vale and village paths"]
    if not ground.get_component_by_class(unreal.WaterTerrainComponent):
        subs = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
        root = subs.k2_gather_subobject_data_for_instance(ground)[0]
        handle, error = subs.add_new_subobject(unreal.AddNewSubobjectParams(
                parent_handle=root, new_class=unreal.WaterTerrainComponent))
        if not ground.get_component_by_class(unreal.WaterTerrainComponent):
            raise RuntimeError(str(error))

    def actor(label, cls, position):
        obj = by_label.get(label) or actors.spawn_actor_from_class(cls, unreal.Vector(*position))
        obj.set_actor_label(label)
        obj.set_folder_path("Veles/Water")
        return obj

    zone = actor("Veles water zone", unreal.WaterZone, (5000, 3000, 0))
    zone.set_editor_property("zone_extent", unreal.Vector2D(72400, 14400))
    zone.set_editor_property("render_target_resolution", unreal.IntPoint(4096, 1024))
    zone.set_editor_property("velocity_blur_radius", 0)
    info = unreal.load_asset("/Water/Materials/WaterInfo/DrawWaterInfo")
    folder = "/Game/Level/VelesRiversideAssets"
    surface=unreal.load_asset("/Water/Materials/WaterSurface/Water_Material_River")
    brook_material=stream_surface(folder)

    def river(label, samples, material, cls=unreal.WaterBodyRiver):
        first = samples[0][:3]
        existing = by_label.get(label)
        wanted_class = cls.static_class() if isinstance(cls,type) else cls
        if existing and existing.get_class()!=wanted_class:
            actors.destroy_actor(by_label.pop(label))
        body = actor(label, cls, first)
        comp = body.get_water_body_component()
        comp.set_editor_property("affects_landscape", False)
        comp.set_water_zone_override(zone)
        comp.set_water_material(material)
        comp.set_water_info_material(info)
        comp.set_editor_property("water_mesh_override",unreal.load_asset("/Water/Meshes/RiverMesh") if material==brook_material else None)
        spline = body.get_water_spline()
        space = unreal.SplineCoordinateSpace.WORLD
        spline.set_spline_points([unreal.Vector(*row[:3]) for row in samples], space, True)
        for i, (_, _, _, width, depth, velocity) in enumerate(samples):
            comp.set_river_width_at_spline_input_key(i, width)
            comp.set_river_depth_at_spline_input_key(i, depth)
            comp.set_water_velocity_at_spline_input_key(i, velocity)
        spline.call_method("K2_SynchronizeAndBroadcastDataChange")
        for i, row in enumerate(samples):
            assert abs(comp.get_river_width_at_spline_input_key(i)-row[3]) < .1
        return body

    main = []
    for x in range(-300, 401, 4):
        width = 29.6 + 3 * math.sin(x*.032)
        center = art["shore"](x) + .30 - width/2
        gradient = (art["shore"](x+.1)-art["shore"](x-.1))/.2 - .048*math.cos(x*.032)
        main.append((x*100, center*100, 9, width*100/math.hypot(1, gradient), 260, 75))
    main_body = river("Veles main river", main, surface)
    sections = list(art["spring_sections"]().items())
    creek = []
    for i in range(0, len(sections), 2):
        y, (x, z, half_width) = sections[i]
        ay, (ax, az, _) = sections[max(i-1, 0)]
        by, (bx, bz, _) = sections[min(i+1, len(sections)-1)]
        tangent = math.hypot(bx-ax, by-ay)
        width = 200*(half_width+.04)*abs(by-ay)/tangent
        creek.append((x*100, y*100, z*100, width, 17, 22+min(38, (az-bz)/tangent*180)))
    creek_body = river("Veles village brook", creek, brook_material,brook_class(folder))
    current = visible_water(folder, impact_collection(folder))
    draw_surface(art, main_body, current, "MainRiverCurrent", True)
    draw_surface(art, creek_body, current, "BrookCurrent", False)
    old = by_label.get("River and spring runnel")
    if old:
        actors.destroy_actor(old)
