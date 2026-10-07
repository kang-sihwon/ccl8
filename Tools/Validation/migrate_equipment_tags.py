"""Run with the rebuilt, closed editor project. Back up before migrating tag assets."""
from datetime import datetime
from pathlib import Path
import shutil
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
backup = project / "Saved" / "EquipmentTagBackup" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
packages = [
    "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple",
    "/Game/Combat/DA_Unarmed", "/Game/Combat/DA_EnemyUnarmed",
    "/Game/Progression/DA_IronGauntlets", "/Game/Progression/DA_RecoveryPotion",
] + ["/Game/Progression/DA_Training" + name for name in
     ("Sword", "Shield", "Staff", "Armor", "Boots", "Cloak", "Necklace", "Ring")]
optional = "/Game/Progression/DA_HumanoidAttachments"
if unreal.EditorAssetLibrary.does_asset_exist(optional):
    packages.append(optional)

# Fail without touching assets if the old DLL is still installed.
getattr(unreal.CCLProgressionAssetLibrary, "migrate_equipment_tag_assets")
for package in packages:
    relative = Path("Content") / package.removeprefix("/Game/")
    source = (project / relative).with_suffix(".uasset")
    if not source.is_file():
        raise RuntimeError(f"Missing asset: {package}")
    destination = (backup / relative).with_suffix(".uasset")
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    for extension in (".uexp", ".ubulk"):
        companion = source.with_suffix(extension)
        if companion.exists():
            shutil.copy2(companion, destination.with_suffix(extension))

if not unreal.CCLProgressionAssetLibrary.migrate_equipment_tag_assets():
    raise RuntimeError(f"Equipment tag migration failed; backup: {backup}")
unreal.log(f"CCL_TAG_MIGRATION PASS definitions=12 backup={backup}")
