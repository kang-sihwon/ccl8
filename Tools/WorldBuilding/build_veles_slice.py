"""Create an art trial at the existing Veles coordinates, preserving the source map.

Run in editor Python after prepare_veles_blockout.py. The cloned map's VB_
actors may be hidden or receive component overrides. New actors use VA_ and
new assets stay under VelesArtSlice. Shared meshes/materials are read-only.
"""
from pathlib import Path
import json
import math
import random
import unreal

ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
DATA=json.loads((ROOT/'Saved/WorldBuilding/VelesBlockout/layout.json').read_text(encoding='utf-8'))
MAP='/Game/Maps/VelesArtSlice'
ASSETS='/Game/WorldBuilding/VelesArtSlice'
LIB=unreal.EditorAssetLibrary
MEL=unreal.MaterialEditingLibrary
ROUTE=next(r['points'] for r in DATA['roads'] if r['id']=='R01')[:4]


def segment_distance(p,a,b):
    dx,dy=b[0]-a[0],b[1]-a[1]
    t=max(0,min(1,((p[0]-a[0])*dx+(p[1]-a[1])*dy)/max(.0001,dx*dx+dy*dy)))
    return math.hypot(p[0]-a[0]-t*dx,p[1]-a[1]-t*dy)


def route_distance(p):
    return min(segment_distance(p,a,b) for a,b in zip(ROUTE,ROUTE[1:]))


def height(x,y):
    # Match the two triangles of the original 8 m terrain cell exactly.
    fx=(x+512)/8; fy=(y+128)/8
    ix=max(0,min(143,int(fx))); iy=max(0,min(207,int(fy)))
    u=max(0,min(1,fx-ix)); v=max(0,min(1,fy-iy))
    n=iy*145+ix
    z=[DATA['terrain']['vertices'][i][2]/100 for i in (n,n+1,n+145,n+146)]
    return z[0]+u*(z[1]-z[0])+v*(z[2]-z[0]) if u+v<=1 else z[3]+(1-u)*(z[2]-z[3])+(1-v)*(z[1]-z[3])


def make_material(name,color,texture=None,normal=None,rough=.9,noise=False):
    path=ASSETS+'/M_'+name
    if LIB.does_asset_exist(path):return unreal.load_asset(path)
    m=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_'+name,ASSETS,unreal.Material,unreal.MaterialFactoryNew())
    def node(cls):return MEL.create_material_expression(m,cls)
    c=node(unreal.MaterialExpressionConstant3Vector)
    c.set_editor_property('constant',unreal.LinearColor(*color,1))
    source=c; channel=''
    if texture:
        tex=node(unreal.MaterialExpressionTextureSample)
        tex.set_editor_property('texture',unreal.load_asset(texture))
        mul=node(unreal.MaterialExpressionMultiply)
        MEL.connect_material_expressions(tex,'RGB',mul,'A'); MEL.connect_material_expressions(c,'',mul,'B')
        source=mul
    elif noise:
        n=node(unreal.MaterialExpressionNoise)
        n.set_editor_property('scale',.025)
        n.set_editor_property('levels',2)
        n.set_editor_property('output_min',.48)
        n.set_editor_property('output_max',1.15)
        mul=node(unreal.MaterialExpressionMultiply)
        MEL.connect_material_expressions(n,'',mul,'A'); MEL.connect_material_expressions(c,'',mul,'B')
        source=mul
    MEL.connect_material_property(source,channel,unreal.MaterialProperty.MP_BASE_COLOR)
    if normal:
        n=node(unreal.MaterialExpressionTextureSample)
        n.set_editor_property('texture',unreal.load_asset(normal))
        n.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        MEL.connect_material_property(n,'RGB',unreal.MaterialProperty.MP_NORMAL)
    r=node(unreal.MaterialExpressionConstant);r.set_editor_property('r',rough)
    MEL.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
    m.set_editor_property('two_sided',True)
    MEL.recompile_material(m);LIB.save_loaded_asset(m,False)
    return m


def make_mesh(name,vertices,triangles,uv=None,collision=True):
    path=ASSETS+'/SM_'+name
    if LIB.does_asset_exist(path):return unreal.load_asset(path)
    dynamic=unreal.DynamicMesh()
    b=unreal.GeometryScriptSimpleMeshBuffers()
    b.set_editor_property('vertices',[unreal.Vector(*v) for v in vertices])
    b.set_editor_property('normals',[unreal.Vector(0,0,1) for v in vertices])
    b.set_editor_property('uv0',[unreal.Vector2D(*p) for p in (uv or [(v[0]/200,v[1]/200) for v in vertices])])
    b.set_editor_property('triangles',[unreal.IntVector(*t) for t in triangles])
    unreal.GeometryScript_MeshEdits.append_buffers_to_mesh(dynamic,b)
    opts=unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
    opts.set_editor_property('enable_recompute_normals',True)
    opts.set_editor_property('enable_recompute_tangents',True)
    opts.set_editor_property('enable_collision',collision)
    opts.set_editor_property('collision_mode',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    mesh,result=unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(dynamic,path,opts)
    assert mesh,str(result)
    LIB.save_loaded_asset(mesh,False)
    return mesh


def foliage_material():
    path=ASSETS+'/M_MapleLeaves'
    if LIB.does_asset_exist(path):return unreal.load_asset(path)
    m=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_MapleLeaves',ASSETS,unreal.Material,unreal.MaterialFactoryNew())
    m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property('two_sided',True)
    m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    t=MEL.create_material_expression(m,unreal.MaterialExpressionTextureSample)
    t.set_editor_property('texture',unreal.load_asset('/Game/TestResources/NorwayMaple/Textures/Twosided/Summer/T_NorwayMaple_Twosided_Summer_Albedo'))
    MEL.connect_material_property(t,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(t,'A',unreal.MaterialProperty.MP_OPACITY_MASK)
    MEL.connect_material_property(t,'RGB',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    MEL.recompile_material(m);LIB.save_loaded_asset(m,False)
    return m


def build():
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert not levels.is_in_play_in_editor(),'Stop PIE before authoring.'
    if not LIB.does_asset_exist(MAP):
        assert levels.new_level_from_template(MAP,'/Game/Maps/VelesBlockout')
    assert levels.load_level(MAP)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_name()=='VelesArtSlice'
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    by_label={a.get_actor_label():a for a in actors.get_all_level_actors()}
    existing=LIB.get_metadata_tag(world,'ArtSliceOwner')
    assert existing in ('','VelesArtSlice_20261116')
    LIB.set_metadata_tag(world,'ArtSliceOwner','VelesArtSlice_20261116')
    cube=unreal.load_asset('/Engine/BasicShapes/Cube')
    woodroot='/Game/TestResources/Fishermans_Cabin/Textures/Tiling_Textures/Wood_Exterior/'
    wood=make_material('Timber',(.85,.72,.56),woodroot+'T_Wood_Exterior_DO_01',woodroot+'T_Wood_Exterior_N')
    plaster=make_material('PlasterSoft',(.53,.46,.32))
    stone=make_material('Rock',(.23,.25,.22),noise=True)
    roof=make_material('Roof',(.24,.20,.12),woodroot+'T_Wood_Exterior_DO_01',woodroot+'T_Wood_Exterior_N')
    dirt=make_material('Earth',(.28,.20,.105),noise=True)
    grass=make_material('GroundSoft',(.09,.13,.045))
    dark=make_material('Window',(.035,.047,.037),rough=.35)
    water=make_material('Water',(.025,.12,.14),rough=.18)
    counts={}
    def spawn(label,mesh,loc,scale=(1,1,1),yaw=0,mat=None,collision=True,folder='Architecture',pitch=0):
        a=by_label.get(label)
        if a is None:
            a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*[v*100 for v in loc]))
            a.set_actor_label(label);by_label[label]=a
        a.set_actor_location(unreal.Vector(*[v*100 for v in loc]),False,False)
        a.set_actor_rotation(unreal.Rotator(pitch=pitch,yaw=yaw,roll=0),False)
        a.set_actor_scale3d(unreal.Vector(*scale))
        a.static_mesh_component.set_static_mesh(mesh)
        if mat:a.static_mesh_component.set_material(0,mat)
        a.set_actor_enable_collision(collision)
        a.set_folder_path('VelesArtSlice/'+folder)
        counts[folder]=counts.get(folder,0)+1
        return a
    def hide(a,keep_collision=False):
        a.set_actor_hidden_in_game(True)
        a.set_is_temporarily_hidden_in_editor(True)
        if hasattr(a,'static_mesh_component'):a.static_mesh_component.set_visibility(False)
        a.set_actor_enable_collision(keep_collision)
    # The original two-sided surface hid downward-facing collision triangles.
    # Create an upward-facing, independently owned terrain; never edit the source.
    terrain=make_mesh('TerrainUpward',DATA['terrain']['vertices'],[[a,c,b] for a,b,c in DATA['terrain']['triangles']])
    by_label['VB_Terrain'].static_mesh_component.set_static_mesh(terrain)
    by_label['VB_Terrain'].static_mesh_component.set_material(0,grass)
    buildings=[a for a in DATA['actors'] if a['label'].startswith('VB_Building_')]
    selected=[b for b in buildings if route_distance([b['location'][0]/100,b['location'][1]/100])<60]
    for label,a in list(by_label.items()):
        p=a.get_actor_location();d=route_distance([p.x/100,p.y/100])
        if label.startswith(('VB_POI_','VB_Door_')) and d<90:hide(a)
        if label.startswith('VB_TreeProxy_') and (d<110 or math.hypot(p.x/100-260,p.y/100-1250)<65):hide(a)
        if label.startswith('VB_Road_') and d<80:hide(a)
        if label.startswith('VB_Water_'):a.static_mesh_component.set_material(0,water)
    # Ground-conforming road surfaces; nearby old slabs are hidden and disabled.
    verts=[];tris=[]
    for road in DATA['roads']:
        for a,b in zip(road['points'],road['points'][1:]):
            length=math.dist(a,b);steps=max(1,math.ceil(length/1.5))
            dx=(b[0]-a[0])/max(.001,length);dy=(b[1]-a[1])/max(.001,length)
            for j in range(steps):
                t0=j/steps;t1=(j+1)/steps
                c=[a[k]+(b[k]-a[k])*(t0+t1)/2 for k in range(2)]
                if route_distance(c)>82:continue
                width=road.get('width',3)*.5+.12
                base=len(verts)
                for t,s in ((t0,-1),(t0,1),(t1,-1),(t1,1)):
                    x=a[0]+(b[0]-a[0])*t-dy*width*s;y=a[1]+(b[1]-a[1])*t+dx*width*s
                    verts.append([x*100,y*100,(height(x,y)+.025)*100])
                tris.extend([[base,base+2,base+1],[base+1,base+2,base+3]])
    road_actor=spawn('VA_RoadSurface',make_mesh('RoadSurfaceUpward',verts,[[a,c,b] for a,b,c in tris],collision=False),(0,0,0),mat=dirt,collision=False,folder='Ground')
    road_actor.static_mesh_component.set_editor_property('cast_shadow',False)
    # Pitched roofs, timber framing, windows, doors and foundations at source footprints.
    for b in selected:
        key=b['label'].replace('VB_Building_','');x,y,zc=[v/100 for v in b['location']]
        w,d,h=b['scale'];z=zc-h/2;wh=h*.66;rh=h-wh;yaw=b['yaw'];rad=math.radians(yaw)
        hide(by_label[b['label']])
        def local(name,offset,size,material=wood,collide=True):
            ox,oy,oz=offset
            return spawn('VA_'+key+'_'+name,cube,(x+ox*math.cos(rad)-oy*math.sin(rad),y+ox*math.sin(rad)+oy*math.cos(rad),z+oz),size,yaw,material,collide)
        local('Foundation',(0,0,.18),(w+.2,d+.2,.36),stone)
        local('Walls',(0,0,wh/2+.22),(w,d,wh),plaster)
        # Roof is a six-vertex triangular prism with its own metre-scaled UVs.
        rv=[[-(w/2+.65)*100,-(d/2+.7)*100,wh*100],[(w/2+.65)*100,-(d/2+.7)*100,wh*100],
            [0,-(d/2+.7)*100,(wh+rh+.35)*100],[-(w/2+.65)*100,(d/2+.7)*100,wh*100],
            [(w/2+.65)*100,(d/2+.7)*100,wh*100],[0,(d/2+.7)*100,(wh+rh+.35)*100]]
        rt=[[0,2,1],[3,4,5],[0,3,5],[0,5,2],[2,5,4],[2,4,1],[0,1,4],[0,4,3]]
        spawn('VA_'+key+'_Roof',make_mesh('Roof_'+key,rv,rt),(x,y,z+.22),yaw=yaw,mat=roof)
        for k,ox in enumerate((-w/2+.12,0,w/2-.12)):
            for side in (-1,1):local('Post_'+str(k)+'_'+str(side),(ox,side*(d/2+.04),wh/2+.2),(.20,.18,wh),wood)
        for side in (-1,1):
            local('Beam_'+str(side),(0,side*(d/2+.08),wh-.1),(w,.22,.24),wood)
            for k,ox in enumerate((-w*.28,w*.28)):
                local('Window_'+str(side)+'_'+str(k),(ox,side*(d/2+.13),wh*.52),(1.25,.10,1.35),dark,False)
                local('Sill_'+str(side)+'_'+str(k),(ox,side*(d/2+.20),wh*.52-.72),(1.55,.35,.15),wood)
        local('Door',(0,-d/2-.12,1.35),(1.4,.18,2.5),wood)
        local('Chimney',(w*.27,d*.2,h-.2),(1.1,1.2,2.1),stone)
        # Low yard fences leave the source doorway and street connections open.
        for side in (-1,1):
            for k in range(5):
                local('FencePost_'+str(side)+'_'+str(k),(side*(w/2+2.2),-d/2+k*d/4,.55),(.16,.16,1.1),wood)
            for zh in (.4,.9):local('FenceRail_'+str(side)+'_'+str(zh),(side*(w/2+2.2),0,zh),(.1,d,.12),wood)
    unreal.log('VELES_ART architecture '+str(len(selected)))
    # Reusable irregular rock mesh, including a cave mass outside the art trial.
    rng=random.Random(116)
    rv=[];rt=[];rings=6;sides=12
    for i in range(rings+1):
        phi=math.pi*i/rings
        for j in range(sides):
            t=2*math.pi*j/sides;r=1+rng.uniform(-.14,.14)
            rv.append([math.sin(phi)*math.cos(t)*50*r,math.sin(phi)*math.sin(t)*50*r,math.cos(phi)*50*r])
    for i in range(rings):
        for j in range(sides):
            a=i*sides+j;b=i*sides+(j+1)%sides;c=a+sides;d=b+sides
            rt.extend([[a,b,c],[b,d,c]])
    rock=make_mesh('WeatheredRock',rv,rt)
    # Stream banks are offset from the centreline, preserving bridge crossings.
    connections=json.loads((ROOT/'Docs/ArtDirection/EtherWorldMap/Atlas/VelesDetailTerrain-v1/data/connections.json').read_text(encoding='utf-8'))
    bank_count=0
    for a,b in zip(connections['stream']['points'],connections['stream']['points'][1:]):
        length=math.dist(a,b);dx=(b[0]-a[0])/length;dy=(b[1]-a[1])/length
        for j in range(max(1,int(length/4))):
            t=j/max(1,int(length/4));cx=a[0]+(b[0]-a[0])*t;cy=a[1]+(b[1]-a[1])*t
            if route_distance([cx,cy])>70:continue
            for side in (-1,1):
                x=cx-dy*2.1*side;y=cy+dx*2.1*side
                if route_distance([x,y])<4:continue
                spawn('VA_Bank_'+str(bank_count),rock,(x,y,height(x,y)+.15),(rng.uniform(1.3,2.5),rng.uniform(1.5,3),rng.uniform(.8,1.4)),rng.uniform(0,360),stone,True,'Banks')
                bank_count+=1
    # Shape a rocky entrance; keep the 5 m opening and both approach paths clear.
    for k,(x,y,sx,sy,sz) in enumerate([(248,1257,16,22,17),(273,1259,17,24,19),(260,1260,10,19,7),(241,1269,23,29,24),(282,1272,22,31,22),(269,1287,26,20,22)]):
        z=height(x,y)+sz*.33+(10 if k==2 else 0)
        spawn('VA_CaveMass_'+str(k),rock,(x,y,z),(sx,sy,sz),k*37,stone,True,'Cave')
    # Actual imported vegetation. Keep trunks, bushes and fences off authored roads.
    paths={
        'Tree':'/Game/TestResources/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Field_01',
        'Sapling':'/Game/TestResources/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Sapling_01',
        'Bush':'/Game/TestResources/Fishermans_Cabin/Meshes/Foliage/Bush/SM_Bush_01',
        'Grass':'/Game/TestResources/Fishermans_Cabin/Meshes/Foliage/Grass/SM_Grass_01',
        'Fern':'/Game/TestResources/PN_Fern/Meshes/fern_01_01'}
    meshes={k:unreal.load_asset(v) for k,v in paths.items()}
    assert all(meshes.values())
    leaves=foliage_material()
    def clear(x,y,margin):
        for b in buildings:
            bx,by=b['location'][0]/100,b['location'][1]/100;r=math.radians(-b['yaw'])
            lx=(x-bx)*math.cos(r)-(y-by)*math.sin(r);ly=(x-bx)*math.sin(r)+(y-by)*math.cos(r)
            if abs(lx)<b['scale'][0]/2+margin and abs(ly)<b['scale'][1]/2+margin:return False
        for road in DATA['roads']:
            if any(segment_distance([x,y],a,b)<road.get('width',3)/2+margin for a,b in zip(road['points'],road['points'][1:])):return False
        if any(segment_distance([x,y],a,b)<3 for a,b in zip(connections['stream']['points'],connections['stream']['points'][1:])):return False
        return True
    for kind,total in [('Tree',70),('Sapling',95),('Bush',140),('Fern',170),('Grass',320)]:
        placed=0
        for attempt in range(total*40):
            if placed>=total:break
            x=rng.uniform(-80,195);y=rng.uniform(45,375)
            if kind=='Tree' and y<235 and rng.random()<.8:continue
            if route_distance([x,y])>105 or not clear(x,y,4 if kind=='Tree' else 1.1):continue
            s=rng.uniform(.6,.95) if kind=='Tree' else rng.uniform(.75,1.4)
            plant=spawn('VA_'+kind+'_'+str(placed),meshes[kind],(x,y,height(x,y)),(s,s,s),rng.uniform(0,360),None,kind in ('Tree','Sapling'),'Vegetation')
            if kind in ('Tree','Sapling'):
                plant.static_mesh_component.set_editor_property('disallow_nanite',False)
                plant.static_mesh_component.set_editor_property('evaluate_world_position_offset',True)
                plant.static_mesh_component.set_material(0,leaves)
            placed+=1
    # Existing imported props supply close-range scale and material detail.
    pallet=unreal.load_asset('/Game/TestResources/Fishermans_Cabin/Meshes/Wood_Pallet/SM_Wood_Pallet')
    barrel=unreal.load_asset('/Game/TestResources/Weapon_Pack/Mesh/Props/Interior/SM_Barrel')
    for i,(x,y) in enumerate([(48,116),(49.8,116),(50,119),(24,186),(26,185),(-27,91),(-28,93)]):
        spawn('VA_Prop_'+str(i),pallet if i<4 else barrel,(x,y,height(x,y)),yaw=i*29,folder='Props')
    for label,a in list(by_label.items()):
        if label.startswith('VB_Bridge') and route_distance([a.get_actor_location().x/100,a.get_actor_location().y/100])<90:
            a.static_mesh_component.set_material(0,wood)
            source=next(b for b in DATA['actors'] if b['label']==label)
            a.set_actor_location(unreal.Vector(source['location'][0],source['location'][1],source['location'][2]+25),False,False)
            a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=source['yaw'],roll=0),False)
            # A sloped apron on all four edges also supports a turn on a bridge.
            bx,by,bz=[v/100 for v in source['location']]
            sx,sy,sz=source['scale'];rad=math.radians(source['yaw'])
            vertices=[]
            for outer in (False,True):
                hx=sx/2+(2 if outer else 0);hy=sy/2+(2 if outer else 0)
                for ox,oy in ((-hx,-hy),(hx,-hy),(hx,hy),(-hx,hy)):
                    x=bx+ox*math.cos(rad)-oy*math.sin(rad);y=by+ox*math.sin(rad)+oy*math.cos(rad)
                    z=height(x,y)+.015 if outer else bz+sz/2+.25
                    vertices.append([x*100,y*100,z*100])
            triangles=[]
            for i in range(4):
                j=(i+1)%4
                triangles.extend([[i,j,j+4],[i,j+4,i+4]])
            spawn('VA_Apron_'+label,make_mesh('Apron_'+label,vertices,triangles),(0,0,0),mat=wood,folder='Bridges')
    assert levels.save_current_level()
    LIB.save_directory(ASSETS,False,True)
    report={'map':MAP,'source_map':'/Game/Maps/VelesBlockout','route_m':sum(math.dist(a,b) for a,b in zip(ROUTE,ROUTE[1:])),
        'counts':counts,'buildings':len(selected),'actors_total':len(actors.get_all_level_actors()),
        'imported_resources':paths,'limits':'Exterior trial; no building interiors, combat or final water simulation.'}
    (ROOT/'Saved/WorldBuilding/VelesBlockout/art-build.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log('VELES_ART_BUILD '+json.dumps(report))


build()
