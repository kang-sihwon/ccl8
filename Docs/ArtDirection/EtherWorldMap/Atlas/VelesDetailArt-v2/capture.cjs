const fs=require('node:fs'),path=require('node:path');
const root=__dirname,wait=ms=>new Promise(r=>setTimeout(r,ms));
async function connect(){
 const tabs=await(await fetch('http://127.0.0.1:9448/json')).json(),tab=tabs.find(t=>t.type==='page');
 const ws=new WebSocket(tab.webSocketDebuggerUrl);await new Promise((r,j)=>{ws.onopen=r;ws.onerror=j;});let seq=0;const pending=new Map();
 ws.onmessage=e=>{const m=JSON.parse(e.data);if(m.id){const p=pending.get(m.id);pending.delete(m.id);m.error?p.reject(Error(JSON.stringify(m.error))):p.resolve(m.result);}};
 ws.onclose=()=>{for(const p of pending.values())p.reject(Error('CDP closed'));};
 const send=(method,params={})=>new Promise((resolve,reject)=>{const id=++seq;pending.set(id,{resolve,reject});ws.send(JSON.stringify({id,method,params}));});
 const evaluate=async expression=>{const r=await send('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(r.exceptionDetails)throw Error(JSON.stringify(r.exceptionDetails));return r.result.value;};
 async function save(file,expression){const size=await evaluate('(window.captureData='+expression+').length'),parts=[];for(let o=0;o<size;o+=524288)parts.push(await evaluate(`captureData.slice(${o},${o+524288})`));fs.writeFileSync(path.join(root,file),Buffer.from(parts.join('').split(',')[1],'base64'));await evaluate('captureData=null');}
 return{ws,send,evaluate,save};
}
async function main(){
 for(const d of ['inputs','assets','previews','records','tiles'])fs.mkdirSync(path.join(root,d),{recursive:true});
 const c=await connect();await c.send('Page.enable');await c.send('Runtime.enable');await c.send('Emulation.setDeviceMetricsOverride',{width:1672,height:941,deviceScaleFactor:1,mobile:false});
 if(process.argv.includes('--input')){
  await c.send('Page.navigate',{url:'file:///'+path.resolve(root,'../VelesDetailTerrain-v1/index.html').replaceAll('\\','/')});
  for(let i=0;i<100;i++){if(await c.evaluate('window.detailDebug?.().ready'))break;await wait(150);}
  await c.save('inputs/core.png','detailExport([-800,-250,300,450],2200,1400,false)');
  await c.save('inputs/forest.png','detailExport([-600,350,600,1550],1800,1800,false)');
 }else{
  await c.send('Page.navigate',{url:'file:///'+path.join(root,'index.html').replaceAll('\\','/')});
  for(let i=0;i<100;i++){if(await c.evaluate('window.artReady'))break;await wait(150);}
  if(!await c.evaluate('window.artReady'))throw Error('Art viewer failed to load');
  for(const frame of ['village','forest','all']){await c.evaluate(`artFrame('${frame}')`);await wait(200);const p=await c.send('Page.captureScreenshot',{format:'png'});fs.writeFileSync(path.join(root,'previews',frame+'.png'),Buffer.from(p.data,'base64'));}
  await c.evaluate('artFrame("village");document.querySelector("#geometry").click()');await wait(200);const p=await c.send('Page.captureScreenshot',{format:'png'});fs.writeFileSync(path.join(root,'previews/geometry.png'),Buffer.from(p.data,'base64'));
  console.log(await c.evaluate('artDebug()'));
  const checks=[];const check=(name,pass)=>checks.push({name,pass});
  check('renderer loads',await c.evaluate('artReady && !artDebug().errors.length'));
  check('geometry overlay toggles',await c.evaluate('artDebug().geometry'));
  await c.evaluate('document.querySelector("#geometry").click();document.querySelector("#grid").click()');check('tile boundary overlay toggles',await c.evaluate('artDebug().grid'));
  await c.evaluate('document.querySelector("#grid").click();document.querySelector("#before").click()');check('before comparison toggles',await c.evaluate('artDebug().before'));await c.evaluate('document.querySelector("#before").click()');
  const initial=await c.evaluate('artDebug()');await c.send('Input.dispatchMouseEvent',{type:'mouseWheel',x:800,y:440,deltaY:-120,deltaX:0});await wait(100);const zoomed=await c.evaluate('artDebug()');check('actual wheel zoom',zoomed.zoom>initial.zoom);
  await c.send('Input.dispatchMouseEvent',{type:'mousePressed',x:800,y:440,button:'left',clickCount:1});await c.send('Input.dispatchMouseEvent',{type:'mouseMoved',x:900,y:480,button:'left',buttons:1});await c.send('Input.dispatchMouseEvent',{type:'mouseReleased',x:900,y:480,button:'left',clickCount:1});const moved=await c.evaluate('artDebug()');check('actual pointer pan',Math.abs((zoomed.cx-moved.cx)*moved.zoom-100)<1);
  await c.save('previews/composite.png','artExport([-2048,-2560,2048,3584],2048,3072)');
  check('no JavaScript errors',await c.evaluate('!artDebug().errors.length'));
  fs.writeFileSync(path.join(root,'records/browser-verification.json'),JSON.stringify({status:checks.every(c=>c.pass)?'PASS':'FAIL',checks},null,2)+'\n');
 }
 c.ws.close();
}
if(require.main===module)main().catch(e=>{console.error(e);process.exit(1);});
module.exports={connect};
