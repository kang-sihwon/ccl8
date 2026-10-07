"""Create missing equipment prototypes without resaving existing item definitions."""
import unreal


if not unreal.CCLProgressionAssetLibrary.create_equipment_assets():
    raise RuntimeError("Equipment prototype creation failed")

for name in (
    "Sword", "Shield", "Staff", "Armor", "Boots", "Cloak", "Necklace", "Ring"
):
    package = f"/Game/Progression/DA_Training{name}"
    asset = unreal.load_asset(package)
    if not isinstance(asset, unreal.CCLItemDefinition):
        raise RuntimeError(f"Missing equipment definition: {package}")
    if not asset.get_editor_property("item_fragments"):
        raise RuntimeError(f"Equipment has no struct fragments: {package}")

unreal.log("CCL_EQUIPMENT_ASSETS PASS eight prototype definitions")
