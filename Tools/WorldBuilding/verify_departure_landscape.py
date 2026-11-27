"""Verify saved tree contact and ground surface, then photograph the approach during a full PIE walk."""
from pathlib import Path
import runpy
import time

import unreal


here=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))/"Tools/WorldBuilding"
walk=runpy.run_path(str(here/"verify_veles_departure_world.py"))
dress=runpy.run_path(str(here/"finish_departure_landscape.py"))
out=walk["out"]
editor=walk["editor"]
vec=walk["vec"]


class LandscapeAudit(walk["Audit"]):
    def editor_check(self):
        super().editor_check()
        world=editor.get_editor_world()
        all_actors=walk["actors"].get_all_level_actors()
        terrain=dress["Terrain"](all_actors)
        expected=dress["LAND"]["ROOT"]+"/Geometry/M_DepartureValleyGround"
        for a in terrain.tiles.values():
            assert a.static_mesh_component.get_material(0).get_path_name().split(".")[0]==expected
        trees=dress["seat_trees"](world,all_actors,terrain,False)
        assert trees["total"]>10000 and trees["max_cm"]<=0,trees
        self.report["tree_contact"]=trees
        plants=dress["seat_understory"](world,all_actors,terrain,False)
        assert plants["above_2cm"]==0,plants
        self.report["understory_contact"]=plants
        self.report["ground_material"]=expected
        self.shots={}
        self.pause_since=None
        self.poses=self.poses[:1]
        creek=dress["LAND"]["stream"]
        for name,x,y,z,tx,ty,tz in (("village edge",-175,54,148,-43,216,12),
                                    ("stream bank",creek(285)-7,285,5,creek(308),308,0),
                                    ("stream meander",creek(201)+14,201,19,creek(222),222,0),
                                    ("tree bases",-111,286,1.8,-97,306,2)):
            eye=vec((x,y,terrain.z(x*100,y*100)/100+z))
            aim=vec((tx,ty,terrain.z(tx*100,ty*100)/100+tz))
            self.poses.append((name,eye,unreal.MathLibrary.find_look_at_rotation(eye,aim)))
        self.save()

    def drive(self):
        key=(self.stage,self.way)
        names={(0,12):"village-trail",(1,2):"woodland-trail",(7,2):"return-trail"}
        if key in names and key not in self.shots:
            self.pawn.character_movement.stop_movement_immediately()
            if self.pause_since is None:
                self.pause_since=time.monotonic()
                return
            if time.monotonic()-self.pause_since<1.5:
                return
            path=out/("pie-"+names[key]+".png")
            unreal.SystemLibrary.execute_console_command(self.world,'shot filename="'+path.as_posix()+'" -nosuffix')
            self.shots[key]=path
            self.pause_since=None
            return
        super().drive()

    def finish(self,status):
        if status=="PASS":
            assert len(self.shots)==3 and all(p.exists() and p.stat().st_size>5000 for p in self.shots.values())
            self.report["pie_views"]=[p.name for p in self.shots.values()]
        super().finish(status)


audit=LandscapeAudit()
