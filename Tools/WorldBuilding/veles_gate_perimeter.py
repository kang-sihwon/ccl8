"""Continuous terrain outside the original GateFork tiles. Loaded by the GateFork builder."""
import math
import random

import unreal

CX,CY = -80,224
HX,HY = 112,96
FLOOR_MAT = "/Game/Level/VelesNorthWoodAssets/M_NorthWoodFloor"


def edge_points():
    return ([(x,128) for x in range(-192,32)]
            + [(32,y) for y in range(128,320)]
            + [(x,320) for x in range(32,-192,-1)]
            + [(-192,y) for y in range(320,128,-1)])


def beyond(x,y):
    return max(abs(x-CX)-HX,abs(y-CY)-HY,0)


def elevation(x,y):
    r = beyond(x,y)
    t = ramp((r-40)/55)
    near = ground(x,y) if r<95 else 0
    ridge = (18+4*math.sin(x/29+y/19)+3*math.cos(y/17-x/23))
    onset = 14+4*math.cos(x/19)+2*math.sin(y/11)
    z = near + ridge*ramp((r-onset)/14)
    hills = 17+8*math.sin(x/41+y/77)+7*math.cos(y/28-x/54)+min(43,r*.105)
    return z*(1-t)+hills*t


def perimeter_mesh():
    g = ART["group"]()
    edge = edge_points()
    n = len(edge)
    distances = [*range(0,41),44,50,58,68,82,100,128,162,206,280,370,460]
    for row,d in enumerate(distances):
        for j,(ex,ey) in enumerate(edge):
            x = ex+(ex-CX)*d/HX
            y = ey+(ey-CY)*d/HY
            z = elevation(x,y)
            g[0].append((x-CX,y-CY,z))
            g[2].append((x/3,y/3))
            dirt = ramp(.62-ART["kit"]["margin"](x,y)+.24*math.sin(x*.8+y*.3)+.13*math.sin(y*1.4))
            bank = max(0,1-abs(abs(x-creek(y))-width(y))/2)
            g[3].append((max(dirt,.8*bank)*(1-ramp(d/12)),.08*(1-dirt),0,1))
            if row:
                a = (row-1)*n+j
                b = (row-1)*n+(j+1)%n
                g[1].extend(((a,b,a+n),(b,b+n,a+n)))
    return ART["mesh"]("GateContinuousHills",[g],[unreal.load_asset(FLOOR_MAT)],True)


def distant_ground():
    # 멀리서 아직 로드되지 않은 타일 너머로 하늘이 비치지 않게 하는 비충돌 지면이다.
    g = ART["group"]()
    xs = list(range(-196,37,4))
    for j,y in enumerate(range(124,325,4)):
        for i,x in enumerate(xs):
            g[0].append((x-CX,y-CY,elevation(x,y)-2.2))
            g[2].append((x/3,y/3))
            g[3].append((0,.06,0,1))
            if i and j:
                k = len(g[0])-1
                g[1].extend(((k-len(xs)-1,k-1,k-len(xs)),(k-len(xs),k-1,k)))
    a = BASE["shape"]("Distant valley ground","GateDistantGround",[g],[FLOOR_MAT],False,(CX,CY,0))
    a.set_editor_property("is_spatially_loaded",False)
    a.static_mesh_component.set_cast_shadow(False)


def dress_boundary():
    rng = random.Random(8192)
    for ex,ey in edge_points()[::6]:
        d = rng.uniform(6,33)
        x = ex+(ex-CX)*d/HX
        y = ey+(ey-CY)*d/HY
        if abs(x-creek(y))<width(y)+2:
            continue
        tree = rng.choice(("SM_White_Oak_Young_01","SM_Fir_Tree_01","SM_White_Oak_02"))
        BASE["prop"]("Hillside trees","Nature/Trees/"+tree,x,y,elevation(x,y)-.35,
                     height=rng.uniform(6,15),yaw=rng.randrange(360))
    seat_stream_rocks()
    # 능선 뒤의 한계선은 바깥 지형 끝보다 안쪽에 둔다.
    cube = unreal.load_asset("/Engine/BasicShapes/Cube")
    for label,x,y,sx,sy in (("west",-239,CY,2,290),("east",79,CY,2,290),
                            ("south",CX,81,322,2),("north",CX,367,322,2)):
        a = ART["actor_mesh"]("Outer limit "+label,cube,x,y,65,0)
        a.set_actor_scale3d(unreal.Vector(sx,sy,150))
        a.set_editor_property("is_spatially_loaded",False)
        a.set_actor_hidden_in_game(True)
        a.static_mesh_component.set_visibility(False)
        a.static_mesh_component.set_collision_profile_name("BlockAll")


def seat_stream_rocks():
    nearby = [a for a in ACTORS.get_all_level_actors() if a.get_actor_label().startswith("Brook rock throat")]
    for side_name,edge,sign in (("North",320,1),("South",128,-1)):
        # 실제 수면과 비탈이 만나는 단면을 찾아 그 자리에 바위턱을 묻는다.
        sections = (edge+sign*i*.25 for i in range(1,137))
        y = next(v for v in sections if elevation(creek(v),v)>FOREST["surface"](v))
        w = width(y)
        old = sorted((a for a in nearby if (a.get_actor_location().y/100-CY)*sign>0),key=lambda a:a.get_name())
        for i,(dx,dy,size,base,angle) in enumerate(((-.95*w,-.55,2.4+.7*w,-.65,17),
                (.96*w,.06,2.7+.7*w,-.7,-21),(0,.9,3.5+2*w,-.20,9))):
            yy = y+dy*sign
            xx = creek(yy)+dx
            z = FOREST["surface"](yy)+base
            label = f"Brook rock throat {side_name} {i}"
            if not old:
                BASE["prop"](label,"Nature/RocksAndCliffs/SM_MossyRock",xx,yy,z,length=size,yaw=angle)
                continue
            a = old[i]
            a.modify()
            b = a.static_mesh_component.static_mesh.get_bounding_box()
            scale = size*100/max(b.max.x-b.min.x,b.max.y-b.min.y)
            a.set_actor_scale3d(unreal.Vector(scale,scale,scale))
            a.set_actor_rotation(unreal.Rotator(yaw=angle),False)
            p,e = a.get_actor_bounds(False)
            a.set_actor_location(a.get_actor_location()+VEC((xx,yy,z))-p+unreal.Vector(0,0,e.z),False,False)
            a.set_actor_label(label)
        rng = random.Random(edge)
        for i in range(22):
            yy = y-sign*rng.uniform(1,13)
            xx = creek(yy)+rng.choice((-1,1))*(width(yy)+rng.uniform(-.42,1.25))
            size = rng.uniform(.36,1.18)
            BASE["prop"]("Stream mouth gravel","Nature/RocksAndCliffs/Rock_small_VarB_mdl",xx,yy,
                         elevation(xx,yy)-size*.14,length=size,yaw=rng.randrange(360),solid=False)


def build_perimeter():
    scene = {a.get_actor_label():a for a in ACTORS.get_all_level_actors()}
    if "Continuous valley hills" in scene:
        if not any(label.startswith("Stream mouth gravel") for label in scene):
            seat_stream_rocks()
        return
    old = scene.get("Valley rim WoodedFoothills")
    if old:
        old.modify()
        old.static_mesh_component.modify()
    a = ART["actor_mesh"]("Continuous valley hills",perimeter_mesh(),CX,CY,0,0,old)
    a.set_actor_enable_collision(True)
    a.static_mesh_component.set_collision_profile_name("BlockAll")
    a.static_mesh_component.set_cast_shadow(True)
    a.set_editor_property("is_spatially_loaded",False)
    a.set_folder_path("GateFork/Perimeter")
    distant_ground()
    dress_boundary()
