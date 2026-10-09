"""Update display text only; keep asset IDs, gameplay data and map layout intact."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
translations = json.loads((root / 'Tools/Validation/korean_ui_text.json').read_text(encoding='utf-8'))
translations.update({'DA_Unarmed': '맨손', 'DA_EnemyUnarmed': '적의 맨손', 'VILLAGE  >  EAST GATE': '마을 → 동쪽 성문'})
font = unreal.load_asset('/Game/UI/Fonts/F_KoreanLabel')
assert font, 'Generate F_KoreanLabel before translating world labels.'
rows = []
unknown = []

def translated(value, context):
    text = str(value)
    result = translations.get(text, text)
    if text and not any('\uac00' <= c <= '\ud7a3' for c in result):
        unknown.append({'context': context, 'text': text})
    return result

for path in sorted((root / 'Content').rglob('*.uasset')):
    if '__ExternalActors__' in path.parts:
        continue
    package = '/Game/' + path.relative_to(root / 'Content').with_suffix('').as_posix()
    # Only project item/skill definitions have player-facing editable data here.
    if not package.startswith(('/Game/Combat/', '/Game/Progression/', '/Game/Items/')):
        continue
    asset = unreal.load_asset(package)
    props = ['item_name', 'description'] if isinstance(asset, unreal.CCLItemDefinition) else ['label'] if isinstance(asset, unreal.CCLSkillDefinition) else []
    changed = False
    for prop in props:
        old = str(asset.get_editor_property(prop))
        new = translated(old, package + ':' + prop)
        rows.append({'package': package, 'property': prop, 'text': new})
        if old != new:
            asset.set_editor_property(prop, new)
            changed = True
    if changed:
        assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False), package

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for name in ('MultiplayerPlayground', 'CombatPlayground', 'Campaign', 'FrontEnd'):
    assert levels.load_level('/Game/Maps/' + name)
    descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
    if descs:
        unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in descs])
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.CCLVillageSteward):
            old = str(actor.get_editor_property('display_name'))
            actor.set_editor_property('display_name', translated(old, name + ':display_name'))
        if isinstance(actor, unreal.CCLEnemyCharacter):
            old = str(actor.get_editor_property('display_name'))
            actor.set_editor_property('display_name', translated(old, name + ':enemy'))
        for component in actor.get_components_by_class(unreal.TextRenderComponent):
            old = str(component.get_editor_property('text'))
            new = translated(old, name + ':' + actor.get_actor_label())
            component.set_editor_property('text', new)
            component.set_editor_property('font', font)
            rows.append({'map': name, 'actor': actor.get_actor_label(), 'text': new})
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

out = root / 'Saved/KoreanUI'
out.mkdir(parents=True, exist_ok=True)
(out / 'assets.json').write_text(json.dumps({'text': rows, 'unknown': unknown}, ensure_ascii=False, indent=2), encoding='utf-8')
assert not unknown, 'Untranslated display text: ' + str(unknown)
unreal.log('CCL_KOREAN_ASSETS PASS fields=%d' % len(rows))
