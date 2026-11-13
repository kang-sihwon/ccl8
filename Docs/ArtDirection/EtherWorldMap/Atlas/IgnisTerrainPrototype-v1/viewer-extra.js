let terrain,terrainError=null,terrainMs=0;
const coastFeature=DATA.features.find(f=>f.kind==='coast');
const pilot=DATA.features.find(f=>f.id==='B04');
const edgeCache=new Map();
function edgeImage(url,im){
 const s=DATA.settlements.find(s=>s.context===url),tile=allTiles.find(t=>t.path===url);
 if(!s&&!tile)return im;
 if(edgeCache.has(url))return edgeCache.get(url);
 const c=document.createElement('canvas');c.width=im.width;c.height=im.height;const g=c.getContext('2d');g.drawImage(im,0,0);
 let edges=[true,true,true,true];
 if(tile){const b=tile.worldPixelBounds,near=allTiles.filter(t=>t.settlement===tile.settlement&&t.path!==url).map(t=>t.worldPixelBounds);
 edges=[!near.some(n=>n[2]===b[0]&&n[1]===b[1]),!near.some(n=>n[0]===b[2]&&n[1]===b[1]),!near.some(n=>n[3]===b[1]&&n[0]===b[0]),!near.some(n=>n[1]===b[3]&&n[0]===b[0])];}
 const m=tile?100:Math.round(Math.min(c.width,c.height)*.16);g.globalCompositeOperation='destination-out';
 if(s?.contextPolygon){
  const b=s.contextBounds;g.beginPath();s.contextPolygon.forEach((p,i)=>{const q=[(p[0]-b[0])/(b[2]-b[0])*c.width,(p[1]-b[1])/(b[3]-b[1])*c.height];i?g.lineTo(...q):g.moveTo(...q)});g.closePath();
  for(let d=m;d>0;d-=8){g.lineWidth=d*2;g.strokeStyle=`rgba(0,0,0,${Math.min(1,8/d)})`;g.stroke();}
  edgeCache.set(url,c);return c;
 }
 for(let k=0;k<4;k++){if(!edges[k])continue;const w=c.width,h=c.height;
 const gr=k===0?g.createLinearGradient(0,0,m,0):k===1?g.createLinearGradient(w,0,w-m,0):k===2?g.createLinearGradient(0,0,0,m):g.createLinearGradient(0,h,0,h-m);
 gr.addColorStop(0,'rgba(0,0,0,1)');gr.addColorStop(1,'rgba(0,0,0,0)');g.fillStyle=gr;
 if(k===0)g.fillRect(0,0,m,h);if(k===1)g.fillRect(w-m,0,m,h);if(k===2)g.fillRect(0,0,w,m);if(k===3)g.fillRect(0,h-m,w,m);}
 edgeCache.set(url,c);if(edgeCache.size>24)edgeCache.delete(edgeCache.keys().next().value);return c;
}
function featurePath(feature){ctx.beginPath();feature.worldPixelPoints.forEach((p,i)=>{const q=screen(...p);i?ctx.lineTo(...q):ctx.moveTo(...q)});}
function drawSharedTerrain(){
 if(compare||zoom<.0008||!terrain?.loaded)return;
 const start=performance.now(),a=world(0,0),b=world(width,height);
 const amount=Math.max(0,Math.min(1,(Math.log2(zoom/.0008))/3));
 const surface=terrain.render(width,height,[...a,...b],DATA.background.bounds,amount,DATA.features);
 ctx.save();featurePath(coastFeature);ctx.closePath();ctx.clip();ctx.drawImage(surface,0,0,width,height);
 ctx.restore();terrainMs=performance.now()-start;
}
function pilotFrame(){selected=null;select.value='';document.querySelector('#title').textContent='길목 → 케르딘';
 const p=pilot.worldPixelPoints,b=[Math.min(...p.map(p=>p[0]))-5000,Math.min(...p.map(p=>p[1]))-5000,Math.max(...p.map(p=>p[0]))+5000,Math.max(...p.map(p=>p[1]))+5000];frame(b,true);}
function pilotMiddle(t=.5,z=.4){selected=null;select.value='';const a=pilot.worldPixelPoints[0],b=pilot.worldPixelPoints.at(-1);cx=a[0]*(1-t)+b[0]*t;cy=a[1]*(1-t)+b[1]*t;aspect=Math.cos(Math.PI/2-cy/DATA.worldSize[1]*Math.PI);zoom=z;document.querySelector('#title').textContent='길목 → 케르딘 · 연결 지형';requestDraw();}
function pilotJunction(){focus('T11',true);zoom*=1.7;const a=pilot.worldPixelPoints[0],b=pilot.worldPixelPoints.at(-1);cx=b[0]+(a[0]-b[0])*.025;cy=b[1]+(a[1]-b[1])*.025;requestDraw();}
function initPrototype(){
 document.querySelector('#corridor').onclick=pilotFrame;document.querySelector('#middle').onclick=()=>pilotMiddle();document.querySelector('#junction').onclick=pilotJunction;
 try{terrain=new SharedTerrain(MATERIAL,MACRO);terrain.joins=DATA.prototype.joins;terrain.ready.then(requestDraw).catch(e=>{terrainError=e.message;note.textContent='지형 렌더러 오류: '+e.message;});}catch(e){terrainError=e.message;}
 window.terrainDebug=()=>({loaded:!!terrain?.loaded,error:terrainError,frameMs:terrainMs,edgeCache:edgeCache.size,pilot:pilot.id,scope:'connection_quality_prototype'});
 window.pilotFrame=pilotFrame;window.pilotMiddle=pilotMiddle;window.pilotJunction=pilotJunction;
 window.terrainCapture=(w,h,b)=>terrain.render(w,h,b,DATA.background.bounds,1,DATA.features).toDataURL('image/png');
 pilotFrame();
}
