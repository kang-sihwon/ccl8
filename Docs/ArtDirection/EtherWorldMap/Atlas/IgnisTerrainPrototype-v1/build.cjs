const fs=require('node:fs'),path=require('node:path');
const root=__dirname,old=path.join(root,'../IgnisSettlementDetails-v1');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const plan=read(path.join(old,'records/plan.json'));
const geo=read(path.join(root,'../../Data/ignis-continent-georeference-v1.json'));
const remap=p=>path.posix.normalize('../IgnisSettlementDetails-v1/'+p);
function world(x,y){const R=6366.197723675814,p0=54*Math.PI/180,l0=-Math.PI/180,r=Math.hypot(x,y),c=r/R;
 if(r===0)return[(l0+Math.PI)/(2*Math.PI)*67108864,(Math.PI/2-p0)/Math.PI*33554432];
 const p=Math.asin(Math.cos(c)*Math.sin(p0)+y*Math.sin(c)*Math.cos(p0)/r),l=l0+Math.atan2(x*Math.sin(c),r*Math.cos(p0)*Math.cos(c)-y*Math.sin(p0)*Math.sin(c));
 return[(l+Math.PI)/(2*Math.PI)*67108864,(Math.PI/2-p)/Math.PI*33554432];}
for(const s of plan.settlements){s.context=remap(s.context);s.source=remap(s.source);const b=s.sourceBounds;s.contextPolygon=[[b[0],b[3]],[b[2],b[3]],[b[2],b[1]],[b[0],b[1]]].map(p=>world(...p));for(const t of s.tiles){for(const key of ['path','source','guide','generated'])if(t[key])t[key]=remap(t[key]);t.available=fs.existsSync(path.join(root,t.path));}}
plan.background.preview=remap(plan.background.preview);
for(const t of plan.background.tiles)t.path=remap(t.path);
for(const f of plan.features){const route=geo.routes.find(r=>r.id===f.id);if(route){f.mode=route.mode;f.stops=route.stops;}}
delete plan.jobs;
plan.prototype={scope:'B04 V14 to T11 first connection quality gate',status:'prototype_not_whole_continent_approved',fixedSeed:'shared-world-v1',widthsAreCandidates:true,roadMeters:6,mainRiverMeters:180,tributaryMeters:55,hasMeasuredHeightmap:false};
plan.prototype.joins=[{id:'T11',center:plan.settlements.find(s=>s.id==='T11').worldPixel,inner:3300,outer:7000,roadHalf:32,riverExtra:(200-55)/.596/2},{id:'V14',center:plan.settlements.find(s=>s.id==='V14').worldPixel,inner:2000,outer:4500,roadHalf:22,riverExtra:0}];
plan.prototype.joinStatus='visual_width_transition_candidates_not_surveyed_dimensions';
const material='data:image/png;base64,'+fs.readFileSync(path.join(root,'materials/aerial-material-atlas.png')).toString('base64');
const macro='data:image/png;base64,'+fs.readFileSync(path.join(root,plan.background.preview)).toString('base64');
let template=fs.readFileSync(path.join(old,'viewer-template.html'),'utf8');
template=template.replace('<title>이그니스 대륙 상세 도안</title>','<title>이그니스 공통 지형 연결 구간 검토</title>');
template=template.replace('<button id="focus">','<button id="corridor">연결 구간</button><button id="middle">길 확대</button><button id="junction">접합 확인</button><button id="focus">');
template=template.replace('<script>','<script src="terrain.js"></script><script>');
template=template.replace('const DATA=__DATA__;','const DATA=__DATA__;\nconst MATERIAL='+JSON.stringify(material)+';const MACRO='+JSON.stringify(macro)+';');
template=template.replace('const images=new Map()', 'const images=new Map()');
template=template.replace('function requestDraw()',fs.readFileSync(path.join(root,'viewer-extra.js'),'utf8')+'\nfunction requestDraw()');
template=template.replace('ctx.drawImage(im,a[0],a[1],z[0]-a[0],z[1]-a[1]);','ctx.drawImage(edgeImage(url,im),a[0],a[1],z[0]-a[0],z[1]-a[1]);');
template=template.replace('if(zoom>.003)for(const s', 'drawSharedTerrain();\n if(zoom>.003)for(const s');
template=template.replace("status.textContent=zoom>.003?(compare?'기존 배치 참고':detailHere?'새 상세 도안':'상세 구역 밖 · 기존 참고'):'정착지 36곳';", "status.textContent=compare?'기존 지형 비교':detailHere?'정착지 상세 도안':'공통 재질·식생 시제품';");
template=template.replace("note.textContent=selected?.registration?'벨레스: 나루·동굴 기준의 임시 정합 · 다른 시설 좌표 차이 남음':selected?'새 상세 구역 + 기존 일대 참고 · 개별 건물 치수 미확정':'대륙 배경과 정착지 상세 도안 · 정착지 사이 배경은 개략도';", "note.textContent='연결 구간 품질 검토 · 공통 좌표 고정 · 재질/식생은 제작 후보 · 전체 품질 승인 전';");
template=template.replace("window.mapFocus=focus;resize();", "window.mapFocus=focus;resize();initPrototype();");
template=template.replace('__DATA__',JSON.stringify(plan).replaceAll('<','\\u003c'));
fs.writeFileSync(path.join(root,'index.html'),template);
fs.writeFileSync(path.join(root,'records/plan.json'),JSON.stringify({...plan,material:'materials/aerial-material-atlas.png'},null,2)+'\n');
console.log('Prototype viewer built. Existing 36 settlement coordinates and 150 images referenced without duplication.');
