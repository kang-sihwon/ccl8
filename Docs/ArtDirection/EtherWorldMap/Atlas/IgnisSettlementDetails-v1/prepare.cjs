const fs=require('node:fs'),path=require('node:path');
const root=__dirname,atlas=path.dirname(root),W=67108864,H=33554432,R=6366.197723675814,rad=Math.PI/180,p0=54*rad,l0=-rad;
function world(x,y){const r=Math.hypot(x,y);if(r<1e-10)return[(l0+Math.PI)/(2*Math.PI)*W,(Math.PI/2-p0)/Math.PI*H];const c=r/R,p=Math.asin(Math.cos(c)*Math.sin(p0)+y*Math.sin(c)*Math.cos(p0)/r),l=l0+Math.atan2(x*Math.sin(c),r*Math.cos(p0)*Math.cos(c)-y*Math.sin(p0)*Math.sin(c));return[(l+Math.PI)/(2*Math.PI)*W,(Math.PI/2-p)/Math.PI*H];}
function local(wx,wy){const p=Math.PI/2-wy/H*Math.PI,l=wx/W*2*Math.PI-Math.PI,c=Math.acos(Math.max(-1,Math.min(1,Math.sin(p0)*Math.sin(p)+Math.cos(p0)*Math.cos(p)*Math.cos(l-l0)))),k=c<1e-10?1:c/Math.sin(c);return[R*k*Math.cos(p)*Math.sin(l-l0),R*k*(Math.cos(p0)*Math.sin(p)-Math.sin(p0)*Math.cos(p)*Math.cos(l-l0))];}
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
for(const d of ['records','guides','generated','tiles','contexts','previews'])fs.mkdirSync(path.join(root,d),{recursive:true});
const old=read(path.join(atlas,'Ignis-v1/provenance/detail-plans.json')),global=read(path.join(atlas,'WorldSectorPlan-v1/sector-plan.json')),jobs=[],settlements=[];
const veles=old.find(s=>s.id==='V01');
veles.bounds_xy_km=[150-860*0.26/165,145-(891-578)*1.25/460,150+(1672-860)*0.26/165,145+(578-50)*1.25/460];
veles.center_xy_km=[150,145.3];veles.role='Woodland riverside village; preserve quay, workshop and paths. No large city.';
veles.prompt='Veles approved terrain reference, approximate two-anchor registration from quay and cave. Preserve river, village footprint and woodland. No new bridge or city.';
for(const s of old){
 const item={id:s.id,name:s.name,kind:s.kind,xy_km:s.xy_km,worldPixel:world(...s.xy_km),sourceBounds:s.bounds_xy_km,source:`../Ignis-v1/${s.id}-terrain.png`,points:(s.details||[]).map(p=>({...p,worldPixel:world(...p.xy_km)})),tiles:[],status:'context_only'};
 if(s.id==='V01'){item.source='contexts/source-V01.png';item.registration='approximate_quay_cave_two_anchor; forest gate residual about 140 m; other facilities not aligned';}
 if(s.id==='M01'){const m=read(path.join(atlas,'IgnisM01Details-v1/records/plan.json'));item.tiles=m.tiles.map(t=>({...t,level:10,path:`../IgnisM01Details-v1/tiles/${t.id}.png`}));item.status='existing_12_detail_tiles';}
 if(!s.bounds_xy_km){item.source='../Ignis-v1/V01.png';item.status='approved_reference_registration_pending';settlements.push(item);continue;}
 const b=s.bounds_xy_km,center=world(...(s.details?.length?s.center_xy_km||s.xy_km:s.xy_km)),corners=[[b[0],b[1]],[b[0],b[3]],[b[2],b[1]],[b[2],b[3]]].map(p=>world(...p));
 item.contextBounds=[Math.floor(Math.min(...corners.map(p=>p[0]))),Math.floor(Math.min(...corners.map(p=>p[1]))),Math.ceil(Math.max(...corners.map(p=>p[0]))),Math.ceil(Math.max(...corners.map(p=>p[1])))];item.context=`contexts/${s.id}.png`;
 if(s.id!=='M01'){
  let candidates=[],factor=s.kind==='village'?2:1,level=s.kind==='village'?11:10;
  for(;level<=11;level++,factor*=2){
  const span=2048/factor,margin=128/factor;
  for(let r=Math.floor(item.contextBounds[1]/span);r<=Math.floor(item.contextBounds[3]/span);r++)for(let c=Math.floor(item.contextBounds[0]/span);c<=Math.floor(item.contextBounds[2]/span);c++){
   const checks=[];for(const dy of [-margin,span/2,span+margin])for(const dx of [-margin,span/2,span+margin])checks.push(local(c*span+dx,r*span+dy));
   if(checks.every(p=>p[0]>=b[0]&&p[0]<=b[2]&&p[1]>=b[1]&&p[1]<=b[3]))candidates.push({c,r,d:Math.hypot((c+.5)*span-center[0],(r+.5)*span-center[1])});
  }
  if(candidates.length)break;
  }
  candidates.sort((a,b)=>a.d-b.d);const chosen=[];
  while(chosen.length<4){const i=candidates.findIndex(t=>!chosen.length||chosen.some(p=>Math.abs(t.c-p.c)+Math.abs(t.r-p.r)===1));if(i<0)break;chosen.push(candidates.splice(i,1)[0]);}
  for(const t of chosen){const span=2048/factor,id=`L${level}-C${t.c}-R${t.r}`,job={id,level,settlement:s.id,name:s.name,column:t.c,row:t.r,worldPixelBounds:[t.c*span,t.r*span,(t.c+1)*span,(t.r+1)*span],worldMargin:128/factor,source:item.source,sourceBounds:b,guide:`guides/${id}.png`,generated:`generated/${id}.png`,path:`tiles/${id}.png`,role:s.role||'',sourcePrompt:s.prompt};jobs.push(job);item.tiles.push({...job,sourcePrompt:undefined});}item.status=chosen.length?'planned_detail_core':'context_only_no_complete_tile';
 }
 settlements.push(item);
}
const data={version:1,date:'2026-11-13',status:'in_production_candidate_not_runtime_map',worldSize:[W,H],core:2048,margin:128,workingSize:2304,settlements,jobs,background:{preview:'../IgnisSectors-v1/previews/ignis-assembled-preview.png',bounds:[22528*1024,2048*1024,43008*1024,12288*1024],tiles:global.tiles.map(t=>({...t,worldPixelBounds:t.worldPixelBounds.map(x=>x*1024),path:`../IgnisSectors-v1/tiles/${t.id}.png`}))},features:global.sharedGeometry.features.map(f=>({...f,worldPixelPoints:f.worldPixelPoints.map(p=>p.map(x=>x*1024))})),limitations:['Only planned settlement cores are newly detailed; context images remain old layout references.','Veles approved art has no exact shared georeference; registration is pending.','Background and settlement reference drawings were not generated from one measured terrain surface.','Pixel addresses do not certify building dimensions or semantic road continuity.']};
data.worldPixelBoundsLevel=10;
data.limitations[1]='Veles uses approximate quay/cave registration, not an exact georeference; the forest gate residual is about 140 m.';
fs.writeFileSync(path.join(root,'records/plan.json'),JSON.stringify(data,null,2));console.log(JSON.stringify({settlements:settlements.length,newTiles:jobs.length,reusedTiles:12,bySettlement:settlements.map(s=>({id:s.id,tiles:s.tiles.length,status:s.status}))}));
module.exports={world,local};
