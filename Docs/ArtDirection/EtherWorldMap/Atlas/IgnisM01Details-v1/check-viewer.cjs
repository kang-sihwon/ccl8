const fs=require('node:fs'),path=require('node:path');
const root=__dirname,port=9443;
const pause=ms=>new Promise(r=>setTimeout(r,ms));
(async()=>{
let tabs;for(let i=0;i<40;i++){try{tabs=await(await fetch(`http://127.0.0.1:${port}/json`)).json();break;}catch{await pause(250)}}
if(!tabs)throw Error('Browser debugging endpoint unavailable');
const tab=tabs.find(t=>t.type==='page'),ws=new WebSocket(tab.webSocketDebuggerUrl);
await new Promise((r,j)=>{ws.onopen=r;ws.onerror=j});let n=0;const pending=new Map(),errors=[];
ws.onmessage=e=>{const m=JSON.parse(e.data);if(m.id){const p=pending.get(m.id);pending.delete(m.id);m.error?p.reject(Error(JSON.stringify(m.error))):p.resolve(m.result);}else if(m.method==='Runtime.exceptionThrown')errors.push(m.params.exceptionDetails);};
function send(method,params={}){return new Promise((resolve,reject)=>{const id=++n;pending.set(id,{resolve,reject});ws.send(JSON.stringify({id,method,params}));})}
async function evalJs(expression){return(await send('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true})).result.value;}
await send('Page.enable');await send('Runtime.enable');await send('Emulation.setDeviceMetricsOverride',{width:1672,height:941,deviceScaleFactor:1,mobile:false});
await send('Page.navigate',{url:'about:blank'});
for(let i=0;i<80;i++){if(await evalJs('location.href==="about:blank"&&!window.mapDebug'))break;await pause(50);}
await send('Page.navigate',{url:'file:///'+path.join(root,'index.html').replaceAll('\\','/')});
for(let i=0;i<120;i++){if(await evalJs('window.mapDebug?.().ready'))break;await pause(250);}
const checks=[];const check=(name,pass,detail)=>checks.push({name,pass:!!pass,detail});let state=await evalJs('window.mapDebug?.()');check('12 tiles loaded',state?.ready&&state.loaded===12,state);
async function shot(name){await pause(350);const r=await send('Page.captureScreenshot',{format:'png',fromSurface:true,captureBeyondViewport:true,clip:{x:0,y:0,width:1672,height:941,scale:1}});fs.writeFileSync(path.join(root,'previews',name),Buffer.from(r.data,'base64'));}
await shot('m01-ui-overview.png');
await evalJs('document.querySelector("#grid").click()');check('grid toggles',await evalJs('mapDebug().grid'));await shot('m01-ui-grid.png');await evalJs('document.querySelector("#grid").click()');
await evalJs('document.querySelector("#focus").click()');const cityZoom=await evalJs('mapDebug().zoom');await shot('m01-ui-city.png');check('city zoom persists in captured view',cityZoom>state.zoom&&Math.abs((await evalJs('mapDebug().zoom'))-cityZoom)<.0001,{beforeCapture:cityZoom,afterCapture:await evalJs('mapDebug().zoom')});
const pre=await evalJs('mapDebug()');await send('Input.dispatchMouseEvent',{type:'mousePressed',x:800,y:450,button:'left',clickCount:1});await send('Input.dispatchMouseEvent',{type:'mouseMoved',x:920,y:510,button:'left',buttons:1});await send('Input.dispatchMouseEvent',{type:'mouseReleased',x:920,y:510,button:'left',clickCount:1});const post=await evalJs('mapDebug()');check('drag moves camera by delivered input',Math.abs(post.ox-pre.ox-120)<1&&Math.abs(post.oy-pre.oy-60)<1,{pre,post});
await send('Input.dispatchMouseEvent',{type:'mouseMoved',x:800,y:450});await pause(100);await send('Input.dispatchMouseEvent',{type:'mouseWheel',x:800,y:450,deltaX:0,deltaY:-500});await pause(1000);const wheel=await evalJs('({state:mapDebug(),scrollY,rect:document.querySelector("#map").getBoundingClientRect().toJSON(),target:document.elementFromPoint(800,450)?.tagName})');check('wheel zoom changes scale',wheel.state.zoom>post.zoom,wheel);
await evalJs('document.querySelector("#labels").click()');check('labels toggle off',await evalJs('!mapDebug().labels'));await shot('m01-ui-close.png');
await send('Input.dispatchKeyEvent',{type:'keyDown',key:'Escape',code:'Escape',windowsVirtualKeyCode:27});await send('Input.dispatchKeyEvent',{type:'keyUp',key:'Escape',code:'Escape',windowsVirtualKeyCode:27});check('Escape returns to fit',Math.abs((await evalJs('mapDebug().zoom'))-state.zoom)<.0001);
check('no runtime exceptions',errors.length===0,errors);const report={status:checks.every(c=>c.pass)?'PASS':'FAIL',checks};fs.writeFileSync(path.join(root,'records/viewer-verification.json'),JSON.stringify(report,null,2));console.log(JSON.stringify(report));await send('Browser.close').catch(()=>{});ws.close();
})().catch(e=>{console.error(e);process.exitCode=1;});
