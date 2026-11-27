"""Open the full authoring region when the departure map is opened in the level editor."""
import unreal


MAP = "/Game/Level/VelesDeparture/Connected/WP_VelesDeparture"
VOLUME = "Departure route editor region"
_installed = False
_startup_tick = None


def load_region(filename="", as_template=False):
    if as_template:
        return
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = editor.get_editor_world()
    if not world or world.get_package().get_name() != MAP:
        return
    for volume in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.LocationVolume):
        if volume.get_actor_label() == VOLUME:
            if not volume.is_loaded():
                volume.load()
            return


def install():
    global _installed
    if not _installed:
        levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        levels.on_map_opened.add_callable(load_region)
        _installed = True
    load_region()


def install_on_first_editor_tick():
    global _startup_tick
    if _installed or _startup_tick is not None:
        return
    def first_tick(delta):
        global _startup_tick
        unreal.unregister_slate_post_tick_callback(_startup_tick)
        _startup_tick = None
        if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem):
            install()
    _startup_tick = unreal.register_slate_post_tick_callback(first_tick)
