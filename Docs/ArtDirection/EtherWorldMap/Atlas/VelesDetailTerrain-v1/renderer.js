'use strict';
const groundCanvas=document.createElement('canvas'),gl=groundCanvas.getContext('webgl2',{preserveDrawingBuffer:true,antialias:false});
const errors=[],images={};let ready=false;
function shader(type,source){const s=gl.createShader(type);gl.shaderSource(s,source);gl.compileShader(s);if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))throw Error(gl.getShaderInfoLog(s));return s;}
const vertex=`#version 300 es
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}`;
const fragment=`#version 300 es
precision highp float;
uniform sampler2D uMaterial,uHeight;
uniform vec4 uBounds;
uniform float uMpp;
uniform vec3 uRiver[24];
uniform int uRiverCount;
uniform vec2 uStream[16];
uniform int uStreamCount;
out vec4 outColor;
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
vec3 texCell(vec2 p,vec2 key,vec2 quad){vec2 r=vec2(hash(key+71.),hash(key+39.));vec2 uv=abs(fract(p*.5+r)*2.-1.);return texture(uMaterial,quad+vec2(.012)+uv*.476).rgb;}
vec3 material(vec2 p,vec2 quad){p/=72.;vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(texCell(p,i,quad),texCell(p,i+vec2(1,0),quad),f.x),mix(texCell(p,i+vec2(0,1),quad),texCell(p,i+vec2(1,1),quad),f.x),f.y);}
float elevation(vec2 p){vec2 uv=vec2((p.x+2048.)/4096.,(3584.-p.y)/6144.);vec2 rg=texture(uHeight,uv).rg*255.;return (rg.x*256.+rg.y)*.125-1024.;}
void main(){
 vec2 p=uBounds.xy+gl_FragCoord.xy*uMpp;
 vec3 grass=material(p,vec2(0,0)),rock=material(p,vec2(0,.5)),dirt=material(p,vec2(.5,.5));
 float town=1.-smoothstep(.85,1.25,length(vec2((p.x+160.)/340.,(p.y-130.)/220.)));
 float farm=(1.-smoothstep(140.,200.,abs(p.x+607.)))*(1.-smoothstep(230.,310.,abs(p.y-135.)));
 vec3 base=grass*mix(.72,1.05,max(town,farm));
 float hill=exp(-pow((p.x+950.)/600.,2.)-pow((p.y+800.)/620.,2.))+exp(-pow((p.x-390.)/200.,2.)-pow((p.y-1330.)/250.,2.));
 float rocky=smoothstep(.45,.8,noise(p/24.)+hill*.28)*clamp(hill,0.,.8);base=mix(base,rock,rocky);
 float e=elevation(p),dx=(elevation(p+vec2(2,0))-elevation(p-vec2(2,0)))/4.,dy=(elevation(p+vec2(0,2))-elevation(p-vec2(0,2)))/4.;
 base*=clamp(.72+.38*dot(normalize(vec3(-dx,-dy,1)),normalize(vec3(-.45,.6,1.2))),.55,1.15);
 float best=1e8,halfWidth=30.;vec2 tangent=vec2(1,0);
 for(int i=0;i<23;i++){if(i>=uRiverCount-1)break;vec2 a=uRiver[i].xy,b=uRiver[i+1].xy,v=b-a;float t=clamp(dot(p-a,v)/dot(v,v),0.,1.),d=length(p-a-v*t);if(d<best){best=d;halfWidth=mix(uRiver[i].z,uRiver[i+1].z,t)*.5;tangent=normalize(v);}}
 float edge=best-halfWidth+(noise(p*.12)-.5)*1.6,bank=1.-smoothstep(2.,8.,edge);base=mix(base,mix(rock,dirt,.25)*1.08,bank);
 float water=1.-smoothstep(-.6,.6,edge),detail=noise(p*.35)*.10+noise(p*.041)*.13;
 vec3 waterColor=mix(vec3(.075,.19,.19),vec3(.17,.29,.25),pow(clamp(best/halfWidth,0.,1.),3.));
 waterColor+=detail*vec3(.3,.38,.32);waterColor+=pow(max(0.,sin(dot(p,tangent)*1.5+noise(p*.2)*6.)),16.)*.015;
 base=mix(base,waterColor,water);
 float sd=1e8;for(int i=0;i<15;i++){if(i>=uStreamCount-1)break;vec2 a=uStream[i],v=uStream[i+1]-a;float t=clamp(dot(p-a,v)/dot(v,v),0.,1.);sd=min(sd,length(p-a-v*t));}
 base=mix(base,rock*.75,1.-smoothstep(1.5,3.2,sd));base=mix(base,vec3(.12,.26,.25)+noise(p*.8)*.03,1.-smoothstep(.9,1.4,sd));
 outColor=vec4(base,1.);
}`;
const program=gl.createProgram();gl.attachShader(program,shader(gl.VERTEX_SHADER,vertex));gl.attachShader(program,shader(gl.FRAGMENT_SHADER,fragment));gl.linkProgram(program);if(!gl.getProgramParameter(program,gl.LINK_STATUS))throw Error(gl.getProgramInfoLog(program));
gl.useProgram(program);const uniform=n=>gl.getUniformLocation(program,n);
function texture(img,unit,name,filter=gl.LINEAR){const t=gl.createTexture();gl.activeTexture(gl.TEXTURE0+unit);gl.bindTexture(gl.TEXTURE_2D,t);gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,img);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,filter);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,filter);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);gl.uniform1i(uniform(name),unit);}
function loadImage(url){return new Promise((resolve,reject)=>{const i=new Image();i.onload=()=>resolve(i);i.onerror=()=>reject(Error('asset load failed'));i.src=url;});}
const bins=new Map();for(const t of SCENE.vegetation){const k=Math.floor(t[0]/128)+','+Math.floor(t[1]/128);if(!bins.has(k))bins.set(k,[]);bins.get(k).push(t);}
function visibleTrees(b){const list=[];for(let y=Math.floor((b[1]-40)/128);y<=Math.floor((b[3]+40)/128);y++)for(let x=Math.floor((b[0]-40)/128);x<=Math.floor((b[2]+40)/128);x++)if(bins.has(x+','+y))list.push(...bins.get(x+','+y));return list;}
function render(target,bounds,labels=false,grid=false,footprints=false){
 const ctx=target.getContext('2d'),w=target.width,h=target.height,scale=w/(bounds[2]-bounds[0]),p=a=>[(a[0]-bounds[0])*scale,(bounds[3]-a[1])*scale];
 groundCanvas.width=w;groundCanvas.height=h;gl.viewport(0,0,w,h);gl.useProgram(program);gl.uniform4f(uniform('uBounds'),bounds[0],bounds[1],bounds[2],bounds[3]);gl.uniform1f(uniform('uMpp'),1/scale);gl.drawArrays(gl.TRIANGLES,0,3);ctx.drawImage(groundCanvas,0,0);
 function poly(points,fill,stroke,line=1){ctx.beginPath();points.forEach((q,i)=>{const[x,y]=p(q);i?ctx.lineTo(x,y):ctx.moveTo(x,y);});ctx.closePath();if(fill){ctx.fillStyle=fill;ctx.fill();}if(stroke){ctx.strokeStyle=stroke;ctx.lineWidth=line;ctx.stroke();}}
 function line(points,color,width){ctx.beginPath();points.forEach((q,i)=>{const[x,y]=p(q);i?ctx.lineTo(x,y):ctx.moveTo(x,y);});ctx.lineWidth=width*scale;ctx.lineJoin='round';ctx.lineCap='round';ctx.strokeStyle=color;ctx.stroke();}
 const groundPattern=ctx.createPattern(images.dirt,'repeat');groundPattern.setTransform(new DOMMatrix([.11*scale,0,0,.11*scale,-bounds[0]*scale,bounds[3]*scale]));
 for(const f of SCENE.fields){poly(f.polygon,f.crop==='crop'?'#666b40':'#6b6348','#525b39',Math.max(.4,scale*.5));ctx.save();ctx.beginPath();f.polygon.forEach((q,i)=>{const[x,y]=p(q);i?ctx.lineTo(x,y):ctx.moveTo(x,y);});ctx.clip();for(let x=f.center[0]-f.width;x<f.center[0]+f.width;x+=2.8)line([[x,f.center[1]-f.depth],[x+f.depth*.12,f.center[1]+f.depth]],f.crop==='crop'?'#4e5937':'#7f7555',.7);ctx.restore();}
 for(const b of SCENE.buildings){ctx.save();ctx.globalAlpha=.32;poly(b.yard,groundPattern);poly(b.yard,'#a3906540');ctx.restore();const q=b.yard;line([q[0],q[1]],'#585e37',.55);line([q[1],q[2]],'#616443',.55);}
 for(const r of SCENE.roads){line(r.points,'#6a6551',r.width+1.2);line(r.points,groundPattern,r.width);line(r.points,'#ad95674d',r.width*.9);}
 for(const b of SCENE.buildings){const [x,y]=p(b.center);if(x<-100||x>w+100||y<-100||y>h+100)continue;poly(b.footprint.map(q=>[q[0]+1.2,q[1]-1.3]),'#17221bb0');}
 // Wood pier connects the exact quay anchor to the dry west bank; no bridge across H01.
 line([[-46,20],[0,0]],'#463d2f',5.5);line([[-46,20],[0,0]],'#8e8063',4.4);for(let t=0;t<1;t+=.045){const q=[-46*(1-t),20*(1-t)];line([[q[0]-1,q[1]-2],[q[0]+1,q[1]+2]],'#4f4738',.22);}
 for(const b of SCENE.bridges){const c=Math.cos(b.angle),s=Math.sin(b.angle),q=(x,y)=>[b.center[0]+x*c-y*s,b.center[1]+x*s+y*c];poly([q(-b.length/2,-b.width/2),q(b.length/2,-b.width/2),q(b.length/2,b.width/2),q(-b.length/2,b.width/2)],'#9a8b6c','#423f31',Math.max(.5,scale*.35));for(let t=-b.length/2;t<b.length/2;t+=.65)line([q(t,-b.width/2),q(t,b.width/2)],'#625d48',.18);}
 function sprite(asset,center,width,depth,angle,shadow=false){const[x,y]=p(center),r=ASSETS.rects[asset];ctx.save();ctx.translate(x,y);ctx.rotate(-angle);if(shadow){ctx.shadowColor='#14231b88';ctx.shadowBlur=2*scale;ctx.shadowOffsetX=1.2*scale;ctx.shadowOffsetY=1.8*scale;}ctx.drawImage(images.props,...r,-width*scale/2,-depth*scale/2,width*scale,depth*scale);ctx.restore();}
 for(const b of SCENE.buildings){const[x,y]=p(b.center);if(x<-100||x>w+100||y<-100||y>h+100)continue;sprite(b.asset,b.center,b.width,b.depth,b.angle);if(footprints){poly(b.footprint,null,'#8de1e4',1);const[dX,dY]=p(b.door);ctx.fillStyle='#e6bd77';ctx.fillRect(dX-2,dY-2,4,4);}}
 for(const t of visibleTrees(bounds)){const[x,y]=p(t);if(x<-40*scale||x>w+40*scale||y<-40*scale||y>h+40*scale)continue;sprite(t[3],[t[0],t[1]],t[2],t[2],t[4],scale>.25);}
 // Surface entrance only. Underground rooms are not drawn as surface buildings.
 const [cx,cy]=p([260,1250]);ctx.save();ctx.translate(cx,cy);ctx.rotate(-.25);ctx.fillStyle='#202621';ctx.beginPath();ctx.ellipse(0,0,7*scale,4*scale,0,0,Math.PI*2);ctx.fill();ctx.strokeStyle='#989784';ctx.lineWidth=scale*1.2;ctx.stroke();ctx.restore();
 if(grid){ctx.strokeStyle='#d5dedf66';ctx.lineWidth=1;for(let x=SCENE.bounds[0];x<=SCENE.bounds[2];x+=1024)line([[x,SCENE.bounds[1]],[x,SCENE.bounds[3]]],'#d5dedf66',1/scale);for(let y=SCENE.bounds[1];y<=SCENE.bounds[3];y+=1024)line([[SCENE.bounds[0],y],[SCENE.bounds[2],y]],'#d5dedf66',1/scale);}
 if(labels){
  const offsets={workshop:[45,-48],quay:[40,35],water_point:[-150,24],water_store:[-160,-42],forest_gate:[40,-25],cave:[40,-30],spring:[45,-30],local_grove_tree:[-170,-28],reveal:[40,30]};
  ctx.font='18px "Malgun Gothic",sans-serif';for(const poi of SCENE.pois){if(!poi.visible||poi.surface!=='ground')continue;const[x,y]=p(poi.point);if(x<15||x>w-220||y<20||y>h-30)continue;if(scale<.35&&!['workshop','cave','quay'].includes(poi.id))continue;const[dx,dy]=offsets[poi.id]||[40,-25],left=dx<0,tx=x+dx,ty=y+dy,end=left?tx+ctx.measureText(poi.name).width+7:tx-7,elbow=left?end+12:end-12;ctx.strokeStyle='#c4ccc6';ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(x,y);ctx.lineTo(elbow,ty-6);ctx.lineTo(end,ty-6);ctx.stroke();ctx.fillStyle=poi.id==='workshop'?'#92e7ef':'#e0e2de';if(poi.id==='cave'){ctx.strokeStyle='#d7b56b';ctx.strokeRect(x-3,y-3,6,6);}else{ctx.beginPath();ctx.arc(x,y,3.5,0,Math.PI*2);ctx.fill();}ctx.lineWidth=3;ctx.strokeStyle='#1c2725';ctx.strokeText(poi.name,tx,ty);ctx.fillStyle='#efefea';ctx.fillText(poi.name,tx,ty);}
 }
 return {width:w,height:h,bounds,scale};
}
async function initRenderer(){
 [images.props,images.materials,images.height]=await Promise.all([loadImage(ASSETS.props),loadImage(ASSETS.materials),loadImage(ASSETS.height)]);
 texture(images.materials,0,'uMaterial');texture(images.height,1,'uHeight',gl.NEAREST);
 gl.uniform1i(uniform('uRiverCount'),SCENE.river.points.length);gl.uniform3fv(uniform('uRiver[0]'),SCENE.river.points.flatMap((p,i)=>[...p,SCENE.river.widths[i]]));gl.uniform1i(uniform('uStreamCount'),SCENE.stream.points.length);gl.uniform2fv(uniform('uStream[0]'),SCENE.stream.points.flat());
 images.dirt=document.createElement('canvas');images.dirt.width=512;images.dirt.height=512;images.dirt.getContext('2d').drawImage(images.materials,627,627,627,627,0,0,512,512);ready=true;
}
window.detailExport=(bounds,w,h,labels=false,grid=false,footprints=false)=>{
 // Render beyond the requested footprint so sprite filtering and shadows are
 // never clipped at an exported border. Crop only after rasterization.
 const pad=64,mpp=(bounds[2]-bounds[0])/w,expanded=document.createElement('canvas');
 expanded.width=w+pad*2;expanded.height=h+pad*2;
 render(expanded,[bounds[0]-pad*mpp,bounds[1]-pad*mpp,bounds[2]+pad*mpp,bounds[3]+pad*mpp],labels,grid,footprints);
 const canvas=document.createElement('canvas');canvas.width=w;canvas.height=h;
 canvas.getContext('2d').drawImage(expanded,pad,pad,w,h,0,0,w,h);
 return canvas.toDataURL('image/png');
};
