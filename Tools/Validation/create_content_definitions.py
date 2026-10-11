"""Create enemy definitions and the item catalog, and declare environment map data.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Existing assets are updated in place so repeated runs converge on the same data.
"""
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def asset(folder, name, cls):
    path = f'{folder}/{name}'
    existing = unreal.load_asset(path)
    if existing:
        assert isinstance(existing, cls), path
        return existing
    created = tools.create_asset(name, folder, cls, unreal.DataAssetFactory())
    assert created, path
    return created


def load(path):
    value = unreal.load_asset(path)
    assert value, 'Missing asset: ' + path
    return value


def pattern(attack, label, fraction):
    value = unreal.CCLEnemyAttackPattern()
    value.set_editor_property('attack', load(attack) if attack else None)
    value.set_editor_property('label', label)
    value.set_editor_property('max_health_fraction', fraction)
    return value


enemies = {
    'DA_GateGuard': dict(display_name='성문 경비병', initial_health=80.0, walk_speed=260.0,
                         patterns=[pattern(None, '신중한 공격: 방어 / 패링', 1.0)]),
    'DA_RoadRaider': dict(display_name='길목 약탈자', initial_health=80.0, walk_speed=330.0,
                          patterns=[pattern('/Game/Progression/DA_RaiderStrike', '빠른 공격: 방어 / 패링', 1.0)]),
    # The sweep unlocks at half health and then alternates with the heavy attack.
    'DA_GateWarden': dict(display_name='성문 수호자', initial_health=240.0, walk_speed=210.0,
                          patterns=[pattern('/Game/Progression/DA_WardenSweep', '격노의 휩쓸기: 회피 (방어 / 패링 불가)', 0.5),
                                    pattern('/Game/Progression/DA_WardenHeavy', '강한 공격 준비: 패링 / 회피', 1.0)]),
}
for name, values in enemies.items():
    definition = asset('/Game/Progression', name, unreal.CCLEnemyDefinition)
    definition.modify()
    for key, value in values.items():
        definition.set_editor_property(key, value)
    definition.set_editor_property('detection_radius', 650.0)
    definition.set_editor_property('leash_radius', 900.0)
    assert assets.save_loaded_asset(definition, False), name

catalog = asset('/Game/Progression', 'DA_ItemCatalog', unreal.CCLItemCatalog)
catalog.modify()
item_paths = [f'/Game/Progression/{name}' for name in (
    'DA_IronGauntlets', 'DA_RecoveryPotion', 'DA_TrainingSword', 'DA_TrainingShield', 'DA_TrainingStaff',
    'DA_TrainingArmor', 'DA_TrainingBoots', 'DA_TrainingCloak', 'DA_TrainingNecklace', 'DA_TrainingRing',
    'DA_Pistol', 'DA_Rifle', 'DA_Bullets', 'DA_Bread')] + ['/Game/Environment/Experiments/DA_FoodSample']
catalog.set_editor_property('items', [load(path) for path in item_paths])
catalog.set_editor_property('training_skills', [load('/Game/Progression/DA_PowerTraining'),
                                                load('/Game/Progression/DA_VitalityTraining')])
assert assets.save_loaded_asset(catalog, False)

seed = load('/Game/Environment/Experiments/DA_Environment_00').get_editor_property('seed')
domains = {'EnvironmentPlayground': unreal.CCLWorldContentDomain.PLAYGROUND,
           'EnvironmentScenario': unreal.CCLWorldContentDomain.SCENARIO}
post_process = unreal.load_class(None, '/Game/Environment/Animation/ABP_SnowPostProcess.ABP_SnowPostProcess_C')
assert post_process
for name, domain in domains.items():
    assert levels.load_level('/Game/Maps/' + name)
    unreal.WorldPartitionBlueprintLibrary.load_actors([x.guid for x in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
    configs = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.CCLWorldEnvironmentConfig)]
    assert len(configs) == 1, name
    config = configs[0]
    config.modify()
    config.set_editor_property('domain', domain)
    config.set_editor_property('population_seed', seed)
    config.set_editor_property('character_post_process_anim', post_process)
    config.set_editor_property('experiment_resident_class', unreal.CCLLifeVillager.static_class())
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f'CCL_CONTENT_DEFINITIONS map {name} domain={domain} seed={seed}')

unreal.log('CCL_CONTENT_DEFINITIONS PASS')
