'use strict';
const byId = new Map(cases.map(c => [c.id, c]));
const get = id => document.getElementById(id);
const collate = new Intl.NumberFormat('ko-KR');
function measuredRoute(m) {
  const length = Math.hypot(m.a[0] - m.b[0], m.a[1] - m.b[1]) * m.metres / m.pixels;
  return m.route + ' · 직선 약 ' + collate.format(Math.round(length / 5) * 5) + 'm';
}
function element(tag, text, className) {
  const node = document.createElement(tag);
  if (text !== undefined) node.textContent = text;
  if (className) node.className = className;
  return node;
}
function link(label, href) {
  const a = element('a', label);
  a.href = href;
  return a;
}
function plateButton(c) {
  const button = element('button', undefined, 'plate');
  button.dataset.open = c.id;
  button.setAttribute('aria-label', c.name + ' 도판 확대');
  const img = element('img');
  img.alt = c.plate[1];
  img.loading = 'lazy';
  img.src = 'plates/' + c.plate[0] + '.webp';
  button.append(img);
  return button;
}
const rows = [
  ['riverfarm','눈사태 당시 마을 주민'],['solace','외곽 농민 제외 · 같은 세계의 모험서 DL1'],
  ['carvahall','출발 계획에 잡은 인원'],['bree','인간이 사는 돌집만 집계'],
  ['midcopse','게임 화면의 건물 · 커뮤니티 기록'],['riverwood','두 분기 인물·경비·동물 제외'],
  ['faerie','운영 미니게임의 인구 상한'],['haven','초대 PS판의 이민자 수용 한도'],
  ['willow','벌목 취락의 공식 인구표'],['otari','목재 항구의 공식 인구표'],['sand','해안 교역지의 공식 인구표']
];
for (const [key, scope] of rows) {
  const c = byId.get(key);
  const row = element('tr');
  const place = element('th');
  place.scope = 'row';
  place.append(link(c.name, '#case-' + key));
  const source = element('td');
  const entry = key === 'solace' ? c.links[1] : c.links[0];
  source.append(link(entry[0] + ' ↗', entry[1]));
  row.append(place, element('td', c.count), element('td', scope), source);
  get('numbers').append(row);
}
for (const c of cases.filter(c => c.metric)) {
  const figure = element('figure');
  const frame = plateButton(c);
  frame.className = 'frame';
  const img = frame.querySelector('img');
  img.loading = 'eager';
  const tile = element('span', undefined, 'tile');
  tile.setAttribute('aria-hidden', 'true');
  frame.append(tile, element('span','100 m','ruler'));
  const cap = element('figcaption');
  cap.append(link(c.name, '#case-' + c.id), element('span', c.count));
  figure.append(frame, cap, element('p', measuredRoute(c.metric),'fine'));
  get('maps').append(figure);
  function fit() {
    if (!img.naturalWidth || !frame.clientWidth) return;
    const scaled = frame.clientWidth / 600 * c.metric.metres / c.metric.pixels;
    const w = img.naturalWidth * scaled;
    const h = img.naturalHeight * scaled;
    img.style.width = w + 'px';
    img.style.height = h + 'px';
    img.style.left = (frame.clientWidth / 2 - w * c.metric.focus[0]) + 'px';
    img.style.top = (frame.clientHeight / 2 - h * c.metric.focus[1]) + 'px';
    tile.style.width = (255.05882352941177 / 600 * frame.clientWidth) + 'px';
    tile.style.height = (272.94117647058823 / 600 * frame.clientWidth) + 'px';
  }
  img.addEventListener('load', fit);
  new ResizeObserver(fit).observe(frame);
}
get('tiles').addEventListener('change', () => get('maps').classList.toggle('show-tile', get('tiles').checked));
for (const [i, name] of groups.entries()) get('group').add(new Option(name, i));
for (const unit of new Set(cases.map(c => c.unit))) get('unit').add(new Option(unit, unit));
function makeCase(c) {
  const card = element('article', undefined, 'case');
  card.id = 'case-' + c.id;
  const body = element('div', undefined, 'case-body');
  body.append(element('p', c.work, 'work'), element('h4', c.name), element('small', c.original, 'original'),
    element('strong', c.count, 'count'), element('p', c.when, 'when'), element('p', c.space, 'space'), element('p', c.note));
  const sources = element('div', undefined, 'sources');
  for (const [label, url] of c.links) sources.append(link(label + ' ↗', url));
  body.append(sources);
  if (c.plate) card.append(plateButton(c));
  card.append(body);
  return card;
}
function render() {
  const query = get('query').value.trim().toLocaleLowerCase();
  const group = get('group').value;
  const unit = get('unit').value;
  const matches = cases.filter(c => (group === '' || c.group === Number(group)) && (!unit || c.unit === unit) &&
    [c.name, c.original, c.work, c.space, c.count].join(' ').toLocaleLowerCase().includes(query));
  const out = get('cards');
  out.replaceChildren();
  for (const [i, name] of groups.entries()) {
    const entries = matches.filter(c => c.group === i);
    if (!entries.length) continue;
    const section = element('section', undefined, 'shelf');
    section.append(element('h3', name + ' · ' + entries.length + '곳'));
    const grid = element('div', undefined, 'grid');
    grid.append(...entries.map(makeCase));
    section.append(grid);
    out.append(section);
  }
  get('status').textContent = matches.length ? collate.format(matches.length) + '곳 표시' : '해당 사례가 없어. 검색어나 분류를 바꿔 봐.';
}
for (const key of ['query','group','unit']) get(key).addEventListener('input', render);
function reveal(hash) {
  document.querySelector('.case.current')?.classList.remove('current');
  if (!hash.startsWith('#case-') || !byId.has(hash.slice(6))) return;
  if (!document.querySelector(hash)) {
    for (const key of ['query','group','unit']) get(key).value = '';
    render();
  }
  const card = document.querySelector(hash);
  card.classList.add('current');
  card.scrollIntoView();
}
render();
window.addEventListener('hashchange', () => reveal(location.hash));
if (location.hash.startsWith('#case-')) requestAnimationFrame(() => reveal(location.hash));
const viewer = get('viewer');
function zoom() {
  const factor = Number(get('zoom').value);
  get('sheet').style.width = (100 * factor) + '%';
  get('factor').textContent = factor + '×';
}
document.addEventListener('click', event => {
  const button = event.target.closest('[data-open]');
  if (button) {
    const c = byId.get(button.dataset.open);
    get('view-title').textContent = c.name;
    get('view-work').textContent = c.work;
    const img = get('enlarged');
    img.alt = c.plate[1];
    img.src = 'plates/' + c.plate[0] + '.webp';
    const m = c.metric;
    const trace = get('trace');
    get('trace-label').hidden = !m;
    get('show-trace').checked = Boolean(m);
    trace.style.display = m ? 'block' : 'none';
    trace.replaceChildren();
    get('distance').textContent = m ? measuredRoute(m) + '. 원도판 ' + m.label + ' 눈금과 표식 중심을 기준으로 잰 거리다.' : '';
    if (m) {
      trace.setAttribute('viewBox', '0 0 ' + m.image.join(' '));
      const svg = (name, attrs) => {
        const node = document.createElementNS('http://www.w3.org/2000/svg', name);
        for (const [key, value] of Object.entries(attrs)) node.setAttribute(key, value);
        trace.append(node);
      };
      const line = {x1:m.a[0],y1:m.a[1],x2:m.b[0],y2:m.b[1]};
      svg('line',{...line,stroke:'#201a11','stroke-width':7});
      svg('line',{...line,stroke:'#fff4a5','stroke-width':4,'stroke-dasharray':'10 5'});
      for (const [cx,cy] of [m.a,m.b]) svg('circle',{cx,cy,r:9,fill:'none',stroke:'#ffef83','stroke-width':4});
    }
    const credit = get('credit');
    credit.replaceChildren(element('span', c.plate[2] + '　'));
    c.links.forEach(([label, url]) => credit.append(link(label + ' ↗', url), '　'));
    get('zoom').value = '1';
    zoom();
    viewer.showModal();
    viewer.querySelector('.pane').scrollTo(0,0);
    return;
  }
  const anchor = event.target.closest('a[href^="#case-"]');
  if (anchor) reveal(anchor.hash);
});
get('close').addEventListener('click', () => viewer.close());
get('zoom').addEventListener('input', zoom);
get('show-trace').addEventListener('change', event => get('trace').style.display = event.target.checked ? 'block' : 'none');
