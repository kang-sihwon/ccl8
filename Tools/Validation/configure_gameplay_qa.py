"""Add guard practice pickups and refresh zone 07 without rebuilding edited maps."""
import unreal

assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/CombatPlayground')
unreal.WorldPartitionBlueprintLibrary.load_actors(
    [desc.guid for desc in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
shield = assets.load_asset('/Game/Progression/DA_TrainingShield')
font = assets.load_asset('/Game/UI/Fonts/F_KoreanLabel')
assert shield and font
hint_text = 'E 방패 줍기 / I 장비\n오른손 방패 장착\n우클릭 유지: 가드'
font_chars = str(font.get_editor_property('import_options').get_editor_property('chars'))
missing_chars = {char for char in hint_text if '\uac00' <= char <= '\ud7a3' and char not in font_chars}
assert not missing_chars, 'Missing hint font glyphs: ' + ''.join(sorted(missing_chars))
existing = {actor.get_actor_label(): actor for actor in actors.get_all_level_actors()}
starts = sorted((actor for actor in existing.values() if isinstance(actor, unreal.PlayerStart)),
                key=lambda actor: actor.get_actor_label())
assert starts, 'CombatPlayground needs player starts'
for index, start in enumerate(starts):
    location = start.get_actor_location()
    label = 'GuardPracticeShield_' + str(index)
    pickup = existing.get(label)
    if pickup is None:
        pickup = actors.spawn_actor_from_class(unreal.CCLWorldPickup,
            unreal.Vector(location.x - 140, location.y, 60))
        pickup.set_actor_label(label)
        pickup.set_editor_property('is_spatially_loaded', False)
        pickup.set_editor_property('persistent_id', unreal.GuidLibrary.new_guid())
    assert isinstance(pickup, unreal.CCLWorldPickup)
    pickup.set_editor_property('definition', shield)
    pickup.set_editor_property('quantity', 1)

    label = 'GuardPracticeHint_' + str(index)
    hint = existing.get(label)
    if hint is None:
        hint = actors.spawn_actor_from_class(unreal.TextRenderActor,
            unreal.Vector(location.x + 150, location.y, 140))
        hint.set_actor_label(label)
        hint.set_editor_property('is_spatially_loaded', False)
    hint.modify()
    hint.set_actor_rotation(unreal.Rotator(pitch=0, yaw=180, roll=0), False)
    text = hint.get_component_by_class(unreal.TextRenderComponent)
    text.modify()
    text.set_font(font)
    text.set_text(hint_text)
    text.set_world_size(20)
    text.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
assert levels.save_current_level()
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
experiment = assets.load_asset('/Game/Environment/Experiments/DA_Environment_07')
assert experiment
experiment.configure_zone(7)
assert assets.save_loaded_asset(experiment, False)
unreal.log('CCL_QA_ASSETS PASS shields=%d zone07=updated' % len(starts))
for name in ('EnvironmentPlayground', 'EnvironmentScenario'):
    assert levels.load_level('/Game/Maps/' + name)
    unreal.WorldPartitionBlueprintLibrary.load_actors(
        [desc.guid for desc in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
    directors = [actor for actor in actors.get_all_level_actors()
                 if isinstance(actor, unreal.CCLExperimentDirector)]
    assert len(directors) == 1
    assert experiment in directors[0].get_editor_property('definitions')
    assert '후속 연결' not in str(experiment.get_editor_property('instructions'))
    assert '수동으로 조작할 수 있다' in str(experiment.get_editor_property('instructions'))
    unreal.log('CCL_QA_ZONE07 PASS ' + name)
unreal.SystemLibrary.quit_editor()
