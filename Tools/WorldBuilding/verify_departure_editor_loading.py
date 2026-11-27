"""Open the departure map normally twice in a disposable editor, without PIE or load commands."""
import json
from pathlib import Path
import time
import traceback

import unreal


target = "/Game/Level/VelesDeparture/Connected/WP_VelesDeparture"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
started = time.monotonic()


def survey():
    assert not levels.is_in_play_in_editor() and not editor.get_game_world()
    world = editor.get_editor_world()
    assert world.get_package().get_name() == target
    loaded = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
    tiles = sum(label.startswith("Departure ground ") for label in loaded)
    region = loaded["Departure route editor region"]
    assert region.is_loaded() and tiles == 144, tiles
    poses = []
    for label in ("V01 Workshop", "F00 Golden mist watershed", "C03 Bartul cavern"):
        camera = loaded[label]
        eye = camera.get_actor_location()
        editor.set_level_viewport_camera_info(eye, camera.get_actor_rotation())
        observed, _ = editor.get_level_viewport_camera_info()
        assert (observed - eye).length() < 1, label
        poses.append(label)
    return {"terrain_tiles": tiles, "volume_loaded": True, "camera_views": poses}


def verify(delta):
    if time.monotonic() - started < 1:
        return
    unreal.unregister_slate_post_tick_callback(callback)
    try:
        report = {"cold_open": survey()}
        for attempt in ("reopen_1", "reopen_2"):
            assert levels.load_level("/Game/Level/Empty")
            assert levels.load_level(target)
            report[attempt] = survey()
        path = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
        out = path / "Tests/DepartureEditorReopen"
        out.mkdir(exist_ok=True)
        (out / "cold.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        unreal.log("DEPARTURE_EDITOR_COLD_PASS")
    except Exception:
        unreal.log_error(traceback.format_exc())
    finally:
        unreal.SystemLibrary.quit_editor()


callback = unreal.register_slate_post_tick_callback(verify)
