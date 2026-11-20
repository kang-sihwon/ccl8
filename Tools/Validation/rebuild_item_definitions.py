"""Rebuild every item definition from CCLProgressionAssetLibrary (decision 70).

Run with UnrealEditor-Cmd CCL.uproject -run=pythonscript -script=<this file>.
"""
import unreal

if not unreal.CCLProgressionAssetLibrary.rebuild_item_definitions():
    raise RuntimeError('Item definition rebuild failed')

paths = ['/Game/Combat/DA_Unarmed', '/Game/Combat/DA_EnemyUnarmed', '/Game/Environment/Experiments/DA_FoodSample',
         '/Game/Progression/DA_Bread', '/Game/Progression/DA_Bullets', '/Game/Progression/DA_IronGauntlets',
         '/Game/Progression/DA_Pistol', '/Game/Progression/DA_Rifle', '/Game/Progression/DA_RecoveryPotion',
         '/Game/Progression/DA_TrainingSword', '/Game/Progression/DA_TrainingShield', '/Game/Progression/DA_TrainingStaff',
         '/Game/Progression/DA_TrainingArmor', '/Game/Progression/DA_TrainingBoots', '/Game/Progression/DA_TrainingCloak',
         '/Game/Progression/DA_TrainingNecklace', '/Game/Progression/DA_TrainingRing']
for path in paths:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError('Missing item definition: ' + path)
    # The dump records the rebuilt values next to the run log.
    unreal.log('CCL_ITEM_REBUILD ITEM ' + path)
    unreal.SystemLibrary.execute_console_command(None, 'obj dump ' + asset.get_path_name())

unreal.log('CCL_ITEM_REBUILD PASS')
