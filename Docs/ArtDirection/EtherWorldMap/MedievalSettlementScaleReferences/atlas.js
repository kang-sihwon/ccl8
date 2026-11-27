'use strict';
const cases = window.settlementCases;
const byId = new Map(cases.map(item => [item.key, item]));
const tile = { east:255.05882352941177, north:272.94117647058823 };
const tileHa = tile.east * tile.north / 10000;
const groupNames = ['마을','도시','대도시','수도'];
const heroKeys = ['Pfaffenschlag','Cambridge','Bristol','London'];
const regions = ['동아시아','서아시아','북아프리카'];
const $ = selector => document.querySelector(selector);
const nf = new Intl.NumberFormat('ko-KR',{maximumFractionDigits:1});
const fmt = n => nf.format(n);
function esc(s) {
    return String(s).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}
function population(c) {
    const [lo,hi] = c.pop;
    if (hi === null) return fmt(lo)+'명 초과';
    return '약 '+fmt(lo)+(lo === hi ? '' : '–'+fmt(hi))+'명';
}
function link([label,url]) {
    return '<a href="'+esc(url)+'" target="_blank" rel="noopener">'+esc(label)+'</a>';
}
function bounds(m) {
    const p = m.boundary || m.witness;
    if (!p) return null;
    const rad = -m.n*Math.PI/180;
    const rotated = p.map(([x,y]) => [(x*Math.cos(rad)-y*Math.sin(rad))*m.m,(x*Math.sin(rad)+y*Math.cos(rad))*m.m]);
    return [0,1].map(i=>Math.max(...rotated.map(v=>v[i]))-Math.min(...rotated.map(v=>v[i])));
}
function groundSize(v) { return v>=1000 ? (v/1000).toFixed(2)+' km' : Math.round(v)+' m'; }
function spanInfo(c) {
    if (!c.metric) return '';
    const m=c.metric, b=bounds(m);
    if (!b) return '';
    return (m.boundaryLabel||m.spanLabel)+' 약 '+b.map(groundSize).join(' × ')+' · '+(b[0]/tile.east).toFixed(1)+' × '+(b[1]/tile.north).toFixed(1)+'칸 폭';
}
let figureId=0;
function mapSvg(c, options={}) {
    const m=c.metric, id='grid-'+(++figureId), fit=options.width || 4000;
    const aspect=options.aspect || 1.45, h=fit/aspect;
    const grid=!!options.grid, centre=options.centre || m.focus;
    const radians=-m.n*Math.PI/180;
    const anchorX=fit/2-m.m*(centre[0]*Math.cos(radians)-centre[1]*Math.sin(radians));
    const anchorY=h/2-m.m*(centre[0]*Math.sin(radians)+centre[1]*Math.cos(radians));
    const shape=m.boundary && options.mark ? '<polygon points="'+m.boundary.map(p=>p.join(',')).join(' ')+'" fill="#b172331e" stroke="#a34d29" stroke-dasharray="6 3" vector-effect="non-scaling-stroke" stroke-width="1.8"/>' : '';
    const marked=m.witness && options.mark && !m.points ? m.witness.map(([x,y])=>'<circle cx="'+x+'" cy="'+y+'" r="3.5" fill="#a34d29"/>').join('') : '';
    const length=options.bar || (fit>=3000 ? 1000 : fit>=1200 ? 500 : fit>=500 ? 200 : fit>=225 ? 100 : fit>=90 ? 50 : 10);
    const bx=fit*.055, by=h*.92, strip=h*.019;
    const tint=grid ? '<rect width="'+fit+'" height="'+h+'" fill="url(#'+id+')"/>' : '';
    return '<svg xmlns="http://www.w3.org/2000/svg" role="img" aria-label="'+esc(c.name+' 축척 지도')+'" viewBox="0 0 '+fit+' '+h+'"><defs><pattern id="'+id+'" x="'+anchorX+'" y="'+anchorY+'" width="'+tile.east+'" height="'+tile.north+'" patternUnits="userSpaceOnUse"><path d="M'+tile.east+' 0H0V'+tile.north+'" fill="none" stroke="#32654e" stroke-width="'+(fit/450)+'" opacity=".74"/></pattern></defs><rect width="100%" height="100%" fill="#eae7da"/><g transform="translate('+fit/2+' '+h/2+') scale('+m.m+') rotate('+(-m.n)+') translate('+(-centre[0])+' '+(-centre[1])+')"><image href="'+esc(m.src)+'" width="'+m.w+'" height="'+m.h+'"/>'+shape+marked+'</g>'+tint+'<rect x="'+(bx-strip)+'" y="'+(by-strip*2.65)+'" width="'+(length+strip*2)+'" height="'+strip*4+'" fill="#f6f2e4" opacity=".94"/><path d="M'+bx+' '+by+'h'+length+'" stroke="#283d34" stroke-width="'+strip*.42+'"/><path d="M'+bx+' '+(by-strip*.55)+'v'+strip+'m'+length+' 0v-'+strip+'" stroke="#283d34" stroke-width="'+strip*.33+'"/><text x="'+bx+'" y="'+(by-strip*.88)+'" font-size="'+(h*.043)+'" fill="#233a2c">'+(length===1000?'1 km':length+' m')+'</text></svg>';
}
function imgButton(c, label, cls='thumb') {
    return '<button class="'+cls+'" type="button" data-open="'+c.key+'" aria-label="'+esc(c.name+' '+label)+'"><img loading="lazy" alt="'+esc(c.name+' · '+c.look)+'" src="'+esc(c.image)+'"></button>';
}
function hero() {
    $('#hero').innerHTML=heroKeys.map(k=>{
        const c=byId.get(k);
        const sketchLabel={Pfaffenschlag:'취락과 개울',Cambridge:'성·장터·남쪽 큰길',Bristol:'항구 서쪽 구역',London:'시티와 서더크'}[k];
        return '<article><p class="small">'+esc(c.group)+' · '+esc(c.year)+'</p><h3>'+esc(c.name)+'</h3><p>'+population(c)+' · '+(c.area ? fmt(c.area)+' ha' : '집터 16곳')+'</p>'+imgButton(c,'배치 도판 확대','hero-art')+'<button class="hero-scale" type="button" data-open="'+k+'" data-mode="measure" aria-label="'+esc(c.name+' 축척 확대')+'">'+mapSvg(c,{grid:$('#hero-grid').checked,aspect:2.1,mark:true})+'</button><p class="hint">'+sketchLabel+' · 4 km 창</p></article>';
    }).join('');
}
function summary() {
    $('#summary').innerHTML=groupNames.map((g,i)=>{
        const rows=cases.filter(c=>c.group===g),lo=Math.min(...rows.map(c=>c.pop[0])),high=Math.max(...rows.map(c=>c.pop[1]??c.pop[0]));
        const areas=rows.map(c=>c.area).filter(Number.isFinite);
        const c=byId.get(heroKeys[i]);const span=bounds(c.metric);
        return '<tr><th>'+g+' · '+rows.length+'곳</th><td>'+(i===0?'1086년–15세기':'1300년 전후')+'</td><td>'+fmt(lo)+'–'+fmt(high)+'명'+(i===0?' 초과':'')+'</td><td>'+(i===0?'집터 16곳 · 어촌 주택 100채 초과':i===3?'피렌체 9,780가구 (1427년 별도 기록)':'동시점 가구 수 미상')+'</td><td>'+(areas.length?fmt(Math.min(...areas))+'–'+fmt(Math.max(...areas))+' ha ('+areas.length+'곳)':'전역 합계 미상 · '+fmt(c.partial)+' ha (파펜슐라크 집터 외곽)')+'</td><td>'+span.map(groundSize).join(' × ')+'<br><small>'+c.name+' · '+esc(c.metric.boundaryLabel||c.metric.spanLabel)+'</small></td></tr>';
    }).join('');
}
function field(k,v){return '<dt>'+k+'</dt><dd>'+esc(v)+'</dd>';}
function card(c) {
    const sources=c.links.map(link);
    const area=c.area?fmt(c.area)+' ha · 면적 '+fmt(c.area/tileHa)+'타일분':c.extraArea|| (c.partial?fmt(c.partial)+' ha · '+(c.partial/tileHa).toFixed(2)+'타일분 ('+c.metric.boundaryLabel+')':'당시 총면적 미상');
    const direct=c.links[0];
    const numberField=(label,value,src)=>'<dt>'+label+'</dt><dd>'+link([value,src]).replace('<a ','<a class="num-link" ')+'</dd>';
    return '<article id="case-'+c.key+'" class="card">'+imgButton(c,'도판 확대')+'<p class="small">'+esc(c.region)+'　/　'+esc(c.year)+'</p><h3>'+esc(c.name)+' <span lang="en">'+esc(c.latin)+'</span></h3><p class="title">'+esc(c.title||c.metric?.scope||'')+'</p><dl class="facts">'+numberField('주민',population(c),direct[1])+(c.area?numberField('정주 면적',area,c.links[1][1]):field('공간 범위',area))+(c.houseNote?field('가구·주택',c.houseNote):'')+'</dl><p class="look">'+esc(c.look)+'</p><div class="links">'+link(direct)+(c.area?link(c.links[1]):'')+(c.metric?'<button type="button" class="action" data-open="'+c.key+'" data-mode="measure">축척 판독</button>':'')+'</div><details><summary>집계 방법 · 도판과 출처</summary><div class="more"><p>'+esc(c.method)+'</p><p>'+esc(c.edge)+'</p>'+(c.density?'<p>대응 범위의 연구 추정 밀도 약 '+fmt(c.density)+'명/ha.</p>':'')+(spanInfo(c)?'<p>'+esc(spanInfo(c))+'</p>':'')+'<p class="sub">'+esc(c.plate)+'</p><div class="links">'+sources.join('')+'</div></div></details></article>';
}
let filter='전체';
function list() {
    const term=$('#query').value.trim().toLocaleLowerCase();
    const rows=cases.filter(c=>(filter==='전체'||c.group===filter||c.region===filter)&&[c.name,c.latin,c.region,c.title].filter(Boolean).join(' ').toLocaleLowerCase().includes(term));
    $('#results').textContent=(filter==='전체'?'전체 자료':filter)+' · '+rows.length+'곳';
    $('#cards').innerHTML=rows.map(card).join('') || '<p>해당하는 지명이 없어. 다른 표기나 권역을 선택해 줘.</p>';
    document.querySelectorAll('#filters button').forEach(b=>b.setAttribute('aria-pressed',String(b.dataset.filter===filter)));
}
const pairs=[
    ['Pfaffenschlag','Edgcote',650,'개울 양안의 집 16곳과 오목길 양옆의 집터','파펜슐라크는 집 뒤로 경작지가 이어지고 가운데에 개울이 흐른다. 에지코트는 교회로 내려가는 길 옆에 집터가 모인다. 표시한 선은 실측도에서 남은 주거 흔적을 둘러 읽은 범위다.'],
    ['Cambridge','Coventry',2100,'다리를 건너 퍼지는 길과 교외를 관통하는 큰길','케임브리지는 성 언덕에서 다리를 건넌 뒤 동쪽 반웰과 남쪽 시장길로 갈라진다. 코번트리의 큰길은 서쪽 교외와 중심 교차로, 동쪽 출구를 한 줄로 잇는다. 두 약도에서 같은 길이의 500 m가 얼마나 많은 갈림길을 지나는지 비교한다.'],
    ['Bristol','Norwich',2500,'성 밖 항구를 더한 면적과 성 안 습지를 뺀 면적','브리스틀은 성 안 55 ha에 교외를 더해 130 ha를 얻었다. 노리치는 성 안 약 219 ha 가운데 습지를 가려내 185 ha를 얻었다. 왼쪽은 항구 서쪽, 오른쪽은 북쪽 성벽과 웬섬강이다.'],
    ['London','Paris',4600,'다리 하나에 모인 길과 강섬을 거쳐 갈라지는 길','런던교는 서더크의 큰길을 북쪽 시가지에 잇는다. 파리에서는 시테섬 양편의 다리로 건넌 뒤 양안의 길로 다시 퍼진다. 런던 도판은 약 1300년, 파리 도판은 1380년의 복원이다. 파리의 인구와 면적은 1300년 전후 표에서 읽는다.']
];
function studies() {
    $('#studies').innerHTML=pairs.map(([a,b,width,title,body])=>{
        const figures=[a,b].map(k=>{const c=byId.get(k);return '<figure><button type="button" class="study-plate" data-open="'+k+'" data-mode="measure" aria-label="'+esc(c.name+' 확대')+'">'+mapSvg(c,{width,mark:true,grid:$('#hero-grid').checked})+'</button><figcaption>'+esc(c.name)+' · '+population(c)+'</figcaption></figure>';}).join('');
        return '<article class="study">'+figures+'<div><p class="small">'+byId.get(a).group+'</p><h3>'+title+'</h3><p>'+body+'</p><div class="links">'+[a,b].map(k=>{const c=byId.get(k);return link([c.name+' 근거',c.links.find(l=>l[0].startsWith('실측 축척'))?.[1]||c.links.find(l=>/좌표 도판|RCHME|복원도/.test(l[0]))?.[1]||c.links[0][1]]);}).join('')+'</div></div></article>';
    }).join('');
}
let active=null, mode='plate', opener=null;
const modal=$('#viewer'),pane=$('#pane');
function renderViewer(reset=false) {
    if(!active)return;
    const c=active,m=c.metric, measured=mode==='measure' && !!m;
    $('#mode-measure').hidden=!m;
    $('#mode-measure').setAttribute('aria-pressed',String(measured));
    $('#mode-plate').setAttribute('aria-pressed',String(!measured));
    $('#v-grid').disabled=!measured;
    $('#v-title').textContent=c.name;
    $('#v-kind').textContent=c.region+'　/　'+c.year+'　/　'+population(c);
    const z=Number($('#zoom').value);$('#zoom-out').textContent=z.toFixed(2).replace(/\.00$/,'')+'×';
    const prevX=(pane.scrollLeft+pane.clientWidth/2)/Math.max(pane.scrollWidth,1);
    const prevY=(pane.scrollTop+pane.clientHeight/2)/Math.max(pane.scrollHeight,1);
    const surface=$('#surface');
    const artWidth=Math.max(140,pane.clientWidth-10),artHeight=Math.max(140,pane.clientHeight-10);
    if(measured){
        const rad=m.n*Math.PI/180,cos=Math.abs(Math.cos(rad)),sin=Math.abs(Math.sin(rad));
        const w=(m.w*cos+m.h*sin)*m.m,h=(m.w*sin+m.h*cos)*m.m;
        const aspect=w/h, base=Math.min(artWidth,artHeight*aspect);
        surface.innerHTML=mapSvg(c,{width:w,aspect,centre:[m.w/2,m.h/2],grid:$('#v-grid').checked,mark:true});
        const svg=surface.firstElementChild;svg.style.width=base*z+'px';svg.style.height=base*z/aspect+'px';
        $('#v-caption').textContent=[m.scope,m.ref,spanInfo(c),m.points?.map((v,i)=>(i+1)+'. '+v).join('　')].filter(Boolean).join(' · ');
    } else {
        const im=new Image();im.alt=c.name+' 원본 도판';im.src=c.image;
        im.style.width=artWidth*z+'px';im.style.height=artHeight*z+'px';im.style.objectFit='contain';surface.replaceChildren(im);
        $('#v-caption').textContent=c.plate+'　'+c.look;
    }
    $('#v-source').innerHTML=c.links.map(link).join('　');
    pane.scrollLeft=(reset ? 0.5 : prevX)*pane.scrollWidth-pane.clientWidth/2;
    pane.scrollTop=(reset ? 0.5 : prevY)*pane.scrollHeight-pane.clientHeight/2;
}
function openCase(key,initial,el) {
    active=byId.get(key);opener=el;mode=initial||'plate';
    $('#zoom').value='1';$('#v-grid').checked=$('#hero-grid').checked;
    modal.showModal();document.body.style.overflow='hidden';renderViewer(true);
}
$('#filters').innerHTML=['전체',...groupNames,'세계 비교',...regions].map(g=>'<button type="button" data-filter="'+g+'">'+g+'</button>').join('');
document.addEventListener('click',e=>{
    const trigger=e.target.closest('[data-open],[data-filter]');
    if(trigger?.dataset.open)openCase(trigger.dataset.open,trigger.dataset.mode,trigger);
    if(trigger?.dataset.filter){filter=trigger.dataset.filter;list();}
});
$('#query').addEventListener('input',list);
$('#hero-grid').addEventListener('change',()=>{hero();studies();});
$('#close').onclick=()=>modal.close();
modal.addEventListener('close',()=>{active=null;document.body.style.overflow='';opener?.focus();});
modal.addEventListener('click',e=>{if(e.target===modal){const r=modal.getBoundingClientRect();if(e.clientX<r.left||e.clientX>r.right||e.clientY<r.top||e.clientY>r.bottom)modal.close();}});
$('#v-grid').onchange=()=>renderViewer();
$('#zoom').oninput=()=>renderViewer();
for(const [id,kind] of [['#mode-plate','plate'],['#mode-measure','measure']])$(id).onclick=()=>{mode=kind;$('#zoom').value='1';renderViewer(true);};
window.addEventListener('resize',()=>{if(modal.open)renderViewer();});
function revealCase() {
    if(!location.hash.startsWith('#case-'))return;
    const k=location.hash.slice(6);if(!byId.has(k))return;
    if(!document.getElementById('case-'+k)){filter='전체';$('#query').value='';list();}
    document.getElementById('case-'+k)?.scrollIntoView();
}
window.addEventListener('hashchange',revealCase);
hero();summary();studies();list();revealCase();

