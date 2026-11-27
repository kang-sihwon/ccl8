"""Inspect every archived plate, its coordinate frame, and recorded linear crossings."""
import argparse
import base64
from collections import Counter, defaultdict
import hashlib
import io
import json
import math
from pathlib import Path
import re
from xml.etree import ElementTree as ET
from zipfile import ZipFile
from PIL import Image

from build_tern_passage import CITIES, NS, box, key, source_geo
from build_veles371 import OUT, ATLAS, W, H, R, BLOCK, PW, PH, inside, load_js

NUM = re.compile(r'-?\d+(?:\.\d+)?')
XLINK = '{http://www.w3.org/1999/xlink}href'
NORTH = (0,1)
OFFSETS = [(-1,0),(0,-1),(1,0),NORTH]


def cubic(path):
    ns=list(map(float,NUM.findall(path)))
    p=ns[:2];out=[p]
    for k in range(2,len(ns),6):
        a,b,q=ns[k:k+2],ns[k+2:k+4],ns[k+4:k+6]
        for n in range(1,97):
            t=n/96;u=1-t
            out.append([u*u*u*x+3*t*u*u*y+3*t*t*u*z+t*t*t*w for x,y,z,w in zip(p,a,b,q)])
        p=q
    return out


def trace_edges(ij, ps, name, width, allowed, hits):
    bounds=box(ij)
    for side in allowed:
        axis=side%2;other=1-axis;edge=bounds[side]
        a,b=ij
        addr=(a-(side==0),b-(side==1),axis)
        for p,q in zip(ps,ps[1:]):
            delta=q[axis]-p[axis]
            if not delta:continue
            t=(edge-p[axis])/delta
            if not 0<=t<1:continue
            pos=p[other]+t*(q[other]-p[other])
            if not bounds[other]<=pos<bounds[other+2]:continue
            angle=math.degrees(math.atan2(q[1]-p[1],q[0]-p[0]))
            hits[addr][int(side>=2)].append((name,pos,width,angle))


def validate():
    catalog=load_js(OUT/'catalog.js')
    raw=load_js(ATLAS/'TernPassage/atlas.js')
    old=catalog['originals']
    assert old=={k:v for k,v in raw['tiles'].items() if inside(tuple(map(int,k.split('_'))))}
    seen=set();textures=set();zones=Counter();sources=Counter()
    hits=defaultdict(lambda:[[],[]]);painted=0;originals=0
    aliases={w['id']:w['name'] for w in source_geo()['lines']}
    aliases.update({'bank0':'테른 강안','bank1':'테른 강안','서안':'테른 강안','동안':'테른 강안',
                    '서쪽 시장길':'서쪽 시장길 · 교외 집재장'})
    blocks=list((OUT/'blocks').glob('*.js'))
    assert len(blocks)==math.ceil((2*R+1)/BLOCK)**2
    for path in blocks:
        bx,by=map(int,path.stem.split('_'))
        entry=path.read_bytes();prefix=f'plateBlock("{bx}_{by}","'.encode()
        assert entry.startswith(prefix) and entry.rstrip().endswith(b'");')
        packed=base64.b64decode(entry[len(prefix):entry.rindex(b'"')],validate=True)
        with ZipFile(io.BytesIO(packed)) as archive:
            for member in archive.infolist():
                k=member.filename.removesuffix('.svg')
                i,j=ij=tuple(map(int,k.split('_')))
                assert member.filename==key(ij)+'.svg' and ij not in seen and inside(ij)
                assert ((i+R)//BLOCK,(j+R)//BLOCK)==(bx,by)
                seen.add(ij)
                data=archive.read(member)
                src=old.get(k)
                if src:
                    before=(ATLAS/'TernPassage'/src['url']).read_bytes()
                    if k in catalog['overprints']:
                        begin=data.index(b'<g id="regional-transfers">')
                        assert data[:begin]+data[data.rindex(b'</svg>'):]==before
                        painted+=1
                    else:assert before==data
                    originals+=1;sources[src['region']]+=1
                root=ET.fromstring(data)
                sides=[s for s,(dx,dy) in enumerate(OFFSETS) if inside((i+dx,j+dy))]
                mixed=[s for s in sides if key((i+OFFSETS[s][0],j+OFFSETS[s][1])) not in old]
                if not src:
                    assert root.get('data-tile')==k
                    frame=root.find('.//*[@id="frame"]/') 
                    assert [float(frame.get(n)) for n in ('x','y','width','height')]==[22,26,W,H]
                    shift=list(map(float,NUM.findall(root.find('.//*[@stroke-linejoin="round"]').get('transform'))))
                    a,b,c,d=box(ij)
                    assert math.dist(shift,[22-a,26+d,1,-1])<.00001
                    img=root.find('.//'+NS+'image')
                    png=base64.b64decode(img.get(XLINK).split(',')[1],validate=True)
                    with Image.open(io.BytesIO(png)) as im:
                        assert im.size==(PW,PH);im.verify()
                    textures.add(hashlib.blake2s(png,digest_size=12).digest())
                    zones[root.find(NS+'title').text.split(' (')[0]]+=1
                for line in root.iter():
                    name=line.get('data-trace')
                    if name:
                        use=sides;offset=(0,0)
                    elif src and mixed:
                        ident=line.get('data-line') or line.get('data-feature')
                        if not ident:continue
                        if ident not in aliases:
                            if src['region'] not in ('VT','TR'):continue
                            ident={'west':'서안','east':'동안','channel':'항로','track1':'강변길','track2':'강변길'}.get(ident,ident)
                        name=aliases.get(ident,ident);use=mixed
                        offset=CITIES[{'타르넬':'Tarnel','로덴':'Roden'}[src['region']]]['offset'] if src['region'] in ('타르넬','로덴') else (0,0)
                    else:continue
                    if line.get('points'):
                        ps=[tuple(map(float,p.split(','))) for p in line.get('points').split()]
                    elif line.get('data-line'):ps=cubic(line.get('d'))
                    else:continue
                    if offset!=(0,0):ps=[(x+offset[0],y+offset[1]) for x,y in ps]
                    trace_edges(ij,ps,name,float(line.get('stroke-width')),use,hits)
    assert len(seen)==(2*R+1)**2
    assert len(textures)==len(seen)-len(old)
    assert sum(zones.values())==len(textures) and len(zones)==len(catalog['regions'])==13
    adjacent=matched=mixed_count=0;max_gap=max_turn=0;errors=[]
    for i,j in seen:
        for axis,(dx,dy) in enumerate(((1,0),NORTH)):
            nbr=(i+dx,j+dy)
            if nbr not in seen:continue
            assert box((i,j))[axis+2]==box(nbr)[axis]
            adjacent+=1
            a,b=hits.get((i,j,axis),([],[]))
            a,b=sorted(a),sorted(b)
            if len(a)!=len(b):
                errors.append(((i,j,axis),a,b));continue
            is_mixed=(key((i,j)) in old)!=(key(nbr) in old)
            for left,right in zip(a,b):
                gap=abs(left[1]-right[1]);turn=abs((left[3]-right[3]+90)%180-90)
                same_width=left[0]=='테른 강안' or abs(left[2]-right[2])<.001
                if left[0]!=right[0] or not same_width or gap>(.03 if is_mixed else .02) or turn>(2 if is_mixed else .15):
                    errors.append(((i,j,axis),left,right));continue
                matched+=1;mixed_count+=is_mixed
                max_gap=max(gap,max_gap);max_turn=max(turn,max_turn)
    if errors:
        for issue in sorted(errors)[:45]:print(issue,flush=True)
        raise ValueError(f'{len(errors)} mismatches')
    assert adjacent==2*371*370
    return dict(grid=[371,371],tile_range=[-R,R],sheets=len(seen),packed_blocks=len(blocks),
                original_sheets=originals,original_source_files_verified=originals,regional_overprints=painted,
                original_sources=sources,regional_sheets=len(textures),regional_tiles_by_zone=zones,
                shared_grid_edges=adjacent,regional_line_crossings=matched,mixed_crossings=mixed_count,
                max_crossing_error_m=round(max_gap,5),max_tangent_error_deg=round(max_turn,5))


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--write-report',action='store_true')
    result=json.dumps(validate(),indent=2,ensure_ascii=False)
    if parser.parse_args().write_report:(OUT/'verification.json').write_text(result+'\n',encoding='utf-8')
    print(result)

