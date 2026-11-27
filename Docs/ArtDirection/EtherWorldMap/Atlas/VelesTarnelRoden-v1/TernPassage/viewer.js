'use strict';
const el=id=>document.getElementById(id);
const canvas=el('map'),ctx=canvas.getContext('2d'),art=el('art');
const [w,h]=atlas.size,[ox,oy]=atlas.origin;
const entries=Object.entries(atlas.tiles).map(([k,t])=>({k,...t,ij:k.split('_').map(Number)}));
const legacy={벨레스:'벨레스 마을권',타르넬:'타르넬 도시와 근교',로덴:'로덴 도시와 근교'};
const sites=[
  ['벨레스 출항 나루',[16.3,5.5]],['서쪽 판재길',atlas.legs.VT.ports.track1[0].world_m],
  ...atlas.legs.VT.places.map(s=>[s.name,s.p]),['타르넬 공동 나루',[-12640,-19920]],['타르넬 하류 쉼나루',[-13360,-21510]],
  ...atlas.legs.TR.places.map(s=>[s.name,s.p]),['로덴 상류 수레길',atlas.legs.TR.ports.track2[0].world_m],['로덴 북나루',[-39960,-49910]]
];
const tileAt=([x,y])=>[Math.floor((x-ox)/w),Math.floor((y-oy)/h)].join('_');
let selected='',center=[0,0],scale=1,routeIndex=-1;
let viewportWidth=0,viewportHeight=0;
function frame(){return canvas.getBoundingClientRect()}
function toScreen(x,y){return [(x-center[0])*scale+viewportWidth/2,(center[1]-y)*scale+viewportHeight/2]}
function toWorld(x,y){return [(x-frame().width/2)/scale+center[0],(frame().height/2-y)/scale+center[1]]}
function coords(k){const [i,j]=k.split('_').map(Number);return [ox+i*w,oy+j*h]}
function fit(){
  const s=el('scope').value, list=entries.filter(t=>s==='all'||t.region===s);
  const xs=list.map(t=>t.ij[0]),ys=list.map(t=>t.ij[1]);
  const a=ox+Math.min(...xs)*w,b=oy+Math.min(...ys)*h,c=ox+(Math.max(...xs)+1)*w,d=oy+(Math.max(...ys)+1)*h;
  center=[(a+c)/2,(b+d)/2];scale=Math.min((frame().width-26)/(c-a),(frame().height-26)/(d-b));draw();
}
function draw(){
  const {width,height}=frame(),ratio=window.devicePixelRatio||1;
  viewportWidth=width;viewportHeight=height;
  if(canvas.width!==Math.round(width*ratio)||canvas.height!==Math.round(height*ratio)){canvas.width=Math.round(width*ratio);canvas.height=Math.round(height*ratio)}
  ctx.setTransform(ratio,0,0,ratio,0,0);ctx.fillStyle='#e5e1ce';ctx.fillRect(0,0,width,height);
  for(const t of entries){
    const [x,y]=coords(t.k),[sx,sy]=toScreen(x,y+h);
    if(sx+w*scale<0||sy+h*scale<0||sx>width||sy>height)continue;
    ctx.fillStyle=legacy[t.region]?'#bca982':'#98b3a6';ctx.fillRect(sx,sy,w*scale+.2,h*scale+.2);
    if(w*scale>7){ctx.strokeStyle='#a0a58e';ctx.lineWidth=.5;ctx.strokeRect(sx,sy,w*scale,h*scale)}
  }
  const mode=el('mode').value;
  for(const ps of atlas.paths[mode]){
    ctx.beginPath();ps.forEach(([x,y],i)=>{const [sx,sy]=toScreen(x,y);if(i===0)ctx.moveTo(sx,sy);else ctx.lineTo(sx,sy)});
    ctx.strokeStyle=mode==='boat'?'#346f75':'#855934';ctx.lineWidth=1.7;ctx.stroke();
  }
  if(selected){
    const [x,y]=coords(selected),[sx,sy]=toScreen(x+w/2,y+h/2);
    ctx.strokeStyle='#9b4129';ctx.lineWidth=2;const pw=Math.max(w*scale,7),ph=Math.max(h*scale,7);
    ctx.strokeRect(sx-pw/2,sy-ph/2,pw,ph);
  }
  for(const [name,p] of [['벨레스',[0,0]],['타르넬',[-13000,-20000]],['로덴',[-40000,-50000]]]){
    const [sx,sy]=toScreen(...p);
    ctx.font='bold 12px Malgun Gothic';ctx.fillStyle='#343b30';ctx.textAlign=sx>width-47?'right':'left';
    ctx.strokeStyle='#f1ebd8';ctx.lineWidth=3;ctx.strokeText(name,sx+3,sy-4);ctx.fillText(name,sx+3,sy-4);
  }
}
function magnify(f){scale=Math.max(.0025,Math.min(.8,scale*f));draw()}
function select(k,index=null){
  const t=atlas.tiles[k];if(!t)return;
  selected=k;art.src=t.url;art.alt=(atlas.legs[t.region]?.name||t.region)+' 세계 타일 '+k;
  el('region').textContent=atlas.legs[t.region]?.name||legacy[t.region];
  el('title').textContent='세계 타일 '+k.replace('_',', ');el('source').href=t.url;
  const [x,y]=coords(k);el('world').textContent='동 X '+Math.round(x)+' ~ '+Math.round(x+w)+' m　북 Y '+Math.round(y)+' ~ '+Math.round(y+h)+' m';
  el('message').textContent='';el('place').value='';el('position').textContent='벨레스 기준 동 X · 북 Y (m)';
  el('port').scrollTo(0,0);el('neighbors').replaceChildren();
  const [i,j]=k.split('_').map(Number);
  for(const [u,v,label] of [[-1,0,'← 서'],[0,1,'↑ 북'],[0,-1,'↓ 남'],[1,0,'→ 동']]){
    const n=[i+u,j+v].join('_');if(!atlas.tiles[n])continue;
    const b=document.createElement('button');b.textContent=label;b.title=n;b.onclick=()=>select(n);el('neighbors').append(b);
  }
  const journey=atlas.routes[el('mode').value];
  routeIndex=index??journey.indexOf(k);
  el('progress').textContent=routeIndex<0?'여정 바깥 타일':(routeIndex+1)+' / '+journey.length+' 순서';
  el('prev').disabled=routeIndex<=0;el('next').disabled=routeIndex>=journey.length-1;
  if(routeIndex<0)el('next').title='여정 시작으로 이동';else el('next').title='다음 경로 타일';
  const url=new URL(location.href);url.searchParams.set('tile',k);url.searchParams.set('mode',el('mode').value);history.replaceState(null,'',url);
  const [sx,sy]=toScreen(x+w/2,y+h/2);
  if(sx<0||sy<0||sx>frame().width||sy>frame().height)center=[x+w/2,y+h/2];draw();
}
function step(delta){const list=atlas.routes[el('mode').value],i=routeIndex+delta;if(list[i])select(list[i],i)}
el('prev').onclick=()=>step(-1);el('next').onclick=()=>step(1);
el('mode').onchange=()=>select(atlas.routes[el('mode').value][0],0);
el('scope').onchange=fit;el('plus').onclick=()=>magnify(1.8);el('minus').onclick=()=>magnify(1/1.8);
el('place').add(new Option('거점으로 이동',''));sites.forEach(([name,p],idx)=>el('place').add(new Option(name,String(idx))));
el('place').onchange=e=>{if(e.target.value!==''){const idx=Number(e.target.value);select(tileAt(sites[idx][1]));el('place').value=String(idx)}};
el('jump').onsubmit=e=>{e.preventDefault();const m=el('address').value.trim().match(/^(-?\d+)[,\s_]+(-?\d+)$/);if(m&&atlas.tiles[m[1]+'_'+m[2]])select(m[1]+'_'+m[2]);else el('message').textContent='연결도에 포함된 세계 타일 번호를 입력해 줘.'};
el('zoom').onchange=e=>art.style.width=e.target.value;
art.onpointermove=e=>{
  const r=art.getBoundingClientRect();if(!art.naturalWidth)return;
  const [x,y]=coords(selected),legacyV=atlas.tiles[selected].region==='벨레스';
  const px=(e.clientX-r.left)*art.naturalWidth/r.width,py=(e.clientY-r.top)*art.naturalHeight/r.height;
  const u=(px-48)/4.25,v=(py-(legacyV?142:86))/4.25;if(u>=0&&u<=w&&v>=0&&v<=h)el('position').textContent='동 X '+(x+u).toFixed(1)+'　북 Y '+(y+h-v).toFixed(1)+' m';
};
let drag=null;
canvas.onpointerdown=e=>{drag=[e.clientX,e.clientY,...center];canvas.setPointerCapture(e.pointerId)};
canvas.onpointermove=e=>{if(drag){center=[drag[2]-(e.clientX-drag[0])/scale,drag[3]+(e.clientY-drag[1])/scale];draw()}};
canvas.onpointerup=e=>{
  if(drag&&Math.hypot(e.clientX-drag[0],e.clientY-drag[1])<4){
    const r=frame(),p=toWorld(e.clientX-r.left,e.clientY-r.top),k=tileAt(p);
    if(atlas.tiles[k])select(k);else el('message').textContent='도시와 강을 따라 채색된 칸을 선택해 줘.';
  }
  drag=null;
};
canvas.onpointercancel=()=>drag=null;
canvas.onwheel=e=>{e.preventDefault();magnify(e.deltaY<0?1.35:1/1.35)};canvas.addEventListener('keydown',e=>{const v={ArrowUp:[0,1],ArrowDown:[0,-1],ArrowLeft:[-1,0],ArrowRight:[1,0]}[e.key];if(v){e.preventDefault();center=[center[0]+v[0]*35/scale,center[1]+v[1]*35/scale];draw()}});
const query=new URLSearchParams(location.search);el('mode').value=query.get('mode')==='road'?'road':'boat';
const firstFerry=atlas.legs.VT.overview.crossing;
const ferryMid=firstFerry[0].map((v,i)=>(v+firstFerry.at(-1)[i])/2);
fit();select(atlas.tiles[query.get('tile')]?query.get('tile'):tileAt(ferryMid));new ResizeObserver(fit).observe(canvas);

