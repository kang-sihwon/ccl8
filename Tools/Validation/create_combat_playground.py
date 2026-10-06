"""Create the project combat assets and a separate World Partition test arena."""
import unreal

MAP = "/Game/Maps/CombatPlayground"
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    raise RuntimeError("CombatPlayground already exists; preserve edited content.")

for source, destination in (
    ("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01", "/Game/Combat/AS_UnarmedAttack"),
    ("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash", "/Game/Combat/AS_Dodge"),
):
    sequence = unreal.EditorAssetLibrary.load_asset(destination)
    if not sequence:
        sequence = unreal.EditorAssetLibrary.duplicate_asset(source, destination)
    if not sequence:
        raise RuntimeError("Could not create project animation sequence: " + destination)
    sequence.set_editor_property("enable_root_motion", False)
    sequence.set_editor_property("force_root_lock", True)
    unreal.EditorAssetLibrary.save_loaded_asset(sequence)

if not unreal.CCLCombatAssetLibrary.create_combat_assets():
    raise RuntimeError("Combat asset creation or StateTree compilation failed.")
if not unreal.EditorAssetLibrary.does_asset_exist("/Game/Combat/ABP_Combat"):
    if not unreal.EditorAssetLibrary.duplicate_asset(
        "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed", "/Game/Combat/ABP_Combat"):
        raise RuntimeError("Could not create project animation blueprint.")
unreal.EditorAssetLibrary.save_asset("/Game/Combat/ABP_Combat")

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.new_level(MAP, True):
    raise RuntimeError("Could not create World Partition combat map.")

mesh_path = "/Game/Combat/SM_ArenaBlock"
mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
if not mesh:
    mesh = unreal.EditorAssetLibrary.duplicate_asset("/Engine/BasicShapes/Cube", mesh_path)
settings = mesh.get_editor_property("nanite_settings")
settings.set_editor_property("enabled", True)
mesh.set_editor_property("nanite_settings", settings)
unreal.EditorAssetLibrary.save_loaded_asset(mesh)

def block(label, location, scale):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor

block("ArenaFloor", (0, 0, -50), (36, 36, 1))
block("Cover", (500, 650, 100), (1, 6, 2))
for i, y in enumerate((-600, -200, 200, 600)):
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-800, y, 110))
    start.set_actor_label("CombatStart_" + str(i))
enemy = actors.spawn_actor_from_class(unreal.CCLEnemyCharacter, unreal.Vector(100, 0, 110))
enemy.set_actor_label("MeleeEnemy")
target = actors.spawn_actor_from_class(unreal.CCLHealthTarget, unreal.Vector(-500, 950, 60))
target.set_actor_label("HealthOnlyTarget")
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 700))
sun.set_actor_rotation(unreal.Rotator(-45, -30, 0), False)
sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
sky.light_component.set_editor_property("intensity", 1.0)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("kill_z", -700.0)
world.get_world_settings().set_editor_property("default_game_mode", unreal.CCLGameModeBase)
if not unreal.CCLCombatAssetLibrary.configure_combat_world():
    raise RuntimeError("World Partition or navigation setup failed.")
if not levels.save_current_level():
    raise RuntimeError("Combat map save failed.")
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log("CCL_COMBAT_ASSETS PASS WorldPartition=True Streaming=False Nanite=True")
