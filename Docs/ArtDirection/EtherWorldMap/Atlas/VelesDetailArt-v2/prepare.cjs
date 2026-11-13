const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=__dirname,old=path.resolve(root,'../VelesDetailTerrain-v1');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const images={base:'../VelesDetailTerrain-v1/previews/assembled.png',region:'assets/region-refined.png',forest:'assets/forest.png',core:'assets/core.png'};
const manifest={status:'photographic_art_revision_candidate',origin:[150000,145000],bounds:[-2048,-2560,2048,3584],core_bounds:[-800,-250,300,450],source_geometry:'../VelesDetailTerrain-v1/manifest.json',geometry_sha256:{},images:{},region_edge_lock_m:8,region_transition_m:96,core_transition_m:70,limitations:['Source coordinates and height files are unchanged; painted roof silhouettes are not exact surveyed geometry.','The regional image is 4m per native pixel; core image is approximately 0.7m per native pixel. Export resolution does not create new photographic detail.','Outer regional image edge is inherited from the original assembled preview. Internal tiles are crops of one common composite.','No UE or gameplay validation; no new neighboring settlement centers.']};
for(const f of ['manifest.json','data/placements.json','data/connections.json','height/master16.png','scene.js'])manifest.geometry_sha256[f]=hash(path.join(old,f));
const bundle={};for(const[k,f]of Object.entries(images)){const b=fs.readFileSync(path.join(root,f));bundle[k]='data:image/png;base64,'+b.toString('base64');manifest.images[k]={file:f,width:b.readUInt32BE(16),height:b.readUInt32BE(20),sha256:hash(path.join(root,f))};}
fs.writeFileSync(path.join(root,'art-assets.js'),'const ART_ASSETS='+JSON.stringify(bundle)+';\n');
manifest.forest_bounds=[-600,350,600,1550];manifest.forest_transition_m=90;
fs.writeFileSync(path.join(root,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(JSON.stringify(manifest.images));
