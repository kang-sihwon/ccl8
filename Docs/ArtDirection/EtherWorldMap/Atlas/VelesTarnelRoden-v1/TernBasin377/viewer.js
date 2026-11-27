"use strict";
const locations=new Map();
function register(id,name,x,y,desc,access,href,level){
 locations.set(id,{id,name,x,y,desc,access,href,level});
}
register('V01','벨레스',0,0,'테른 강의 수리 공방과 출항 나루','북쪽 숲문에서 취수구까지 올라갔다가 마을로 돌아온다.','../02-veles-village.html',1);
register('T01','타르넬',-13000,-20000,'공동 나루와 목재 검수장','동쪽 마당에서 출발해 어살과 쉼나루를 거쳐 높은 운반길로 돌아온다.','../05-tarnel.svg',.42);
register('M02','로덴',-40000,-50000,'공공 나루와 조합 주거단','작업구로 오르는 길과 제방을 따라 돌아오는 길이 나루에서 만난다.','../06-roden-quay.svg',.4);
for(const [id,name,x,y,,desc,access] of sites)register(id,name,x,y,desc,access,'settings.html#'+id,2.4);
for(const [id,name,x,y,,desc,access] of districts)register(id,name,x,y,desc,access,id==='T-YARD'?'../05-tarnel.svg':'../06-roden-quay.svg',1.7);
for(const d of detail.filter(d=>d.kind==='point'||['animal','camp','wildlife','home','feeding'].includes(d.kind))){
 const [x,y]=d.p[0];let href=d.frame==='T'?'../05-tarnel.svg':d.frame==='R'?(/^B0[789]$|^B1[02]$|^R-(GATE|MOOR)/.test(d.id)?'../08-roden-rescue.svg':'../07-roden-approach.svg'):d.id.startsWith('C-')?'../04-cave.svg':'../03-forest.svg';
 if(d.id==='A07'||d.id==='A08')href='../05-tarnel-lower.html';
 if(d.id.startsWith('V-'))href='../02-veles-village.html';
 register(d.id,d.name,x,y,d.frame==='V'?'벨레스 생활권과 탐색 좌표':'기존 탐색도에 기록된 장소',d.id==='B08'?'수동문이 열리면 작업뜰 대피로를 통해 선착장으로 이송한다.':'',href,d.id.startsWith('C-')?4:1.7);
}
for(const [i,r] of regions.entries())register('Z'+i,r[0],r[1],r[2],r[4],r[5],'settings.html#Z'+i,.075);
for(const row of ways){
 let p=row.points[Math.floor(row.points.length*.55)];
 register('line-'+row.id,row.name,...p,row.source,row.kind==='stream'?'하류 합류점까지 이어지는 물길':'권역 사이의 길을 따라 이동한다.','settings.html#line-'+row.id,.45);
}
let selected=null,focus=[0,0],drag=null,clip;
const stageSelect=$('stage');
for(let v=7;v<=377;v+=2){let opt=new Option(v+' × '+v,v);stageSelect.add(opt)}
stageSelect.value=n;
for(const p of locations.values())$('places').append(new Option(p.name,p.name+' · '+p.id));
function extent(){const k=(n-1)/2,[l,b]=tileBounds(-k,-k),[,,r,t]=tileBounds(k,k);return [l,b,r,t]}
function fitScale(){return Math.min((width-20)/(n*tw),(height-20)/(n*th))}
function constrain(){
 clip=extent();let [l,b,r,t]=clip;
 zoom=Math.max(fitScale(),Math.min(7,zoom));
 const dx=width/(2*zoom),dy=height/(2*zoom);
 cx=r-l<dx*2?(r+l)/2:Math.max(l+dx,Math.min(r-dx,cx));
 cy=t-b<dy*2?(b+t)/2:Math.max(b+dy,Math.min(t-dy,cy));
}
function fit(){zoom=fitScale();let e=extent();cx=(e[0]+e[2])/2;cy=(e[1]+e[3])/2;constrain();schedule()}
function project(x,y){return [(x-cx)*zoom+width/2,(cy-y)*zoom+height/2]}
function inverse(u,v){return [cx+(u-width/2)/zoom,cy-(v-height/2)/zoom]}
function label(x,y,name,col,size,occupied){
 let [u,v]=project(x,y);if(u<0||v<0||u>width||v>height)return;
 ctx.font='600 '+size+'px "Malgun Gothic",sans-serif';let w=ctx.measureText(name).width;
 let rect=[u+5,v-size-3,u+w+11,v+5];
 if(occupied.some(r=>!(r[0]>rect[2]||r[2]<rect[0]||r[1]>rect[3]||r[3]<rect[1])))return;
 occupied.push(rect);
 ctx.fillStyle=col;ctx.beginPath();ctx.arc(u,v,2.7,0,Math.PI*2);ctx.fill();
 ctx.lineWidth=3.7;ctx.strokeStyle='#f1ead8';ctx.lineJoin='round';ctx.strokeText(name,u+6,v);ctx.fillText(name,u+6,v);
}
function labels(){
 const occupied=[];
 if(selected)label(selected.x,selected.y,selected.name,'#553f35',14,occupied);
 if(!$('names').checked)return;
 for(const p of locations.values()){
  const major=/^(M02|T01|V0[1-4])$/.test(p.id),extra=/^K\d+$/.test(p.id);
  if(!major&&!extra)continue;
  if(extra&&zoom<.010)continue;
  label(p.x,p.y,p.name,major?'#393c30':'#57533e',major?14:12,occupied);
 }
 if(zoom<.045){
  for(const r of regions)label(r[1],r[2],r[0],'#636850',11,occupied);
 }else if(zoom>.19){
  for(const d of detail){
   if(!visible(...d.p[0])||d.p.length!==1)continue;
   if(d.kind==='point')label(...d.p[0],d.name,'#474934',12,occupied);
   else if($('animals').checked&&['animal','camp','home','feeding','wildlife'].includes(d.kind))label(...d.p[0],d.name,'#793e42',11,occupied);
  }
  for(const p of localPlans)for(const [name,x,y] of p.land)label(x,y,name,'#494a36',11,occupied);
  if($('animals').checked)for(const [id,name,,,dx,dy] of habitats){
   let s=sites.find(k=>k[0]===id);label(s[2]+dx,s[3]+dy,name,'#793e42',12,occupied)
  }
  for(const b of crossings)if(visible(b.x,b.y)&&zoom>.55)label(b.x,b.y,b.mode,'#57513c',11,occupied);
 }
}
function paint(){
 const ratio=Math.min(2,devicePixelRatio||1);constrain();
 const [l,t]=inverse(0,0),[r,b]=inverse(width,height);box=[Math.max(clip[0],l),Math.max(clip[1],b),Math.min(clip[2],r),Math.min(clip[3],t)];
 ctx.setTransform(ratio,0,0,ratio,0,0);ctx.fillStyle='#e5dfce';ctx.fillRect(0,0,width,height);ctx.save();
 ctx.translate(width/2,height/2);ctx.scale(zoom,-zoom);ctx.translate(-cx,-cy);ctx.beginPath();ctx.rect(clip[0],clip[1],n*tw,n*th);ctx.clip();ctx.lineCap='round';ctx.lineJoin='round';
 const relief=picture('relief.webp');if(relief)blit(relief,...geo.limits);
 const k=(n-1)/2,first=tileAt(box[0],box[1]),last=tileAt(box[2],box[3]);
 if(zoom>.027){
  ctx.globalAlpha=Math.min(.90,(zoom-.027)*5);
  const stride=zoom<.12?16:zoom<.62?4:1;
  for(let i=Math.floor(first[0]/stride)*stride;i<=last[0];i+=stride)for(let j=Math.floor(first[1]/stride)*stride;j<=last[1];j+=stride){
   const [x,y]=tileBounds(i,j);blit(ground(i,j,stride),x,y,x+tw*stride+.18/zoom,y+th*stride+.18/zoom);
  }
  ctx.globalAlpha=1;
 }
 vegetation();
 for(const way of ways)if(way.kind==='road'){
  viewLine(way,'#948567',Math.max(way.width+1.5,1.45/zoom));
  viewLine(way,'#e4ce9d',Math.max(way.width,.87/zoom));
 }
 bluewater();
 for(const way of ways)if(way.kind==='stream')streamWater(way);
 drawPlans();showDetail();weirDetail();bridgeSymbols();stamps();overlays();
 if($('mesh').checked){
  const step=zoom*tw<10?10:1;ctx.lineWidth=.65/zoom;ctx.strokeStyle='#545e4d66';ctx.beginPath();
  for(let i=Math.ceil(first[0]/step)*step;i<=last[0]+1;i+=step){let x=tileBounds(i,0)[0];ctx.moveTo(x,box[1]);ctx.lineTo(x,box[3])}
  for(let j=Math.ceil(first[1]/step)*step;j<=last[1]+1;j+=step){let y=tileBounds(0,j)[1];ctx.moveTo(box[0],y);ctx.lineTo(box[2],y)}ctx.stroke();
 }
 ctx.restore();ctx.save();let [u,v]=project(clip[0],clip[3]);ctx.beginPath();ctx.rect(u,v,n*tw*zoom,n*th*zoom);ctx.clip();labels();ctx.restore();
 let span=90/zoom,power=10**Math.floor(Math.log10(span)),unit=[5,2,1].map(x=>x*power).find(x=>x<=span);
 $('scale').innerHTML='<i style="width:'+unit*zoom+'px"></i>'+(unit>=1000?unit/1000+' km':unit+' m');
}
function info(x,y,p=null){
 focus=[x,y];selected=p;const reg=regions[regionAt(x,y)],t=tileAt(x,y);
 $('x').value=Math.round(x);$('y').value=Math.round(y);$('loc').textContent=p?.name||reg[0];
 $('description').textContent=p?.desc||reg[4];$('access').textContent=p?.access||reg[5];
 $('position').textContent='타일 ('+t.join(', ')+') · X '+Math.round(x)+' / Y '+Math.round(y)+' m';
 const old=legacyImages[t.join(',')];let a=$('source');
 a.href=p?.href||(old?(old.id==='center'?'../02-veles-village.svg':'../VelesCardinalTiles/'+old.id+'.svg'):'settings.html#Z'+regionAt(x,y));
 a.textContent=p?.href.startsWith('settings')?'설정 보기':old||p?'연결 도면 열기':'권역 설정 보기';$('msg').textContent='';
}
function goto(x,y,p=null,scale=1.8){
 if(!Number.isFinite(x)||!Number.isFinite(y)||stageAt(x,y)>377){$('msg').textContent='377×377 범위 안의 좌표를 입력해.';return}
 if(stageAt(x,y)>n){n=stageAt(x,y);stageSelect.value=n}
 cx=x;cy=y;zoom=scale;info(x,y,p);constrain();schedule();
}
function zoomAt(f,u=width/2,v=height/2){
 const [x,y]=inverse(u,v);zoom=Math.min(7,Math.max(fitScale(),zoom*f));cx=x-(u-width/2)/zoom;cy=y+(v-height/2)/zoom;constrain();schedule()
}
function resize(){
 let oldFit=width&&zoom<=fitScale()*1.001;let d=canvas.getBoundingClientRect();width=d.width;height=d.height;
 const r=Math.min(2,devicePixelRatio||1);canvas.width=Math.round(width*r);canvas.height=Math.round(height*r);
 if(oldFit)fit();else schedule();
}
stageSelect.onchange=()=>{n=Number(stageSelect.value);if(stageAt(...focus)>n)info(0,0);fit()};
for(const id of ['names','mesh','animals','journey','state'])$(id).onchange=schedule;
$('fit').onclick=fit;$('inc').onclick=()=>zoomAt(1.8);$('dec').onclick=()=>zoomAt(1/1.8);
$('coords').onsubmit=e=>{e.preventDefault();goto(Number($('x').value),Number($('y').value))};
$('search').onsubmit=e=>{
 e.preventDefault();let q=$('place').value.trim(),p=locations.get(q.split(' · ').at(-1))||[...locations.values()].find(p=>p.name===q)||[...locations.values()].find(p=>q&&p.name.includes(q));
 if(p)goto(p.x,p.y,p,p.level);else $('msg').textContent='목록에서 장소 이름이나 번호를 골라 줘.';
};
function mouse(e){let d=canvas.getBoundingClientRect();return [e.clientX-d.left,e.clientY-d.top]}
canvas.onwheel=e=>{e.preventDefault();zoomAt(Math.exp(-Math.sign(e.deltaY)*.28),...mouse(e))};
canvas.onpointerdown=e=>{drag={u:e.clientX,v:e.clientY,x:cx,y:cy,moved:false};canvas.setPointerCapture(e.pointerId)};
canvas.onpointermove=e=>{if(!drag)return;let dx=e.clientX-drag.u,dy=e.clientY-drag.v;if(Math.abs(dx)+Math.abs(dy)>3)drag.moved=true;cx=drag.x-dx/zoom;cy=drag.y+dy/zoom;constrain();schedule()};
canvas.onpointercancel=()=>drag=null;
canvas.onpointerup=e=>{
 if(!drag)return;
 if(!drag.moved){const [x,y]=inverse(...mouse(e));if(stageAt(x,y)<=n){
  let nearest=null,dist=13;for(const p of locations.values()){if(p.id.startsWith('line-')||p.id.startsWith('Z'))continue;let d=Math.hypot(x-p.x,y-p.y)*zoom;if(d<dist){nearest=p;dist=d}}
  if(nearest)info(nearest.x,nearest.y,nearest);else info(x,y);schedule();
 }}drag=null;
};
canvas.ondblclick=e=>{let [x,y]=inverse(...mouse(e));if(stageAt(x,y)<=n)goto(x,y,null,Math.min(width/tw,height/th)*.88)};
canvas.onkeydown=e=>{let step=90/zoom;if(e.key==='+'||e.key==='=')zoomAt(1.8);else if(e.key==='-')zoomAt(1/1.8);else if(e.key.startsWith('Arrow')){cx+=({'ArrowRight':1,'ArrowLeft':-1}[e.key]||0)*step;cy+=({'ArrowUp':1,'ArrowDown':-1}[e.key]||0)*step;constrain();schedule()}else return;e.preventDefault()};
new ResizeObserver(resize).observe(canvas);
resize();fit();info(0,0,locations.get('V01'));
const query=new URLSearchParams(location.search);
if(query.has('id')&&locations.has(query.get('id'))){const p=locations.get(query.get('id'));goto(p.x,p.y,p,p.level)}

