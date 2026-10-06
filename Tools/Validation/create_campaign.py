"""Create the village-road-boss prototype without replacing existing maps."""
import unreal

MAP = '/Game/Maps/Campaign'
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    raise RuntimeError('Campaign already exists; preserve edited content.')
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mesh = unreal.load_asset('/Game/Combat/SM_ArenaBlock')
if not mesh or not levels.new_level(MAP, True):
    raise RuntimeError('Combat assets or new campaign level unavailable.')

def block(label, location, scale):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    if not actor:
        raise RuntimeError('Cannot spawn ' + label)
    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor

def sign(text, location):
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*location))
    actor.text_render.set_text(text)
    actor.text_render.set_editor_property('world_size', 48.0)
    actor.set_actor_rotation(unreal.Rotator(pitch=0, yaw=180, roll=0), False)

block('ExpeditionGround', (0, 0, -50), (38, 30, 1))
block('VillageHall', (-1500, -950, 200), (6, 5, 4))
block('VillageWorkshop', (-1200, 1000, 150), (5, 5, 3))
block('RoadNorthWall', (100, 800, 90), (17, 1, 1.8))
block('RoadSouthWall', (100, -800, 90), (17, 1, 1.8))
block('GateNorthPillar', (900, 600, 180), (1.5, 1.5, 3.6))
block('GateSouthPillar', (900, -600, 180), (1.5, 1.5, 3.6))
block('GateLintel', (900, 0, 400), (1.5, 13.5, 1))
block('CourtyardEnd', (1850, 0, 180), (1, 30, 3.6))
for y in (-400, -150, 150, 400):
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-1450, y, 110))
sign('VILLAGE  >  EAST GATE', (-750, 550, 230))
sign('GATE WARDEN', (1000, 0, 450))
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 700))
sun.set_actor_rotation(unreal.Rotator(pitch=-45, yaw=-30, roll=0), False)
actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
actors.spawn_actor_from_class(unreal.CCLCampaignDirector, unreal.Vector(0, 0, 0))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('kill_z', -700.0)
world.get_world_settings().set_editor_property('default_game_mode', unreal.CCLCampaignGameMode)
if not unreal.CCLCombatAssetLibrary.configure_combat_world():
    raise RuntimeError('World Partition or navigation configuration failed.')
if not levels.save_current_level() or not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
    raise RuntimeError('Campaign save failed.')
unreal.log('CCL_CAMPAIGN_ASSETS PASS')
