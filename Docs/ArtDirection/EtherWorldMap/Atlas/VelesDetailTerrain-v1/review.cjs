const fs=require('node:fs'),path=require('node:path');
const root=__dirname,pause=ms=>new Promise(r=>setTimeout(r,ms));
(async()=>{
 const tabs=await(await fetch('http://127.0.0.1:9447/json')).json(),tab=tabs.find(t=>t.type==='page'&&t.url.includes('VelesDetailTerrain-v1'))||tabs.find(t=>t.type==='page');
 const ws=new WebSocket(tab.webSocketDebuggerUrl);await new Promise((r,j)=>{ws.onopen=r;ws.onerror=j;});let seq=0;const pending=new Map(),errors=[];
 ws.onmessage=e=>{const m=JSON.parse(e.data);if(m.id){const p=pending.get(m.id);pending.delete(m.id);m.error?p.reject(Error(JSON.stringify(m.error))):p.resolve(m.result);}else if(m.method==='Runtime.exceptionThrown')errors.push(m.params.exceptionDetails);};
 ws.onclose=e=>{for(const p of pending.values())p.reject(Error('CDP closed '+e.code+' '+e.reason));};
 const send=(method,params={})=>new Promise((resolve,reject)=>{const id=++seq;pending.set(id,{resolve,reject});ws.send(JSON.stringify({id,method,params}));});
 const evaluate=async expression=>{const r=await send('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(r.exceptionDetails)throw Error(JSON.stringify(r.exceptionDetails));return r.result.value;};
 const shot=async name=>{await pause(250);const r=await send('Page.captureScreenshot',{format:'png'});fs.writeFileSync(path.join(root,'previews',name+'.png'),Buffer.from(r.data,'base64'));};
 await send('Runtime.enable');await send('Page.enable');await send('Emulation.setDeviceMetricsOverride',{width:1672,height:941,deviceScaleFactor:1,mobile:false});
 await send('Page.navigate',{url:'file:///'+path.join(root,'index.html').replaceAll('\\','/')});
 for(let i=0;i<100;i++){if(await evaluate('window.detailDebug?.().ready'))break;await pause(150);}
 const state=await evaluate('window.detailDebug?.()');if(!state?.ready)throw Error('Renderer did not become ready: '+JSON.stringify({state,errors}));
 await shot('village');console.log(JSON.stringify(state));
 if(process.argv.includes('--preview')){ws.close();return;}
 const checks=[],check=(name,pass,detail)=>checks.push({name,pass:!!pass,detail});
 check('assets and renderer load',state.ready&&state.errors.length===0,state);
 for(const name of ['forest','west','north','all']){await evaluate(`detailFrame('${name}')`);await shot(name);check(name+' view renders',await evaluate(`detailDebug().view==='${name}'&&detailDebug().errors.length===0`));}
 await evaluate('detailFrame("village");document.querySelector("#footprints").click()');await shot('building-footprints');check('footprint overlay toggles',await evaluate('detailDebug().footprints'));await evaluate('document.querySelector("#footprints").click()');
 console.log('Checking actual input');
 const before=await evaluate('detailDebug()');await send('Input.dispatchMouseEvent',{type:'mouseWheel',x:780,y:430,deltaY:-180,deltaX:0});await pause(250);const zoomed=await evaluate('detailDebug()');check('real wheel zoom',zoomed.zoom>before.zoom);
 await send('Input.dispatchMouseEvent',{type:'mousePressed',x:780,y:430,button:'left',clickCount:1});await send('Input.dispatchMouseEvent',{type:'mouseMoved',x:880,y:470,button:'left',buttons:1});await send('Input.dispatchMouseEvent',{type:'mouseReleased',x:880,y:470,button:'left',clickCount:1});await pause(250);const moved=await evaluate('detailDebug()');check('real pointer drag',Math.abs((zoomed.cx-moved.cx)*moved.zoom-100)<1&&Math.abs((moved.cy-zoomed.cy)*moved.zoom-40)<1);
 await evaluate('detailFrame("village")');
 const exportImage=async(file,bounds,w,h,labels=false,grid=false)=>{
   const size=await evaluate(`(window.exportResult=detailExport(${JSON.stringify(bounds)},${w},${h},${labels},${grid})).length`),parts=[];
   for(let offset=0;offset<size;offset+=524288)parts.push(await evaluate(`window.exportResult.slice(${offset},${offset+524288})`));
   const uri=parts.join('');fs.writeFileSync(path.join(root,file),Buffer.from(uri.split(',')[1],'base64'));await evaluate('window.exportResult=null');
 };
 if(process.argv.includes('--export')){
   console.log('Exporting native detail tiles');
   const manifest=JSON.parse(fs.readFileSync(path.join(root,'manifest.json')));
   for(const t of manifest.tile_layout.tiles){const b=t.bounds,margin=16;await exportImage(t.texture,[b[0]-margin,b[1]-margin,b[2]+margin,b[3]+margin],2112,2112);console.log('Exported '+t.id);}
   // verify.py assembles the exported tile cores into the overview previews.
   await exportImage('previews/village-detail.png',[-400,-30,120,230],2080,1040,true);
   // Two independent renders of the same world overlap, from neighboring contexts.
   await exportImage('previews/seam-west.png',[-1056,1440,32,2528],2176,2176);
   await exportImage('previews/seam-east.png',[-32,1440,1056,2528],2176,2176);
 }
 check('no JavaScript errors',errors.length===0&&(await evaluate('detailDebug().errors.length'))===0,errors);
 const report={status:checks.every(c=>c.pass)?'PASS':'FAIL',scope:'browser asset rendering and actual input, not UE',checks};fs.writeFileSync(path.join(root,'records/browser-verification.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify({status:report.status,checks:checks.length}));
 if(process.argv.includes('--close'))await send('Browser.close');ws.close();if(report.status!=='PASS')process.exitCode=1;
})().catch(e=>{console.error(e);process.exit(1);});
