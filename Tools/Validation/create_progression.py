"""Create progression definitions and migrate placed supplies into the campaign layout."""
import unreal

# Preserve authored definitions on reruns; generate them only for a new project.
if not unreal.EditorAssetLibrary.does_asset_exist('/Game/Progression/DA_IronGauntlets'):
    assert unreal.CCLProgressionAssetLibrary.create_progression_assets()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/Campaign')
all_actors = actors.get_all_level_actors()
directors = [a for a in all_actors if isinstance(a, unreal.CCLCampaignDirector)]
assert len(directors) == 1, 'Campaign requires exactly one director'
director = directors[0]
supplies = list(director.get_editor_property('supplies'))
legacy = {'Supply_Gauntlets': ('DA_IronGauntlets', 4), 'Supply_Potions': ('DA_RecoveryPotion', 5)}
remove = []
for pickup in all_actors:
    if not isinstance(pickup, unreal.CCLWorldPickup) or pickup.get_actor_label() not in legacy:
        continue
    name, number = legacy[pickup.get_actor_label()]
    definition = pickup.get_editor_property('definition')
    assert definition and definition.get_name() == name
    identity = unreal.Guid()
    identity.import_text(f'CC1900000000000000000002{number:08X}')
    matches = [s for s in supplies if s.get_editor_property('placement_id').export_text() == identity.export_text()]
    assert len(matches) <= 1
    spawn = matches[0] if matches else unreal.CCLSupplySpawn()
    spawn.set_editor_property('placement_id', identity)
    spawn.set_editor_property('location', pickup.get_actor_location())
    spawn.set_editor_property('item', definition)
    spawn.set_editor_property('quantity', pickup.get_editor_property('quantity'))
    if not matches:
        supplies.append(spawn)
    remove.append(pickup)
ids = [s.get_editor_property('placement_id').export_text() for s in supplies]
assert len(ids) == len(set(ids)), 'Duplicate supply identities'
director.modify()
director.set_editor_property('supplies', supplies)
for pickup in remove:
    assert actors.destroy_actor(pickup)
assert levels.save_current_level()
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log('CCL_PROGRESSION_ASSETS PASS supply placements=' + str(len(supplies)))
