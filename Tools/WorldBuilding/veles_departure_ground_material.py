"""One world-space ground palette and a surveyed trail mask for the departure valley."""
from pathlib import Path
import math
import struct
import zlib

import unreal


def trail_mask(land, folder):
    art, forest = land["ART"], land["FOREST"]
    size, metres = 2048, .75
    mask = bytearray(size * size)
    roads = [(line, 2.3 if i < 3 else 1.55) for i, line in enumerate(art["lanes"])]
    roads.extend((line, 1.8) for line in forest["routes"])
    for line, radius in roads:
        for (ax, ay), (bx, by) in zip(line, line[1:]):
            pad = radius + 2
            dx, dy = bx-ax, by-ay
            length_sq = max(.0001, dx*dx+dy*dy)
            for j in range(max(0, int((min(ay,by)-pad+128)/metres)), min(size, int((max(ay,by)+pad+128)/metres)+1)):
                y = (j+.5)*metres-128
                for i in range(max(0,int((min(ax,bx)-pad+384)/metres)), min(size,int((max(ax,bx)+pad+384)/metres)+1)):
                    x = (i+.5)*metres-384
                    t = max(0,min(1,((x-ax)*dx+(y-ay)*dy)/length_sq))
                    dist = math.hypot(x-ax-t*dx,y-ay-t*dy)
                    edge = radius+.32*math.sin(.52*x+.31*y)+.24*math.sin(.68*y-.22*x)
                    k = j*size+i
                    mask[k] = max(mask[k],round(255*land["smooth"]((edge+1.05-dist)/1.75)))
    def chunk(kind, data):
        return struct.pack(">I",len(data))+kind+data+struct.pack(">I",zlib.crc32(kind+data))
    rows=b"".join(b"\x00"+mask[j*size:(j+1)*size] for j in range(size))
    name="T_DepartureTrails"
    saved=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/"WorldBuilding"
    saved.mkdir(exist_ok=True)
    path=saved/(name+".png")
    path.write_bytes(b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",size,size,8,0,0,0,0))
                     +chunk(b"IDAT",zlib.compress(rows,9))+chunk(b"IEND",b""))
    task=unreal.AssetImportTask()
    task.filename=str(path); task.destination_path=folder; task.automated=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    tex=unreal.load_asset(folder+"/"+name)
    tex.set_editor_property("srgb",False)
    tex.set_editor_property("compression_settings",unreal.TextureCompressionSettings.TC_MASKS)
    for axis in ("address_x","address_y"):
        tex.set_editor_property(axis,unreal.TextureAddress.TA_CLAMP)
    assert unreal.EditorAssetLibrary.save_loaded_asset(tex,False)
    return tex


def make_material(land, refresh=False):
    folder=land["ROOT"]+"/Geometry"
    name="M_DepartureValleyGround"
    lib=unreal.EditorAssetLibrary
    mat=unreal.load_asset(folder+"/"+name) if lib.does_asset_exist(folder+"/"+name) else None
    if mat and not refresh:
        return mat
    mask_path=folder+"/T_DepartureTrails"
    mask=unreal.load_asset(mask_path) if lib.does_asset_exist(mask_path) else trail_mask(land,folder)
    edit=unreal.MaterialEditingLibrary
    if mat:
        mat.modify()
        edit.delete_all_material_expressions(mat)
    else:
        mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    def node(cls):
        return edit.create_material_expression(mat,cls)
    def custom(code, inputs, output):
        expr=node(unreal.MaterialExpressionCustom)
        ports=[]
        for key in inputs:
            port=unreal.CustomInput();port.set_editor_property("input_name",key);ports.append(port)
        expr.set_editor_property("inputs",ports);expr.set_editor_property("output_type",output)
        expr.set_editor_property("code",code)
        for key,(source,channel) in inputs.items():
            assert edit.connect_material_expressions(source,channel,expr,key),key
        return expr
    pos=node(unreal.MaterialExpressionWorldPosition)
    world={"Pos":(pos,"")}
    atlas_uv=custom("return (Pos.xy*0.01+float2(384,128))/1536;",world,unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    tex_uv=custom("return Pos.xy/300.0;",world,unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    def sample(texture, uv, masks=False):
        tex=node(unreal.MaterialExpressionTextureSample);tex.texture=texture
        if masks:tex.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
        assert edit.connect_material_expressions(uv,"",tex,"UVs") or edit.connect_material_expressions(uv,"",tex,"")
        return tex
    inputs=dict(world)
    inputs["Trail"]=(sample(mask,atlas_uv,True),"R")
    for key,tex in (("Grass","Grass"),("Litter","Forest"),("Earth","Dirt"),("Sand","Sand"),("Herbs","Clovers")):
        asset=land["ART"]["asset"]("Textures/Medieval_Environment/T_Ground_"+tex+"_Near_01_BC")
        assert asset,key
        inputs[key]=(sample(asset,tex_uv),"RGB")
    shade=custom("""
float2 p=Pos.xy*.01;
float x=p.x,y=p.y;
float2 cell=p/66.0;
float2 grid=floor(cell), f=frac(cell);
f=f*f*(3-2*f);
float4 h=frac(sin(dot(grid,float2(83.21,27.19))+float4(0,83.21,27.19,110.40))*4327.253);
float broad=(lerp(lerp(h.x,h.y,f.x),lerp(h.z,h.w,f.x),f.y)-.5)*2.8;
float detail=sin(.74*x+.39*y)*.36+sin(1.21*y-.68*x)*.22;
float wood=smoothstep(95,300,y+36*broad);
float clearing=smoothstep(-1.2,1.6,broad);
float mainShore=11+3.4*sin(x*.047)+2.5*sin(x*.096+.5);
float mainDist=max(max(y-mainShore, mainShore-29-3*sin(x*.032)-y),max(-300-x,x-400));
float join=smoothstep(74,180,y);
float creek=lerp(-165,-74+.325*(y-205)+13*sin((y-215)/58)+5*sin(y/25),join);
float river=lerp(315,320+.46*(y-205)+22*sin(y/95),join);
float dist=min(max(abs(x-creek)-2.35-.65*cos(max(y,180)/38),y-1248),max(abs(x-river)-16,y-1320));
dist=lerp(100,dist,smoothstep(-3,8,y));
dist=min(mainDist,dist);
float wet=1-smoothstep(-.35,2.2+.6*broad,dist+detail);
float bank=1-smoothstep(1,7.5+broad*1.4,dist+detail);
float yard=1-smoothstep(8,12+detail,length((p-float2(-3,94))*float2(.89,1)));
float worn=saturate(max(Trail,yard*.85)*(1+.12*detail));
float3 meadow=lerp(Grass*float3(.52,.53,.34),Herbs*float3(.46,.51,.30),.15+.23*clearing);
float3 forest=lerp(Litter*float3(.57,.53,.37),meadow,.32+.29*clearing);
float3 soil=lerp(meadow,forest,wood);
float3 verge=lerp(soil,Litter*float3(.66,.53,.36),.43*bank);
float3 loam=Earth*float3(.75,.59,.40);
float3 path=lerp(verge,loam,worn*.91);
float3 shore=lerp(loam*.72,Sand*float3(.65,.56,.40),.52+.15*clearing);
return lerp(path,shore,wet*.88)*(.94+.06*broad);
""",inputs,unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    assert edit.connect_material_property(shade,"",unreal.MaterialProperty.MP_BASE_COLOR)
    rough=node(unreal.MaterialExpressionConstant);rough.r=.89
    edit.connect_material_property(rough,"",unreal.MaterialProperty.MP_ROUGHNESS)
    edit.layout_material_expressions(mat)
    edit.recompile_material(mat)
    assert lib.save_loaded_asset(mat,False)
    return mat
