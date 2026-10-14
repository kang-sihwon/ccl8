"""Connect owned foliage and CC0 skeletal prototypes after building the Editor target."""
import unreal
import runpy
from pathlib import Path
A = unreal.EditorAssetLibrary
L = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
E = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def asset(p):
    a = A.load_asset(p)
    assert a is not None, p
    return a

base = '/Game/TestResources/Animals/'
visuals = []
for name, move, idle, scale, yaw, z, speed in [
    ('SheepRig', 'A_SheepWalk', 'A_SheepIdle', .14, -90., -22., 25.),
    ('FishRig', 'SK_FishRig_Anim', 'SK_FishRig_Anim', .075, -90., 0., 40.),
    ('Pigeon', 'SK_Pigeon_Anim_pigeon-armature_flap-wings-animation', 'SK_Pigeon_Anim_pigeon-armature_idle-animation', 1., -90., -12., 80.),
    ('AntRig', 'SK_AntRig_Anim_AntRig_walk', 'SK_AntRig_Anim_AntRig_bite', .3, -90., -16., 25.)]:
    p = base + name + '/'
    v = unreal.CCLCreatureVisual()
    v.mesh = asset(p + 'SK_' + name)
    v.move = asset(p + move)
    v.idle = asset(p + idle)
    v.transform = unreal.Transform(location=unreal.Vector(0, 0, z), rotation=unreal.Rotator(yaw=yaw), scale=unreal.Vector(scale, scale, scale))
    v.gait_speed_cm_per_second = speed
    visuals.append(v)

for path in ['/Game/Maps/Tests/Ecology', '/Game/Maps/Tests/Vegetation']:
    assert L.load_level(path)
    for actor in E.get_all_level_actors():
        if isinstance(actor, unreal.CCLWorldEnvironmentConfig): actor.set_editor_property('creature_visuals', visuals)
        if isinstance(actor, unreal.CCLVegetationPresentation):
            meshes = [None] * 9
            meshes[0] = actor.get_editor_property('living_tree_mesh')
            for index, p in [(1, 'Grass/SM_Grass_01'), (2, 'Flower/SM_Flower_01'), (3, 'Bush/SM_Bush_01')]:
                meshes[index] = asset('/Game/TestResources/Fishermans_Cabin/Meshes/Foliage/' + p)
            actor.set_editor_property('species_meshes', meshes)
            actor.refresh_preview()
    assert L.save_current_level()
unreal.log('CCL_LIFE_RESOURCES_CONFIGURED')
if A.does_asset_exist('/Game/TestResources/PN_Fern/Meshes/fern_01_01') and A.does_asset_exist('/Game/TestResources/AnimatedPondFish/Fish/Meshes/SKM_Fish'):
    runpy.run_path(str(Path(unreal.Paths.project_dir()) / 'Tools/Validation/configure_laboratory_fern_pond.py'))
