"""Run with Blender --background --disable-autoexec <source.blend> --python this.py -- <name> <output.fbx>.

Preserve an explicit rig object as the single FBX root. Unreal's special Armature removal
otherwise produces incompatible scale tracks for these older Blender sources.
"""
import bpy, sys
args = sys.argv[sys.argv.index('--') + 1:]
name, output = args[:2]
action_name = args[2] if len(args) > 2 else None
rigs = [o for o in bpy.data.objects if o.type == 'ARMATURE']
assert len(rigs) == 1
rig = rigs[0]
rig.name = name + 'Rig'
if name == 'Sheep':
    for action in list(bpy.data.actions):
        if action.name not in ('Walk', 'Idle'): bpy.data.actions.remove(action, do_unlink=True)
if action_name:
    rig.animation_data_create()
    rig.animation_data.action = bpy.data.actions[action_name]
    for track in rig.animation_data.nla_tracks: track.mute = True
    bpy.context.scene.frame_start, bpy.context.scene.frame_end = [int(v) for v in rig.animation_data.action.frame_range]
for o in bpy.data.objects:
    o.select = o == rig or (o.type == 'MESH' and any(m.type == 'ARMATURE' and m.object == rig for m in o.modifiers))
bpy.ops.export_scene.fbx(filepath=output, use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False,
    bake_anim=True, bake_anim_use_all_actions=not bool(action_name), bake_anim_use_nla_strips=False, mesh_smooth_type='FACE', axis_forward='-Y', axis_up='Z')
