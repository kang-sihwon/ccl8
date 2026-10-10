"""Add authored biology only to missing known resident features; preserve all other data."""
import unreal

if not unreal.CCLProgressionAssetLibrary.migrate_agent_biology_assets():
    raise RuntimeError("Resident biology asset validation or save failed")
unreal.log("CCL_AGENT_BIOLOGY_AUTHORING PASS")
