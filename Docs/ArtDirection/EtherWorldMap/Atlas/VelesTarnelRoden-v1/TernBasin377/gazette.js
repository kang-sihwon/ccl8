"use strict";
function entry(parent,id,title,paragraphs,target){
 const block=document.createElement('article');block.id=id;
 let heading=document.createElement('h3');heading.textContent=title;block.append(heading);
 for(const text of paragraphs){const p=document.createElement('p');p.textContent=text;block.append(p)}
 if(target){let a=document.createElement('a');a.href='index.html?id='+encodeURIComponent(target);a.textContent='지도에서 위치 보기';block.append(a)}
 document.getElementById(parent).append(block);
}
const kinds={settlements:'정착지',rivers:'본류',routes:'노선',ridges:'산맥 축',passes:'대륙 고개',lakes:'호수 표식'};
document.getElementById('inventory').textContent=Object.entries(geo.inventory).map(([k,ids])=>kinds[k]+' '+(ids.join(' · ')||'없음')).join(' / ');
regions.forEach(([name,x,y,color,desc,access],i)=>entry('regionList','Z'+i,name,[desc,access],'Z'+i));
sites.forEach(([id,name,x,y,kind,desc,access])=>entry('placeList',id,name+' · '+id,[desc,access,'동 '+x+' m　북 '+y+' m'+(id.startsWith('V')?'　기존 정착지 · 필지와 사건은 신규 후보':'　신규 탐험 후보')],id));
for(const d of geo.details.filter(d=>/^([AB]\d\d|F-UP|C-MOUTH|V-BOARD)$/.test(d.id))){
 entry('placeList',d.id,d.id+' · '+d.name,['기존 탐색 좌표　동 '+Math.round(d.p[0][0])+' m　북 '+Math.round(d.p[0][1])+' m'],d.id);
}
entry('routeList','H01','테른 본류와 하류 항로',['벨레스 나루를 지나 타르넬에 기항하고 로덴으로 내려간다. 로덴 남쪽으로 나가는 R1 수상 회랑은 지역 경계 밖에서도 계속된다.'],'V01');
for(const l of geo.lines){
 let first=l.points[0],last=l.points.at(-1),cross=crossings.filter(b=>b.road===l.id||b.river===l.id);
 const coords=p=>'('+p.map(Math.round).join(', ')+')';
 entry('routeList','line-'+l.id,l.name,[l.source+' · '+(l.kind==='stream'?'수로':'육로')+'　'+coords(first)+' → '+coords(last),
 ...(cross.length?[cross.map(b=>b.mode+' '+coords([b.x,b.y])).join(' · ')]:[])],'line-'+l.id);
}
for(const [id,name,food,limit] of habitats)entry('lifeList','life-'+id,sites.find(p=>p[0]===id)[1]+' · '+name,[food,limit],id);
for(const g of geo.groups)entry('lifeList','life-'+g.id,g.id+' · '+g.name,['기존 '+(g.area==='T'?'타르넬':'로덴')+' 분포 후보. 거점 반경 '+g.home_radius_m+' m, 이동대 반폭 '+g.patrol_halfwidth_m+' m. 지도에서 활동권을 켜면 공공길과 겹치는 구간을 볼 수 있다.'],g.id);
entry('lifeList','managed','황금안개숲 관리 구조체와 야생동물',['뿌리 파수꾼은 수로 관리 구조체다. 늑대·고블린의 활동권과 추격 경계, 다람쥐·새·토끼·사슴의 관찰 위치는 기존 좌표를 따른다. 표시가 없는 깊은 숲은 개체 수와 조우가 정해지지 않은 자연 통과지다.'],'G01');
if(location.hash)document.getElementById(decodeURIComponent(location.hash.slice(1)))?.scrollIntoView();

