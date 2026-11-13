"""Read-only raster/coordinate checks. Does not edit or generate image pixels."""
import hashlib
import json
import math
from pathlib import Path
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parent
def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))
plan = read(ROOT / "records/plan.json")
source = read(ROOT.parent / "WorldSectorPlan-v1/sector-plan.json")
checks = []
def check(name, passed, detail=None):
    checks.append({"name": name, "pass": bool(passed), "detail": detail})
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

check("36 settlement IDs preserved", {s["id"] for s in plan["settlements"]} == {s["id"] for s in source["settlements"]})
check("new tile IDs unique", len({j["id"] for j in plan["jobs"]}) == len(plan["jobs"]))
records = []
for s in plan["settlements"]:
    old = next(p for p in source["settlements"] if p["id"] == s["id"])
    delta = math.dist(s["worldPixel"], [v*1024 for v in old["world_pixel"]])
    check(s["id"]+" world anchor unchanged", delta < .001, delta)
    with Image.open(ROOT / s["context"]) as context:
        check(s["id"]+" context readable", context.size == (2048, 1024))
    if s["id"] == "M01":
        check("M01 references existing 12 tiles", len(s["tiles"]) == 12 and all(t["path"].startswith("../IgnisM01Details-v1/") for t in s["tiles"]))
        continue
    jobs = [j for j in plan["jobs"] if j["settlement"] == s["id"]]
    minc, minr = min(j["column"] for j in jobs), min(j["row"] for j in jobs)
    maxc, maxr = max(j["column"] for j in jobs), max(j["row"] for j in jobs)
    available = [j for j in jobs if (ROOT/j["generated"]).exists() and (ROOT/j["path"]).exists()]
    check(s["id"]+" planned generation and assembly complete", len(available) == len(jobs), {"available":len(available),"planned":len(jobs)})
    if len(available) != len(jobs):
        continue
    with Image.open(ROOT/f"previews/{s['id']}-assembled.png") as atlas:
        check(s["id"]+" atlas dimensions", atlas.size == ((maxc-minc+1)*2048,(maxr-minr+1)*2048))
        for j in jobs:
            span = 2048 / 2**(j["level"]-10)
            expected = [j["column"]*span,j["row"]*span,(j["column"]+1)*span,(j["row"]+1)*span]
            check(j["id"]+" integer registered bounds", j["worldPixelBounds"] == expected and all(float(x).is_integer() for x in expected))
            with Image.open(ROOT/j["guide"]) as guide:
                check(j["id"]+" guide size and valid coverage", guide.size == (2304,2304) and guide.getchannel("A").getextrema() == (255,255))
            with Image.open(ROOT/j["generated"]) as native:
                native_size = native.size
                check(j["id"]+" native generation readable", min(native.size) >= 1024)
            with Image.open(ROOT/j["path"]) as tile:
                check(j["id"]+" exported tile size", tile.size == (2048,2048))
                x,y=(j["column"]-minc)*2048,(j["row"]-minr)*2048
                crop=atlas.crop((x,y,x+2048,y+2048)).convert("RGB")
                check(j["id"]+" tile equals atlas pixels", ImageChops.difference(tile.convert("RGB"),crop).getbbox() is None)
            records.append({"id":j["id"],"nativeSize":native_size,"nativeSha256":sha(ROOT/j["generated"]),"guideSha256":sha(ROOT/j["guide"]),"tileSha256":sha(ROOT/j["path"])})
result={"status":"PASS" if all(c["pass"] for c in checks) else "FAIL","scope":"file_integrity_integer_grid_and_assembly_only_not_semantic_continuity","checks":checks,"files":records}
(ROOT/"records/assembly-verification.json").write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
print(json.dumps({"status":result["status"],"checks":len(checks),"failed":[c for c in checks if not c["pass"]]},ensure_ascii=False))
