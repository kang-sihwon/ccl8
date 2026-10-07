"""Create a minimal widget class used to exercise cold asynchronous UI loading."""
import unreal

package = "/Game/Tests/UI/BP_UIAsyncProbe"
if not unreal.EditorAssetLibrary.does_asset_exist(package):
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.CCLScreen)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "BP_UIAsyncProbe", "/Game/Tests/UI", unreal.WidgetBlueprint, factory)
    if not asset or not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError("Cannot create asynchronous UI probe widget")
unreal.log("CCL_UI_FIXTURE PASS")
