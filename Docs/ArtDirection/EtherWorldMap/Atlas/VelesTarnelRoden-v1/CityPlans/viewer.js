'use strict';
const byId=id=>document.getElementById(id);
const data=JSON.parse(byId('data').textContent);
const plate=byId('plate'), port=byId('port'), paper=byId('paper');
const tiles=byId('tiles'), trip=byId('trip'), stage=byId('stage');
const [w,h]=data.tileSize,[ox,oy]=data.worldTileOrigin,[dx,dy]=data.offset;
let key='', focus='', projection, artSize;
const parameters=new URLSearchParams(location.search);
const localGuide=byId('guide').textContent;
const incidentGuides={
  broken:'B06과 B07 사이의 부식 계단이 무너졌다. 승강기는 아직 작동하고 들것 문은 닫혀 있다. B01 하역과 B02 급수는 계속된다.',
  closed:'공급을 차단하면 승강기가 정지한다. B08에 부상자가 대기하는 동안 보행자는 B07에서 B12를 거쳐 B09로 돌아간다. B08의 좁은 계단으로는 들것을 옮길 수 없다.',
  rescued:'작업자가 B09 쪽에서 문을 열고 주인공이 마지막 작업팔을 멈춘 뒤, 들것은 B08에서 작업뜰 남쪽을 지나 B10의 구조선으로 간다. 보행자는 B11 제방길로 돌아간다.'
};
const tileKeys=Object.keys(data.tiles);
const available=k=>Object.hasOwn(data.tiles,k);
tiles.add(new Option('도시와 근교 전체',''));
tileKeys.forEach(k=>tiles.add(new Option(k.replace('_',', ')+' · '+data.tiles[k],k)));
trip.add(new Option('직접 둘러보기','-1'));
data.trips.forEach((r,i)=>trip.add(new Option(r.name,i)));
const initial=Number(parameters.get('route')??-1);
trip.value=data.trips[initial]?String(initial):'-1';
if(stage) stage.value=['broken','closed','rescued'].includes(parameters.get('state'))?parameters.get('state'):'normal';
byId('ecology').checked=parameters.get('wild')==='1';
function bounds(k){
  const [i,j]=k.split('_').map(Number);
  const x=ox+i*w-dx, y=oy+j*h-dy;
  return [x,y,x+w,y+h];
}
function point(x,y){
  const [left,top,scale]=projection;
  return [(left+x*scale)/artSize[0]*100,(top-y*scale)/artSize[1]*100];
}
function flag(){return '#s-'+(stage?.value||'normal')+'-'+Number(byId('ecology').checked)+'-'+trip.value}
function button(parent,name,action){
  const b=document.createElement('button'); b.type='button'; b.textContent=name;
  b.addEventListener('click',action);parent.append(b);return b;
}
function show(k,place='',write=true){
  key=available(k)?k:''; focus=place; tiles.value=key;
  const file=(key||'overview')+'.svg'+flag();
  plate.src=file;byId('original').href=file;
  const itinerary=data.trips[+trip.value];
  byId('guide').textContent=itinerary?.notes?.[focus]||itinerary?.guide||incidentGuides[stage?.value]||localGuide;
  plate.alt=data.name+' '+(key?'세계 타일 '+key:'전체도');
  const links=byId('hits');links.replaceChildren();byId('neighbors').replaceChildren();
  if(key){
    const [a,b,c,d]=bounds(key);
    artSize=[1180,1344];projection=[48-a*4.25,86+d*4.25,4.25];
    const [i,j]=key.split('_').map(Number);
    for(const [u,v,name] of [[-1,0,'← 서쪽'],[0,1,'↑ 북쪽'],[0,-1,'↓ 남쪽'],[1,0,'→ 동쪽']]){
      const next=[i+u,j+v].join('_');
      if(available(next))button(byId('neighbors'),name,()=>show(next));
    }
  } else {
    const [a,b,c,d]=data.extent,scale=Math.min(1180/(c-a),1090/(d-b));
    artSize=[1260,Math.ceil((d-b)*scale+160)];projection=[(1260-(c-a)*scale)/2-a*scale,78+d*scale,scale];
    for(const tile of tileKeys){
      const [x,y,u,v]=bounds(tile),[left,top]=point(x,v),[right,bottom]=point(u,y);
      const link=document.createElement('a');link.href='?tile='+tile;
      link.setAttribute('aria-label',data.name+' 세계 타일 '+tile);link.title=tile.replace('_',', ')+' · '+data.tiles[tile];
      Object.assign(link.style,{left:left+'%',top:top+'%',width:(right-left)+'%',height:(bottom-top)+'%'});
      link.onclick=e=>{if(!e.ctrlKey&&!e.metaKey){e.preventDefault();show(tile)}};links.append(link);
    }
  }
  for(const house of document.querySelectorAll('#housing [data-house-stop]')){
    const stop=data.stops[house.dataset.houseStop];
    if(key && key!==stop.tile)continue;
    const [west,south,east,north]=house.dataset.bounds.split(' ').map(Number);
    const [left,top]=point(west,north),[right,bottom]=point(east,south);
    const link=document.createElement('a');
    link.href=house.querySelector('a[href*=".html#"]').href;
    link.className='house-hit';
    link.title=stop.name+' · 가구 명부';
    link.setAttribute('aria-label',link.title);
    Object.assign(link.style,{left:left+'%',top:top+'%',width:(right-left)+'%',height:(bottom-top)+'%'});
    links.append(link);
  }
  const mark=byId('pin');mark.hidden=!focus;
  if(focus){
    const p=data.stops[focus].point,[x,y]=point(...p);
    Object.assign(mark.style,{left:x+'%',top:y+'%'});readout(...p);
  }else byId('position').textContent='동 X · 북 Y (m)　'+(key?'세계 타일 '+key.replace('_',', '):'지도 위에서 좌표 확인');
  listStops();
  if(write){
    const url=new URL(location.href);
    url.search=new URLSearchParams({tile:key,route:trip.value,state:stage?.value||'normal',wild:Number(byId('ecology').checked),place:focus});
    history.replaceState(null,'',url);
  }
}
function listStops(){
  const group=byId('stops');group.replaceChildren();
  const order=data.trips[+trip.value]?.stops||Object.keys(data.stops).filter(k=>/^[AB]\d\d$/.test(k));
  for(const k of order){
    const s=data.stops[k];const b=button(group,s.name,()=>show(s.tile,k));
    b.title=k+'　'+s.point.join(', ')+' m';b.setAttribute('aria-current',String(focus===k));
  }
}
function readout(x,y){
  byId('position').textContent='도시 로컬 '+x.toFixed(1)+', '+y.toFixed(1)+' m　·　벨레스 기준 '+(x+dx).toFixed(1)+', '+(y+dy).toFixed(1)+' m';
}
paper.addEventListener('pointermove',e=>{
  const r=plate.getBoundingClientRect();
  readout(((e.clientX-r.left)*artSize[0]/r.width-projection[0])/projection[2],
          (projection[1]-(e.clientY-r.top)*artSize[1]/r.height)/projection[2]);
});
tiles.onchange=()=>show(tiles.value);
byId('overview').onclick=()=>show('');
byId('zoom').onchange=e=>{paper.style.width=e.target.value;if(focus)byId('pin').scrollIntoView({block:'nearest',inline:'nearest'})};
byId('ecology').onchange=()=>show(key,focus);
trip.onchange=()=>{const r=data.trips[+trip.value];if(stage&&r?.state)stage.value=r.state;show('')};
if(stage)stage.onchange=()=>{const r=data.trips[+trip.value];if(r?.state&&r.state!==stage.value)trip.value='-1';show(key,focus)};
const selected=parameters.get('tile'),place=parameters.get('place');
show(selected,data.stops[place]?.tile===selected?place:'',false);

