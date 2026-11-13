// Shared-coordinate child terrain and editable placement candidates, not runtime UE assets.
const fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib'),crypto=require('node:crypto');
const ROOT=__dirname,MASTER=path.resolve(ROOT,'../IgnisTerrainMaster-v1'),DATA=path.resolve(ROOT,'../../Data');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,'')),hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const write=(p,v)=>fs.writeFileSync(path.join(ROOT,p),JSON.stringify(v,null,2)+'\n');
const m=read(path.join(MASTER,'manifest.json')),basis=read(path.join(DATA,'worldtree-settlement-basis-v1.json')),ecology=read(path.join(DATA,'veles-forest-ecology-v2.json'));
const ORIGIN=[150000,145000],B=[-2048,-2560,2048,3584],STEP=2,TILE=1024,W=2049,H=3073;
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v)),smooth=x=>{x=clamp(x,0,1);return x*x*(3-2*x);};
function rng(x,y,s=0){let n=Math.imul(x,374761393)^Math.imul(y,668265263)^s;n=Math.imul(n^(n>>>13),1274126177);return ((n^(n>>>16))>>>0)/4294967295;}
function noise(x,y,s=0){const i=Math.floor(x),j=Math.floor(y),a=smooth(x-i),b=smooth(y-j);return(rng(i,j,s)*(1-a)+rng(i+1,j,s)*a)*(1-b)+(rng(i,j+1,s)*(1-a)+rng(i+1,j+1,s)*a)*b;}
function nearest(p,a,b){const dx=b[0]-a[0],dy=b[1]-a[1],q=dx*dx+dy*dy,t=q?clamp(((p[0]-a[0])*dx+(p[1]-a[1])*dy)/q,0,1):0;const point=[a[0]+dx*t,a[1]+dy*t];return{t,point,d:Math.hypot(p[0]-point[0],p[1]-point[1])};}
function lineDistance(p,points){let best={d:Infinity};for(let i=0;i<points.length-1;i++){const q=nearest(p,points[i],points[i+1]);if(q.d<best.d)best={...q,segment:i};}return best;}
function decode(file){const b=fs.readFileSync(file);let pos=8,w,h,depth,type,parts=[];while(pos<b.length){const n=b.readUInt32BE(pos),t=b.toString('ascii',pos+4,pos+8),d=b.subarray(pos+8,pos+8+n);if(t==='IHDR'){w=d.readUInt32BE(0);h=d.readUInt32BE(4);depth=d[8];type=d[9];}if(t==='IDAT')parts.push(d);pos+=12+n;}if(depth!==16||type!==0)throw Error('Expected 16-bit gray master');const raw=zlib.inflateSync(Buffer.concat(parts)),stride=w*2,out=new Uint16Array(w*h),prev=Buffer.alloc(stride);for(let y=0;y<h;y++){const filter=raw[y*(stride+1)],row=Buffer.alloc(stride);for(let x=0;x<stride;x++){const a=x>=2?row[x-2]:0,c=x>=2?prev[x-2]:0,u=prev[x];let v=0;if(filter===1)v=a;else if(filter===2)v=u;else if(filter===3)v=Math.floor((a+u)/2);else if(filter===4){const p=a+u-c,pa=Math.abs(p-a),pb=Math.abs(p-u),pc=Math.abs(p-c);v=pa<=pb&&pa<=pc?a:pb<=pc?u:c;}row[x]=(raw[y*(stride+1)+1+x]+v)&255;}for(let x=0;x<w;x++)out[y*w+x]=row.readUInt16BE(x*2);row.copy(prev);}return{w,h,data:out};}
const master=decode(path.join(MASTER,m.height.file));
function parent(local){const u=(local[0]+ORIGIN[0]-m.grid.origin_xy_m[0])/2000,v=(m.grid.origin_xy_m[1]-local[1]-ORIGIN[1])/2000,i=Math.floor(u),j=Math.floor(v),a=u-i,b=v-j,q=(x,y)=>master.data[y*master.w+x]*.125-1024;return(q(i,j)*(1-a)+q(i+1,j)*a)*(1-b)+(q(i,j+1)*(1-a)+q(i+1,j+1)*a)*b;}
const mainPoints=[[8000,12000],[2200,3300],[1800,2700],...basis.local_river_m,[-1900,-1900],[-5000,-5000]];
const widths=mainPoints.map(p=>55+125*smooth((Math.hypot(...p)-500)/1700));
const river={id:'H01-V01',parent:'H01',points:mainPoints,widths,status:'local_refinement_preserving_quay_and_local_reference; returns_to_parent_outside_patch'};
const stream={id:'spring-water',points:basis.spring_water_m,widths:basis.spring_water_m.map(()=>2.5),status:'existing_local_reference'};
const roads=ecology.paths.map(p=>({id:p.id,name:p.name,width:p.id==='R01'?4:3,points:p.nodes.map(n=>ecology.route_nodes_m[n]),status:'existing_reference_centerline'}));
function road(id,name,width,points,status='new_layout_candidate'){roads.push({id,name,width,points,status});}
road('C-W','숲곁 방향 연결로',6,[[-7000,-2000],[-2600,-650],[-1500,-260],[-800,-80],[-330,60],[-170,80],[-20,90]]);
road('C-SW','들보리·타르넬 방향 연결로',6,[[-10000,-8000],[-3000,-1900],[-2000,-1200],[-1600,-600],[-1500,-260]]);
road('C-N','솔숨 방향 연결로',5,[[90,260],[-300,500],[-500,1600],[200,2500],[1200,3500],[3700,6100],[4000,6000]]);
road('V-WATER','나루 접근로',4,[[-80,80],[-55,50],[-46,20]]);
function roundLane(points){const out=[];for(let i=0;i<points.length-1;i++){const a=points[Math.max(0,i-1)],b=points[i],c=points[i+1],d=points[Math.min(points.length-1,i+2)],count=Math.max(2,Math.ceil(Math.hypot(c[0]-b[0],c[1]-b[1])/8));for(let k=0;k<count;k++){const t=k/count;out.push([0,1].map(j=>.5*(2*b[j]+(-a[j]+c[j])*t+(2*a[j]-5*b[j]+4*c[j]-d[j])*t*t+(-a[j]+3*b[j]-3*c[j]+d[j])*t*t*t)));}}out.push(points.at(-1));return out;}
const lanes=[
 [[-380,40],[-290,48],[-185,36],[-95,68],[-20,90]],
 [[-385,130],[-310,124],[-210,138],[-120,135],[-40,142],[-5,170]],
 [[-335,220],[-260,228],[-150,215],[-80,238],[-20,245],[90,260]],
 [[-290,310],[-210,295],[-100,315],[-40,282],[-20,245]],
 [[-290,48],[-280,90],[-310,124],[-275,180],[-260,228],[-290,310]],
 [[-185,36],[-150,80],[-120,135],[-100,170],[-150,215],[-100,315]]
];
lanes.forEach((points,i)=>road('A'+i,'마을 골목',i<4?3.5:3,roundLane(points)));
const buildings=[];
function polygon(center,w,d,angle){const c=Math.cos(angle),s=Math.sin(angle);return[[-w/2,-d/2],[w/2,-d/2],[w/2,d/2],[-w/2,d/2]].map(([x,y])=>[center[0]+x*c-y*s,center[1]+x*s+y*c]);}
function rectDistance(p,b){const a=-b.angle,c=Math.cos(a),s=Math.sin(a),dx=p[0]-b.center[0],dy=p[1]-b.center[1],x=Math.abs(dx*c-dy*s)-b.width/2,y=Math.abs(dx*s+dy*c)-b.depth/2;return Math.hypot(Math.max(0,x),Math.max(0,y));}
function addBuilding(id,center,width,depth,angle,asset,role){const b={id,center,width,depth,angle,height_m:role==='workshop'?6.5:3.5+rng(Math.round(center[0]),Math.round(center[1]),91)*3,asset,role,footprint:polygon(center,width,depth,angle),yard:polygon(center,width+9,depth+12,angle),status:'new_dimensions_and_footprint_candidate'};buildings.push(b);return b;}
addBuilding('workshop',[60,124],18,20,0,12,'workshop');
addBuilding('water-store',[23,195],14,10,.12,13,'store');
for(let row=0;row<6;row++){
 const lane=roads.find(r=>r.id==='A'+row),segments=lane.points.slice(1).map((p,i)=>Math.hypot(p[0]-lane.points[i][0],p[1]-lane.points[i][1])),total=segments.reduce((a,b)=>a+b,0);
 for(let along=14,col=0;along<total-8;along+=25,col++)for(const side of[-1,1]){
  let rest=along,s=0;while(s<segments.length-1&&rest>segments[s])rest-=segments[s++];const a=lane.points[s],b=lane.points[s+1],angle=Math.atan2(b[1]-a[1],b[0]-a[0]),t=rest/segments[s],offset=16+rng(col,row,side+30)*5;
  const x=a[0]+(b[0]-a[0])*t-Math.sin(angle)*offset*side,y=a[1]+(b[1]-a[1])*t+Math.cos(angle)*offset*side,w=12+rng(col,row,side+40)*6,d=9+rng(col,row,side+50)*4,candidate={center:[x,y],width:w,depth:d,angle},corners=polygon(candidate.center,w+4,d+4,angle);
  if(roads.some(r=>corners.some(p=>lineDistance(p,r.points).d<r.width/2+1.5)||lineDistance(candidate.center,r.points).d<Math.hypot(w,d)/2+r.width/2+1))continue;
  if(buildings.some(b=>Math.hypot(x-b.center[0],y-b.center[1])<(Math.max(w,d)+Math.max(b.width,b.depth))/2+7))continue;
  if(lineDistance(candidate.center,stream.points).d<20||lineDistance(candidate.center,river.points).d<55)continue;
  addBuilding('house-'+String(buildings.length).padStart(3,'0'),[x,y],w,d,angle,8+Math.floor(rng(col,row,side+60)*8),'house');
 }
}
const entrances=[];
for(const b of buildings){
  let best={d:Infinity};for(const r of roads){if(r.id.startsWith('C-'))continue;const q=lineDistance(b.center,r.points);if(q.d<best.d)best={...q,road:r.id};}
  const dx=best.point[0]-b.center[0],dy=best.point[1]-b.center[1],a=-b.angle,rx=dx*Math.cos(a)-dy*Math.sin(a),ry=dx*Math.sin(a)+dy*Math.cos(a),t=Math.min(Math.abs((b.width/2)/rx)||Infinity,Math.abs((b.depth/2)/ry)||Infinity),door=[b.center[0]+dx*t,b.center[1]+dy*t];
  b.door=door;entrances.push({id:'door-'+b.id,points:[door,best.point],width:2.2,status:'building_access_candidate',building:b.id,joins:best.road});
}
roads.push(...entrances);
const fields=[];for(let r=0;r<5;r++)for(let c=0;c<3;c++){const x=-690+c*83,y=-65+r*100;fields.push({id:`field-${c}-${r}`,center:[x,y],width:70,depth:86,angle:.06,polygon:polygon([x,y],70,86,.06),crop:rng(c,r,75)>.5?'crop':'fallow',status:'land_use_candidate'});}
const poiNames={quay:'나루',workshop:'공방',water_point:'취수 지점',water_store:'물 저장고',forest_gate:'숲 입구',cave:'동굴 입구',spring:'샘',local_grove_tree:'숲의 고목',reveal:'남서쪽 조망길'};
const pois=Object.entries(basis.local_points_m).map(([id,point])=>({id,name:poiNames[id]||id,point,visible:!!poiNames[id],source:'worldtree-settlement-basis-v1',surface:['old_workshop','boss'].includes(id)?'underground':'ground'}));
function intersection(a,b,c,d){const u=[b[0]-a[0],b[1]-a[1]],v=[d[0]-c[0],d[1]-c[1]],den=u[0]*v[1]-u[1]*v[0];if(Math.abs(den)<1e-8)return null;const q=[c[0]-a[0],c[1]-a[1]],t=(q[0]*v[1]-q[1]*v[0])/den,s=(q[0]*u[1]-q[1]*u[0])/den;return t>=0&&t<=1&&s>=0&&s<=1?[a[0]+t*u[0],a[1]+t*u[1]]:null;}
const bridges=[];for(const r of roads)for(let i=0;i<r.points.length-1;i++)for(let j=0;j<stream.points.length-1;j++){const p=intersection(r.points[i],r.points[i+1],stream.points[j],stream.points[j+1]);if(p&&!bridges.some(b=>Math.hypot(b.center[0]-p[0],b.center[1]-p[1])<2))bridges.push({id:'bridge-'+(bridges.length+1),center:p,angle:Math.atan2(r.points[i+1][1]-r.points[i][1],r.points[i+1][0]-r.points[i][0]),length:12,width:r.width+1.5,road:r.id,status:'bridge_deck_candidate_not_structural_design'});}
const bridgeSource=read(path.join(DATA,'veles-opening-connections-v1.json')).bridges;
for(const s of bridgeSource){const b=bridges.find(b=>Math.hypot(b.center[0]-s.xy_m[0],b.center[1]-s.xy_m[1])<2);if(b)b.reference_id=s.id;}
const isField=p=>fields.some(f=>rectDistance(p,{...f,center:f.center,width:f.width+8,depth:f.depth+8})===0);
function rawDetail(p){
  const n=(noise((p[0]+ORIGIN[0])/170,(p[1]+ORIGIN[1])/170,991)-.5)*18+(noise((p[0]+ORIGIN[0])/43,(p[1]+ORIGIN[1])/43,817)-.5)*4;
  const rockHill=28*Math.exp(-(((p[0]+950)/650)**2+((p[1]+800)/700)**2))+22*Math.exp(-(((p[0]-450)/300)**2+((p[1]-1350)/400)**2));
  const near=lineDistance(p,river.points),half=(widths[near.segment]*(1-near.t)+widths[near.segment+1]*near.t)/2;
  const town=smooth(Math.max(Math.abs((p[0]+170)/480),Math.abs((p[1]-170)/320))-1);
  return (n+rockHill)*smooth((near.d-half)/80)*town-Math.max(0,1-near.d/half)*2;
}
const coarseCache=new Map();
function coarseDetail(x,y){const key=x*100000+y;if(!coarseCache.has(key))coarseCache.set(key,rawDetail([x,y]));return coarseCache.get(key);}
function residual(p){const gx=ORIGIN[0]+p[0],gy=ORIGIN[1]+p[1],x=Math.floor(gx/2000)*2000-ORIGIN[0],y=Math.floor(gy/2000)*2000-ORIGIN[1],a=(p[0]-x)/2000,b=(p[1]-y)/2000;
  const coarse=(coarseDetail(x,y)*(1-a)+coarseDetail(x+2000,y)*a)*(1-b)+(coarseDetail(x,y+2000)*(1-a)+coarseDetail(x+2000,y+2000)*a)*b;
  const edge=Math.min(p[0]-B[0],B[2]-p[0],p[1]-B[1],B[3]-p[1]);return(rawDetail(p)-coarse)*smooth(edge/256);
}
function height(p){return parent(p)+residual(p);}
// Numeric PNG writer, including packed height for browser texture decoding.
const table=Array.from({length:256},(_,n)=>{for(let k=0;k<8;k++)n=n&1?0xedb88320^(n>>>1):n>>>1;return n>>>0;});
function chunk(t,d){const a=Buffer.concat([Buffer.from(t),d]);let c=0xffffffff;for(const v of a)c=table[(c^v)&255]^(c>>>8);const o=Buffer.alloc(d.length+12);o.writeUInt32BE(d.length);a.copy(o,4);o.writeUInt32BE((c^0xffffffff)>>>0,o.length-4);return o;}
function png(file,w,h,depth,channels,get){const bpp=channels*depth/8,row=w*bpp,raw=Buffer.alloc((row+1)*h),prev=new Uint8Array(bpp),pixel=new Uint8Array(bpp);for(let j=0;j<h;j++){raw[j*(row+1)]=1;prev.fill(0);for(let i=0;i<w;i++){const v=get(i,j);if(depth===16){pixel[0]=v>>>8;pixel[1]=v&255;}else pixel.set(v);for(let k=0;k<bpp;k++){raw[j*(row+1)+1+i*bpp+k]=(pixel[k]-prev[k])&255;prev[k]=pixel[k];}}}const head=Buffer.alloc(13);head.writeUInt32BE(w);head.writeUInt32BE(h,4);head[8]=depth;head[9]=channels===3?2:0;fs.writeFileSync(file,Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',head),chunk('IDAT',zlib.deflateSync(raw)),chunk('IEND',Buffer.alloc(0))]));}
function build(){
 const values=new Uint16Array(W*H);let min=Infinity,max=-Infinity;
 for(let j=0;j<H;j++)for(let i=0;i<W;i++){const v=Math.round((height([B[0]+i*STEP,B[3]-j*STEP])+1024)*8);values[j*W+i]=v;min=Math.min(min,v*.125-1024);max=Math.max(max,v*.125-1024);}
 png(path.join(ROOT,'height/master16.png'),W,H,16,1,(i,j)=>values[j*W+i]);png(path.join(ROOT,'height/packed.png'),W,H,8,3,(i,j)=>{const v=values[j*W+i];return[v>>>8,v&255,0];});
 const tiles=[];for(let r=0;r<6;r++)for(let c=0;c<4;c++){
   const id=`C${c}-R${r}`,bounds=[B[0]+c*TILE,B[3]-(r+1)*TILE,B[0]+(c+1)*TILE,B[3]-r*TILE],file=`height/tiles/${id}.png`;
   png(path.join(ROOT,file),513,513,16,1,(i,j)=>values[(r*512+j)*W+c*512+i]);tiles.push({id,bounds,height:file,texture:`tiles/${id}.png`,height_sha256:hash(path.join(ROOT,file)),texture_spec:{size:2112,core_size:2048,margin_pixels:32,meters_per_pixel:.5}});
 }
 const vegetation=[];
 for(let j=Math.floor((B[1]-32)/12);j<=Math.ceil((B[3]+32)/12);j++)for(let i=Math.floor((B[0]-32)/12);i<=Math.ceil((B[2]+32)/12);i++){
   const p=[i*12+(rng(i,j,301)-.5)*11,j*12+(rng(i,j,302)-.5)*11],diameter=11+rng(i,j,303)*8;
   const landuse=noise((p[0]+ORIGIN[0])/450,(p[1]+ORIGIN[1])/450,30),town=Math.hypot((p[0]+160)/340,(p[1]-130)/220);
   if(rng(i,j,304)>(town<1?.10:.58+landuse*.38)||isField(p))continue;
   const nr=lineDistance(p,river.points),half=(widths[nr.segment]*(1-nr.t)+widths[nr.segment+1]*nr.t)/2;
   if(nr.d<half+diameter/2+5||lineDistance(p,stream.points).d<diameter/2+3)continue;
   if(roads.some(r=>lineDistance(p,r.points).d<r.width/2+diameter/2+1.5)||buildings.some(b=>rectDistance(p,b)<diameter/2+4))continue;
   if(Math.hypot(p[0]-260,p[1]-1250)<22||Math.hypot(p[0]-320,p[1]-1390)<12)continue;
   vegetation.push([Number(p[0].toFixed(3)),Number(p[1].toFixed(3)),Number(diameter.toFixed(3)),Math.floor(rng(i,j,305)*8),Number((rng(i,j,306)*Math.PI*2).toFixed(5))]);
 }
 const boundaries=[];
 for(const r of roads.filter(r=>r.id.startsWith('C-')))for(let i=0;i<r.points.length-1;i++)for(const [edge,a,b]of[['west',[B[0],B[1]],[B[0],B[3]]],['east',[B[2],B[1]],[B[2],B[3]]],['north',[B[0],B[3]],[B[2],B[3]]],['south',[B[0],B[1]],[B[2],B[1]]]]){const p=intersection(r.points[i],r.points[i+1],a,b);if(p)boundaries.push({kind:'road',id:r.id,edge,point:p,width:r.width,global_xy_m:p.map((v,k)=>v+ORIGIN[k]),direction:[r.points[i+1][0]-r.points[i][0],r.points[i+1][1]-r.points[i][1]]});}
 for(let i=0;i<river.points.length-1;i++)for(const[edge,a,b]of[['west',[B[0],B[1]],[B[0],B[3]]],['east',[B[2],B[1]],[B[2],B[3]]],['north',[B[0],B[3]],[B[2],B[3]]],['south',[B[0],B[1]],[B[2],B[1]]]]){const p=intersection(river.points[i],river.points[i+1],a,b);if(p)boundaries.push({kind:'river',id:river.id,edge,point:p,width:180,global_xy_m:p.map((v,k)=>v+ORIGIN[k]),status:'matches_parent_H01_centerline_and_width'});}
 const manifest={schema_version:1,status:'detail_terrain_and_layout_candidate',origin_regional_xy_m:ORIGIN,bounds_local_m:B,extent_m:[4096,6144],height:{file:'height/master16.png',width:W,height:H,spacing_m:STEP,meters_per_unit:.125,offset_m:-1024,range_m:[min,max]},parent:{manifest:'../IgnisTerrainMaster-v1/manifest.json',height_sha256:hash(path.join(MASTER,m.height.file)),basis_sha256:hash(path.join(DATA,'worldtree-settlement-basis-v1.json')),ecology_sha256:hash(path.join(DATA,'veles-forest-ecology-v2.json')),inheritance:'parent lattice vertices preserved; outer 256m residual fades to parent; .125m encoding'},tile_layout:{columns:4,rows:6,tile_m:TILE,shared_height_samples:1,tiles},boundaries,counts:{buildings:buildings.length,roads:roads.length,bridges:bridges.length,trees:vegetation.length,source_pois:pois.length},limitations:['Detailed patch covers Veles and approaches only, not neighboring settlement centers.','New building footprints, dimensions, lanes, fields and regional approach corridors are candidates.','Local river refinement uses existing local reference and returns to master H01 at outer boundaries; local width narrows to 55m.','Cave interior nodes retained as underground; no surface building substitutes.','No UE import, collision, navigation, structural bridge or full drainage validation.']};
 for(const b of buildings)b.ground_elevation_m=height(b.center);
 const scene={origin:ORIGIN,bounds:B,river,stream,roads,buildings,fields,bridges,pois,vegetation,tiles,neighbor_anchors:[{id:'V02',name:'솔숨',point:[4000,6000]},{id:'V03',name:'들보리',point:[-10000,-8000]},{id:'V04',name:'숲곁',point:[-7000,-2000]},{id:'T01',name:'타르넬',point:[-13000,-20000]}]};
 write('manifest.json',manifest);write('data/placements.json',{status:manifest.status,origin:ORIGIN,buildings,fields,bridges,pois});write('data/connections.json',{river,stream,roads,boundaries,neighbor_anchors:scene.neighbor_anchors});write('data/vegetation.json',{schema:'[local_x_m,local_y_m,crown_diameter_m,asset_index,angle_radians]',seed:'global_origin_150000_145000',items:vegetation});
 fs.writeFileSync(path.join(ROOT,'scene.js'),'const SCENE='+JSON.stringify(scene)+';\n');
 const ur=p=>'data:image/png;base64,'+fs.readFileSync(p).toString('base64');
 const rects=[[36,49,302,298],[41,54,286,290],[34,50,282,291],[24,51,275,291],[33,32,300,288],[29,41,297,289],[38,32,285,291],[36,49,280,285],[31,68,305,243],[24,81,298,246],[23,79,289,246],[10,78,286,244],[25,69,306,244],[23,71,298,245],[23,82,285,236],[10,74,288,239]].map((r,i)=>{const o=[0,314,627,940];return[r[0]+o[i%4],r[1]+o[Math.floor(i/4)],r[2]-r[0],r[3]-r[1]];});
 fs.writeFileSync(path.join(ROOT,'assets.js'),'const ASSETS='+JSON.stringify({props:ur(path.join(ROOT,'assets/vegetation-roofs.png')),materials:ur(path.resolve(ROOT,'../IgnisTerrainPrototype-v1/materials/aerial-material-atlas.png')),height:ur(path.join(ROOT,'height/packed.png')),rects})+';\n');
 console.log(JSON.stringify({counts:manifest.counts,height:manifest.height,tiles:tiles.length,boundaries},null,2));
}
if(require.main===module)build();
module.exports={parent,height,residual,nearest,lineDistance,rectDistance,decode,roads,river,stream,buildings,B,ORIGIN,ROOT,master,m,STEP,W,H};
