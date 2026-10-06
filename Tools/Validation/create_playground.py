"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>."""
import unreal

MAP = "/Game/Maps/MultiplayerPlayground"
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    raise RuntimeError("Playground already exists; preserve the edited map.")

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.new_level(MAP):
    raise RuntimeError("Could not create playground")

cube = unreal.load_asset("/Engine/BasicShapes/Cube")


def block(label, location, scale):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(cube)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


block("Floor", (0, 0, -50), (40, 40, 1))
block("CameraOcclusionWall", (600, 500, 150), (1, 8, 3))
block("JumpStep", (0, 650, 25), (4, 3, 0.5))
for i, y in enumerate((-600, -200, 200, 600)):
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-800, y, 110))
    start.set_actor_label("PlayerStart_" + str(i))

sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500))
sun.set_actor_rotation(unreal.Rotator(-45, -30, 0), False)
sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 400))
sky.light_component.set_editor_property("intensity", 1.0)
settings = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings()
settings.set_editor_property("kill_z", -700.0)
settings.set_editor_property("default_game_mode", unreal.load_class(None, "/Script/CCL.CCLGameModeBase"))
if not levels.save_current_level():
    raise RuntimeError("Could not save playground")
unreal.log("CCL_PLAYGROUND_CREATED " + MAP)
