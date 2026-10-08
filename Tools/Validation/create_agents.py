"""Author the population scenario and compile the life execution StateTree."""
import unreal

if not unreal.CCLProgressionAssetLibrary.create_agent_assets():
    raise RuntimeError("Agent scenario or StateTree creation failed")
unreal.log("CCL_AGENT_ASSETS PASS")
