"use strict";
function bounds(ps){
 let l=Infinity,r=-Infinity,b=Infinity,t=-Infinity;
 for(const [x,y] of ps){l=Math.min(l,x);r=Math.max(r,x);b=Math.min(b,y);t=Math.max(t,y)}
 return [l,b,r,t];
}

function decimate(ps,gap){
 let out=[ps[0]];
 for(let i=1;i<ps.length-1;i++){let a=out[out.length-1];if(Math.hypot(ps[i][0]-a[0],ps[i][1]-a[1])>gap)out.push(ps[i])}
 out.push(ps.at(-1));return out;
}
function settlementEntries(line){
 if(line.kind!=='road')return line.points;
 let points=line.points;
 for(const p of localPlans){
  const [,,x,y]=p.site, radius=p.site[0][0]==='V'?205:120;
  const first=points.findIndex(pt=>Math.hypot(pt[0]-x,pt[1]-y)<radius);
  if(first<0||!points.some(pt=>Math.hypot(pt[0]-x,pt[1]-y)<8))continue;
  let last=first;while(last+1<points.length&&Math.hypot(points[last+1][0]-x,points[last+1][1]-y)<radius)last++;
  const ends=[p.lanes[0][0],p.lanes[0].at(-1)];
  const near=a=>ends.reduce((b,c)=>Math.hypot(a[0]-b[0],a[1]-b[1])<Math.hypot(a[0]-c[0],a[1]-c[1])?b:c);
  const before=points.slice(0,first),after=points.slice(last+1),start=before.length?near(before.at(-1)):null,end=after.length?near(after[0]):null;
  const lane=start&&end&&start!==end?(start===ends[0]?p.lanes[0]:p.lanes[0].slice().reverse()):[start||end];
  points=[...before,...lane,...after];
 }
 return points;
}
const ways=geo.lines.map(row=>{
 const chunks=[];
 const points=settlementEntries(row);
 for(let i=0;i<points.length-1;i+=90){
  let ps=points.slice(i,i+91);
  chunks.push({ps,far:decimate(ps,60),b:bounds(ps)});
 }
 return {...row,points,chunks};
});

function streamWidth(id,d,w,side){
 if(id==='creek')return w/2;
 return w*(.50+.085*Math.sin(d/137+side*2.3)+.07*Math.sin(d/57+side));
}
for(const way of ways.filter(w=>w.kind==='stream')){
 let distance=0;
 way.banks=way.points.map((p,i,ps)=>{
  if(i)distance+=Math.hypot(p[0]-ps[i-1][0],p[1]-ps[i-1][1]);
  let a=ps[Math.max(0,i-1)],b=ps[Math.min(i+1,ps.length-1)],dx=b[0]-a[0],dy=b[1]-a[1],len=Math.hypot(dx,dy)||1;
  return [-1,1].map(side=>{let r=side*streamWidth(way.id,distance,way.width,side);return [p[0]-dy*r/len,p[1]+dx*r/len]});
 });
}

function waterCrossings(){
 const bins=new Map(),records=[];
 const cell=(x,y)=>Math.floor(x/500)+','+Math.floor(y/500);
 function register(id,a,b){
  for(let i=Math.floor(Math.min(a[0],b[0])/500);i<=Math.floor(Math.max(a[0],b[0])/500);i++)
   for(let j=Math.floor(Math.min(a[1],b[1])/500);j<=Math.floor(Math.max(a[1],b[1])/500);j++){
    const key=i+','+j;if(!bins.has(key))bins.set(key,[]);
    bins.get(key).push({id,a,b});
   }
 }
 for(let k=1;k<geo.water.length;k++){
  const a=geo.water[k-1],b=geo.water[k];
  for(let side=1;side<=2;side++)register('H01',[a[side],a[0]],[b[side],b[0]]);
 }
 for(const way of ways.filter(l=>l.banks))for(let k=1;k<way.banks.length;k++)
  for(let side=0;side<2;side++)register(way.id,way.banks[k-1][side],way.banks[k][side]);
 for(const way of ways.filter(l=>l.kind==='road')){
  const ps=way.points,hits=new Map();
  for(let k=1;k<ps.length;k++){
   const a=ps[k-1],b=ps[k],dx=b[0]-a[0],dy=b[1]-a[1];
   const cells=new Set([cell(...a),cell(...b),cell(a[0],b[1]),cell(b[0],a[1])]);
   for(const key of cells)for(const edge of bins.get(key)||[]){
    const ux=edge.b[0]-edge.a[0],uy=edge.b[1]-edge.a[1],den=dx*uy-dy*ux;
    if(Math.abs(den)<1e-9)continue;
    const px=edge.a[0]-a[0],py=edge.a[1]-a[1],t=(px*uy-py*ux)/den,s=(px*dy-py*dx)/den;
    if(t<0||s<0||t>1||s>1)continue;
    if(!hits.has(edge.id))hits.set(edge.id,[]);
    const arr=hits.get(edge.id),u=k-1+t;
    if(!arr.some(q=>Math.abs(q.u-u)<1e-7))arr.push({u,p:[a[0]+t*dx,a[1]+t*dy]});
   }
  }
  for(const [river,points] of hits){
   points.sort((a,b)=>a.u-b.u);
   // 도로 양끝은 마른 땅이다. 중앙 원도 안쪽의 도로는 원도 쪽에서 따로 잇는다.
   for(let k=1;k<points.length;k++){
    const s=points[k-1],e=points[k],pts=[s.p,...ps.slice(Math.floor(s.u)+1,Math.ceil(e.u)),e.p];
    const mid=pts.length>2?pts[Math.floor(pts.length/2)]:[(s.p[0]+e.p[0])/2,(s.p[1]+e.p[1])/2];
    const [x,y]=mid,[i,j]=tileAt(x,y);if(Math.max(Math.abs(i),Math.abs(j))<=2)continue;
    let submerged=false;
    if(river==='H01'){
     let lo=0,hi=geo.water.length-1;while(lo+1<hi){let h=(lo+hi)>>1;if(geo.water[h][0]<y)lo=h;else hi=h}
     const a=geo.water[lo],b=geo.water[hi],t=(y-a[0])/(b[0]-a[0]);
     submerged=x>=a[1]+(b[1]-a[1])*t&&x<=a[2]+(b[2]-a[2])*t;
    }else{
     const waterway=ways.find(l=>l.id===river);
     for(const chunk of waterway.chunks){
      if(x<chunk.b[0]-waterway.width||x>chunk.b[2]+waterway.width||y<chunk.b[1]-waterway.width||y>chunk.b[3]+waterway.width)continue;
      if(chunk.ps.some((p,j)=>j&&segDistance(x,y,chunk.ps[j-1],p)<waterway.width*.55)){submerged=true;break}
     }
    }
    if(!submerged)continue;
    const span=pts.reduce((sum,p,i)=>sum+(i?Math.hypot(p[0]-pts[i-1][0],p[1]-pts[i-1][1]):0),0);
    if(span<1)continue;
    records.push({road:way.id,river,x,y,span,points:pts,mode:span>90?'나루':'다리'});
   }
  }
 }
 return records;
}
const crossings=waterCrossings();

