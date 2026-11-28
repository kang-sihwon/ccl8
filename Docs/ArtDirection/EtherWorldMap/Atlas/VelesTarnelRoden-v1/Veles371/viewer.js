'use strict';
const el=id=>document.getElementById(id);
const originalBase=new URL(document.currentScript.dataset.originalBase||'../TernPassage/',document.baseURI);
const [tw,th]=world.size,[ox,oy]=world.origin,R=world.radius,side=2*R+1;
const valid=(i,j)=>Number.isInteger(i)&&Number.isInteger(j)&&Math.max(Math.abs(i),Math.abs(j))<=R;
const at=(x,y)=>[Math.floor((x-ox)/tw),Math.floor((y-oy)/th)];
const center=(i,j)=>[ox+(i+.5)*tw,oy+(j+.5)*th];
const originalName={VT:'벨레스 → 타르넬 강변',TR:'타르넬 → 로덴 강변'};
const publishedLinks=new Set(world.overprints);
const svgNS='http://www.w3.org/2000/svg';
const map=el('overview'),ctx=map.getContext('2d'),base=new Image();
const sizeSelect=el('grid-size');
for(let n=1;n<=side;n+=2)sizeSelect.add(new Option(n===1?'한 장':n+'×'+n,String(n)));
let selected=[0,0],gridSize=3,generation=0;
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
  div.onclick=()=>{if(i===selected[0]&&j===selected[1])gridSize=1;selectTile(i,j)};
  return div;
}
async function wideGrid(i,j,n,version){
  const radius=(n-1)/2;
  const west=clamp(i-radius,-R,R-n+1),north=clamp(j+radius,-R+n-1,R);
  const canvas=document.createElement('canvas');
  canvas.width=1200;canvas.height=Math.round(canvas.width*th/tw);
  canvas.tabIndex=0;
  canvas.setAttribute('aria-label',n+'×'+n+' 타일. 방향키로 이동하고 Enter로 선택한 타일을 열어.');
  const brush=canvas.getContext('2d'),w=canvas.width/n,h=canvas.height/n;
  const images=[];
  if(n>63){
    images.push({file:'world',x:-R,y:R,cols:side,rows:side});
  }else{
    const block=world.block;
    for(let by=Math.floor((north-n+1+R)/block);by<=Math.floor((north+R)/block);by++){
      for(let bx=Math.floor((west+R)/block);bx<=Math.floor((west+n-1+R)/block);bx++){
        const x=bx*block-R,south=by*block-R;
        const cols=Math.min(block,R-x+1),rows=Math.min(block,R-south+1);
        images.push({file:bx+'_'+by,x,y:south+rows-1,cols,rows});
      }
    }
  }
  async function paint(){
    while(images.length&&version===generation){
      const tile=images.pop(),im=new Image();
      im.src='previews/'+tile.file+'.webp';
      await im.decode();
      if(version!==generation)return;
      const left=Math.round((tile.x-west)*w),top=Math.round((north-tile.y)*h);
      const right=Math.round((tile.x+tile.cols-west)*w),bottom=Math.round((north-tile.y+tile.rows)*h);
      brush.drawImage(im,left,top,right-left,bottom-top);
    }
  }
  await Promise.all(Array.from({length:Math.min(images.length,4)},paint));
  if(version!==generation)return;
  brush.beginPath();
  for(let k=1;k<n;k++){
    brush.moveTo(k*w,0);brush.lineTo(k*w,canvas.height);
    brush.moveTo(0,k*h);brush.lineTo(canvas.width,k*h);
  }
  brush.lineWidth=Math.min(1,w*.06);brush.strokeStyle='#46523b55';brush.stroke();
  brush.lineWidth=Math.min(3,w*.28);brush.strokeStyle='#914930';
  brush.strokeRect((i-west)*w,(north-j)*h,w,h);
  if(n>63){
    const cx=(i-west+.5)*w,cy=(north-j+.5)*h;
    brush.beginPath();brush.lineWidth=2;
    brush.moveTo(cx-w/2-12,cy);brush.lineTo(cx-w/2,cy);
    brush.moveTo(cx+w/2,cy);brush.lineTo(cx+w/2+12,cy);
    brush.moveTo(cx,cy-h/2-12);brush.lineTo(cx,cy-h/2);
    brush.moveTo(cx,cy+h/2);brush.lineTo(cx,cy+h/2+12);
    brush.stroke();
  }
  const tileAt=event=>{
    const rect=canvas.getBoundingClientRect();
    return [west+clamp(Math.floor((event.clientX-rect.left)*n/rect.width),0,n-1),
      north-clamp(Math.floor((event.clientY-rect.top)*n/rect.height),0,n-1)];
  };
  canvas.onpointermove=event=>{canvas.title='('+tileAt(event).join(', ')+')'};
  canvas.onclick=event=>{
    const [x,y]=tileAt(event);
    if(x===i&&y===j)gridSize=1;
    selectTile(x,y);
  };
  canvas.onkeydown=event=>{
    if(event.key==='Enter'){event.preventDefault();gridSize=1;selectTile(i,j)}
    else map.onkeydown(event);
  };
  return canvas;
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
    const a=document.createElement('a');a.href=new URL(src.url,originalBase);a.textContent='기존 원도 열기';source.append(a);
    if(publishedLinks.has(k))source.append('　·　권역 운반로 겹침');
  }else source.textContent='유역 지형·생활권 평면도';
  el('metres').textContent='서단 X '+(x-tw/2).toFixed(1)+' m　동단 X '+(x+tw/2).toFixed(1)+' m　남단 Y '+(y-th/2).toFixed(1)+' m　북단 Y '+(y+th/2).toFixed(1)+' m';
  sizeSelect.value=String(gridSize);
  for(const button of el('neighbors').children){const [u,v]=button.dataset.step.split(',').map(Number);button.disabled=!valid(i+u,j+v)}
  const [u,v]=project(x,y);
  if(!keepView&&zoom>1&&(u<0||u>mw||v<0||v>mh))view=[x,y];
  draw();
  const suffix=gridSize===1?'/sheet':gridSize===3?'':'/'+gridSize+'x'+gridSize;
  const hash='#'+i+','+j+suffix;
  if(location.hash!==hash)history.replaceState(null,'',hash);
  const display=el('display');display.setAttribute('aria-busy','true');el('status').textContent='도면을 여는 중…';
  const save=el('save');
  save.removeAttribute('href');save.onclick=null;
  const radius=(gridSize-1)/2;
  visibleDrawings.clear();
  visibleDrawings.add(k);
  try{
    if(gridSize>7){
      const canvas=await wideGrid(i,j,gridSize,version);
      if(version!==generation)return;
      const keepFocus=display.contains(document.activeElement);
      display.className='wide';display.replaceChildren(canvas);
      if(keepFocus)canvas.focus({preventScroll:true});
      save.href='#';save.download=k+'.svg';
      save.onclick=async event=>{
        event.preventDefault();
        try{
          const plate=await loadDrawing(i,j);
          const link=document.createElement('a');link.href=plate.url;link.download=k+'.svg';link.click();
        }catch(error){if(version===generation)el('status').textContent=error.message}
      };
      el('status').textContent='';
      return;
    }
    const plate=await loadDrawing(i,j);if(version!==generation)return;
    const nodes=[];
    if(gridSize===1){
      const im=new Image();im.src=plate.url;im.alt=el('title').textContent+' 좌표 평면도';await im.decode();nodes.push(im);
    }else{
      const tasks=[];
      for(let b=j+radius;b>=j-radius;b--)for(let a=i-radius;a<=i+radius;a++){
        if(valid(a,b)){
          visibleDrawings.add(a+'_'+b);
          tasks.push(loadDrawing(a,b).then(d=>frameImage(a,b,d)));
        }
        else {const e=document.createElement('span');e.className='beyond';e.textContent='도면 경계';tasks.push(e)}
      }
      nodes.push(...await Promise.all(tasks));
    }
    if(version!==generation)return;
    display.className=gridSize===1?'sheet':'grid';
    display.style.setProperty('--grid-size',gridSize);
    display.replaceChildren(...nodes);
    el('save').href=plate.url;el('save').download=k+'.svg';el('status').textContent='';
  }catch(e){if(version===generation){display.replaceChildren();el('status').textContent=e.message}}
  finally{if(version===generation)display.setAttribute('aria-busy','false')}
}
base.onload=draw;base.src='overview.svg';new ResizeObserver(draw).observe(map);
el('places').add(new Option('장소 선택',''));
world.places.forEach(([n],i)=>el('places').add(new Option(n,String(i))));
el('places').onchange=e=>{const p=world.places[Number(e.target.value)];if(e.target.value!=='')selectTile(...at(p[1],p[2]))};
el('jump').onsubmit=e=>{e.preventDefault();selectTile(el('ix').valueAsNumber,el('iy').valueAsNumber)};
sizeSelect.onchange=()=>{
  gridSize=Number(sizeSelect.value);
  selectTile(...selected);
};
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
  const match=location.hash.match(/^#(-?\d+),(-?\d+)(?:\/(sheet|(\d{1,3})x\4))?$/);
  const n=match?.[3]==='sheet'?1:Number(match?.[4]||3);
  gridSize=n>=1&&n<=side&&n%2===1?n:3;
  if(match&&valid(+match[1],+match[2]))selectTile(+match[1],+match[2]);
  else selectTile(0,0);
}
window.addEventListener('hashchange',restore);draw();restore();

