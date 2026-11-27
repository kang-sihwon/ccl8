'use strict';
const el=id=>document.getElementById(id);
const [tw,th]=world.size,[ox,oy]=world.origin,R=world.radius,side=2*R+1;
const valid=(i,j)=>Number.isInteger(i)&&Number.isInteger(j)&&Math.max(Math.abs(i),Math.abs(j))<=R;
const at=(x,y)=>[Math.floor((x-ox)/tw),Math.floor((y-oy)/th)];
const center=(i,j)=>[ox+(i+.5)*tw,oy+(j+.5)*th];
const originalName={VT:'벨레스 → 타르넬 강변',TR:'타르넬 → 로덴 강변'};
const publishedLinks=new Set(world.overprints);
const svgNS='http://www.w3.org/2000/svg';
const map=el('overview'),ctx=map.getContext('2d'),base=new Image();
let selected=[0,0],layout='grid',generation=0;
let view=center(0,0),zoom=1,mw=0,mh=0,scale=0,hold=null;
const regionName=(x,y)=>{
  const u=x+1000*Math.sin(y/6800)+490*Math.cos((x+y)/3600),v=y+850*Math.sin(x/5100);
  let min=Infinity,name='';
  for(const r of world.regions){const d=(u-r[1])**2+(v-r[2])**2;if(d<min){min=d;name=r[0]}}
  return name;
};
const project=(x,y)=>[mw/2+(x-view[0])*scale,mh/2-(y-view[1])*scale];
const inverse=(u,v)=>[view[0]+(u-mw/2)/scale,view[1]-(v-mh/2)/scale];
const clamp=(v,lo,hi)=>Math.max(lo,Math.min(v,hi));
function draw(){
  const r=map.getBoundingClientRect();mw=map.clientWidth;mh=map.clientHeight;
  if(!r.width||!mh)return;
  const ratio=Math.min(devicePixelRatio||1,2);
  if(map.width!==Math.round(mw*ratio)||map.height!==Math.round(mh*ratio)){map.width=Math.round(mw*ratio);map.height=Math.round(mh*ratio)}
  scale=Math.min((mw-14)/(tw*side),(mh-14)/(th*side))*zoom;
  const mid=center(0,0),dx=Math.max(0,(side*tw-mw/scale)*.5),dy=Math.max(0,(side*th-mh/scale)*.5);
  view=[clamp(view[0],mid[0]-dx,mid[0]+dx),clamp(view[1],mid[1]-dy,mid[1]+dy)];
  ctx.setTransform(ratio,0,0,ratio,0,0);ctx.fillStyle='#dedccc';ctx.fillRect(0,0,mw,mh);
  const [left,top]=project(ox-R*tw,oy+(R+1)*th),w=side*tw*scale,h=side*th*scale;
  if(base.complete&&base.naturalWidth)ctx.drawImage(base,left,top,w,h);
  ctx.save();ctx.beginPath();ctx.rect(left,top,w,h);ctx.clip();
  const step=tw*scale>11?1:tw*scale>2.5?10:50;
  ctx.beginPath();ctx.strokeStyle='#5d665355';ctx.lineWidth=.6;
  for(let k=Math.ceil(-R/step)*step;k<=R+1;k+=step){
    const [u,v]=project(ox+k*tw,oy+k*th);ctx.moveTo(u,top);ctx.lineTo(u,top+h);ctx.moveTo(left,v);ctx.lineTo(left+w,v);
  }
  ctx.stroke();ctx.font='600 12px Malgun Gothic';ctx.lineJoin='round';
  const tags=[];
  for(const [name,x,y] of world.places.slice(0,zoom>6?16:6)){
    const [u,v]=project(x,y);if(u<0||v<0||u>mw||v>mh)continue;
    const len=ctx.measureText(name).width,tx=clamp(u+5,left+2,Math.min(mw,left+w)-len-3),ty=v<18?v+15:v-5;
    if(tags.some(([a,b,c,d])=>a<tx+len&&tx<c&&b<ty+3&&ty-12<d))continue;
    tags.push([tx,ty-12,tx+len,ty+3]);ctx.fillStyle='#485647';ctx.fillRect(u-2,v-2,4,4);
    ctx.strokeStyle='#f3eddb';ctx.lineWidth=3;ctx.strokeText(name,tx,ty);ctx.fillText(name,tx,ty);
  }
  const [x,y]=project(...center(...selected)),sw=Math.max(6,tw*scale),sh=Math.max(6,th*scale);
  ctx.strokeStyle='#914930';ctx.lineWidth=2;ctx.strokeRect(x-sw/2,y-sh/2,sw,sh);ctx.restore();
}
function magnify(f,px=mw/2,py=mh/2){
  const before=inverse(px,py);zoom=clamp(zoom*f,1,160);draw();
  const after=inverse(px,py);view[0]+=before[0]-after[0];view[1]+=before[1]-after[1];draw();
}
function frameImage(i,j,data){
  const div=document.createElement('button'),k=i+'_'+j,src=world.originals[k];
  const box=src?(src.region==='벨레스'?'48 142 1084 1160':'48 86 1084 1160'):'22 26 '+tw+' '+th;
  const svg=document.createElementNS(svgNS,'svg');svg.setAttribute('viewBox',box);
  const image=document.createElementNS(svgNS,'image');image.setAttribute('href',data.url);
  image.setAttribute('width',data.w);image.setAttribute('height',data.h);svg.append(image);
  const text=document.createElement('span');text.className='caption';text.textContent=i+', '+j;
  div.title='('+i+', '+j+')';div.setAttribute('aria-label',div.title+' 열기');div.append(svg,text);
  if(i===selected[0]&&j===selected[1])div.className='current';
  div.onclick=()=>{if(i===selected[0]&&j===selected[1])layout='sheet';selectTile(i,j)};
  return div;
}
async function selectTile(i,j,keepView=false){
  if(!valid(i,j)){el('status').textContent='동 X와 북 Y를 −185부터 +185 사이로 입력해 줘.';return}
  const version=++generation,k=i+'_'+j,src=world.originals[k];
  selected=[i,j];el('ix').value=i;el('iy').value=j;
  const [x,y]=center(i,j),zone=regionName(x,y);
  const place=world.places.find(p=>at(p[1],p[2]).join('_')===k);
  el('places').value=place?String(world.places.indexOf(place)):'';
  el('zone').textContent=zone;
  el('title').textContent=(place?place[0]+'　':'세계 타일　')+'('+i+', '+j+')';
  const source=el('source');source.replaceChildren();
  if(src){
    source.append((originalName[src.region]||src.region)+' 상세도　');
    const a=document.createElement('a');a.href='../TernPassage/'+src.url;a.textContent='기존 원도 열기';source.append(a);
    if(publishedLinks.has(k))source.append('　·　권역 운반로 겹침');
  }else source.textContent='유역 지형·생활권 평면도';
  el('metres').textContent='서단 X '+(x-tw/2).toFixed(1)+' m　동단 X '+(x+tw/2).toFixed(1)+' m　남단 Y '+(y-th/2).toFixed(1)+' m　북단 Y '+(y+th/2).toFixed(1)+' m';
  el('single').setAttribute('aria-pressed',layout==='sheet');el('mosaic').setAttribute('aria-pressed',layout==='grid');
  for(const button of el('neighbors').children){const [u,v]=button.dataset.step.split(',').map(Number);button.disabled=!valid(i+u,j+v)}
  const [u,v]=project(x,y);
  if(!keepView&&zoom>1&&(u<0||u>mw||v<0||v>mh))view=[x,y];
  draw();
  const hash='#'+i+','+j+(layout==='sheet'?'/sheet':'');
  if(location.hash!==hash)history.replaceState(null,'',hash);
  const display=el('display');display.setAttribute('aria-busy','true');el('status').textContent='도면을 여는 중…';
  el('save').removeAttribute('href');
  try{
    const plate=await loadDrawing(i,j);if(version!==generation)return;
    const nodes=[];
    if(layout==='sheet'){
      const im=new Image();im.src=plate.url;im.alt=el('title').textContent+' 좌표 평면도';await im.decode();nodes.push(im);
    }else{
      const tasks=[];
      for(let b=j+1;b>=j-1;b--)for(let a=i-1;a<=i+1;a++){
        if(valid(a,b))tasks.push(loadDrawing(a,b).then(d=>frameImage(a,b,d)));
        else {const e=document.createElement('span');e.className='beyond';e.textContent='도면 경계';tasks.push(e)}
      }
      nodes.push(...await Promise.all(tasks));
    }
    if(version!==generation)return;
    display.className=layout;display.replaceChildren(...nodes);
    el('save').href=plate.url;el('save').download=k+'.svg';el('status').textContent='';
  }catch(e){if(version===generation){display.replaceChildren();el('status').textContent=e.message}}
  finally{if(version===generation)display.setAttribute('aria-busy','false')}
}
base.onload=draw;base.src='overview.svg';new ResizeObserver(draw).observe(map);
el('places').add(new Option('장소 선택',''));
world.places.forEach(([n],i)=>el('places').add(new Option(n,String(i))));
el('places').onchange=e=>{const p=world.places[Number(e.target.value)];if(e.target.value!=='')selectTile(...at(p[1],p[2]))};
el('jump').onsubmit=e=>{e.preventDefault();selectTile(el('ix').valueAsNumber,el('iy').valueAsNumber)};
el('single').onclick=()=>{layout='sheet';selectTile(...selected)};
el('mosaic').onclick=()=>{layout='grid';selectTile(...selected)};
for(const b of el('neighbors').children)b.onclick=()=>{const [u,v]=b.dataset.step.split(',').map(Number);selectTile(selected[0]+u,selected[1]+v)};
el('pack').onclick=async()=>{
  const id=blockId(...selected),block=await getBlock(id),url=URL.createObjectURL(new Blob([block.raw],{type:'application/zip'}));
  const a=document.createElement('a');a.href=url;a.download='Veles371-'+id+'.zip';a.click();setTimeout(()=>URL.revokeObjectURL(url),60000);
};
el('in').onclick=()=>magnify(1.9);el('out').onclick=()=>magnify(1/1.9);el('fit').onclick=()=>{zoom=1;view=center(0,0);draw()};
const mouse=e=>{const r=map.getBoundingClientRect();return [e.clientX-r.left-map.clientLeft,e.clientY-r.top-map.clientTop]};
map.onwheel=e=>{e.preventDefault();magnify(e.deltaY<0?1.5:1/1.5,...mouse(e))};
map.onpointerdown=e=>{hold={p:mouse(e),view:[...view],drag:false};map.setPointerCapture(e.pointerId)};
map.onpointermove=e=>{if(!hold)return;const [u,v]=mouse(e),dx=u-hold.p[0],dy=v-hold.p[1];if(Math.abs(dx)+Math.abs(dy)>4)hold.drag=true;if(hold.drag){view=[hold.view[0]-dx/scale,hold.view[1]+dy/scale];draw()}};
map.onpointerup=e=>{if(hold&&!hold.drag){const ij=at(...inverse(...mouse(e)));if(valid(...ij))selectTile(...ij,true)}hold=null};
map.onpointercancel=()=>{hold=null};map.onkeydown=e=>{const step={ArrowLeft:[-1,0],ArrowRight:[1,0],ArrowUp:[0,1],ArrowDown:[0,-1]}[e.key];if(step){e.preventDefault();selectTile(selected[0]+step[0],selected[1]+step[1])}};
function restore(){
  const match=location.hash.match(/^#(-?\d+),(-?\d+)(\/sheet)?$/);
  if(match&&valid(+match[1],+match[2])){layout=match[3]?'sheet':'grid';selectTile(+match[1],+match[2])}
  else selectTile(0,0);
}
window.addEventListener('hashchange',restore);draw();restore();

