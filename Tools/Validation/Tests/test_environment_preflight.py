import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location("environment_preflight", Path(__file__).resolve().parents[1] / "environment_preflight.py")
preflight = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(preflight)


class EnvironmentPreflightTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.install = Path(self.temp.name) / "UE_5.8"
        self.engine = self.install / "Engine"
        for name in preflight.REQUIRED_FILES:
            path = self.engine / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("fixture", encoding="utf-8")
        (self.engine / "Build/Build.version").write_text(json.dumps({"MajorVersion": 5, "MinorVersion": 8, "PatchVersion": 3}), encoding="utf-8")
        self.entry = {"AppName": "UE_5.8", "InstallLocation": str(self.install)}
        self.installed = {"InstallationList": [self.entry]}

    def inspect(self, manifests=()):
        return preflight.inspect_engine(self.engine, self.installed, manifests)

    def test_plugin_registration_does_not_mean_engine_complete(self):
        self.installed["InstallationList"] = [{**self.entry, "AppName": "FabPlugin_5.8"}]
        self.assertFalse(self.inspect()["installation_ready"])

    def test_missing_required_file_blocks_readiness(self):
        (self.engine / "Binaries/Win64/UnrealEditor.exe").unlink()
        self.assertFalse(self.inspect()["installation_ready"])

    def test_pending_install_blocks_even_with_editor_present(self):
        pending = self.install / ".egstore/Pending/pending.manifest"
        pending.parent.mkdir(parents=True)
        pending.write_text("pending")
        self.assertFalse(self.inspect()["installation_ready"])

    def test_incomplete_manifest_blocks_registered_engine(self):
        self.assertFalse(self.inspect([{**self.entry, "bIsIncompleteInstall": True}])["installation_ready"])

    def test_other_install_cannot_confirm_this_install(self):
        self.entry["InstallLocation"] = str(self.install.parent / "Another")
        self.assertFalse(self.inspect()["installation_ready"])

    def test_complete_install_does_not_require_source_distribution_bat(self):
        result = self.inspect()
        self.assertTrue(result["installation_ready"])
        self.assertFalse(result["generate_project_files_bat"])

    def source_checkout(self):
        (self.install / ".git").mkdir()
        (self.install / "Setup.bat").write_text("fixture")
        (self.install / "GenerateProjectFiles.bat").write_text("fixture")
        (self.engine / "Source/Runtime/Core").mkdir(parents=True)
        self.installed = {}

    def test_source_checkout_needs_no_launcher_registration(self):
        self.source_checkout()
        result = self.inspect()
        self.assertEqual(result["distribution"], "source")
        self.assertTrue(result["installation_ready"])

    def test_source_checkout_requires_generation_entrypoint(self):
        self.source_checkout()
        (self.install / "GenerateProjectFiles.bat").unlink()
        self.assertFalse(self.inspect()["installation_ready"])

    def test_installed_build_does_not_bypass_launcher_checks(self):
        self.source_checkout()
        (self.engine / "Build/InstalledBuild.txt").write_text("fixture")
        self.assertFalse(self.inspect()["installation_ready"])

    def test_corrupt_version_blocks_readiness(self):
        (self.engine / "Build/Build.version").write_text("{}")
        self.assertFalse(self.inspect()["installation_ready"])

    def test_commented_build_rule_does_not_enable_tests(self):
        self.assertFalse(preflight.tests_policy("//bForceIncludeTestsFolder = true;"))
        self.assertFalse(preflight.tests_policy("/* bForceIncludeTestsFolder = true; */"))
        self.assertTrue(preflight.tests_policy("bForceIncludeTestsFolder = true;"))

    def test_lfs_pointer_is_not_a_loaded_package(self):
        content = self.install / "Content"
        content.mkdir()
        (content / "Pending.umap").write_bytes(b"version https://git-lfs.github.com/spec/v1\noid sha256:test\nsize 100")
        (content / "Real.uasset").write_bytes(bytes.fromhex("c1832a9e") + b"fixture")
        result = preflight.asset_inventory(content)
        self.assertEqual(result["packages"], 2)
        self.assertEqual(result["lfs_pointers"], 1)
        self.assertEqual(result["unknown_headers"], 0)


if __name__ == "__main__":
    unittest.main()
