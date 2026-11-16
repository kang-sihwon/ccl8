"""Resolve Veles references without modifying originals; host Python requires Pillow."""
from pathlib import Path
import copy
import hashlib
import json
import math
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
MAP_ROOT = ROOT / 'Docs/ArtDirection/EtherWorldMap'
TERRAIN = MAP_ROOT / 'Atlas/VelesDetailTerrain-v1'
OUT = ROOT / 'Saved/WorldBuilding/VelesBlockout'
BOUNDS = [-512, -128, 640, 1536]
DATUM = 596.0


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def length(points):
    return sum(math.dist(a, b) for a, b in zip(points, points[1:]))


def prepare():
    sources = [TERRAIN / 'manifest.json', TERRAIN / 'data/connections.json',
               TERRAIN / 'data/placements.json', TERRAIN / 'height/master16.png',
               MAP_ROOT / 'Data/veles-forest-detail-v1.json',
               MAP_ROOT / 'Data/veles-forest-ecology-v2.json']
    hashes = {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
    manifest, connections, placements = map(read, sources[:3])
    detail = read(sources[4])
    heights = Image.open(sources[3])
    assert heights.size == (2049, 3073), heights.size
    pixels = heights.load()
    xmin, ymin, xmax, ymax = manifest['bounds_local_m']

    def height(x, y):
        px = min(heights.width - 1.001, max(0, (x - xmin) / 2))
        py = min(heights.height - 1.001, max(0, (ymax - y) / 2))
        ix, iy = int(px), int(py)
        tx, ty = px - ix, py - iy
        v = ((pixels[ix, iy] * (1 - tx) + pixels[ix + 1, iy] * tx) * (1 - ty)
             + (pixels[ix, iy + 1] * (1 - tx) + pixels[ix + 1, iy + 1] * tx) * ty)
        return v * manifest['height']['meters_per_unit'] + manifest['height']['offset_m'] - DATUM

    roads = copy.deepcopy(connections['roads'])
    indexed = {r['id']: r for r in roads}
    changes = []
    for area in detail['areas']:
        road = indexed['R04' if area['id'] == 'workshop_link' else 'R03']
        points = list(reversed(area['detailed_centerline_m']))
        widths = list(reversed(area['segment_clear_width_m']))
        previous = copy.deepcopy(road['points'])
        if road['id'] == 'R04':
            assert previous[0] == points[0] and previous[-1] == points[-1]
            road['points'], road['clear_widths'] = points, widths
        else:
            assert previous[-2] == points[0] and previous[-1] == points[-1]
            road['points'] = previous[:-2] + points
            road['clear_widths'] = [road['width']] * (len(previous) - 2) + widths
        changes.append({'id': road['id'], 'surface_width_m': road['width'],
                        'clear_widths_m': road['clear_widths'], 'old_length_m': length(previous),
                        'new_length_m': length(road['points']),
                        'detail_length_m': length(area['detailed_centerline_m'])})
    assert len(roads) == 103 and len(placements['buildings']) == 86
    assert abs(changes[0]['detail_length_m'] - 334.758) < .002
    assert abs(changes[1]['detail_length_m'] - 326.2134655736722) < .002
    for original, road in zip(connections['roads'], roads):
        assert original['id'] == road['id']
        assert original['points'][0] == road['points'][0]
        assert original['points'][-1] == road['points'][-1]
        if road['id'] not in ('R03', 'R04'):
            assert original == road

    # Reversible blockout mesh. Final Landscape and fine ground dressing are separate.
    x0, y0, x1, y1 = BOUNDS
    nx, ny = int((x1 - x0) / 8) + 1, int((y1 - y0) / 8) + 1
    vertices = [[(x0 + ix * 8) * 100, (y0 + iy * 8) * 100,
                 height(x0 + ix * 8, y0 + iy * 8) * 100]
                for iy in range(ny) for ix in range(nx)]
    triangles = []
    for iy in range(ny - 1):
        for ix in range(nx - 1):
            a = iy * nx + ix
            triangles.extend([[a, a + 1, a + nx], [a + 1, a + nx + 1, a + nx]])
    actors = []

    def box(label, xy, size, z=None, yaw=0, surface='Building', folder='Village', collision=True):
        if z is None:
            z = height(*xy) + size[2] / 2
        actors.append({'label': 'VB_' + label, 'location': [xy[0] * 100, xy[1] * 100, z * 100],
                       'scale': size, 'yaw': yaw, 'pitch': 0, 'surface': surface,
                       'folder': folder, 'collision': collision})

    def grade_last_box():
        spec=actors[-1]
        x,y=[v/100 for v in spec['location'][:2]]
        angle=math.radians(spec['yaw'])
        dx,dy=math.cos(angle),math.sin(angle)
        lx,ly=[v/2 for v in spec['scale'][:2]]
        gx=(height(x+dx*lx,y+dy*lx)-height(x-dx*lx,y-dy*lx))/(2*lx)
        gy=(height(x-dy*ly,y+dx*ly)-height(x+dy*ly,y-dx*ly))/(2*ly)
        spec['pitch']=math.degrees(math.atan(gx))
        spec['roll']=-math.degrees(math.atan(gy))

    for b in placements['buildings']:
        box('Building_' + b['id'], b['center'], [b['width'], b['depth'], b['height_m']],
            yaw=math.degrees(b['angle']))
        box('Door_' + b['id'], b['door'], [1.2, .3, 2.2],
            yaw=math.degrees(b['angle']), surface='Marker', folder='Village/Doors', collision=False)
    samples = []
    for road in roads:
        for si, (a, b) in enumerate(zip(road['points'], road['points'][1:])):
            count = max(1, math.ceil(math.dist(a, b) / 8))
            clear = road.get('clear_widths', [road['width']] * (len(road['points']) - 1))[si]
            for j in range(count):
                p = [a[k] + (b[k] - a[k]) * j / count for k in range(2)]
                q = [a[k] + (b[k] - a[k]) * (j + 1) / count for k in range(2)]
                mid = [(p[k] + q[k]) / 2 for k in range(2)]
                if not (x0 + 4 <= mid[0] <= x1 - 4 and y0 + 4 <= mid[1] <= y1 - 4):
                    continue
                hp, hq = height(*p) + .12, height(*q) + .12
                flat = math.dist(p, q)
                slope = math.degrees(math.atan2(hq - hp, flat))
                box(f'Road_{road["id"]}_{si}_{j}', mid,
                    [math.hypot(flat, hq - hp) + .08, clear, .2],
                    z=(hp + hq) / 2 - .1, yaw=math.degrees(math.atan2(q[1] - p[1], q[0] - p[0])),
                    surface='Road', folder='Routes/' + road['id'])
                actors[-1]['pitch'] = slope
                samples.append({'road': road['id'], 'point_m': mid, 'z_m': (hp + hq) / 2,
                                'clear_width_m': clear, 'slope_deg': slope})
    for area in detail['areas']:
        for pad in area['pads']:
            box('Pad_' + pad['id'], pad['center_m'], pad['size_m'] + [.3],
                z=height(*pad['center_m']), surface='Pad', folder='EncounterSpaces')
            grade_last_box()
        for o in area['obstacles']:
            poly = o['polygon_m']
            center = [sum(p[k] for p in poly) / len(poly) for k in range(2)]
            box('Cover_' + o['id'], center,
                [math.dist(poly[0], poly[1]), math.dist(poly[1], poly[2]), o['height_above_local_floor_m']],
                yaw=math.degrees(math.atan2(poly[1][1] - poly[0][1], poly[1][0] - poly[0][0])),
                surface='Cover', folder='EncounterSpaces/Cover')
    for b in placements['bridges']:
        box('Bridge_' + b['id'], b['center'], [b['length'], b['width'], .25],
            z=height(*b['center']) + .1, yaw=math.degrees(b['angle']), surface='Bridge', folder='Crossings')
        grade_last_box()
    # Sparse, deterministic forest masses preserve clear strips and authored encounter pads.
    def distance_to_segment(p, a, b):
        d = [b[k]-a[k] for k in range(2)]
        t = max(0,min(1,sum((p[k]-a[k])*d[k] for k in range(2))/max(.0001,sum(v*v for v in d))))
        return math.dist(p,[a[k]+t*d[k] for k in range(2)])
    for x in range(-440,541,55):
        for y in range(350,1451,55):
            p = [x+math.sin(x+y)*8,y+math.cos(x-y)*8]
            if any(distance_to_segment(p,a,b)<max(12,r.get('clear_widths',[r['width']]*(len(r['points'])-1))[i]/2+5)
                   for r in roads for i,(a,b) in enumerate(zip(r['points'],r['points'][1:]))):
                continue
            if any(math.dist(p,pad['center_m'])<24 for area in detail['areas'] for pad in area['pads']):
                continue
            if any(distance_to_segment(p,a,b)<35 for a,b in zip(connections['river']['points'],connections['river']['points'][1:])):
                continue
            box(f'TreeProxy_{x}_{y}',p,[4,4,9],surface='Terrain',folder='ForestProxies')
    for key in ('river', 'stream'):
        for i, (a, b) in enumerate(zip(connections[key]['points'], connections[key]['points'][1:])):
            n = max(1, math.ceil(math.dist(a, b) / 16))
            for j in range(n):
                p = [a[k] + (b[k] - a[k]) * j / n for k in range(2)]
                q = [a[k] + (b[k] - a[k]) * (j + 1) / n for k in range(2)]
                m = [(p[k] + q[k]) / 2 for k in range(2)]
                if x0 <= m[0] <= x1 and y0 <= m[1] <= y1:
                    box(f'Water_{key}_{i}_{j}', m, [math.dist(p,q)+.05,55 if key=='river' else 2.5,.04],
                        z=height(*m)+.3,yaw=math.degrees(math.atan2(q[1]-p[1],q[0]-p[0])),
                        surface='Water',folder='WaterReference',collision=False)
                    grade_last_box()
    a, b = [-46, 20], [0, 0]
    deck_z = max(height(*a), height(*b)) + .12
    box('QuayPier', [(a[0]+b[0])/2,(a[1]+b[1])/2], [math.dist(a,b),3,.3],
        z=deck_z-.15,yaw=math.degrees(math.atan2(b[1]-a[1],b[0]-a[0])),surface='Bridge',folder='Village')
    route_in = [[90,260],[180,440]] + indexed['R02']['points'][1:] + indexed['R01']['points'][-2:]
    data = {'schema':1,'status':'blockout_candidate','bounds_m':BOUNDS,'height_datum_m':DATUM,
            'source_sha256':hashes,'road_resolution':changes,'roads':roads,'actors':actors,
            'terrain':{'vertices':vertices,'triangles':triangles,'spacing_m':8},
            'pois':[dict(p,z_m=height(*p['point'])) for p in placements['pois']],
            'road_samples':samples,'route_in':route_in,'route_out':detail['return_sample']['centerline_m'],
            'player_start_cm':[6000,11000,(height(60,110)+1.1)*100],
            'limitations':['Eight-metre static-mesh terrain is a blockout, not final Landscape.',
                           'No building interiors, underground cave, combat AI, gameplay water or final foliage.',
                           'Quay pier is a new candidate preserving the quay point.']}
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'layout.json').write_text(json.dumps(data,ensure_ascii=False,separators=(',',':')),encoding='utf-8')
    summary={'sources_preserved':all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==v for p,v in hashes.items()),
             'roads':len(roads),'buildings':len(placements['buildings']),'actors':len(actors),
             'vertices':len(vertices),'triangles':len(triangles),'road_resolution':changes,
             'max_road_slope_degrees':max(abs(s['slope_deg']) for s in samples)}
    (OUT/'preparation.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in summary.items() if k!='road_resolution'},ensure_ascii=False))


if __name__ == '__main__':
    prepare()
