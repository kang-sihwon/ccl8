const fs=require('node:fs'),path=require('node:path');
const {connect}=require('../VelesDetailArt-v2/capture.cjs');
const root=__dirname,pause=ms=>new Promise(r=>setTimeout(r,ms));
const specs={core:{bounds:[-800,-250,300,450],size:[2200,1400]},west:{bounds:[-1500,150,-300,1550],size:[1500,1750]},east:{bounds:[300,150,1500,1550],size:[1500,1750]}};
(async()=>{for(const d of ['inputs','assets','previews','records','tiles'])fs.mkdirSync(path.join(root,d),{recursive:true});const c=await connect();await c.send('Page.enable');await c.send('Runtime.enable');
 async function navigate(folder,ready){await c.send('Page.navigate',{url:'file:///'+path.resolve(root,'../'+folder+'/index.html').replaceAll('\\','/')});for(let i=0;i<150;i++){if(await c.evaluate(ready))return;await pause(100);}throw Error('page not ready');}
 async function save(name,expression){await c.save('../VelesDetailArt-v3/inputs/'+name+'.png',expression);}
 await navigate('VelesDetailArt-v2','window.artReady');
 for(const[name,s]of Object.entries(specs))await save(name+'-art',`artExport(${JSON.stringify(s.bounds)},${s.size.join(',')})`);
 await c.evaluate(`window.guide=(b,w,h)=>{const out=document.createElement('canvas');out.width=w;out.height=h;paint(out,b,false);const ctx=out.getContext('2d'),s=w/(b[2]-b[0]),p=q=>[(q[0]-b[0])*s,(b[3]-q[1])*s];ctx.lineWidth=2;ctx.strokeStyle='#00ffff';for(const house of SCENE.buildings){ctx.beginPath();house.footprint.forEach((q,i)=>{const[x,y]=p(q);i?ctx.lineTo(x,y):ctx.moveTo(x,y)});ctx.closePath();ctx.stroke();}ctx.strokeStyle='#ffdb66';ctx.lineWidth=1;for(const r of SCENE.roads){ctx.beginPath();r.points.forEach((q,i)=>{const[x,y]=p(q);i?ctx.lineTo(x,y):ctx.moveTo(x,y)});ctx.stroke();}return out.toDataURL('image/png');}`);
 await save('core-guide','guide([-800,-250,300,450],2200,1400)');
 await navigate('VelesDetailTerrain-v1','window.detailDebug?.().ready');
 for(const name of ['west','east']){const s=specs[name];await save(name+'-layout',`detailExport(${JSON.stringify(s.bounds)},${s.size.join(',')},false)`);}
 fs.writeFileSync(path.join(root,'inputs/specs.json'),JSON.stringify(specs,null,2)+'\n');c.ws.close();console.log('Saved registered art and layout inputs');
})().catch(e=>{console.error(e);process.exit(1);});
