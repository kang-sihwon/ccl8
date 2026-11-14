const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=__dirname,old=path.resolve(root,'../VelesDetailTerrain-v1');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const m={status:'aerial_detail_extension_and_registration_candidate',bounds:[-2048,-2560,2048,3584],origin:[150000,145000],geometry_sha256:{},images:{},layers:[
 {id:'base',file:'../VelesDetailTerrain-v1/previews/assembled.png',bounds:[-2048,-2560,2048,3584],fade:0,lock:0},
 {id:'region',file:'../VelesDetailArt-v2/assets/region-refined.png',bounds:[-2048,-2560,2048,3584],fade:96,lock:8},
 {id:'west',file:'assets/west.png',bounds:[-1500,150,-300,1550],fade:100,lock:0},
 {id:'east',file:'assets/east.png',bounds:[300,150,1500,1550],fade:100,lock:0},
 {id:'forest',file:'../VelesDetailArt-v2/assets/forest.png',bounds:[-600,350,600,1550],fade:90,lock:0},
 {id:'core',file:'assets/core.png',bounds:[-800,-250,300,450],fade:70,lock:0}
 ]};
for(const f of ['manifest.json','data/placements.json','data/connections.json','height/master16.png','scene.js'])m.geometry_sha256[f]=hash(path.join(old,f));
const bundle={};for(const l of m.layers){const p=path.join(root,l.file),b=fs.readFileSync(p);bundle[l.id]='data:image/png;base64,'+b.toString('base64');l.width=b.readUInt32BE(16);l.height=b.readUInt32BE(20);l.mpp=[(l.bounds[2]-l.bounds[0])/l.width,(l.bounds[3]-l.bounds[1])/l.height];m.images[l.id]={file:l.file,width:l.width,height:l.height,sha256:hash(p)};}
fs.writeFileSync(path.join(root,'manifest.json'),JSON.stringify(m,null,2)+'\n');
fs.writeFileSync(path.join(root,'art-assets.js'),'const ART_ASSETS='+JSON.stringify(bundle)+';\nconst ART_LAYERS='+JSON.stringify(m.layers)+';\n');
console.log(m.layers.map(l=>({id:l.id,size:[l.width,l.height],mpp:l.mpp})));
