const fs=require('node:fs'),path=require('node:path');
const root=__dirname,pause=ms=>new Promise(r=>setTimeout(r,ms));
(async()=>{
 const tabs=await(await fetch('http://127.0.0.1:9445/json')).json(),tab=tabs.find(t=>t.type==='page');
 const ws=new WebSocket(tab.webSocketDebuggerUrl);await new Promise((r,j)=>{ws.onopen=r;ws.onerror=j});let seq=0;const pending=new Map(),errors=[];
 ws.onmessage=e=>{const m=JSON.parse(e.data);if(m.id){const p=pending.get(m.id);pending.delete(m.id);m.error?p.reject(Error(JSON.stringify(m.error))):p.resolve(m.result)}else if(m.method==='Runtime.exceptionThrown')errors.push(m.params.exceptionDetails)};
 const send=(method,params={})=>new Promise((resolve,reject)=>{const id=++seq;pending.set(id,{resolve,reject});ws.send(JSON.stringify({id,method,params}))});
 const evalJs=async expression=>{const r=await send('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(r.exceptionDetails)throw Error(JSON.stringify(r.exceptionDetails));return r.result.value;};
 const until=async exp=>{for(let i=0;i<100;i++){if(await evalJs(exp))return true;await pause(150)}return false;};
 const checks=[],check=(name,pass,detail)=>checks.push({name,pass:!!pass,detail});
 const shot=async name=>{await pause(300);const r=await send('Page.captureScreenshot',{format:'png',clip:{x:0,y:0,width:1672,height:941,scale:1}});fs.writeFileSync(path.join(root,'previews',name+'.png'),Buffer.from(r.data,'base64'));};
 await send('Page.enable');await send('Runtime.enable');await send('Emulation.setDeviceMetricsOverride',{width:1672,height:941,deviceScaleFactor:1,mobile:false});
 await send('Page.navigate',{url:'file:///'+path.join(root,'index.html').replaceAll('\\','/')});
 check('terrain textures and WebGL compile',await until('window.terrainDebug?.().loaded'),await evalJs('window.terrainDebug?.()'));
 await until('window.mapDebug?.().ready');await shot('B04-overview');
 for(const [name,exp] of [['B04-middle','pilotMiddle()'],['B04-close','pilotMiddle(.5,1.1)'],['T11-junction','pilotJunction()'],['T11-core','mapFocus("T11")'],['V14-core','mapFocus("V14")'],['V14-junction','pilotMiddle(.035,.3)'],['B04-quarter','pilotMiddle(.25,.7)'],['B04-three-quarter','pilotMiddle(.75,.7)']]){
   await evalJs(exp);await pause(1400);check(name+' renders without terrain error',!(await evalJs('terrainDebug().error')),await evalJs('terrainDebug()'));await shot(name);
 }
 await evalJs('pilotMiddle()');await pause(300);const before=await evalJs('mapDebug()');
 await send('Input.dispatchMouseEvent',{type:'mousePressed',x:800,y:450,button:'left',clickCount:1});await send('Input.dispatchMouseEvent',{type:'mouseMoved',x:920,y:510,button:'left',buttons:1});await send('Input.dispatchMouseEvent',{type:'mouseReleased',x:920,y:510,button:'left',clickCount:1});await pause(300);
 const after=await evalJs('mapDebug()');check('drag preserves world-space coordinates',Math.abs((before.cx-after.cx)*before.zoom*before.aspect-120)<1&&Math.abs((before.cy-after.cy)*before.zoom-60)<1);
 await send('Input.dispatchMouseEvent',{type:'mouseWheel',x:800,y:450,deltaX:0,deltaY:-220});await pause(500);check('wheel zoom works',await evalJs('mapDebug().zoom')>after.zoom);
 await evalJs('document.querySelector("#compare").click()');await pause(300);check('reference comparison toggle',await evalJs('mapDebug().compare'));await shot('B04-reference');
 await evalJs('document.querySelector("#compare").click();pilotMiddle()');await pause(350);
 const b=await evalJs('(()=>{const p=pilot.worldPixelPoints;const x=Math.floor((p[0][0]+p[1][0])/2),y=Math.floor((p[0][1]+p[1][1])/2);return[x,y,x+2048,y+1024]})()');
 for(const [name,w,view] of [['material-whole',1024,b],['material-left',512,[b[0],b[1],b[0]+1024,b[3]]],['material-right',512,[b[0]+1024,b[1],b[2],b[3]]],['material-repeat',1024,b]]){
   const data=await evalJs(`terrainCapture(${w},512,${JSON.stringify(view)})`);fs.writeFileSync(path.join(root,'previews',name+'.png'),Buffer.from(data.split(',')[1],'base64'));
 }
 await evalJs('pilotFrame()');await pause(300);check('no JavaScript, image or GPU errors',errors.length===0&&(await evalJs('mapDebug().errors.length'))===0&&!(await evalJs('terrainDebug().error')),{errors,images:await evalJs('mapDebug().errors')});
 const report={status:checks.every(c=>c.pass)?'PASS':'FAIL',scope:'prototype_render_input_and_texture_determinism_inputs_only',checks};
 fs.writeFileSync(path.join(root,'records/browser-verification.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report));
 ws.close();
})().catch(e=>{console.error(e);process.exit(1)});
