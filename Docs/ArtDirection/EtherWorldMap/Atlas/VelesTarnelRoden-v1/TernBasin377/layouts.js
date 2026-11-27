"use strict";
const plans={
 // 길의 순서, 건물들이 바라보는 길, 거점의 인지 표지. 모든 치수는 미터다.
 grove:{lanes:[[[-164,-95],[-101,-48],[-55,-28],[0,0],[43,-9],[79,14],[89,53],[66,101]],[[0,0],[-21,31],[-30,60],[-9,81],[31,87],[61,68],[79,44],[79,14]]],land:[["거목 돌봄터",20,46,22,"tree"],["공구 창고",-48,-11,9,"shed"],["휴식림 표식",67,97,4,"stone"]],houses:29,fields:[[-92,-80,43,26],[140,5,39,28]]},
 grain:{lanes:[[[-155,-40],[-101,-25],[-56,-6],[0,0],[57,16],[105,14],[151,31]],[[0,0],[-26,28],[-15,64],[28,71],[68,53],[57,16]]],land:[["종자 저장고",-56,12,12,"shed"],["공동 화덕",10,77,8,"kiln"],["높은 우물",71,68,4,"well"]],houses:35,fields:[[-90,-90,84,36],[38,-81,90,40],[112,88,73,43],[-78,98,65,32]]},
 timber:{lanes:[[[-120,30],[-82,15],[-47,18],[0,0],[55,-10],[85,-35],[145,-38]],[[0,0],[-4,35],[16,68],[54,74],[84,44],[55,-10]]],land:[["원목 집재장",-73,-11,17,"logs"],["수리장",22,31,10,"shed"],["겨울집",52,90,8,"shed"]],houses:26,fields:[[-118,70,42,21]]},
 kiln:{lanes:[[[-91,-41],[-63,-13],[-31,-9],[0,0],[28,21],[41,42]],[[0,0],[-14,21],[-7,45],[19,51],[41,42]]],land:[["묵은 가마",-1,30,9,"kiln"],["새 가마",23,33,6,"kiln"],["짐마당",-40,4,8,"logs"]],houses:5},
 watch:{lanes:[[[-75,-32],[-41,-22],[-20,-4],[0,0],[17,21],[12,47]],[[0,0],[-16,16],[-18,33],[12,47]]],land:[["관측턱",-5,42,7,"stone"],["바람막이",12,10,7,"shed"]],houses:3},
 pass:{lanes:[[[-95,-15],[-51,-3],[-22,-12],[0,0],[44,3],[77,25]],[[0,0],[-10,23],[18,36],[48,28],[44,3]]],land:[["짐바꿈 마당",-39,12,10,"logs"],["고개 표지",45,40,4,"stone"]],houses:7},
 reed:{lanes:[[[-90,33],[-42,20],[-21,3],[0,0],[35,-4],[65,-19]],[[0,0],[4,25],[28,40],[44,19],[35,-4]]],land:[["섶단 적치터",19,17,9,"logs"],["높은 물표",41,40,3,"stone"]],houses:3},
 cave:{lanes:[[[-65,-30],[-28,-17],[-11,-10],[0,0],[14,14],[31,16]],[[31,16],[44,29],[36,42],[9,28],[0,0]]],land:[["점검구",0,0,6,"mouth"],["측실",16,14,8,"chamber"],["바깥 출구",31,16,5,"mouth"]],houses:1}
};
function hash(x,y){
 let n=Math.imul(x,374761393)+Math.imul(y,668265263);
 n=Math.imul(n^(n>>>13),1274126177); return ((n^(n>>>16))>>>0)/4294967296;
}
function curve(ps){
 const out=[ps[0]];
 for(let i=0;i+1<ps.length;i++){
  const a=ps[i],b=ps[i+1],u=ps[Math.max(0,i-1)],v=ps[Math.min(ps.length-1,i+2)];
  for(let j=1;j<=12;j++){
   let t=j/12,s=1-t;
   out.push([0,1].map(k=>s*s*s*a[k]+3*s*s*t*(a[k]+(b[k]-u[k])/6)+3*s*t*t*(b[k]-(v[k]-a[k])/6)+t*t*t*b[k]));
  }
 }
 return out;
}
function segDistance(x,y,a,b){
 let dx=b[0]-a[0],dy=b[1]-a[1],s=Math.max(0,Math.min(1,((x-a[0])*dx+(y-a[1])*dy)/(dx*dx+dy*dy||1)));
 return Math.hypot(x-a[0]-s*dx,y-a[1]-s*dy);
}
plans.market={lanes:[[[-165,15],[-83,-24],[-39,-19],[0,0],[31,16],[85,29],[140,18]],[[0,0],[-7,38],[21,64],[65,55],[85,29]]],land:[["운반꾼 뜰",-59,6,8,"logs"],["빈 수레 보관소",79,72,8,"shed"]],houses:30};
plans.guild={lanes:[[[-143,49],[-98,9],[-47,-18],[0,0],[39,-9],[94,-33],[170,-10]],[[0,0],[-16,34],[4,66],[48,77],[87,51],[94,13],[94,-33]]],land:[["공동 안뜰",27,34,10,"well"],["수레 정비소",105,57,11,"shed"]],houses:36};
const districts=[
 ["T-YARD","타르넬 운반꾼 마당",-12195,-19870,"market","공동 나루에서 하역을 마친 운반꾼이 젖은 짐과 빈 수레를 가르는 마당이다.","검수 창고 북쪽으로 들어가 동쪽 수레길로 돌아온다."],
 ["R-HOMES","로덴 제방 안쪽 주거지",-40715,-49475,"guild","조합에 물건을 맡긴 가구가 제방 안쪽 우물과 정비소를 함께 쓴다.","조합 정문에서 북서쪽 안뜰로 오르고 수레 정비소 뒤로 내려온다."]];
const extraLand={
 K01:[["막힌 재받이",-9,27,3,"mouth"],["바람구멍",21,36,2,"stone"]],
 K02:[["물눈 말뚝",54,-31,2,"post"],["자갈등",-42,37,8,"stone"]],
 K03:[["옛 통과 폭",-12,42,6,"stone"]],
 K04:[["깨진 배수관",18,35,3,"pipe"],["점토 건조대",-36,20,6,"rack"]],
 K05:[["침수된 옛 표지",-53,-37,2,"post"],["섶길",-39,-23,7,"rack"]],
 K06:[["옛 종걸이",17,17,4,"rack"],["표지석",-37,-11,3,"stone"]],
 K07:[["수차 축받이",16,14,4,"wheel"],["배사구",33,18,4,"pipe"]],
 K08:[["낮은 집수실",15,13,5,"well"],["윗실 부표",30,18,2,"post"]]
};
function makePlan(site){
 const [id,name,x,y,type]=site,p=plans[type],seed=[...id].reduce((a,c)=>a*3+c.charCodeAt(0),0);
 const angle=id[0]==='K'?hash(seed,2)*5.8:0,ca=Math.cos(angle),sa=Math.sin(angle);
 const world=([a,b])=>[x+a*ca-b*sa,y+a*sa+b*ca];
 const lanes=p.lanes.map(curve);
 const homes=[];
 const fields=p.fields||[];
 const marks=[...p.land,...(extraLand[id]||[])];
 const land=marks.map(([n,a,b,r,k])=>[n,...world([a,b]),r,k]);
 for(let i=0;i<1600&&homes.length<p.houses;i++){
  const lane=lanes[i%lanes.length], j=Math.floor(hash(seed,i+9)*(lane.length-1));
  const a=lane[j],b=lane[j+1],rot=Math.atan2(b[1]-a[1],b[0]-a[0]), flip=i%3?1:-1;
  const setback=flip*(9+hash(i,seed)*16),cx=a[0]-Math.sin(rot)*setback,cy=a[1]+Math.cos(rot)*setback;
  if(marks.some(([,u,v,r])=>Math.hypot(cx-u,cy-v)<r+8)||fields.some(([u,v,w,h])=>Math.abs(cx-u)<w/2+7&&Math.abs(cy-v)<h/2+7))continue;
  if(lanes.some(row=>row.some((pt,k)=>k&&segDistance(cx,cy,row[k-1],pt)<7)))continue;
  const [wx,wy]=world([cx,cy]);
  if(homes.some(home=>Math.hypot(home[0]-wx,home[1]-wy)<14))continue;
  homes.push([wx,wy,8+hash(seed+i,13)*6,5+hash(seed,i+90)*3,rot+angle,...world(a)]);
 }
 return {site,lanes:lanes.map(row=>row.map(world)),land,homes,fields:fields.map(([a,b,w,h])=>[...world([a,b]),w,h,angle])};
}
const localPlans=[...sites,...districts].map(makePlan);
function regionAt(x,y){
 let xw=x+1000*Math.sin(y/6800)+490*Math.cos((x+y)/3600);
 let yw=y+850*Math.sin(x/5100);
 let best=Infinity,id=0;
 regions.forEach((r,i)=>{let d=(xw-r[1])**2+(yw-r[2])**2;if(d<best){best=d;id=i}});
 return id;
}
function tileBounds(i,j){
 const [x,y]=geo.base,[w,h]=geo.size;
 return [x+i*w,y+j*h,x+(i+1)*w,y+(j+1)*h];
}
function tileAt(x,y){return [Math.floor((x-geo.base[0])/geo.size[0]),Math.floor((y-geo.base[1])/geo.size[1])]}
function stageAt(x,y){let [i,j]=tileAt(x,y);return 1+2*Math.max(Math.abs(i),Math.abs(j))}

