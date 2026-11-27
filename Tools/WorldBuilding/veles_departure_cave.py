"""Frostroot spring, service workshop and Bartul cavern in one walkable excavation."""
import random

rng=random.Random(6203)
bronze="Materials/MedievalDungeon/M_CandleHolder_Bronze_Inst"
def center(x):return 1.4*math.sin(x/10)+1.3*math.sin(x/21)
def radius(x):return 3.65+4.5*math.exp(-((x-24)/7.0)**4)+10*math.exp(-((x-60)/14)**4)
def base(x):return .8-.063*x+.18*math.sin(x/6)
def bedrock(x,y):
    u=y-center(x)
    pool=1.55*math.exp(-((x-61)/10)**4-((u+7)/3.5)**4)
    return base(x)-pool+.08*math.sin(x*.54+y*1.3)+.035*math.cos(y*.87-x*.45)
def wall(i,x):
    a=math.pi*i/18; r=radius(x)
    return (x,center(x)+r*math.cos(a)*(1+.044*math.sin(2.4*a+x*.45)),
            base(x)-.42+(.98*r+1.15)*math.sin(a)*(1+.068*math.sin(x*.64+4*a)))
def carve():
    floor,roof=group(),group()
    for row in range(95):
        x=-12+row
        for j in range(31):
            y=center(x)+(j/15-1)*(radius(x)*1.065)
            k=len(floor[0]);floor[0].append((x,y,bedrock(x,y)))
            floor[2].append((x/1.9,y/1.9));floor[3].append((.4+.3*math.sin(x*.28+y*.2),0,0,1))
            if j and row:floor[1].extend(((k-32,k-31,k-1),(k-31,k,k-1)))
        if row and x>-4:
            for j in range(18):
                p=[wall(j,x-1),wall(j+1,x-1),wall(j+1,x),wall(j,x)]
                quad(roof,p,[(x/2,j/3),(x/2,(j+1)/3),((x+1)/2,(j+1)/3),((x+1)/2,j/3)])
    mat=art["surface_material"]("M_FrostrootStone",["Medieval_Environment/T_Ground_Rock_Near_01_BC","Medieval_Environment/T_Ground_Dirt_Near_01_BC"],[(.52,.59,.62),(.51,.44,.35)]).get_path_name()
    shape("C00 Sloping stone maintenance ledge","CaveFloor",[floor],[mat])
    shape("C00 Rock vault and walls","CaveVault",[roof],[mat])
    prop("C03 Rock at spring head",cliff,81,center(81),base(81)-1,height=6.6,yaw=30)
    for i in range(115):
        x=rng.uniform(-4,79);sign=rng.choice((-1,1)); y=center(x)+sign*(radius(x)-rng.uniform(.08,.68))
        prop("C00 Fractured limestone",cliff,x,y,bedrock(x,y)-.37,length=rng.uniform(1.0,4.6),yaw=rng.randrange(360))
    for i in range(24):
        x=rng.uniform(48,73);y=center(x)+rng.uniform(-.58,.64)*radius(x)
        h=base(x)+math.sqrt(max(.5,1-((y-center(x))/radius(x))**2))*(radius(x)+.75)
        prop("C03 Hanging root mass",roots,x,y,h-.24,length=rng.uniform(.8,1.6),yaw=rng.randrange(360),solid=False)
def shell(g,rings,axis="x",phase=0,n=18):
    # 닫힌 타원 단면들을 연결해 연속된 몸통과 관절 윤곽을 만든다.
    for idx,(x,y,z,r,s) in enumerate(rings):
        for j in range(n+1):
            a=math.tau*j/n+phase;c,d=math.cos(a),math.sin(a)
            point=(x,y+r*c,z+s*d) if axis=="x" else (x+r*c,y+s*d,z)
            k=len(g[0]);g[0].append(point);g[2].append((j/n*2,idx/5))
            if idx and j:
                pair=((k-n-2,k-1,k-n-1),(k-n-1,k-1,k))
                g[1].extend(pair if axis=="x" else [tri[::-1] for tri in pair])
def lobe(g,x,y,z,rx,ry,rz):
    shell(g,[(x+rx*math.sin(a),y,z,max(.002,ry*math.cos(a)),max(.002,rz*math.cos(a))) for a in [-math.pi/2+i*math.pi/10 for i in range(11)]])
def bear():
    fur,light,dark,claw=group(),group(),group(),group()
    rings=[(-1.52,.99,.10,.19),(-1.35,1.05,.43,.53),(-1.04,1.1,.58,.66),
           (-.60,1.12,.63,.66),(-.12,1.22,.65,.77),(.23,1.36,.63,.83),(.54,1.39,.53,.79),
           (.83,1.41,.41,.52),(1.04,1.56,.36,.40),(1.25,1.60,.32,.32),(1.46,1.53,.23,.22)]
    shell(fur,[(x,0,z,r,h) for x,z,r,h in rings])
    shell(light,[(1.27,0,1.53,.31,.24),(1.47,0,1.53,.24,.20),(1.65,0,1.52,.17,.14),(1.72,0,1.54,.1,.09)])
    lobe(dark,1.71,0,1.57,.092,.15,.09)
    for s in (-1,1):
        lobe(fur,.96,.27*s,1.90,.127,.125,.162)
        lobe(dark,1.027,.30*s,1.94,.062,.075,.086)
        lobe(dark,1.32,.276*s,1.68,.045,.039,.044)
        for fore in (False,True):
            shift=.17*s
            x=(.51 if fore else -1.02)+shift
            y=(.405 if fore else .42)*s
            shell(fur,[(x+.06,y,.12,.20,.17),(x,y,.35,.17,.16),(x-.11,y,.63,.20,.18),
                   (x-.12 if fore else x+.16,y,.88,.25,.22),(x,y,1.14,.28,.23),(x,y,1.3,.22,.19)],"z")
            lobe(light,x+.12,y,.135,.266,.183,.138)
            for j in range(5):lobe(claw,x+.329-.024*abs(2-j),y+(j-2)*.055,.085,.076,.017,.022)
    mats=[art["surface_material"](n,["OldVillage/T_Stucco_001_C"],[t]).get_path_name() for n,t in (
        ("M_BartulUmber",(.18,.091,.045)),("M_BartulGuardHair",(.30,.165,.079)),("M_BartulNose",(.024,.019,.017)),("M_BartulClaws",(.19,.15,.103)))]
    p=(61,center(61)+.15,bedrock(61,center(61)+.15))
    shape("C03 Bartul brown bear","BartulCoat",[fur,light,dark,claw],mats,True,p)
    return p

def workshop():
    x,y=22,center(22)+3.3;z=bedrock(x,y)
    table=prop("C02 Drawing desk",bench,x,y,z,height=.93)
    tabletop("C02 Open service folio",table,x,y,"Props/Household/SM_Book_Open",.11)
    tabletop("C02 Survey hammer",table,x-.62,y,"Props/WorkshopTools/SM_OldHammer",.11)
    tabletop("C02 Bench lamp",table,x+.72,y,lamp,.40)
    pipes,frames=group(),group()
    for i in range(4):
        xx=18+i*3.3; yy=center(xx)+5.65; zz=base(xx)
        beam(frames,(xx,yy,zz-.2),(xx,yy,zz+3),.23,.25)
        beam(frames,(xx,yy,zz+2.7),(xx,yy-1.3,zz+2.7),.26,.23)
        if i<3:
            for h in (.81,1.8):
                beam(pipes,(xx,yy-.38,zz+h),(xx+3.3,center(xx+3.3)+5.27,base(xx+3.3)+h),.24,.25)
    beam(pipes,(24,center(24)+5.26,base(24)+.81),(25.2,center(25.2)+5.25,base(25.2)+1.80),.18,.19)
    shape("C02 Separate supply and bypass coupling","BypassWork",[pipes,frames],[bronze,wood])
    body,joints=group(),group()
    for d in (-1,1):
        lobe(joints,0,d*.24,.92,.18,.16,.16)
        beam(body,(.12,d*.25,.19),(0,d*.24,.91),.29,.24)
        beam(body,(0,d*.24,.94),(0,d*.18,1.29),.27,.29)
        box(body,-.12,.37,d*.25-.135,d*.25+.135,.025,.20)
        beam(body,(0,d*.44,1.85),(.04,d*.59,1.45),.26,.27)
        beam(body,(.04,d*.59,1.45),(.27,d*.61,1.15),.22,.21)
    shell(body,[(0,0,1.17,.21,.28),(0,0,1.58,.31,.44),(0,0,1.91,.24,.44),(0,0,1.98,.16,.30)],"z",n=8)
    lobe(joints,0,0,2.14,.24,.24,.26)
    shape("C02 Artisans maintenance sentinel","CarvedSentinelFacets",[body,joints],[wood,bronze],True,(27,center(27)-2,bedrock(27,center(27)-2)))
def chamber():
    px,py,pz=bear();rig,lash,crystals=group(),group(),group()
    for xx in (63,65,67,69):
        yy=center(xx)+6.6;zz=base(64)+.95
        for dy in (-1.15,1.15):
            beam(rig,(xx,yy+dy,bedrock(xx,yy+dy)-.2),(xx,yy+dy,zz),.23,.23)
        for i in range(6):box(rig,xx-1+i/3,xx-.68+i/3,yy-1.38,yy+1.38,zz-.15,zz)
    # 마른 바닥에서 조작단으로 이어지는 낮은 경사.
    sy=center(63)+6.6;ez=base(64)+.96
    quad(rig,[(58,sy-1,bedrock(58,sy)),(58,sy+1,bedrock(58,sy)),(62.1,sy+1,ez),(62.1,sy-1,ez)])
    for sx,sy,sz in ((px+.78,py+.39,pz+.39),(px+.62,py+.34,pz+1.38)):
        pts=[(sx,sy,sz),(sx+1.3,sy+.94,sz-.16),(64.7,py+3.0,base(65)+.15),(66.3,py+5.5,base(65)+1.31)]
        for a,b in zip(pts,pts[1:]):beam(lash,a,b,.077,.077)
        for i in range(6):
            t=i/6;xx=sx+t*.52;yy=sy+t*.28
            shell(crystals,[(xx,yy,sz-.07,.12,.13),(xx,yy,sz+.12,.105,.12),(xx+.08,yy,sz+.31,.007,.005)],"z",n=5)
    shape("C03 Operators gallery and ramp","SpringPlatform",[rig],[wood])
    shape("C03 External foreleg and chest tethers","ExtractionTethers",[lash,crystals],[bronze,"/Game/Level/VelesRiversideAssets/M_WeatheredTeal"])
    prop("C03 Governor wheel",wheel,66.8,center(67)+6.8,ez+.74,length=1.05)
    # 수원은 바닥을 파낸 공동에만 고인다.
    g=group()
    for i in range(71):
        x=51+i*.29;y=center(x)-7;z=base(x)-.51
        w=3.62*math.sqrt(max(.002,1-((x-61)/10.3)**4))
        for j in range(7):
            k=len(g[0]); g[0].append((x,y+w*(j/3-1),z));g[2].append((x/2,j/2));g[3].append((.85 if 0<j<6 else .18,0,0,1))
            if i and j:g[1].extend(((k-8,k-7,k-1),(k-7,k,k-1)))
    shape("C03 Spring pool","SourcePool",[g],[watermat],False)
def build_region():
    world=levels.new_blank_map(False)
    carve()
    x=1;y=center(x)+1.55;z=base(x)
    frame,ice=group(),group()
    for yy in (y-.85,y+.85):beam(frame,(x,yy,z-.18),(x,yy,z+2.2),.29,.24)
    beam(frame,(x,y-1.04,z+2.15),(x,y+1.04,z+2.15),.25,.29)
    for i in range(15):
        yy=y-.83+i*.12
        shell(ice,[(x,yy,z+1.25+rng.random()*.38,.008,.008),(x,yy,z+2.04,.075,.069),(x,yy,z+2.12,.1,.09)],"z",n=7)
    shape("C01 Frozen intake","FrostedIntakeDetail",[frame,ice],[wood,art["surface_material"]("M_FrostLip",["OldVillage/T_Stucco_001_C"],[(.57,.83,.90)]).get_path_name()])
    prop("C01 Moss at entrance",roots,-2,center(-2)+3.2,base(-2)-.18,length=2.6,yaw=-37)
    workshop();chamber()
    for i,x in enumerate((-6,5,15,24,34,44,53,64,74)):
        y=center(x)+radius(x)*.66;z=bedrock(x,y)
        prop("C00 Service lantern",lamp,x,y,z+1.45,height=.42,solid=False)
        glow("C00 Amber service light",(x,y-.50,z+1.77),power=148,radius=13)
        glow("C00 Reflected water light",(x,center(x)-.8,base(x)+3.1),(151,192,215,255),power=85 if i<5 else 165,radius=16)
    glow("C03 Light through roots",(60,center(60)+1,base(60)+8.4),(255,218,162,255),power=2700,radius=25)
    glow("C01 Mouth daylight",(-6,0,4),(199,223,250,255),power=1500,radius=16)
    setup(world,(-7,center(-7),base(-7)+1.12),5)
    overview=camera("C00 Frostroot geological section",(36,-70,23),(34,1,.2),68)
    for code,p,t in (("C01 Frozen spring",(-4,-.8,2.1),(3,1.5,1.1)),("C02 Workshop",(17,-.7,2.05),(26,1.4,1.18)),("C03 Bartul cavern",(62,-6.1,2.15),(61.5,1,.98))):
        camera(code,(p[0],p[1]+center(p[0]),base(p[0])+p[2]),(t[0],t[1]+center(t[0]),base(t[0])+t[2]))
    editor.set_level_viewport_camera_info(overview.get_actor_location(),overview.get_actor_rotation())
    return world
