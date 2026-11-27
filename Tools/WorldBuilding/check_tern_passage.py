"""Check tile edges from serialized SVG geometry and compare corridor endpoints."""
import argparse
import json
import math
import re
from collections import Counter
from itertools import groupby
from xml.etree import ElementTree as ET

from build_tern_passage import OUT, box, key, legacy, moved, read, source_hashes, tile


def tile_edges(path, ij):
    root = ET.parse(path)
    metric = root.find('.//*[@id="metric"]')
    bounds = box(ij)
    transform = list(map(float, re.findall(r"-?[\d.]+", metric.get("transform"))))
    assert math.dist(transform, [48-bounds[0]*4.25, 86+bounds[3]*4.25, 4.25, -4.25]) < .002
    sides = [[] for _ in bounds]
    for line in metric.findall(".//*[@data-feature]"):
        feature = line.get("data-feature")
        width = float(line.get("stroke-width"))
        points = [tuple(map(float, p.split(","))) for p in line.get("points").split()]
        for p, q in zip(points, points[1:]):
            dx, dy = q[0]-p[0], q[1]-p[1]
            angle = math.degrees(math.atan2(dy, dx))
            for side, edge in enumerate(bounds):
                axis = side % 2
                delta = q[axis] - p[axis]
                if not delta:
                    continue
                t = (edge - p[axis])/delta
                if 0 <= t < 1:
                    other = 1 - axis
                    cross = p[other]+t*(q[other]-p[other])
                    if bounds[other] <= cross < bounds[other+2]:
                        sides[side].append((feature, cross, angle, width))
    return [sorted(events) for events in sides]


def angle_error(a, b):
    return abs((a-b+90) % 180-90)


def verify():
    text = (OUT/"atlas.js").read_text(encoding="utf-8")
    data = json.loads(text[text.index("{"):text.rindex("}")+1])
    originals = source_hashes()
    assert read(OUT/"baseline.json") == originals
    tiles = data["tiles"]
    old = legacy()
    groups = Counter(t["region"] for t in tiles.values())
    assert len(tiles) == len(old)+363+593 and groups["VT"] == 363 and groups["TR"] == 593
    for ij, (name, url) in old.items():
        assert tiles[key(ij)] == {"region": name, "url": url} and (OUT/url).is_file()
    result = {"tiles": dict(groups), "original_files_sha256_verified": len(originals), "legs": {}}
    for region, leg in data["legs"].items():
        sheets = {k for k, v in tiles.items() if v["region"] == region}
        assert sheets == {p.stem for p in (OUT/region).glob("*.svg")}
        edges = {}
        for k in sheets:
            ij = tuple(map(int, k.split("_")))
            edges[ij] = tile_edges(OUT/tiles[k]["url"], ij)
        seams = crossings = ports = 0
        max_gap = max_angle = port_gap = port_angle = 0
        for (i, j), sides in edges.items():
            for axis, neighbor in enumerate(((i+1, j), (i, j+1))):
                if neighbor not in edges:
                    continue
                seams += 1
                a, b = sides[axis+2], edges[neighbor][axis]
                assert len(a) == len(b), (region, i, j, neighbor, a, b)
                for v, u in zip(a, b):
                    gap = abs(v[1]-u[1])
                    turn = angle_error(v[2], u[2])
                    assert v[0] == u[0] and v[3] == u[3] and gap < .02 and turn < .15, (v, u)
                    max_gap = max(gap, max_gap)
                    max_angle = max(turn, max_angle)
                    crossings += 1
        for feature, connections in leg["ports"].items():
            for port in connections:
                p, direction = port["world_m"], port["outward_unit"]
                ij = tile(moved(p, direction, .05))
                previous = tile(moved(p, direction, -.05))
                assert previous in old and ij in edges
                side = min(range(4), key=lambda s: abs(box(ij)[s]-p[s % 2]))
                other = 1-side % 2
                hit = min((v for v in edges[ij][side] if v[0] == feature), key=lambda v: abs(v[1]-p[other]))
                gap = abs(hit[1]-p[other])
                turn = angle_error(hit[2], math.degrees(math.atan2(direction[1], direction[0])))
                assert gap < .02 and turn < .15, (region, feature, ij, gap, turn)
                if feature in ("track1", "track2", "market"):
                    assert abs(hit[3]-port["width_m"]) < .001
                port_gap, port_angle = max(port_gap, gap), max(port_angle, turn)
                ports += 1
        result["legs"][region] = dict(shared_edges=seams, feature_crossings=crossings,
            max_position_error_m=round(max_gap, 5), max_tangent_error_deg=round(max_angle, 5),
            legacy_ports=ports, max_port_error_m=round(port_gap, 5), max_port_tangent_deg=round(port_angle, 5),
            tributary_bridges=len(leg["bridges"]))
    result["journeys"] = {}
    for mode, steps in data["routes"].items():
        regions = [name for name, _ in groupby(tiles[k]["region"] for k in steps)]
        assert regions == ["벨레스", "VT", "타르넬", "TR", "로덴"], regions
        indices = [tuple(map(int, k.split("_"))) for k in steps]
        assert all(abs(x-u)+abs(y-v) == 1 for (x, y), (u, v) in zip(indices, indices[1:]))
        paths = data["paths"][mode]
        gap = max(math.dist(a[-1], b[0]) for a, b in zip(paths, paths[1:]))
        assert gap < .02, (mode, gap)
        result["journeys"][mode] = dict(sheets_in_order=len(steps), regions=regions, max_join_gap_m=round(gap, 5))
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--write-report", action="store_true")
    args = parser.parse_args()
    report = json.dumps(verify(), ensure_ascii=False, indent=2)
    if args.write_report:
        (OUT/"verification.json").write_text(report+"\n", encoding="utf-8")
    print(report)

