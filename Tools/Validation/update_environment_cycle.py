"""Upgrade and save the Zone 09 definition through the Unreal asset API."""
import unreal
asset=unreal.load_asset('/Game/Environment/Experiments/DA_Environment_09')
assert asset
asset.configure_zone(9)
assert asset.get_editor_property('kind') == unreal.CCLExperimentKind.CYCLE
assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
unreal.log('CCL_CYCLE_ASSET PASS')
