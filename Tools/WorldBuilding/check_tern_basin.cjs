'use strict';
// 표시되는 월드 선분을 이웃 타일마다 독립적으로 잘라 접점·방향·폭을 비교한다.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const dir=path.resolve(__dirname,'../../Docs/ArtDirection/EtherWorldMap/Atlas/VelesTarnelRoden-v1/TernBasin377');
const context=vm.createContext({document:{getElementById:()=>({getContext:()=>({})})}});
for(const name of ['lore','geography','layouts','hydrology','drawing'])vm.runInContext(fs.readFileSync(path.join(dir,name+'.js'),'utf8'),context);
const {geo,ways,tileAt,tileBounds,regionAt,localPlans,stageAt,detail}=vm.runInContext('({geo,ways,tileAt,tileBounds,regionAt,localPlans,stageAt,detail})',context);
const neighbors=new Map(),legacy=new Map(),zones=Array(13).fill(0),eps=1e-6;
let edgeCount=0,pairCount=0,worstGap=0,worstAngle=0;
const old=(i,j)=>Math.max(Math.abs(i),Math.abs(j))<=2;
const valid=(i,j)=>Math.max(Math.abs(i),Math.abs(j))<=188;
function clipped(a,b,box){
 let lo=0,hi=1;
 for(let k=0;k<2;k++){
  let d=b[k]-a[k];if(!d){if(a[k]<box[k]||a[k]>box[k+2])return null;continue}
  const t=(box[k]-a[k])/d,u=(box[k+2]-a[k])/d;
  lo=Math.max(lo,Math.min(t,u));hi=Math.min(hi,Math.max(t,u));
 }
 return hi-lo>1e-10?[lo,hi]:null;
}
function record(map,i,j,axis,side,id,p,dx,dy,w){
 const key=[i-(axis===0&&!side?1:0),j-(axis===1&&!side?1:0),axis].join('/');
 if(!map.has(key))map.set(key,[[],[]]);
 const list=map.get(key)[side],v=p[1-axis];
 if(!list.some(c=>c.id===id&&Math.abs(v-c.v)<.00001))list.push({id,v,dx,dy,w});
}
function collect(id,ps,w,isolated){
 for(let k=1;k<ps.length;k++){
  const a=ps[k-1],b=ps[k],ax=b[0]-a[0],ay=b[1]-a[1],length=Math.hypot(ax,ay);
  if(length<eps)continue;
  const dx=ax/length,dy=ay/length,from=tileAt(Math.min(a[0],b[0])-eps,Math.min(a[1],b[1])-eps),to=tileAt(Math.max(a[0],b[0])+eps,Math.max(a[1],b[1])+eps);
  for(let i=from[0];i<=to[0];i++)for(let j=from[1];j<=to[1];j++){
   if(!valid(i,j)||(isolated&&(i!==isolated[0]||j!==isolated[1]))||(!isolated&&old(i,j)))continue;
   const box=tileBounds(i,j),ts=clipped(a,b,box);if(!ts)continue;
   for(const t of ts){
    const p=[a[0]+t*ax,a[1]+t*ay];
    for(let axis=0;axis<2;axis++)for(let side=0;side<2;side++){
     if(Math.abs(p[axis]-box[axis+side*2])>eps)continue;
     if(isolated&&old(i+(axis===0?(side?1:-1):0),j+(axis===1?(side?1:-1):0)))continue;
     record(isolated?legacy:neighbors,i,j,axis,side,id,p,dx,dy,w);
    }
   }
  }
 }
}
for(const l of ways){
 collect(l.id,l.points,l.width);
 if(l.banks&&l.id!=='creek')for(let i=0;i<2;i++)collect(l.id+'-bank'+i,l.banks.map(pair=>pair[i]),1.6);
}
for(const d of detail)if(['walk','haul','back','detour','retreat','stream'].includes(d.kind)){
 // 이전 다섯 장을 덮는 국소 이동선은 사건 레이어다. 구형 도판 일반층 접합과 따로 검사한다.
 if(d.frame!=='V')collect(d.id,d.p,d.kind==='stream'?3.2:3.7);
}
for(let i=0;i<2;i++)collect('bank'+i,geo.water.map(row=>[row[i+1],row[0]]),5.1);
for(const [k,t] of Object.entries(geo.grid)){
 if(Math.max(...t.offset.map(Math.abs))!==2)continue;
 const svg=fs.readFileSync(path.join(dir,'../VelesCardinalTiles',k+'.svg'),'utf8');
 for(const el of svg.match(/<path\b[^>]*data-line="[^"]+"[^>]*>/g)||[]){
  const id=el.match(/data-line="([^"]+)"/)[1],w=Number(el.match(/stroke-width="([^"]+)"/)[1]);
  const n=el.match(/ d="([^"]+)"/)[1].match(/-?\d+(?:\.\d+)?/g).map(Number);
  let points=[];
  for(let k=0;k+7<n.length;k+=6){
   const row=n.slice(k,k+8);
   for(let p=0;p<64;p++){let t=p/64,s=1-t;points.push([0,1].map(i=>s**3*row[i]+3*s*s*t*row[i+2]+3*s*t*t*row[i+4]+t**3*row[i+6]))}
  }
  points.push(n.slice(-2));collect(id,points,w,t.offset);
 }
}
let oldContacts=0;
function compare(i,j,axis){
 const other=[i+(axis===0),j+(axis===1)],box=tileBounds(i,j),q=tileBounds(...other);
 assert.equal(box[axis+2],q[axis]);assert.equal(box[1-axis],q[1-axis]);assert.equal(box[3-axis],q[3-axis]);edgeCount++;
 if(old(i,j)&&old(...other))return;
 const key=[i,j,axis].join('/'),inner=old(i,j)||old(...other);
 const sides=[old(...other)?legacy:neighbors,old(i,j)?legacy:neighbors].map((m,s)=>(m.get(key)?.[s]||[]).sort((a,b)=>a.id.localeCompare(b.id)||a.v-b.v));
 const [a,b]=sides;assert.equal(a.length,b.length,'교차 수 '+key+' '+JSON.stringify(sides));
 a.forEach((c,k)=>{
  let d=b[k],gap=Math.abs(c.v-d.v),ang=Math.asin(Math.min(1,Math.abs(c.dx*d.dy-c.dy*d.dx)))*180/Math.PI;
  assert.equal(c.id,d.id,key);assert.equal(c.w,d.w,key);
  assert(gap<(inner?.025:eps)&&ang<(inner?2:.02),[key,c.id,gap,ang].join(' '));
  worstGap=Math.max(worstGap,gap);worstAngle=Math.max(worstAngle,ang);pairCount++;if(inner)oldContacts++;
 });
}
for(let i=-188;i<=188;i++)for(let j=-188;j<=188;j++){
 const [l,b,r,t]=tileBounds(i,j),x=(l+r)/2,y=(b+t)/2;
 assert.equal(tileAt(x,y).join(','),[i,j].join(','));zones[regionAt(x,y)]++;
 if(i<188)compare(i,j,0);if(j<188)compare(i,j,1);
}
assert.equal(edgeCount,283504);assert.equal(zones.reduce((a,b)=>a+b),142129);assert(zones.every(n=>n>0));assert(oldContacts>=7);
const water=geo.water;
function wet(x,y){
 let i=0,j=water.length-1;while(i+1<j){let k=(i+j)>>1;if(water[k][0]<=y)i=k;else j=k}
 let u=(y-water[i][0])/(water[j][0]-water[i][0]);
 const bank=s=>water[i][s]*(1-u)+u*water[j][s];
 return x>=bank(1)&&x<=bank(2);
}
for(const p of localPlans){
 assert(p.homes.length);
 for(const [x,y,w,h,a] of p.homes){
  for(const u of [-.5,.5])for(const v of [-.5,.5]){
   const px=x+u*w*Math.cos(a)-v*h*Math.sin(a),py=y+u*w*Math.sin(a)+v*h*Math.cos(a);
   assert(!wet(px,py),p.site[1]+' 본류 위 건물');
   for(const way of ways.filter(way=>way.kind==='stream'))for(const ch of way.chunks){
    if(px<ch.b[0]-50||px>ch.b[2]+50||py<ch.b[1]-50||py>ch.b[3]+50)continue;
    for(let k=1;k<ch.ps.length;k++){
     const a=ch.ps[k-1],b=ch.ps[k],dx=b[0]-a[0],dy=b[1]-a[1],s=Math.max(0,Math.min(1,((px-a[0])*dx+(py-a[1])*dy)/(dx*dx+dy*dy||1)));
     assert(Math.hypot(px-a[0]-s*dx,py-a[1]-s*dy)>way.width*.62,p.site[1]+' 지류 위 건물');
    }
   }
  }
 }
}
for(const id of ['ash','deer','clay','reed','mill'])assert(wet(...ways.find(l=>l.id===id).points.at(-1)),id+' 합류');
const match=(a,b,tol=.1)=>Math.hypot(a[0]-b[0],a[1]-b[1])<tol;
const coord=id=>geo.details.find(d=>d.id===id).p[0];
for(const [id,ring] of [['F-UP',7],['C-MOUTH',9],['B01',367]])assert.equal(stageAt(...coord(id)),ring);
assert.equal(stageAt(-40000,-50000),369);
const weir=ways.find(l=>l.id==='weir').points;assert(match(weir[0],coord('A07'))&&match(weir.at(-1),coord('A08')));
for(const [id,stops] of [['bypass',['B07','B12','B09']],['r-stretcher',['B08','R-GATE','B10']]]){
 const ps=geo.details.find(d=>d.id===id).p;let j=-1;
 for(const name of stops){const idx=ps.findIndex((p,k)=>k>j&&match(p,coord(name)));assert(idx>j,name);j=idx}
}
console.log(JSON.stringify({tiles:142129,edges:edgeCount,regions:zones.length,vectorCrossings:pairCount,legacyOuterCrossings:oldContacts,maxGapMetres:worstGap,maxAngleDegrees:worstAngle,transportCrossings:vm.runInContext('crossings.length',context),plannedBuildings:localPlans.reduce((n,p)=>n+p.homes.length,0)},null,2));

