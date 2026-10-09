"""Start the opt-in CCLUISmoke suite in a real editor PIE viewport."""
import time
import unreal

started = time.monotonic()

def begin_play(delta):
    if time.monotonic() - started < 3:
        return
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
    unreal.log('CCL_UI_PIE requested')

handle = unreal.register_slate_post_tick_callback(begin_play)
