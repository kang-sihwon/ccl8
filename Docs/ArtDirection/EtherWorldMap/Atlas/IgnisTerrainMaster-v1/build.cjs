// Reproducible design terrain. Source vectors and images are never overwritten.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),zlib=require('node:zlib');
const ROOT=__dirname,ART=path.resolve(ROOT,'../..');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const json=(p,v)=>fs.writeFileSync(path.join(ROOT,p),JSON.stringify(v,null,2)+'\n');
const sources=['../../Data/ignis-continent-georeference-v1.json','../../Data/ether-world-map-land35-precise-v3.json','../IgnisSettlementDetails-v1/records/plan.json'];
const [g,world,detail]=sources.map(p=>read(path.join(ROOT,p)));
const coastSource=world.continents.find(c=>c.id===1),STEP=2000,CELLS=512;
const R=g.coordinate_system.radius_km*1000,RAD=Math.PI/180,L0=-RAD,P0=54*RAD;
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const smooth=v=>{v=clamp(v,0,1);return v*v*(3-2*v);};
const meter=p=>p.map(v=>v*1000);
function project(uv){
  const l=(uv[0]*360-180)*RAD,p=(90-uv[1]*180)*RAD;
  const c=Math.acos(clamp(Math.sin(P0)*Math.sin(p)+Math.cos(P0)*Math.cos(p)*Math.cos(l-L0),-1,1)),k=c<1e-12?1:c/Math.sin(c);
  return [R*k*Math.cos(p)*Math.sin(l-L0),R*k*(Math.cos(P0)*Math.sin(p)-Math.sin(P0)*Math.cos(p)*Math.cos(l-L0))];
}
function inverse([x,y]){
  const r=Math.hypot(x,y),c=r/R;if(!r)return[-1,54];
  return [(L0+Math.atan2(x*Math.sin(c),r*Math.cos(P0)*Math.cos(c)-y*Math.sin(P0)*Math.sin(c)))/RAD,Math.asin(Math.cos(c)*Math.sin(P0)+y*Math.sin(c)*Math.cos(P0)/r)/RAD];
}
const coast=coastSource.geometry.coordinates.map(r=>r.map(project)),vs=coast.flat();
const bounds=[Math.min(...vs.map(p=>p[0])),Math.min(...vs.map(p=>p[1])),Math.max(...vs.map(p=>p[0])),Math.max(...vs.map(p=>p[1]))];
const xmin=Math.floor((bounds[0]-STEP*8)/STEP)*STEP,ymax=Math.ceil((bounds[3]+STEP*8)/STEP)*STEP;
const cols=Math.ceil((bounds[2]+STEP*8-xmin)/STEP/CELLS),rows=Math.ceil((ymax-bounds[1]+STEP*8)/STEP/CELLS);
const W=cols*CELLS+1,H=rows*CELLS+1,N=W*H;
const xy=(i,j)=>[xmin+i*STEP,ymax-j*STEP],grid=([x,y])=>[(x-xmin)/STEP,(ymax-y)/STEP];
function inside([x,y]){
  let hit=false;for(const ring of coast)for(let i=0,j=ring.length-1;i<ring.length;j=i++){
    const [ax,ay]=ring[i],[bx,by]=ring[j];if((ay>y)!==(by>y)&&x<(bx-ax)*(y-ay)/(by-ay)+ax)hit=!hit;
  }return hit;
}
function segment(p,a,b){
  const vx=b[0]-a[0],vy=b[1]-a[1],q=vx*vx+vy*vy,t=q?clamp(((p[0]-a[0])*vx+(p[1]-a[1])*vy)/q,0,1):0;
  return {t,distance:Math.hypot(p[0]-a[0]-t*vx,p[1]-a[1]-t*vy)};
}
function intersect(a,b,c,d){
  const ux=b[0]-a[0],uy=b[1]-a[1],vx=d[0]-c[0],vy=d[1]-c[1],den=ux*vy-uy*vx;
  if(Math.abs(den)<1e-6){
    if(Math.abs((c[0]-a[0])*uy-(c[1]-a[1])*ux)>Math.hypot(ux,uy)*.01)return null;
    const q=ux*ux+uy*uy;if(!q)return null;
    const s=((c[0]-a[0])*ux+(c[1]-a[1])*uy)/q,e=((d[0]-a[0])*ux+(d[1]-a[1])*uy)/q;
    const lo=Math.max(0,Math.min(s,e)),hi=Math.min(1,Math.max(s,e));
    return hi-lo>1e-10?{type:'collinear_overlap',xy_m:[a[0]+(lo+hi)/2*ux,a[1]+(lo+hi)/2*uy],overlap_m:(hi-lo)*Math.sqrt(q)}:null;
  }
  const qx=c[0]-a[0],qy=c[1]-a[1],t=(qx*vy-qy*vx)/den,u=(qx*uy-qy*ux)/den;
  return t>=-1e-10&&t<=1+1e-10&&u>=-1e-10&&u<=1+1e-10?{type:'intersection',xy_m:[a[0]+t*ux,a[1]+t*uy],t,u}:null;
}
function noise(x,y,seed){
  const ix=Math.floor(x),iy=Math.floor(y),tx=smooth(x-ix),ty=smooth(y-iy);
  const h=(a,b)=>{let n=Math.imul(a,374761393)^Math.imul(b,668265263)^seed;n=Math.imul(n^(n>>>13),1274126177);return ((n^(n>>>16))>>>0)/4294967295;};
  return (h(ix,iy)*(1-tx)+h(ix+1,iy)*tx)*(1-ty)+(h(ix,iy+1)*(1-tx)+h(ix+1,iy+1)*tx)*ty;
}
const crcTable=Array.from({length:256},(_,n)=>{for(let k=0;k<8;k++)n=(n&1)?0xedb88320^(n>>>1):n>>>1;return n>>>0;});
function chunk(type,data){
  const all=Buffer.concat([Buffer.from(type),data]);let crc=0xffffffff;for(const b of all)crc=crcTable[(crc^b)&255]^(crc>>>8);
  const out=Buffer.alloc(12+data.length);out.writeUInt32BE(data.length);all.copy(out,4);out.writeUInt32BE((crc^0xffffffff)>>>0,out.length-4);return out;
}
function png(file,w,h,depth,channels,get){
  const bpp=channels*depth/8,stride=w*bpp,raw=Buffer.alloc((stride+1)*h),prev=new Uint8Array(bpp),pixel=new Uint8Array(bpp);
  for(let y=0;y<h;y++){
    raw[y*(stride+1)]=1;prev.fill(0);
    for(let x=0;x<w;x++){
      const v=get(x,y);if(depth===16){pixel[0]=v>>>8;pixel[1]=v&255;}else if(channels===1)pixel[0]=v;else pixel.set(v);
      for(let c=0;c<bpp;c++){raw[y*(stride+1)+1+x*bpp+c]=(pixel[c]-prev[c])&255;prev[c]=pixel[c];}
    }
  }
  const head=Buffer.alloc(13);head.writeUInt32BE(w);head.writeUInt32BE(h,4);head[8]=depth;head[9]=channels===3?2:0;
  fs.writeFileSync(file,Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',head),chunk('IDAT',zlib.deflateSync(raw,{level:6})),chunk('IEND',Buffer.alloc(0))]));
}
function build(){
  for(const dir of ['data','terrain/tiles','previews','records'])fs.mkdirSync(path.join(ROOT,dir),{recursive:true});
  console.log(`Building ${W}x${H}, ${cols}x${rows} tiles at ${STEP}m spacing`);
  const land=new Uint8Array(N),dist=new Float32Array(N),z=new Float32Array(N),ridge=new Float32Array(N);
  for(let j=0;j<H;j++){
    const y=ymax-j*STEP,hits=[];for(const ring of coast)for(let k=0,l=ring.length-1;k<ring.length;l=k++){
      const a=ring[k],b=ring[l];if((a[1]>y)!==(b[1]>y))hits.push(a[0]+(b[0]-a[0])*(y-a[1])/(b[1]-a[1]));
    }hits.sort((a,b)=>a-b);
    for(let k=0;k+1<hits.length;k+=2)for(let i=Math.max(0,Math.ceil((hits[k]-xmin)/STEP));i<W&&xmin+i*STEP<hits[k+1];i++)land[j*W+i]=255;
  }
  for(let n=0;n<N;n++)dist[n]=land[n]?1e6:0;
  for(let j=1;j<H-1;j++)for(let i=1;i<W-1;i++){const n=j*W+i;if(land[n])dist[n]=Math.min(dist[n],dist[n-1]+1,dist[n-W]+1,dist[n-W-1]+Math.SQRT2,dist[n-W+1]+Math.SQRT2);}
  for(let j=H-2;j>0;j--)for(let i=W-2;i>0;i--){const n=j*W+i;if(land[n])dist[n]=Math.min(dist[n],dist[n+1]+1,dist[n+W]+1,dist[n+W+1]+Math.SQRT2,dist[n+W-1]+Math.SQRT2);}
  const paint=(points,radius,callback)=>{
    for(let s=0;s<points.length-1;s++){
      const a=points[s],b=points[s+1],lo=grid([Math.min(a[0],b[0])-radius,Math.max(a[1],b[1])+radius]),hi=grid([Math.max(a[0],b[0])+radius,Math.min(a[1],b[1])-radius]);
      for(let j=Math.max(0,Math.floor(lo[1]));j<=Math.min(H-1,Math.ceil(hi[1]));j++)for(let i=Math.max(0,Math.floor(lo[0]));i<=Math.min(W-1,Math.ceil(hi[0]));i++){
        const n=j*W+i;if(!land[n])continue;const q=segment(xy(i,j),a,b);if(q.distance<radius)callback(n,q,s);
      }
    }
  };
  for(const r of g.ridges)paint(r.xy_km.map(meter),330000,(n,q)=>{ridge[n]=Math.max(ridge[n],(r.id==='D01'?3100:2350)*Math.exp(-Math.pow(q.distance/115000,1.5)));});
  for(let j=0;j<H;j++)for(let i=0;i<W;i++){
    const n=j*W+i;if(!land[n])continue;const [x,y]=xy(i,j);let f=0,amp=1,scale=220000,total=0;
    for(let o=0;o<6;o++){f+=amp*noise(x/scale,y/scale,9127+o*7919);total+=amp;amp*=.52;scale*=.5;}f/=total;
    z[n]=(110+f*680+ridge[n]*(.55+f*.9))*smooth(dist[n]*STEP/85000);
  }
  console.log('Applying authored river profiles and pass constraints');
  const near=new Float32Array(N).fill(1e12),target=new Float32Array(N);
  for(const r of g.rivers)paint(r.xy_km.map(meter),45000,(n,q,s)=>{if(q.distance<near[n]){near[n]=q.distance;target[n]=r.bed_elevation_m[s]*(1-q.t)+r.bed_elevation_m[s+1]*q.t;}});
  // Keep a two-cell interpolation core so sub-pixel channel samples do not climb
  // up the valley sides. This is a coarse DEM constraint, not physical river width.
  for(let n=0;n<N;n++)if(near[n]<45000){const d=Math.max(0,near[n]-2*STEP),t=smooth(d/(45000-2*STEP)),v=target[n]+d*.004;z[n]=v*(1-t)+z[n]*t;}
  for(const p of g.passes){const center=meter(p.xy_km),[cx,cy]=grid(center);
    for(let j=Math.max(0,Math.floor(cy-25));j<=Math.min(H-1,Math.ceil(cy+25));j++)for(let i=Math.max(0,Math.floor(cx-25));i<=Math.min(W-1,Math.ceil(cx+25));i++){
      const n=j*W+i,[x,y]=xy(i,j),d=Math.hypot(x-center[0],y-center[1]);if(land[n]&&d<50000)z[n]=Math.min(z[n],1100+1400*smooth(d/50000));
    }
  }
  // Fit the raster's bilinear samples to the continuous authored channel profiles.
  // This conditions the specified channels only; it does not solve whole catchments.
  const controls=[];
  for(const r of g.rivers)for(let s=0;s<r.xy_km.length-1;s++){
    const a=meter(r.xy_km[s]),b=meter(r.xy_km[s+1]),count=Math.max(1,Math.ceil(Math.hypot(b[0]-a[0],b[1]-a[1])/(STEP/2)));
    for(let k=0;k<=count;k++){
      const t=k/count,[u,v]=grid([a[0]+t*(b[0]-a[0]),a[1]+t*(b[1]-a[1])]),i=Math.floor(u),j=Math.floor(v),fx=u-i,fy=v-j;
      const ids=[j*W+i,j*W+i+1,(j+1)*W+i,(j+1)*W+i+1],weights=[(1-fx)*(1-fy),fx*(1-fy),(1-fx)*fy,fx*fy];
      controls.push({ids,weights,target:r.bed_elevation_m[s]*(1-t)+r.bed_elevation_m[s+1]*t});
    }
  }
  for(let iteration=0;iteration<120;iteration++)for(const c of controls){
    let value=0,den=0;for(let k=0;k<4;k++){value+=z[c.ids[k]]*c.weights[k];if(land[c.ids[k]])den+=c.weights[k]**2;}
    if(den>1e-8)for(let k=0;k<4;k++)if(land[c.ids[k]])z[c.ids[k]]=Math.max(0,z[c.ids[k]]-(value-c.target)*c.weights[k]/den*.75);
  }
  const encoded=new Uint16Array(N);let maxHeight=0;
  for(let n=0;n<N;n++){encoded[n]=Math.round((clamp(z[n],-1024,7167.875)+1024)*8);maxHeight=Math.max(maxHeight,encoded[n]/8-1024);}
  const sample=p=>{const[u,v]=grid(p),i=clamp(Math.floor(u),0,W-2),j=clamp(Math.floor(v),0,H-2),a=clamp(u-i,0,1),b=clamp(v-j,0,1),get=(x,y)=>encoded[y*W+x]/8-1024;return (get(i,j)*(1-a)+get(i+1,j)*a)*(1-b)+(get(i,j+1)*(1-a)+get(i+1,j+1)*a)*b;};
  const header={schema_version:1,status:'design_master_candidate',coordinate_system:{...g.coordinate_system,units:'meters',axis:'x east; y north; z up',vertical_datum:'fictional sea level = 0 m',geographic_warning:'Fictional sphere, not Earth WGS84'}};
  json('data/coastline.json',{...header,source_geometry_sha256:g.source_geometry_sha256,source_geometry_normalized:coastSource.geometry,geometry_xy_m:{type:'Polygon',coordinates:coast},interpolation:'Normalized lon/lat edges are authoritative. Projected vertex chords and raster mask approximate those edges.'});
  const rivers=g.rivers.map(r=>({...r,geometry_xy_m:r.xy_km.map(meter),direction:'first_to_last',width_m_candidate:r.id==='H01'?180:55,profile_status:'source_authored_bed_preserved_not_simulated',water_surface_m_candidate:r.bed_elevation_m.map((v,i)=>v+(r.outlet.startsWith('sea:')&&i===r.bed_elevation_m.length-1?0:2)),terrain_at_vertices_m:r.xy_km.map(p=>sample(meter(p)))}));
  json('data/rivers.json',{...header,rivers,lakes:g.lakes.map(l=>({...l,center_xy_m:meter(l.center_xy_km),extent_status:'symbol_radius_not_physical_shoreline',conflict:'Source lake surface equals river bed at center; reconcile bathymetry/water stage. No lake basin stamped.'}))});
  json('data/terrain-controls.json',{...header,ridges:g.ridges.map(r=>({...r,geometry_xy_m:r.xy_km.map(meter),ridge_amplitude_m_candidate:r.id==='D01'?3100:2350,influence_radius_m_candidate:330000})),passes:g.passes.map(p=>({...p,xy_m:meter(p.xy_km),saddle_cap_m_candidate:1100})),parameters:{seed:9127,river_valley_radius_m:45000,river_profile_interpolation_core_m:2*STEP,core_is_not_physical_river_width:true,valley_side_slope:.004,coast_ramp_m:85000,channel_fit:{spacing_m:STEP/2,iterations:120,relaxation:.75,scope:'authored channels only; not whole catchments'},method:'value noise + ridge envelopes + nearest-river valley constraints + bilinear channel profile fitting',hydraulic_erosion:false}});
  const settlements=g.settlements.map(s=>{const d=detail.settlements.find(t=>t.id===s.id);return {...s,xy_m:meter(s.xy_km),world_pixel_L10:d.worldPixel,terrain_elevation_m:sample(meter(s.xy_km)),source_context_bounds_xy_km:d.sourceBounds,context_bounds_meaning:'illustration coverage, not settlement footprint',footprint_status:'not_inferred_from_image',facilities:d.points.map(p=>({id:p.id,name:p.name,xy_km:p.xy_km,xy_m:meter(p.xy_km),status:p.status||'existing_detail_candidate',terrain_elevation_m:sample(meter(p.xy_km))}))};});
  json('data/settlements.json',{...header,settlements,ports:g.ports,world_tree:g.world_tree,exclusions:[{id:'world-tree-residential-exclusion',center_xy_m:meter(g.world_tree.xy_km),radius_m:g.world_tree.residential_exclusion_radius_km*1000,source:'existing_world_tree_basis',affected:'residential_placement'}]});
  const transport=g.routes.map(r=>{
    const points=r.xy_km.map(meter),profile=[];let along=0;
    for(let s=0;s<points.length-1;s++){const a=points[s],b=points[s+1],d=Math.hypot(b[0]-a[0],b[1]-a[1]),count=Math.max(1,Math.ceil(d/5000));
      for(let j=0;j<count;j++){const t=j/count,p=[a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t];profile.push({distance_m:along+d*t,xy_m:p,terrain_m:sample(p),on_land:inside(p)});}along+=d;
    }profile.push({distance_m:along,xy_m:points.at(-1),terrain_m:sample(points.at(-1)),on_land:inside(points.at(-1))});
    const grades=profile.slice(1).map((p,i)=>Math.abs(p.terrain_m-profile[i].terrain_m)/Math.max(1,p.distance_m-profile[i].distance_m));
    return {...r,geometry_xy_m:points,length_m:along,width_m_candidate:r.mode==='road'?(r.id.startsWith('B')?4:6):null,geometry_role:r.mode==='road'?'preserved_corridor_requires_bank_and_crossing_design':'navigation_route_not_road',profile,macro_max_grade:Math.max(...grades),grade_status:'2km_macro_samples_not_engineering_grade',off_land_sample_count:profile.filter(p=>!p.on_land).length};
  });
  json('data/roads.json',{...header,roads:transport.filter(r=>r.mode==='road')});json('data/navigation.json',{...header,routes:transport.filter(r=>r.mode!=='road')});
  const nodes=new Map(),edges=[];
  const node=p=>{const key=p.join(',');if(!nodes.has(key))nodes.set(key,{id:`N${String(nodes.size+1).padStart(4,'0')}`,xy_m:p,settlements:settlements.filter(s=>Math.hypot(s.xy_m[0]-p[0],s.xy_m[1]-p[1])<.001).map(s=>s.id)});return nodes.get(key).id;};
  for(const r of transport)r.geometry_xy_m.slice(1).forEach((p,i)=>edges.push({id:`${r.id}:${i}`,route:r.id,mode:r.mode,from:node(r.geometry_xy_m[i]),to:node(p)}));
  json('data/route-graph.json',{...header,graph_scope:'source vertices; interior intersections require explicit splitting and are not automatically connected',nodes:[...nodes.values()],edges,settlements_without_exact_route_vertex:settlements.filter(s=>![...nodes.values()].some(n=>n.settlements.includes(s.id))).map(s=>s.id)});
  const interactions=[],riverIntersections=[];
  for(const r of transport.filter(r=>r.mode==='road'))for(const h of rivers)for(let i=0;i<r.geometry_xy_m.length-1;i++)for(let j=0;j<h.geometry_xy_m.length-1;j++){
    const hit=intersect(r.geometry_xy_m[i],r.geometry_xy_m[i+1],h.geometry_xy_m[j],h.geometry_xy_m[j+1]);if(hit)interactions.push({id:`X${interactions.length+1}`,road:r.id,river:h.id,road_segment:i,river_segment:j,...hit,required_review:hit.type==='collinear_overlap'?'choose_bank_alignment':'bridge_ford_ferry_or_shared_anchor',resolved:false});
  }
  for(let k=0;k<rivers.length;k++)for(let l=k+1;l<rivers.length;l++){const a=rivers[k],b=rivers[l];for(let i=0;i<a.geometry_xy_m.length-1;i++)for(let j=0;j<b.geometry_xy_m.length-1;j++){
    const hit=intersect(a.geometry_xy_m[i],a.geometry_xy_m[i+1],b.geometry_xy_m[j],b.geometry_xy_m[j+1]);if(hit?.type==='intersection'){
      const za=a.bed_elevation_m[i]*(1-hit.t)+a.bed_elevation_m[i+1]*hit.t,zb=b.bed_elevation_m[j]*(1-hit.u)+b.bed_elevation_m[j+1]*hit.u;
      riverIntersections.push({rivers:[a.id,b.id],xy_m:hit.xy_m,bed_delta_m:Math.abs(za-zb),status:Math.abs(za-zb)>.1?'source_profile_conflict':'same_bed_elevation'});
    }
  }}
  json('data/interactions.json',{...header,road_river:interactions,river_river:riverIntersections,note:'Segment records may repeat at shared vertices; not unique bridge count.'});
  console.log('Writing lossless 16-bit master and identical-border tiles');
  png(path.join(ROOT,'terrain/height16.png'),W,H,16,1,(i,j)=>encoded[j*W+i]);png(path.join(ROOT,'terrain/land-mask.png'),W,H,8,1,(i,j)=>land[j*W+i]);
  const biome=new Uint8Array(N),slope=new Uint8Array(N),density=new Uint8Array(N);
  for(let j=1;j<H-1;j++)for(let i=1;i<W-1;i++){
    const n=j*W+i;if(!land[n])continue;const grade=Math.hypot(z[n+1]-z[n-1],z[n+W]-z[n-W])/(2*STEP),height=encoded[n]/8-1024,[x,y]=xy(i,j);
    slope[n]=clamp(Math.round(Math.atan(grade)*180/Math.PI*255/90),0,255);
    biome[n]=height>2700?5:height>1800||grade>.25?4:x<-1800000&&y<1100000?3:near[n]<20000?2:1;density[n]=[0,160,190,25,20,0][biome[n]];
  }
  for(const [name,values]of [['biome',biome],['slope',slope],['vegetation-density',density]])png(path.join(ROOT,`terrain/${name}.png`),W,H,8,1,(i,j)=>values[j*W+i]);
  const tiles=[];
  for(let ty=0;ty<rows;ty++)for(let tx=0;tx<cols;tx++){
    const file=`terrain/tiles/C${String(tx).padStart(2,'0')}-R${String(ty).padStart(2,'0')}.png`;
    png(path.join(ROOT,file),CELLS+1,CELLS+1,16,1,(i,j)=>encoded[(ty*CELLS+j)*W+tx*CELLS+i]);
    tiles.push({id:`C${tx}-R${ty}`,file,sample_origin:[tx*CELLS,ty*CELLS],origin_xy_m:xy(tx*CELLS,ty*CELLS),size:[CELLS+1,CELLS+1],sha256:hash(path.join(ROOT,file))});
  }
  const PW=1800,PH=Math.round(H/W*PW),palette=[[19,46,55],[77,99,67],[58,96,75],[150,131,89],[128,128,119],[220,225,220]];
  png(path.join(ROOT,'previews/terrain.png'),PW,PH,8,3,(i,j)=>{
    const x=clamp(Math.round(i*(W-1)/(PW-1)),1,W-2),y=clamp(Math.round(j*(H-1)/(PH-1)),1,H-2),n=y*W+x;if(!land[n])return palette[0];
    const dx=(z[n+1]-z[n-1])/(2*STEP),dy=(z[n+W]-z[n-W])/(2*STEP),shade=clamp(.85+(-dx-dy)*5,.35,1.35);
    return palette[biome[n]].map(v=>clamp(Math.round(v*shade+z[n]/500),0,255));
  });
  const manifest={...header,source_files:sources.map(p=>({path:p,sha256:hash(path.join(ROOT,p))})),source_status:g.status,
    grid:{width:W,height:H,spacing_m:STEP,origin_xy_m:[xmin,ymax],row_axis:'south',sample_registration:'grid_vertices',bounds_xy_m:[xmin,ymax-(H-1)*STEP,xmin+(W-1)*STEP,ymax],coastline_bounds_xy_m:bounds},
    height:{file:'terrain/height16.png',encoding:'unsigned 16-bit grayscale PNG; lossless, no color transform',meters_per_unit:.125,offset_m:-1024,formula:'elevation_m = sample * 0.125 - 1024',range_m:[0,maxHeight],ocean:'zero placeholder; use land-mask; no bathymetry',spatial_limit:'2 km regional form only; no building, alley, river-bank or road-grade resolution'},
    layers:{land:{file:'terrain/land-mask.png',values:{0:'ocean',255:'land'}},biome:{file:'terrain/biome.png',status:'rule_candidate_not_climate_simulation',values:{0:'ocean',1:'mixed_forest_grass',2:'river_corridor',3:'western_dryland',4:'rock_upland',5:'alpine'}},vegetation:{file:'terrain/vegetation-density.png',status:'relative_candidate_0_to_255_not_trees_per_m2',exclusions:'Apply vector roads, rivers, world-tree exclusion and future settlement footprints during detail placement.'},slope:{file:'terrain/slope.png',formula:'degrees = sample * 90 / 255',status:'macro_slope_not_walkability'}},
    tile_layout:{columns:cols,rows,cells_per_tile:CELLS,shared_border_samples:1,tiles},
    refinement:{rule:'Child grids inherit parent vertices and a common global-coordinate detail function; never independently generate tile boundaries.',implemented:'coarse master and exact shared-border exports',not_implemented:['meter-scale child DEM','hydraulic erosion/basin simulation','road grading/bridges/bank alignment','building footprints/dimensions','UE import/traversal']},
    counts:{coast_vertices:vs.length,settlements:settlements.length,facilities:settlements.reduce((n,s)=>n+s.facilities.length,0),rivers:rivers.length,roads:transport.filter(r=>r.mode==='road').length,navigation_routes:transport.filter(r=>r.mode!=='road').length,ridges:g.ridges.length,tiles:tiles.length}};
  json('manifest.json',manifest);
  const preview={grid:manifest.grid,settlements:settlements.map(s=>({id:s.id,name:s.name,xy_m:s.xy_m,elevation:s.terrain_elevation_m})),rivers:rivers.map(r=>({id:r.id,points:r.geometry_xy_m})),roads:transport.filter(r=>r.mode==='road').map(r=>({id:r.id,points:r.geometry_xy_m})),coast,ridges:g.ridges.map(r=>({id:r.id,points:r.xy_km.map(meter)})),interactions:interactions.map(x=>({id:x.id,xy_m:x.xy_m,type:x.type})),tiles:tiles.map(t=>t.sample_origin)};
  fs.writeFileSync(path.join(ROOT,'viewer-data.js'),'const DATA='+JSON.stringify(preview)+';\n');
  console.log(JSON.stringify({grid:manifest.grid,counts:manifest.counts,height_range_m:manifest.height.range_m},null,2));
}
if(require.main===module)build();
module.exports={project,inverse,grid,xy,inside,segment,intersect,g,coast,coastSource,ROOT,ART,STEP,W,H,cols,rows,read};
