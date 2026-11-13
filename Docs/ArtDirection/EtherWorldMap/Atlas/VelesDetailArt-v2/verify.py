"""Check source locks and exact assembly of the common art composite, not inferred image geometry."""
import json,hashlib
from pathlib import Path
from PIL import Image,ImageChops,ImageDraw
root=Path(__file__).resolve().parent
m=json.loads((root/'manifest.json').read_text(encoding='utf-8'))
checks=[]
def check(name,passed,detail=None):checks.append(dict(name=name,passed=bool(passed),detail=detail))
for f,digest in m['geometry_sha256'].items():
    check('unchanged source '+f,hashlib.sha256((root/'../VelesDetailTerrain-v1'/f).read_bytes()).hexdigest()==digest)
base=Image.open(root/m['images']['base']['file']).convert('RGB')
art=Image.open(root/'previews/composite.png').convert('RGB')
check('original region footprint and export dimensions',art.size==base.size==(2048,3072) and m['bounds']==[-2048,-2560,2048,3584])
edge_boxes=[(0,0,2048,3),(0,3069,2048,3072),(0,0,3,3072),(2045,0,2048,3072)]
check('all four outer border strips match original preview exactly',all(ImageChops.difference(art.crop(b),base.crop(b)).getbbox() is None for b in edge_boxes),{'width_pixels':3,'meters_per_pixel':2})
tiles=[];restored=Image.new('RGB',art.size)
for row in range(6):
    for col in range(4):
        b=[max(0,col*512-4),max(0,row*512-4),min(2048,(col+1)*512+4),min(3072,(row+1)*512+4)]
        name=f'C{col}-R{row}';tile=art.crop(b);tile.save(root/'tiles'/f'{name}.png')
        core=[col*512-b[0],row*512-b[1],(col+1)*512-b[0],(row+1)*512-b[1]]
        restored.paste(tile.crop(core),(col*512,row*512))
        tiles.append(dict(id=name,file=f'tiles/{name}.png',bounds_pixels=b,core_pixels=core,world_bounds=[-2048+col*1024,3584-(row+1)*1024,-2048+(col+1)*1024,3584-row*1024]))
check('24 art tile cores reconstruct the common composite exactly',ImageChops.difference(art,restored).getbbox() is None)
overlaps=[]
for i,a in enumerate(tiles):
    for b in tiles[i+1:]:
        ca,ra=int(a['id'][1]),int(a['id'][4]);cb,rb=int(b['id'][1]),int(b['id'][4])
        if abs(ca-cb)+abs(ra-rb)!=1:continue
        aa,bb=a['bounds_pixels'],b['bounds_pixels'];over=[max(aa[0],bb[0]),max(aa[1],bb[1]),min(aa[2],bb[2]),min(aa[3],bb[3])]
        def crop(t):
            q=t['bounds_pixels'];return Image.open(root/t['file']).crop((over[0]-q[0],over[1]-q[1],over[2]-q[0],over[3]-q[1]))
        overlaps.append(ImageChops.difference(crop(a),crop(b)).getbbox() is None)
check('38 shared art margins are pixel-identical',len(overlaps)==38 and all(overlaps))
draw=ImageDraw.Draw(restored)
for x in range(0,2048,512):draw.line((x,0,x,3071),fill='#b8c8c6')
for y in range(0,3072,512):draw.line((0,y,2047,y),fill='#b8c8c6')
for t in tiles:
    c,r=int(t['id'][1]),int(t['id'][4]);draw.text((c*512+10,r*512+10),t['id'],fill='white',stroke_width=2,stroke_fill='#1e2020')
restored.save(root/'previews/assembled-grid.png')
(root/'tiles.json').write_text(json.dumps({'status':'art_review_tiles_not_runtime_height', 'meters_per_export_pixel':2,'source_native_spacing_m':{'region':4,'forest':1200/1254,'core_x':1100/1573,'core_y':.7},'tiles':tiles},indent=2)+'\n',encoding='utf-8')
report={'data_and_assembly_status':'PASS' if all(c['passed'] for c in checks) else 'FAIL','checks':checks,'visual_registration_status':'PARTIAL','visual_observations':['Natural integrated roofs, vegetation clusters and yards replace repeated sprites.','Original building/road overlay reveals local roof outline and path-edge drift. Source geometry remains authoritative.','Four image generations are appearance candidates, not photogrammetry. Art tile identity does not prove painted building geometry.','Core/forest transitions are blended in global coordinates; regional terrain remains lower native resolution.']}
(root/'records/verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'status':report['data_and_assembly_status'],'checks':len(checks),'failed':[c for c in checks if not c['passed']],'visual_registration':'PARTIAL'}))
if report['data_and_assembly_status']!='PASS':raise SystemExit(1)
