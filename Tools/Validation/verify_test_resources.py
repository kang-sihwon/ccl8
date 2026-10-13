"""Fail if a shipped test resource/map still depends on the local Fab library."""
import json
from pathlib import Path
import unreal

R=unreal.AssetRegistryHelpers.get_asset_registry()
A=unreal.EditorAssetLibrary
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out=root/'Saved/Laboratories'
options=unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True)
paths=['/Game/TestResources','/Game/Maps/Tests','/Game/__ExternalActors__/Maps/EnvironmentScenario','/Game/__ExternalActors__/Maps/EnvironmentPlayground','/Game/__ExternalActors__/Maps/CombatPlayground']
R.scan_paths_synchronous(paths,True)
seeds={str(a.package_name) for folder in paths for a in R.get_assets_by_path(folder,recursive=True)}
seeds.update('/Game/Maps/'+x for x in ('EnvironmentScenario','EnvironmentPlayground','CombatPlayground'))
seeds.update('/Game/Progression/'+x for x in ('DA_TrainingSword','DA_TrainingShield'))
pending=list(seeds);seen=set();missing=[];fab=[];parents={}
while pending:
 p=pending.pop()
 if p in seen or not p.startswith('/Game/'):continue
 seen.add(p)
 if p.startswith('/Game/Fab/'):
  fab.append(p);continue
 disk=root/'Content'/p.removeprefix('/Game/')
 if not disk.with_suffix('.uasset').exists() and not disk.with_suffix('.umap').exists():missing.append(p);continue
 for dependency in R.get_dependencies(p,options):
  dependency=str(dependency)
  parents.setdefault(dependency,[]).append(p)
  pending.append(dependency)
files=list((root/'Content/TestResources').rglob('*.uasset'))
resource_missing=[p for p in missing if any(q.startswith('/Game/TestResources/') for q in parents.get(p,[]))]
report={'roots':len(seeds),'checked_packages':len(seen),'fab_references':sorted(fab),'missing_resource_dependencies':resource_missing,'other_missing_references':{p:parents.get(p,[]) for p in missing if p not in resource_missing},'resource_files':len(files),'resource_bytes':sum(p.stat().st_size for p in files)}
(out/'resource-audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('CCL_RESOURCE_AUDIT '+json.dumps(report))
assert not fab and not resource_missing,report
