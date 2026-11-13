// Use a separately launched headless browser with a temporary profile on port 9446.
const fs=require('node:fs'),path=require('node:path');
const pause=ms=>new Promise(r=>setTimeout(r,ms));
(async()=>{
 const tabs=await(await fetch('http://127.0.0.1:9446/json')).json(),tab=tabs.find(t=>t.type==='page');
 const ws=new WebSocket(tab.webSocketDebuggerUrl);await new Promise((r,j)=>{ws.onopen=r;ws.onerror=j;});
 let seq=0;const pending=new Map(),errors=[],checks=[];
 ws.onmessage=e=>{const m=JSON.parse(e.data);if(m.id){const p=pending.get(m.id);pending.delete(m.id);m.error?p.reject(Error(JSON.stringify(m.error))):p.resolve(m.result);}else if(m.method==='Runtime.exceptionThrown')errors.push(m.params.exceptionDetails);};
 const send=(method,params={})=>new Promise((resolve,reject)=>{const id=++seq;pending.set(id,{resolve,reject});ws.send(JSON.stringify({id,method,params}));});
 const evalJs=async expression=>{const r=await send('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(r.exceptionDetails)throw Error(JSON.stringify(r.exceptionDetails));return r.result.value;};
 const check=(name,pass,detail)=>checks.push({name,pass:!!pass,detail});
 const shot=async name=>{await pause(100);const r=await send('Page.captureScreenshot',{format:'png'});fs.writeFileSync(path.join(__dirname,'previews',name+'.png'),Buffer.from(r.data,'base64'));};
 await send('Runtime.enable');await send('Page.enable');await send('Emulation.setDeviceMetricsOverride',{width:1672,height:941,deviceScaleFactor:1,mobile:false});
 await send('Page.navigate',{url:'file:///'+path.join(__dirname,'index.html').replaceAll('\\','/')});
 for(let i=0;i<60;i++){if(await evalJs('window.masterDebug?.().loaded'))break;await pause(100);}
 const first=await evalJs('masterDebug()');check('terrain loaded and all 36 settlements selectable',first.loaded&&first.settlements===36&&await evalJs('document.querySelector("#place").options.length===37'),first);await shot('viewer-overview');
 check('river and road inventory',first.rivers===14&&first.roads===20);
 const a=(await send('Page.captureScreenshot',{format:'png'})).data;await evalJs('document.querySelector("#tiles").click()');check('grid overlay changes rendered image',a!==(await send('Page.captureScreenshot',{format:'png'})).data);await shot('viewer-tiles');
 await evalJs('document.querySelector("#tiles").click()');
 for(const id of await evalJs('DATA.settlements.map(s=>s.id)')){await evalJs(`masterFocus(${JSON.stringify(id)})`);if(!(await evalJs(`masterDebug().selected===${JSON.stringify(id)}`)))throw Error('focus failed '+id);}
 check('all 36 settlement selection targets work',true);await evalJs('masterFocus("V01")');await shot('viewer-veles');
 const before=await evalJs('masterDebug()');
 await send('Input.dispatchMouseEvent',{type:'mouseWheel',x:600,y:400,deltaY:-180,deltaX:0});await pause(150);const zoomed=await evalJs('masterDebug()');check('real wheel zoom changes scale',zoomed.zoom>before.zoom);
 await send('Input.dispatchMouseEvent',{type:'mousePressed',x:600,y:400,button:'left',clickCount:1});await send('Input.dispatchMouseEvent',{type:'mouseMoved',x:700,y:430,button:'left',buttons:1});await send('Input.dispatchMouseEvent',{type:'mouseReleased',x:700,y:430,button:'left',clickCount:1});await pause(150);
 const after=await evalJs('masterDebug()');check('real drag pans by pointer displacement',Math.abs(after.ox-zoomed.ox-100)<1&&Math.abs(after.oy-zoomed.oy-30)<1);
 await evalJs('document.querySelector("#home").click()');check('home restores overall view',Math.abs((await evalJs('masterDebug().zoom'))-first.zoom)<1e-6);
 check('no JavaScript or asset errors',errors.length===0&&(await evalJs('masterDebug().errors.length'))===0,errors);
 const report={status:checks.every(c=>c.pass)?'PASS':'FAIL',scope:'local-file viewer, layer rendering, all settlement selections, wheel and drag; not UE validation',checks};
 fs.writeFileSync(path.join(__dirname,'records/browser-verification.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report));
 await send('Browser.close');ws.close();if(report.status!=='PASS')process.exitCode=1;
})().catch(e=>{console.error(e);process.exit(1);});
