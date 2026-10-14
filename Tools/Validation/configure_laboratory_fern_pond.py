"""Connect the shared Fab copies while preserving other laboratory profiles."""
import unreal
A = unreal.EditorAssetLibrary
L = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
E = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
fish = A.load_asset('/Game/TestResources/AnimatedPondFish/Fish/Meshes/SKM_Fish')
swim = A.load_asset('/Game/TestResources/AnimatedPondFish/Fish/Meshes/SKM_Fish_Animation')
fern = A.load_asset('/Game/TestResources/PN_Fern/Meshes/fern_01_01')
assert fish and swim and fern
assert fish.skeleton == swim.get_editor_property('skeleton')
for path in ['/Game/Maps/Tests/Ecology', '/Game/Maps/Tests/Vegetation']:
    assert L.load_level(path)
    for actor in E.get_all_level_actors():
        if isinstance(actor, unreal.CCLWorldEnvironmentConfig):
            profiles = list(actor.get_editor_property('creature_visuals'))
            assert len(profiles) == 4
            v = profiles[1]
            v.mesh, v.move, v.idle = fish, swim, swim
            v.transform = unreal.Transform(rotation=unreal.Rotator(yaw=-90.), scale=unreal.Vector(1.25, 1.25, 1.25))
            v.gait_speed_cm_per_second = 50.
            profiles[1] = v
            actor.set_editor_property('creature_visuals', profiles)
        if isinstance(actor, unreal.CCLVegetationPresentation):
            meshes = list(actor.get_editor_property('species_meshes'))
            assert len(meshes) == 9
            meshes[5] = fern
            actor.set_editor_property('species_meshes', meshes)
            actor.refresh_preview()
    assert L.save_current_level()
unreal.log('CCL_FERN_POND_CONFIGURED')
