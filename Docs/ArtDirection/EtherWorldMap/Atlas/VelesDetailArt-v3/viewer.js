'use strict';
const B=[-2048,-2560,2048,3584],CORE=[-800,-250,300,450],FOREST=[-600,350,600,1550],canvas=document.querySelector('#map');
const photos={},errors=[];let before=false,geometry=false,grid=false,cx=-120,cy=130,zoom=1,view='village',drag=null;
const frames={village:[-400,-40,160,300],forest:[-1260,190,1360,1510],all:B};
function load(src){return new Promise((r,j)=>{const i=new Image();i.onload=()=>r(i);i.onerror=()=>j(Error('image load failed'));i.src=src;});}
function feather(img,bounds,fade,lock=0){
 const c=document.createElement('canvas');c.width=img.width;c.height=img.height;const ctx=c.getContext('2d');ctx.drawImage(img,0,0);
 ctx.globalCompositeOperation='destination-in';
 // Separate axis masks multiply; the locked frame remains transparent.
 for(const vertical of [false,true]){const n=vertical?c.height:c.width,meters=vertical?bounds[3]-bounds[1]:bounds[2]-bounds[0],f=fade/meters,l=lock/meters,g=ctx.createLinearGradient(0,0,vertical?0:n,vertical?n:0);g.addColorStop(0,'transparent');g.addColorStop(l,'transparent');g.addColorStop(l+f,'white');g.addColorStop(1-l-f,'white');g.addColorStop(1-l,'transparent');g.addColorStop(1,'transparent');ctx.fillStyle=g;ctx.fillRect(0,0,c.width,c.height);}
 return c;
}
function paint(target,bounds,overlay=true){
 const ctx=target.getContext('2d'),w=target.width,h=target.height,s=w/(bounds[2]-bounds[0]),p=q=>[(q[0]-bounds[0])*s,(bounds[3]-q[1])*s];
 ctx.fillStyle='#202a27';ctx.fillRect(0,0,w,h);
 function photo(img,b){const[x,y]=p([b[0],b[3]]);ctx.drawImage(img,x,y,(b[2]-b[0])*s,(b[3]-b[1])*s);}
 if(before){photo(photos.base,B);}else{for(const layer of ART_LAYERS)photo(photos[layer.id],layer.bounds);}
 function line(points,color,width=1){ctx.strokeStyle=color;ctx.lineWidth=width;ctx.beginPath();points.forEach((q,i)=>{const[x,y]=p(q);i?ctx.lineTo(x,y):ctx.moveTo(x,y);});ctx.stroke();}
 if(overlay&&grid){for(let x=B[0];x<=B[2];x+=1024)line([[x,B[1]],[x,B[3]]],'#c6d8d080');for(let y=B[1];y<=B[3];y+=1024)line([[B[0],y],[B[2],y]],'#c6d8d080');}
 if(overlay&&geometry){for(const r of SCENE.roads)line(r.points,'#e9bd7599');for(const b of SCENE.buildings)line([...b.footprint,b.footprint[0]],'#82e1e8');line(SCENE.river.points,'#82e1e8');line(SCENE.stream.points,'#82e1e8');}
 if(overlay){ctx.font='18px "Malgun Gothic",sans-serif';const offsets={workshop:[45,-48],quay:[40,35],water_point:[-150,24],water_store:[-160,-42],forest_gate:[40,-25],cave:[40,-30],spring:[45,-30],local_grove_tree:[-170,-28]};for(const poi of SCENE.pois){if(!poi.visible||poi.surface!=='ground')continue;if(s<.35&&!['workshop','quay','cave'].includes(poi.id))continue;const[x,y]=p(poi.point);if(x<16||x>w-210||y<35||y>h-35)continue;const[dX,dY]=offsets[poi.id]||[40,-25],tx=x+dX,ty=y+dY,end=dX<0?tx+ctx.measureText(poi.name).width+7:tx-7;ctx.strokeStyle='#c4ccc6';ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(x,y);ctx.lineTo(end+(dX<0?12:-12),ty-6);ctx.lineTo(end,ty-6);ctx.stroke();if(poi.id==='cave'){ctx.strokeStyle='#d7b56b';ctx.strokeRect(x-3,y-3,6,6);}else{ctx.fillStyle=poi.id==='workshop'?'#92e7ef':'#dce0dc';ctx.beginPath();ctx.arc(x,y,3.5,0,Math.PI*2);ctx.fill();}ctx.strokeStyle='#202823';ctx.lineWidth=3;ctx.strokeText(poi.name,tx,ty);ctx.fillStyle='#efefea';ctx.fillText(poi.name,tx,ty);}}
}
function draw(){if(!window.artReady)return;paint(canvas,[cx-canvas.width/zoom/2,cy-canvas.height/zoom/2,cx+canvas.width/zoom/2,cy+canvas.height/zoom/2]);document.querySelector('#status').textContent=(before?'좌표 원본':'항공사진 화풍 보정')+' · '+(1/zoom).toFixed(2)+'m/화면px';}
window.artFrame=name=>{view=name;const b=frames[name];cx=(b[0]+b[2])/2;cy=(b[1]+b[3])/2;zoom=Math.min(canvas.width/(b[2]-b[0]),canvas.height/(b[3]-b[1]));document.querySelector('#title').textContent=name==='village'?'벨레스 일대':name==='forest'?'숲·동굴 연결 구간':'벨레스 전체 연결 구역';draw();};
window.artDebug=()=>({ready:window.artReady,view,cx,cy,zoom,before,geometry,grid,errors});
window.artExport=(b,w,h)=>{const c=document.createElement('canvas');c.width=w;c.height=h;paint(c,b,false);return c.toDataURL('image/png');};
for(const id of ['before','geometry','grid'])document.querySelector('#'+id).onclick=e=>{if(id==='before')before=!before;if(id==='geometry')geometry=!geometry;if(id==='grid')grid=!grid;e.target.setAttribute('aria-pressed',String(id==='before'?before:id==='geometry'?geometry:grid));draw();};
document.querySelectorAll('[data-frame]').forEach(b=>b.onclick=()=>artFrame(b.dataset.frame));
canvas.onwheel=e=>{e.preventDefault();const r=canvas.getBoundingClientRect(),x=e.clientX-r.left-canvas.width/2,y=e.clientY-r.top-canvas.height/2,old=zoom;zoom=Math.max(.06,Math.min(6,zoom*Math.exp(-e.deltaY*.0015)));cx+=x/old-x/zoom;cy-=y/old-y/zoom;draw();};
canvas.onpointerdown=e=>{drag=[e.clientX,e.clientY,cx,cy];canvas.setPointerCapture(e.pointerId);};canvas.onpointermove=e=>{if(drag){cx=drag[2]-(e.clientX-drag[0])/zoom;cy=drag[3]+(e.clientY-drag[1])/zoom;draw();}};canvas.onpointerup=canvas.onpointercancel=()=>drag=null;
function resize(){canvas.width=innerWidth;canvas.height=innerHeight-100;if(window.artReady)artFrame(view);}window.onresize=resize;window.onerror=e=>errors.push(String(e));
Promise.all(Object.entries(ART_ASSETS).map(async([k,v])=>photos[k]=await load(v))).then(()=>{for(const layer of ART_LAYERS)if(layer.fade)photos[layer.id]=feather(photos[layer.id],layer.bounds,layer.fade,layer.lock);window.artReady=true;resize();}).catch(e=>{errors.push(String(e));document.querySelector('#status').textContent=String(e);});
