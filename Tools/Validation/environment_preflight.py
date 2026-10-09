"""Read-only environment inventory; never launches Unreal, pulls Git, or edits assets.

The report goes under Saved/Tests/EnvironmentPreflight unless --output is supplied.
Exit 0 means engine installation evidence is complete; 2 means waiting or unknown.
A successful inventory is not a successful build or a regression test.
"""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


REQUIRED_FILES = (
    "Build/Build.version",
    "Build/BatchFiles/Build.bat",
    "Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll",
    "Binaries/Win64/UnrealEditor.exe",
    "Binaries/Win64/UnrealEditor-Cmd.exe",
)


def read_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8-sig")), None
    except (OSError, ValueError) as error:
        return None, str(error)


def same_path(left, right):
    return os.path.normcase(os.path.abspath(left)) == os.path.normcase(os.path.abspath(right))


def inspect_engine(engine, installed, manifests):
    files = {name: (engine / name).is_file() for name in REQUIRED_FILES}
    version, version_error = read_json(engine / "Build/Build.version")
    registered = any(
        str(item.get("AppName", "")).startswith("UE_")
        and same_path(item.get("InstallLocation", ""), engine.parent)
        for item in (installed if isinstance(installed, dict) else {}).get("InstallationList", []) if isinstance(item, dict)
    )
    matching = [item for item in manifests
                if str(item.get("AppName", "")).startswith("UE_")
                and same_path(item.get("InstallLocation", ""), engine.parent)]
    incomplete = any(item.get("bIsIncompleteInstall") is True for item in matching)
    pending = engine.parent / ".egstore/Pending"
    pending_files = pending.exists() and any(path.is_file() for path in pending.rglob("*"))
    reasons = []
    if not registered:
        reasons.append("Engine has no matching LauncherInstalled registration; plugin entries do not count.")
    if incomplete or pending_files:
        reasons.append("Launcher still has incomplete or pending installation data.")
    if not all(files.values()) or version_error:
        reasons.append("Required engine files or a readable Build.version are missing.")
    if not isinstance(version, dict) or not all(type(version.get(key)) is int for key in ("MajorVersion", "MinorVersion", "PatchVersion")):
        reasons.append("Build.version does not contain a valid version tuple.")
    return {
        "engine_root": str(engine),
        "version": version,
        "version_error": version_error,
        "files": files,
        "generate_project_files_bat": (engine / "Build/BatchFiles/GenerateProjectFiles.bat").is_file(),
        "launcher_registered": registered,
        "manifest_incomplete": incomplete,
        "pending_files": bool(pending_files),
        "installation_ready": not reasons,
        "reasons": reasons,
    }


def command(root, *args):
    try:
        result = subprocess.run(args, cwd=root, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=30)
        return {"exit_code": result.returncode, "stdout": result.stdout.strip(), "stderr": result.stderr.strip()}
    except (OSError, subprocess.TimeoutExpired) as error:
        return {"exit_code": None, "error": str(error)}


def tests_policy(build_text):
    # This only describes the rule text. UBT compile actions and test discovery remain required.
    without_comments = re.sub(r"/\*.*?\*/|//[^\n]*", "", build_text, flags=re.S)
    return bool(re.search(r"\bbForceIncludeTestsFolder\s*=\s*true\s*;", without_comments))


def asset_inventory(content):
    counts = {"packages": 0, "lfs_pointers": 0, "unknown_headers": 0, "read_errors": 0}
    pointers = []
    for path in sorted(content.rglob("*")):
        if path.suffix.lower() not in (".umap", ".uasset"):
            continue
        counts["packages"] += 1
        try:
            with path.open("rb") as stream:
                header = stream.read(128)
        except OSError:
            counts["read_errors"] += 1
            continue
        if header.startswith(b"version https://git-lfs.github.com/spec/v1"):
            counts["lfs_pointers"] += 1
            pointers.append(path.relative_to(content).as_posix())
        elif not header.startswith(bytes.fromhex("c1832a9e")):
            counts["unknown_headers"] += 1
    return {**counts, "pointer_paths": pointers, "engine_compatibility": "Not tested; package magic is not a version compatibility check."}


def collect(root, engine, program_data):
    launcher_file = program_data / "Epic/UnrealEngineLauncher/LauncherInstalled.dat"
    installed, installed_error = read_json(launcher_file)
    manifests = []
    manifest_errors = []
    for path in (program_data / "Epic/EpicGamesLauncher/Data/Manifests").glob("*.item"):
        item, error = read_json(path)
        if isinstance(item, dict):
            manifests.append(item)
        elif error:
            manifest_errors.append(str(path))
    installation = inspect_engine(engine, installed, manifests)
    if installed_error or manifest_errors:
        installation["installation_ready"] = False
        installation["reasons"].append("Launcher metadata could not be fully read.")
    build = root / "Source/CCL/CCL.Build.cs"
    test_files = sorted(path.relative_to(root).as_posix() for path in (root / "Source/CCL").rglob("*Tests.cpp"))
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
    visual_studio = command(root, str(vswhere), "-all", "-products", "*", "-format", "json")
    msvc = []
    if visual_studio.get("exit_code") == 0:
        for instance in json.loads(visual_studio["stdout"] or "[]"):
            vc = Path(instance["installationPath"]) / "VC/Tools/MSVC"
            msvc.extend(str(path) for path in vc.glob("*/bin/Hostx64/x64/cl.exe"))
    sdk = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Windows Kits/10/Include"
    return {
        "observed_utc": datetime.now(timezone.utc).isoformat(),
        "repository": str(root),
        "engine": installation,
        "launcher_read_error": installed_error,
        "manifest_read_errors": manifest_errors,
        "tools": {"executables": {name: shutil.which(name) for name in ("git", "python", "node", "rg")},
                  "msvc": msvc, "windows_sdks": sorted(path.name for path in sdk.glob("*") if path.is_dir())},
        "git": {"status": command(root, "git", "status", "--short", "--branch"),
                "upstream": command(root, "git", "rev-parse", "--abbrev-ref", "@{upstream}"),
                "lfs": command(root, "git", "lfs", "version")},
        "tests": {"force_include_tests_folder": tests_policy(build.read_text(encoding="utf-8-sig")),
                  "test_sources": test_files, "compiled_and_discovered": "Not verified"},
        "assets": asset_inventory(root / "Content"),
        "validation": {"project_generation": "Not run", "editor_build": "Not run", "regression": "Not run"},
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine-root", type=Path, help="Engine directory containing Build/Build.version")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    if args.engine_root:
        engine = args.engine_root.resolve()
    else:
        paths, error = read_json(root / ".local/agent-paths.json")
        if error or not isinstance(paths, dict) or not paths.get("engineRoot"):
            parser.error("Pass --engine-root or configure .local/agent-paths.json.")
        engine = Path(paths["engineRoot"])
    data = collect(root, engine, Path(os.environ.get("PROGRAMDATA", "C:/ProgramData")))
    output = args.output or root / "Saved/Tests/EnvironmentPreflight" / (datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") + ".json")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": str(output), "engine": data["engine"], "tools": data["tools"],
                      "tests_force_include": data["tests"]["force_include_tests_folder"], "assets": data["assets"]}, ensure_ascii=False, indent=2))
    return 0 if data["engine"]["installation_ready"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
