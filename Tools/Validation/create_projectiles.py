"""Create projectile definitions without replacing imported weapon resources."""
import unreal

if not unreal.CCLProgressionAssetLibrary.create_projectile_assets():
    raise RuntimeError("Projectile asset creation failed")
unreal.log("CCL_PROJECTILE_ASSETS PASS")
