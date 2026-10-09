"""Open saved experiment maps in a fresh editor process and audit their actor content."""
from pathlib import Path
import json
import unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
registry=unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
rows=[]
for name in ('EnvironmentPlayground','EnvironmentScenario'):
    path='/Game/Maps/'+name
    assert levels.load_level(path), path
    descs=unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
    if descs:
        unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in descs])
    current=actors.get_all_level_actors()
    stations=[a for a in current if isinstance(a,unreal.CCLExperimentStation)]
    directors=[a for a in current if isinstance(a,unreal.CCLExperimentDirector)]
    unreal.log(f'CCL_ENV_AUDIT {name} descriptors={len(descs)} actors={len(current)} stations={len(stations)} directors={len(directors)}')
    assert len(stations)==12 and len(directors)==1, path
    definitions=directors[0].get_editor_property('definitions')
    assert len(definitions)==12 and sorted(d.get_editor_property('zone') for d in definitions)==list(range(12))
    assert sorted(a.get_editor_property('definition').get_editor_property('zone') for a in stations)==list(range(12))
    configs=[a for a in current if isinstance(a,unreal.CCLWorldEnvironmentConfig)]
    presentations=[a for a in current if isinstance(a,unreal.CCLWorldEnvironmentPresentation)]
    assert len(configs)==1 and len(presentations)==1
    config=configs[0]
    assert len(config.get_editor_property('surfaces'))==8
    assert len(config.get_editor_property('openings'))==1
    assert len(config.get_editor_property('probes'))==3
    assert len(config.get_editor_property('view_surface_ids'))==8
    assert config.get_editor_property('celestial_definition')
    sun=presentations[0].get_editor_property('sun')
    assert sun and sun.light_component.get_editor_property('mobility') == unreal.ComponentMobility.MOVABLE
    assert presentations[0].get_editor_property('solid_mesh')
    presentations[0].refresh_preview()
    assert levels.save_current_level()
    rows.append(dict(map=path,stations=len(stations),definitions=len(definitions),opened_and_saved=True,celestial_config=True,surfaces=8,openings=1,probes=3))
(root/'Saved/EnvironmentGoal/experiment-assets.json').write_text(json.dumps(dict(maps=rows,fresh_editor_reload=True),indent=2),encoding='utf-8')
unreal.log('CCL_EXPERIMENT_ASSETS PASS maps=2 stations_per_map=12 fresh_reload=True')
