"""Resave legacy item definitions with the rebuilt editor; preserve a local backup."""
from datetime import datetime
from pathlib import Path
import shutil

import unreal


project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
backup = project / "Saved" / "ItemMigrationBackup" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
packages = (
    "/Game/Combat/DA_Unarmed",
    "/Game/Combat/DA_EnemyUnarmed",
    "/Game/Progression/DA_IronGauntlets",
    "/Game/Progression/DA_RecoveryPotion",
)

# Fail before saving when this script is launched with the old module.
definition = unreal.CCLItemDefinition()
definition.get_editor_property("item_fragments")
definition.get_editor_property("max_stack_count")

for package in packages:
    relative = Path("Content") / package.removeprefix("/Game/")
    source = (project / relative).with_suffix(".uasset")
    if not source.is_file():
        raise RuntimeError(f"Missing source item: {package}")
    destination = (backup / relative).with_suffix(".uasset")
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    for extension in (".uexp", ".ubulk"):
        companion = source.with_suffix(extension)
        if companion.exists():
            shutil.copy2(companion, destination.with_suffix(extension))

for package in packages:
    asset = unreal.load_asset(package)
    if not isinstance(asset, unreal.CCLItemDefinition):
        raise RuntimeError(f"Not an item definition: {package}")
    if not asset.get_editor_property("item_fragments"):
        raise RuntimeError(f"Legacy conversion produced no fragments: {package}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Cannot save converted item: {package}; backup: {backup}")

if not unreal.CCLProgressionAssetLibrary.create_equipment_assets():
    raise RuntimeError(f"Equipment asset creation failed; backup: {backup}")

unreal.log(f"CCL_ITEM_MIGRATION PASS definitions={len(packages)} backup={backup}")
