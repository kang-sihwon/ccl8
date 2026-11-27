"""Fit the forest adit to its bank and expose the cave section along its centreline."""
import math
import os
from pathlib import Path
import runpy
import unreal

root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
here=root/"Tools/WorldBuilding"
base=runpy.run_path(str(here/"build_veles_departure.py"))
globals().update({k:v for k,v in base.items() if not k.startswith("__")})
detail=runpy.run_path(str(here/"veles_departure_finishes.py"),init_globals=base)
def forest():
    f=runpy.run_path(str(here/"veles_departure_forest.py"),init_globals=base)
    floor=f["floor"];x,y=272,1250;z=floor(x,y)
    for actor in actors.get_all_level_actors():
        label=actor.get_actor_label()
        if not label.startswith("F06 ") or isinstance(actor,unreal.CameraActor) or "bridge" in label.lower():continue
        p=actor.get_actor_location();dx=12
        if label.startswith(("F06 Timber","F06 Recessed","F06 Spreading")):dz=z-floor(260,1250)
        else:
            center,ext=actor.get_actor_bounds(False);xx,yy=center.x/100,center.y/100
            dz=floor(xx+dx,yy)-floor(xx,yy)
        actor.set_actor_location(p+vec((dx,0,dz)),False,True)
        if "Exposed roots" in label:
            actor.static_mesh_component.set_visibility(False)
            actor.set_actor_hidden_in_game(True);actor.set_actor_enable_collision(False)
    rock,earth=group(),group()
    # 문턱의 폭과 머리 여유를 비우고 양옆의 흙둑과 상부를 연결한다.
    for ring in range(4):
        for j in range(19):
            a=math.pi*j/18
            inner=ring==0
            rx=(1.46,2.7,5.0,8.2)[ring];rz=(1.0,2.0,2.85,2.2)[ring]
            xx=x+rx*math.cos(a); yy=y+(.05,.28,1.25,3.2)[ring]+.10*math.sin(j*3.2+ring)
            zz=(z+1.7 if inner else floor(xx,yy)+(.55,.55,.40,-.34)[ring])+rz*math.sin(a)
            k=len(rock[0]);rock[0].append((xx,yy,zz));rock[2].append((xx/2.0,zz/2.0))
            if ring and j:
                rock[1].extend(((k-20,k-19,k-1),(k-19,k,k-1),(k-20,k-1,k-19),(k-19,k-1,k)))
    for i in range(17):
        xx=x-8+i;u=abs((xx-x)/8)
        for j in range(9):
            yy=y+1.2+j
            zz=floor(xx,yy)+(1-u*u)*max(0,4.8-abs(j-1)*.64)-.4
            k=len(earth[0]);earth[0].append((xx,yy,zz));earth[2].append((xx/2.5,yy/2.5))
            if i and j:earth[1].extend(((k-10,k-9,k-1),(k-9,k,k-1)))
    shape("F06 Earth bank above the adit","AditEarthShoulders",[rock,earth],
          ["/Game/Level/VelesDeparture/StoryRegions/Geometry/M_CaveWetStrata","/Game/Level/VelesNorthWoodAssets/M_NorthWoodFloor"],False)
    detail["pose_camera"]("F06",(272.6,1239.8,floor(272.6,1239.8)+2.12),(272,1251,z+2.09))
def fur_material(name,low,high):
    folder=package+"/Geometry";path=folder+"/"+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):return material(path)
    mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    lib=unreal.MaterialEditingLibrary
    uv=lib.create_material_expression(mat,unreal.MaterialExpressionTextureCoordinate)
    expr=lib.create_material_expression(mat,unreal.MaterialExpressionCustom)
    arg=unreal.CustomInput();arg.set_editor_property("input_name","UV")
    expr.set_editor_property("inputs",[arg]);expr.set_editor_property("output_type",unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    code="""
float2 p = UV*float2(68,142);
p.y += 0.18*sin(1.7*p.x)+0.13*sin(3.7*p.x+2.0*p.y);
float bands = 0.5+0.5*sin(9.0*p.y+0.58*sin(p.x*4.0));
float tufts = 0.5+0.5*sin(p.y*1.8+p.x*.54+sin(p.x*1.35));
float fleck = frac(sin(dot(floor(p*float2(3,13)),float2(12.9898,78.233)))*43758.5453);
float value = saturate(0.35+0.12*bands+0.15*tufts+0.16*fleck);
return lerp(float3(LOW),float3(HIGH),value);
"""
    expr.set_editor_property("code",code.replace("LOW",",".join(map(str,low))).replace("HIGH",",".join(map(str,high))))
    lib.connect_material_expressions(uv,"",expr,"UV")
    lib.connect_material_property(expr,"",unreal.MaterialProperty.MP_BASE_COLOR)
    for propname,val in ((unreal.MaterialProperty.MP_SPECULAR,.16),(unreal.MaterialProperty.MP_ROUGHNESS,.94)):
        v=lib.create_material_expression(mat,unreal.MaterialExpressionConstant);v.r=val;lib.connect_material_property(v,"",propname)
    lib.recompile_material(mat);assert unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
    return mat
def cave():
    c=runpy.run_path(str(here/"veles_departure_cave.py"),init_globals=base)
    cent,ground,rad=c["center"],c["base"],c["radius"]
    bear=detail["find"]("C03 Bartul brown bear").static_mesh_component
    bear.set_material(0,fur_material("M_BartulCoatGrain",(.027,.015,.009),(.113,.062,.032)))
    bear.set_material(1,fur_material("M_BartulSnoutGrain",(.059,.034,.019),(.148,.086,.048)))
    cam=detail["find"]("C00 Frostroot geological section")
    tag(cam,"Review Former section")
    p=(35,89,5);aim=(35,0,3)
    cut=actors.spawn_actor_from_class(unreal.CineCameraActor,vec(p),unreal.MathLibrary.find_look_at_rotation(vec(p),vec(aim)))
    tag(cut,"C00 Frostroot geological section")
    cc=cut.get_cine_camera_component()
    cc.set_editor_property("current_focal_length",20)
    cc.set_editor_property("override_custom_near_clipping_plane",True)
    cc.set_editor_property("custom_near_clipping_plane",8200)
    strata,topsoil=group(),group()
    for i in range(47):
        x=-11+i*2
        def profile(x):
            h=ground(x)+(rad(x)+1.6 if x>=-3 else 0)
            d=max(0,min(1,(x+6)/9))
            return h,h+d*(1.9+1.1*math.sin(x/11))
        for j in range(2):
            a,b=x,x+2;ha,sa=profile(a);hb,sb=profile(b)
            if j==0:
                face=[(a,0,ha),(b,0,hb),(b,0,sb),(a,0,sa)]
                for side in (face,face[::-1]):quad(strata,side)
            else:
                face=[(a,-9,sa),(a,.2,sa),(b,.2,sb),(b,-9,sb)]
                for side in (face,face[::-1]):quad(topsoil,side)
    shape("C00 Root hill geological cap","CaveSectionOverburden",[strata,topsoil],["/Game/Level/VelesDeparture/StoryRegions/Geometry/M_CaveWetStrata","/Game/Level/VelesNorthWoodAssets/M_NorthWoodFloor"],False)
    x=61;h=ground(x)+rad(x)+3.5
    prop("C00 Oak above the spring","Nature/Trees/SM_White_Oak_02",x,-3,h,height=15.3,solid=False)
    for x in range(-8,80,9):
        h=ground(x)+rad(x)+2.5
        glow("C00 Exterior skylight",(x,3.7,h+3),(207,221,240,255),power=230,radius=15)
    detail["pose_camera"]("C03",(65.7,4.3,ground(65.7)+2.47),(61.3,0,ground(61.3)+1.06))
def run(i):
    world=levels.load_map(maps[i]);marker="F06 Earth bank above the adit" if i==1 else "C00 Root hill geological cap"
    assert world.get_path_name().split(".")[0]==maps[i]
    if i==1:
        f=runpy.run_path(str(here/"veles_departure_forest.py"),init_globals=base)
        cart=detail["find"]("F04 Haul cart")
        loc=cart.get_actor_location()
        if loc.y/100<741:
            cart.set_actor_location(loc+vec((-1,7,f["floor"](-244,743)-f["floor"](-243,736))),False,True)
            assert levels.save_map(world,maps[i])
    if any(a.get_actor_label()==marker for a in actors.get_all_level_actors()):
        if i==2:
            cut=detail["find"]("C00 Frostroot geological section")
            cc=cut.get_cine_camera_component()
            cc.set_editor_property("current_focal_length",20)
            cc.set_editor_property("custom_near_clipping_plane",8200)
            bear=detail["find"]("C03 Bartul brown bear").static_mesh_component
            bear.set_material(0,fur_material("M_BartulCoatGrain",(.027,.015,.009),(.113,.062,.032)))
            bear.set_material(1,fur_material("M_BartulSnoutGrain",(.059,.034,.019),(.148,.086,.048)))
            assert levels.save_map(world,maps[i])
        return
    if i==1:forest()
    else:cave()
    camera=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.CameraActor) and a.get_actor_label()[1:3]=="00")
    editor.set_level_viewport_camera_info(camera.get_actor_location(),camera.get_actor_rotation())
    assert levels.save_map(world,maps[i]);unreal.log("DEPARTURE_FRAMED "+maps[i])
if __name__=="__main__":
    run(int(os.environ["CCL_DEPARTURE_REGION"])-1)
