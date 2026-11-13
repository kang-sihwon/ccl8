// Rebuild only this derived master and compare every generated data artifact.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),{execFileSync}=require('node:child_process');
const root=__dirname;
const walk=dir=>fs.readdirSync(path.join(root,dir),{withFileTypes:true}).flatMap(e=>e.isDirectory()?walk(path.posix.join(dir,e.name)):[path.posix.join(dir,e.name)]);
const files=[...walk('data'),...walk('terrain'),'manifest.json','viewer-data.js','previews/terrain.png'];
const digest=p=>crypto.createHash('sha256').update(fs.readFileSync(path.join(root,p))).digest('hex');
const before=Object.fromEntries(files.map(p=>[p,digest(p)]));
execFileSync(process.execPath,[path.join(root,'build.cjs')],{stdio:'pipe'});
const differences=files.filter(p=>before[p]!==digest(p));
const report={status:differences.length?'FAIL':'PASS',scope:'same Node runtime, same input files and generator; byte-for-byte rebuild',file_count:files.length,differences,sha256:before};
fs.writeFileSync(path.join(root,'records/rebuild-verification.json'),JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify({status:report.status,file_count:files.length,differences}));if(differences.length)process.exitCode=1;
