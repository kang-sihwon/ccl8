"""Shared survey coordinates and terrain for the connected departure chapter (metres)."""
import math
from pathlib import Path
import runpy

import unreal

HERE = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())) / "Tools/WorldBuilding"
BASE = runpy.run_path(str(HERE / "build_veles_departure.py"))
ART = BASE["art"]
ART["plan"]()
FOREST = runpy.run_path(str(HERE / "veles_departure_forest.py"), init_globals=BASE)
CAVE = runpy.run_path(str(HERE / "veles_departure_cave.py"), init_globals=BASE)
ROOT = "/Game/Level/VelesDeparture/Connected"
MAP = ROOT + "/WP_VelesDeparture"
MOUTH = (272, 1250)
MOUTH_Z = FOREST["floor"](*MOUTH)
ENTRY_Y = CAVE["center"](-12)
ENTRY_Z = CAVE["bedrock"](-12, ENTRY_Y)
SPRING = ART["spring_sections"]()


def cave_point(x, y, z):
    return (MOUTH[0] + ENTRY_Y - y, MOUTH[1] + x + 12, MOUTH_Z + z - ENTRY_Z)


def clamp(t):
    return max(0., min(1., t))


def smooth(t):
    t = clamp(t)
    return t*t*(3-2*t)


def stream(y, deep=False):
    source = FOREST["main_river" if deep else "channel"]
    if y >= 180:
        return source(y)
    end = 315 if deep else -165
    t = smooth((y-74)/106)
    return end + (source(y)-end)*t


def water_z(y, deep=False):
    source = 5+.015*(y-200) if deep else FOREST["surface"](y)
    if y >= 180:
        return source
    # 기존 나루의 본류 수면으로 두 지류를 내린다.
    top = 4.7 if deep else FOREST["surface"](180)
    return .055+(top-.055)*clamp((y-8)/172)


def channel_width(y, deep=False):
    return 16. if deep else FOREST["width"](max(y,180))


def surface_height(x,y):
    if y >= 232:
        z = FOREST["floor"](x,y)
    else:
        z = ART["bed"](x,y)
        if y > 172:
            t=smooth((y-172)/60)
            z += (FOREST["floor"](x,y)-z)*t
        if y in SPRING:
            cx,h,w=SPRING[y]
            d=abs(x-cx)
            if d<w+.65:
                low=h-.18*(1-min(1,d/w)**2)
                z=min(z,low+(z-low)*clamp((d-w)/.65))
        if y >= 0:
            for deep in (False,True):
                d=abs(x-stream(y,deep))
                w=channel_width(y,deep)
                if d>w+8:
                    continue
                h=water_z(y,deep)
                bed=h-(2.2 if deep else .64)*(1-min(1,d/w)**2)+max(0,d-w)*.27
                blend=(1-smooth((d-w)/8))*(1-smooth((y-196)/36))
                z += (bed-z)*blend*smooth(y/9)
    if y>=1250 and abs(x-272)<58:
        # 동굴 위의 원지형을 올려 공동과 천장을 덮고 입구 앞은 지표까지 열어 둔다.
        local_x=y-1262
        r=CAVE["radius"](local_x)
        lateral=abs(x-(272+ENTRY_Y-CAVE["center"](local_x)))
        cover=MOUTH_Z-ENTRY_Z+CAVE["base"](local_x)+r+3.8
        weight=(1-smooth((lateral-r-2)/28))*smooth((y-1250)/8)*(1-smooth((y-1346)/37))
        z+=max(0,cover-z)*weight
    return z


def build_ground():
    ART["mesh"].__globals__["folder"]=ROOT+"/Geometry"
    meadow=unreal.load_asset("/Game/Level/VelesRiversideAssets/M_MeadowAndLanes")
    woodland=unreal.load_asset("/Game/Level/VelesNorthWoodAssets/M_NorthWoodFloor")
    margin=ART["kit"]["margin"]
    for y0 in range(-128,1408,128):
        ys=set(range(y0,y0+129,4))
        if y0<256:
            ys.update(range(y0,y0+129,2))
            ys.update(y for y in SPRING if y0<=y<=y0+128)
        if y0<=1258 and y0+128>=1248:
            ys.update(range(max(y0,1248),min(y0+128,1262)+1))
        for x0 in range(-384,1152,128):
            tile=ART["group"]()
            previous=[]
            for y in sorted(ys):
                xs=set(range(x0,x0+129,4 if x0<512 else 8))
                if y<=232 and -256<=x0<=128:
                    xs.update(range(x0,x0+129,2))
                if y in SPRING and x0 in (-128,0):
                    xs.update(i/4 for i in range(-48,13) if x0<i/4<x0+128)
                for deep in (False,True):
                    cx=stream(y,deep); w=channel_width(y,deep)
                    xs.update(round(cx+d,3) for d in (-w-2,-w,-.5*w,0,.5*w,w,w+2) if x0<round(cx+d,3)<x0+128)
                if 1248<=y<=1262 and x0==256:
                    xs.update(range(264,282))
                row=[]
                for x in sorted(xs):
                    k=len(tile[0]); row.append((x,k))
                    tile[0].append((x-x0-64,y-y0-64,surface_height(x,y)))
                    tile[2].append((x/3,y/3))
                    d=margin(x,y)
                    if y<188:
                        d=min(d,math.hypot((x+3)*.89,y-94)-10)
                    road=clamp(.73-(d+.43*math.sin(x*.56+y*.34)+.32*math.sin(y*.73-x*.42))*.44)
                    shore=ART["shore"](x)
                    sand=clamp(.9-.2*max(y-shore,shore-29-3*math.sin(x*.032)-y))
                    tile[3].append((road,sand if y<128 else .1*(1-road),0,1))
                if previous:
                    i=j=0
                    while i+1<len(previous) or j+1<len(row):
                        a,b=previous[i],row[j]
                        if j+1==len(row) or (i+1<len(previous) and previous[i+1][0]<=row[j+1][0]):
                            i+=1; c=previous[i]
                        else:
                            j+=1; c=row[j]
                        if not (1250<y<=1258 and 267<(a[0]+b[0]+c[0])/3<277):
                            tile[1].append((a[1],b[1],c[1]))
                previous=row
            mesh=ART["mesh"](f"DepartureGround_{x0+384}_{y0+128}",[tile],[meadow if y0<128 else woodland])
            actor=ART["actor_mesh"](f"Departure ground {x0} {y0}",mesh,x0+64,y0+64,0,0)
            actor.set_folder_path("Departure/Ground")
        unreal.log(f"DEPARTURE_GROUND {y0}")


def build_surroundings():
    """Carry each outer tile edge up to the ridges visible from the survey camera."""
    mat="/Game/Level/VelesNorthWoodAssets/M_NorthWoodFloor"
    ART["mesh"].__globals__["folder"]=ROOT+"/Geometry"
    edge=([(x,-128) for x in range(-384,1152,2)]+[(1152,y) for y in range(-128,1408,2)]+
          [(x,1408) for x in range(1152,-384,-2)]+[(-384,y) for y in range(1408,-128,-2)])
    rim=ART["group"]()
    for row,d in enumerate((0,8,32,64,110,165,235,325,430,550,720,940,1250,1650,2300)):
        for i,(ex,ey) in enumerate(edge):
            x=ex+(ex-384)*d/768
            y=ey+(ey-640)*d/768
            peaks=130+53*math.sin(x/183+y/237)+37*math.sin(y/101-x/167)+22*math.cos(x/79+y/93)
            height=surface_height(ex,ey)-.14+smooth(d/550)*peaks+min(240,d*.12)
            rim[0].append((x,y,height));rim[2].append((x/3,y/3));rim[3].append((0,.08,0,1))
            if row:
                k=(row-1)*len(edge)+i; n=(row-1)*len(edge)+(i+1)%len(edge)
                rim[1].extend(((k,n,k+len(edge)),(n,n+len(edge),k+len(edge))))
    a=BASE["shape"]("Departure surrounding ridges","SurroundingRidges",[rim],[mat],True)
    a.set_editor_property("is_spatially_loaded",False)
    a.set_folder_path("Departure/Ground")
    # 플레이 중 멀리 있는 고해상도 셀이 언로드되어도 수평선 아래로 하늘이 비치지 않는다.
    a=ART["actor_mesh"]("Departure far terrain",far_terrain(),0,0,0,0)
    a.set_actor_enable_collision(False)
    a.set_editor_property("is_spatially_loaded",False)
    a.static_mesh_component.set_cast_shadow(False)
    a.set_folder_path("Departure/Ground")


def far_terrain():
    ART["mesh"].__globals__["folder"]=ROOT+"/Geometry"
    far=ART["group"]()
    xs=range(-384,1153,8)
    for j,y in enumerate(range(-128,1409,8)):
        for i,x in enumerate(xs):
            clearance=.35
            if 216<=x<=328 and 1210<=y<=1380:
                clearance=45
            elif y<205 or any(abs(x-stream(y,deep))<channel_width(y,deep)+14 for deep in (False,True)):
                clearance=4
            far[0].append((x,y,surface_height(x,y)-clearance))
            far[2].append((x/3,y/3));far[3].append((0,.08,0,1))
            if i and j:
                k=len(far[0])-1
                far[1].extend(((k-len(xs)-1,k-1,k-len(xs)),(k-len(xs),k-1,k)))
    return ART["mesh"]("ValleyDistantFloor",[far],[unreal.load_asset("/Game/Level/VelesNorthWoodAssets/M_NorthWoodFloor")],False)
