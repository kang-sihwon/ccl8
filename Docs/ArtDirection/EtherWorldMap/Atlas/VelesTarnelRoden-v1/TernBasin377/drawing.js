"use strict";
const canvas=document.getElementById("map"),ctx=canvas.getContext("2d");
let cx=0,cy=0,zoom=.01,n=377,width=800,height=600,box,queued=false;
const $=id=>document.getElementById(id), [tw,th]=geo.size;
const images=new Map();
function picture(url){
 if(!images.has(url)){
  const im=new Image();im.onload=schedule;im.src=url;images.set(url,im);
 }
 const im=images.get(url);return im.complete&&im.naturalWidth?im:null;
}
function schedule(){if(!queued){queued=true;requestAnimationFrame(()=>{queued=false;paint()})}}
function stroke(row,color,w,fill=false){
 if(!row.length)return;
 ctx.beginPath();ctx.moveTo(...row[0]);for(let j=1;j<row.length;j++)ctx.lineTo(...row[j]);
 if(fill){ctx.closePath();ctx.fillStyle=color;ctx.fill()}else{ctx.strokeStyle=color;ctx.lineWidth=w;ctx.stroke()}
}
function oval(x,y,a,b,col,angle=0){
 ctx.beginPath();ctx.ellipse(x,y,Math.max(.01,a),Math.max(.01,b),angle,0,2*Math.PI);ctx.fillStyle=col;ctx.fill();
}
function rect(x,y,w,h,a,col){ctx.save();ctx.translate(x,y);ctx.rotate(a);ctx.fillStyle=col;ctx.fillRect(-w/2,-h/2,w,h);ctx.restore()}
function visible(x,y,margin=0){return x>=box[0]-margin&&x<=box[2]+margin&&y>=box[1]-margin&&y<=box[3]+margin}
function overlaps(q,m=0){return q[2]+m>=box[0]&&q[0]-m<=box[2]&&q[3]+m>=box[1]&&q[1]-m<=box[3]}
const detail=geo.details.map(d=>({...d,b:bounds(d.p)}));
function viewLine(line,color,w){for(const c of line.chunks)if(overlaps(c.b,100))stroke(zoom<.08?c.far:c.ps,color,w)}
function streamWater(way){
 if(zoom<.17||way.id==='creek'){viewLine(way,'#a5a384',Math.max(way.width+1.5,1.7/zoom));viewLine(way,'#80a7a0',Math.max(way.width,1.1/zoom));return}
 for(let i=0;i<way.chunks.length;i++){
  if(!overlaps(way.chunks[i].b,80))continue;
  let pairs=way.banks.slice(i*90,i*90+91),a=pairs.map(p=>p[0]),b=pairs.map(p=>p[1]);
  stroke(a.concat(b.slice().reverse()),'#82a59f',0,true);stroke(a,'#c8bc95',1.6);stroke(b,'#b4b18a',1.6);
 }
}
function lowerIndex(y){
 let lo=0,hi=geo.water.length-1;
 while(lo<hi){let m=(lo+hi)>>1;if(geo.water[m][0]<y)lo=m+1;else hi=m}return lo;
}
function noise(x,y,s){
 const u=x/s,v=y/s,i=Math.floor(u),j=Math.floor(v);
 let a=u-i,b=v-j;a=a*a*(3-2*a);b=b*b*(3-2*b);
 return (1-b)*((1-a)*hash(i,j)+a*hash(i+1,j))+b*((1-a)*hash(i,j+1)+a*hash(i+1,j+1));
}
function landUse(x,y){
 let v=noise(x,y,220)*.51+noise(x,y,830)*.33+noise(x,y,57)*.16;
 const id=regionAt(x,y);
 if(id===3)return v-.21;
 if(id===5||id===11||id===8)return v-.10;
 const d=Math.max(Math.abs(x-3.53)-2.5*tw,Math.abs(y-94.53)-2.5*th);
 return v+.07+.30*Math.max(0,1-Math.max(0,d)/150);
}
const groundCache=new Map();
function ground(i,j,stride){
 const key=[i,j,stride].join(',');if(groundCache.has(key))return groundCache.get(key);
 const c=document.createElement('canvas'),sz=stride===1?96:64;c.width=c.height=sz;
 const g=c.getContext('2d'),im=g.createImageData(sz,sz),[l,b]=tileBounds(i,j),t=b+stride*th;
 for(let v=0;v<sz;v++)for(let u=0;u<sz;u++){
  let x=l+(u+.5)*tw*stride/sz,y=t-(v+.5)*th*stride/sz;
  let val=landUse(x,y),w=Math.max(0,Math.min(1,(val-.4)*4));
  let relief=(noise(x,y,88)-.5)*27+(noise(x,y,14)-.5)*15+6*(hash(Math.floor(x*.36),Math.floor(y*.36))-.5)+3*Math.sin(x/6.3+y/8+noise(x,y,65)*6);
  let k=4*(v*sz+u);
  im.data[k]=188-53*w+relief;im.data[k+1]=185-42*w+relief;im.data[k+2]=143-38*w+relief;im.data[k+3]=255;
 }
 g.putImageData(im,0,0);groundCache.set(key,c);
 if(groundCache.size>100)groundCache.delete(groundCache.keys().next().value);
 return c;
}
function blit(im,l,b,r,t){
 ctx.save();ctx.translate(l,t);ctx.scale(1,-1);ctx.drawImage(im,0,0,r-l,t-b);ctx.restore()
}
function vegetation(){
 if(zoom<.34)return;
 // 수관 중심은 월드 격자에서 뽑아 타일 가장자리에서 끊기지 않는다.
 const spacing=zoom<.65?16:7.8;
 const crowns=picture('crowns.svg');
 for(let i=Math.floor((box[0]-20)/spacing);i<=Math.ceil((box[2]+20)/spacing);i++)
 for(let j=Math.floor((box[1]-20)/spacing);j<=Math.ceil((box[3]+20)/spacing);j++){
  let x=(i+hash(i,j)*.85)*spacing,y=(j+hash(j,29+i)*.85)*spacing;
  if(localPlans.some(p=>Math.abs(p.site[2]-x)<178&&Math.abs(p.site[3]-y)<140&&(
   p.lanes.some(l=>l.some((a,k)=>k&&segDistance(x,y,l[k-1],a)<8))||
   p.homes.some(a=>Math.hypot(a[0]-x,a[1]-y)<14)||
   p.land.some(a=>Math.hypot(a[1]-x,a[2]-y)<a[3]+7)||
   p.fields.some(([a,b,w,h])=>Math.abs(x-a)<w/2+4&&Math.abs(y-b)<h/2+4))))continue;
  if(detail.some(d=>['building','facility','rock'].includes(d.kind)&&x>=d.b[0]-4&&x<=d.b[2]+4&&y>=d.b[1]-4&&y<=d.b[3]+4))continue;
  const use=landUse(x,y),seed=hash(j,i);
  const district=regionAt(x,y);
  if((district===10||district===12)&&use<.57&&seed>.31){
   for(let k=0;k<5;k++){let dx=hash(i+k,j)*4-2,dy=hash(j+k,i)*3;stroke([[x+dx,y+dy],[x+dx+1.2,y+dy+2.1+seed]],k%2?'#a7a072':'#8d9069',.37)}
   continue;
  }
  if((district===11||district===8)&&seed>.64){
   let a=2+seed*2;stroke([[x-a,y],[x-.5,y+a*.56],[x+a,y+.4],[x+a*.6,y-a*.5]],'#9b9880',0,true);
   stroke([[x-a+.4,y+.2],[x-.4,y+a*.56],[x+a-.4,y+.5]],'#c0bba0',.6);continue;
  }
  if(use<.40||seed>(use-.35)*2.5){if(seed>.96)oval(x,y,3.1,1.5,'#96947a',.4);continue}
  const r=(3.4+seed*3.7)*(spacing/7.8);
  if(crowns){
   ctx.save();ctx.translate(x,y);ctx.scale(1,-1);ctx.rotate((hash(i+12,j)-.5)*.84);
   ctx.drawImage(crowns,64*Math.floor(hash(i+7,j)*12),0,64,64,-r*1.5,-r*1.5,r*3,r*3);ctx.restore();continue;
  }
  oval(x+1.4,y-2,r*1.08,r*.85,'#73785b');
  oval(x,y,r,r*.90,seed>.5?'#80895f':'#879064');
  for(let k=0;k<3;k++){let a=(k+.6)*2;oval(x+Math.cos(a)*r*.40,y+Math.sin(a)*r*.36,r*.56,r*.46,k===1?'#a0a675':'#909869')}
 }
}
function roof(x,y,w,h,a,color){
 rect(x+1,y-1,w+1.7,h+1.6,a,'#756c53');rect(x,y,w,h,a,color);
 ctx.save();ctx.translate(x,y);ctx.rotate(a);ctx.fillStyle='#66513955';ctx.fillRect(-w/2,-h/2,w,h*.48);
 stroke([[-w*.5,0],[w*.5,0]],'#d4b38a',.5);
 if(zoom>.8){
  for(let j=-h/2+.8;j<h/2;j+=1.1){
   stroke([[-w/2,j],[w/2,j]],'#6d584964',.22);
   for(let k=-w/2+1+(Math.floor(j)%2)*.45;k<w/2;k+=1.45)stroke([[k,j],[k+.12,j+.7]],'#66513c65',.18);
  }
  for(const sign of [-1,1]){stroke([[sign*(w*.5-1.3),0],[sign*w*.5,h*.5]],'#66513c',.28);stroke([[sign*(w*.5-1.3),0],[sign*w*.5,-h*.5]],'#66513c',.28)}
  rect(w*.28,h*.21,1.3,1.1,0,'#665a47');
 }
 ctx.restore();
}
function bluewater(){
 const a=Math.max(0,lowerIndex(box[1]-320)-1),b=Math.min(geo.water.length,lowerIndex(box[3]+320)+2);
 if(b-a<2)return;
 const rows=geo.water.slice(a,b),left=rows.map(([y,l])=>[l,y]),right=rows.map(([y,,r])=>[r,y]);
 const joined=left.concat(right.slice().reverse());
 stroke(joined,'#739b97',0,true);
 let bankW=Math.max(4,1.1/zoom);
 for(let bank of [left,right]){
  stroke(bank,'#8e9268',bankW);
  if(zoom>.13)stroke(bank,'#dac8a2',bankW*.68);
 }
 if(zoom<.02)stroke(rows.map(([y,l,r])=>[(l+r)/2,y]),'#578486',1.75/zoom);
 if(zoom>.15)for(let k=1;k<=5;k++){
  let frac=k/6;stroke(rows.map(([y,l,r])=>[l+(r-l)*(frac+.02*Math.sin(y/64+k)),y]),k%2?'#86a6a0':'#72958e',Math.min(2,.8/zoom));
 }
}
function drawPlans(){
 for(const p of localPlans){
  if(!visible(p.site[2],p.site[3],215)||zoom<.11)continue;
  const [, ,x,y,type]=p.site;
  for(const [u,v,a,b,ang] of p.fields){
   rect(u,v,a,b,ang,'#a7a075');ctx.save();ctx.translate(u,v);ctx.rotate(ang);
   for(let j=-b/2+1;j<b/2;j+=3.5){
    stroke([[-a/2+1,j],[a/2-1,j+.5]],'#c4b083',2.1);
    for(let m=-a/2+2;m<a/2-1;m+=1.8){let off=hash(Math.ceil(m+u),Math.ceil(j+v));stroke([[m,j+.1],[m+.4+off,j+.6+off]],off>.46?'#9f9b66':'#ded09a',.46)}
   }ctx.restore();
  }
  for(const lane of p.lanes){stroke(lane,'#a29570',5.4);stroke(lane,'#dcca9e',4.1)}
  for(const [hx,hy,w,h,a,ax,ay] of p.homes){
   rect(hx,hy,w+3,h+3.8,a,'#b5a883');stroke([[hx,hy],[ax,ay]],'#d2c19c',1.6);
   roof(hx,hy,w,h,a,hash(Math.floor(hx),Math.floor(hy))>.5?'#9d7556':'#ae8d67');
  }
  for(const [name,u,v,r,k] of p.land){
   if(k==='tree'){const im=picture('crowns.svg');if(im){ctx.save();ctx.translate(u,v);ctx.scale(1,-1);ctx.drawImage(im,64*6,0,64,64,-r*1.55,-r*1.55,r*3.1,r*3.1);ctx.restore()}}
   else if(k==='shed')roof(u,v,r*1.8,r*.9,.12,'#9f795c');
   else if(k==='logs'){for(let j=-3;j<=3;j++)rect(u,v+j*2,r*1.6,1.6,.14,'#8f7955')}
   else if(k==='well'||k==='kiln'){oval(u,v,r,r*.87,'#89785e');oval(u,v+.3,r*.72,r*.61,k==='well'?'#6c8982':'#b88b68');oval(u,v,r*.33,r*.32,'#655942')}
   else if(k==='mouth'||k==='chamber'){oval(u,v,r*1.3,r*.8,'#948e77');oval(u,v,r*.8,r*.46,'#635e50')}
   else if(k==='rack'||k==='pipe'){for(let j=-2;j<=2;j++)rect(u+j*r*.35,v,r*.22,r*1.6,.14,k==='pipe'?'#9e7559':'#8f805f')}
   else if(k==='wheel'){oval(u,v,r,r,'#6f624c');for(let j=0;j<5;j++)rect(u,v,r*1.75,.66,j*.63,'#b9a278')}
   else if(k==='post'){rect(u,v,1.7,r*2,.28,'#72634f');stroke([[u-2,v],[u+2,v+.4]],'#b39e76',.8)}
   else {oval(u,v,r,r*.58,'#8d8973');oval(u-1,v+1,r*.72,r*.38,'#b8b296')}
  }
 }
}
const physicalRoads=new Set(['walk','back','detour','retreat','haul']);
function showDetail(){
 if(zoom>.11){
  for(const [id,anchor] of [['T-YARD',[-12622,-19894]],['R-HOMES',[-40628,-49594]]]){
   const p=localPlans.find(q=>q.site[0]===id),end=id==='T-YARD'?p.lanes[0][0]:p.lanes[0].at(-1);
   if(visible(...anchor,350))stroke(curve([anchor,[(anchor[0]+end[0])/2-12,(anchor[1]+end[1])/2],end]),'#d7c59b',3.8);
  }
 }
 for(const d of detail){
  if(!overlaps(d.b,30)||d.p.length<2)continue;
  if(d.frame==='V'&&d.id.startsWith('c-')&&!$('journey').checked)continue;
  if(zoom>.14 && ['building','facility','wood','rock','water'].includes(d.kind)){
   let color={wood:'#8e9a76',rock:'#989781',water:'#81a6a0',building:'#9b775b',facility:'#baa786'}[d.kind];
   stroke(d.p,color,0,true);if(d.kind!=='wood')stroke([...d.p,d.p[0]],'#766e58',.65);
  }
  if(d.kind==='stream'&&zoom>.09)stroke(d.p,'#79a39b',3.2);
  if(d.frame!=='V'&&physicalRoads.has(d.kind)&&zoom>.10){
   if(['stairs','foot','doorpath','embark','bypass','exit','r-stretcher','r-clear'].includes(d.id))continue;
   stroke(d.p,'#b1a27d',4.8);stroke(d.p,'#dac9a0',3.7)
  }
 }
}
const approach=curve([[-12500,-21030],[-12740,-21003],[-12920,-21048],[-13650+1030*.612,-21018-132*.612]]);
const upperReturn=curve([[-13650+1025*.612,-21018-694*.612],[-12760,-21444],[-12580,-21416],[-12280,-21420]]);
function weirDetail(trace=false){
 if(!overlaps([-13700,-21660,-12200,-20900])||zoom<.05)return;
 const color=trace?'#776198':'#d9c59a';
 for(const row of [approach,upperReturn])stroke(row,color,trace?2.1:4.2);
 ctx.save();ctx.translate(-13650,-21018);ctx.scale(.612,-.612);
 for(const p of geo.weirMarks){
  if(trace&&!['#deca9a'].includes(p.stroke))continue;
  const path=new Path2D(p.d);
  if(p.fill!=='none'){ctx.fillStyle=p.fill;ctx.fill(path)}
  if(p.stroke!=='none'){ctx.strokeStyle=trace?color:p.stroke;ctx.lineWidth=trace?3:Number(p['stroke-width']);ctx.stroke(path)}
 }
 ctx.restore();
}
const legacyImages=Object.fromEntries(Object.entries(geo.grid).map(([id,t])=>[t.offset.join(','),{id,...t}]));
function stamps(){
 let old=tileBounds(-2,-2),top=tileBounds(2,2),rect=[old[0],old[1],top[2],top[3]];
 if(!overlaps(rect)||zoom<.018)return;
 const mini=picture('../VelesCardinalTiles/veles-5x5.webp');if(mini)blit(mini,...rect);
 if(zoom<1.9)return;
 for(const t of Object.values(legacyImages)){
  if(!overlaps(t.bounds_xyxy))continue;
  const im=picture(t.id==='center'?'../02-veles-village.svg':'../VelesCardinalTiles/'+t.id+'.svg');
  if(!im)continue;
  const [l,b,r,top]=t.bounds_xyxy;
  ctx.save();ctx.translate(l,top);ctx.scale(1,-1);ctx.drawImage(im,48,142,1084,1160,0,0,r-l,top-b);ctx.restore();
 }
}
function bridgeSymbols(){
 for(const b of crossings){
  if(!visible(b.x,b.y,b.span)||zoom<.1)continue;
  const ps=b.points,first=ps[0],last=ps.at(-1),angle=Math.atan2(last[1]-first[1],last[0]-first[0]);
  if(b.mode==='나루'){ctx.setLineDash([4,4]);stroke(ps,'#eee5c8',2.8);ctx.setLineDash([]);for(const pt of [first,last])rect(...pt,7,4,angle,'#897956')}
  else{stroke(ps,'#796f58',5.2);stroke(ps,'#c5ad81',3.8)}
 }
}
function overlays(){
 if($('animals').checked){
  for(const g of geo.groups){
   const [x,y]=g.home;if(!visible(x,y,g.home_radius_m+4000))continue;
   oval(x,y,g.home_radius_m,g.home_radius_m,'#994e4d44');
   if(g.route.length>1)stroke(g.route,'#8f575e55',g.patrol_halfwidth_m*2);
  }
  for(const d of detail)if(overlaps(d.b)&&['range','limit','campzone'].includes(d.kind)){
   if(d.kind==='limit'){ctx.setLineDash([5,3]);stroke([...d.p,d.p[0]],'#886453',1.7);ctx.setLineDash([])}else stroke(d.p,'#a3625966',0,true);
  }
  for(const [id,,,,x,y,rx,ry] of habitats){
   const site=sites.find(s=>s[0]===id);oval(site[2]+x,site[3]+y,rx,ry,'#9b575b55');
  }
 }
 if($('journey').checked){
  if(zoom<.14)stroke(geo.voyage,'#526e8e',Math.max(2,1.9/zoom));
  const modes=[['guild','drain','stairs','foot'],['bypass','exit','levee'],['r-stretcher','r-pier-link']];
  const ids=['v-out','v-return','f-east','f-west','f-up','t-out','t-back',...modes[$('state').selectedIndex]];
  for(const d of detail)if(ids.includes(d.id)&&overlaps(d.b))stroke(d.p,d.kind==='haul'?'#89546e':'#776198',Math.max(1.7,1.9/zoom));
  for(const q of ways)if(['upstream','workyard','weir'].includes(q.id))viewLine(q,'#776198',Math.max(1.7,1.9/zoom));
  weirDetail(true);
 }
}

