// Measure coverage without generating terrain or changing existing coordinates.
const fs = require('node:fs');
const path = require('node:path');
const root = __dirname;
const read = p => JSON.parse(fs.readFileSync(p, 'utf8').replace(/^\uFEFF/, ''));
const source = read(path.join(root, '../WorldSectorPlan-v1/sector-plan.json'));
const plan = read(path.join(root, 'records/plan.json'));
const coast = source.sharedGeometry.features.find(f => f.kind === 'coast').worldPixelPoints;
let area = 0;
for (let i = 0; i < coast.length; i++) {
  const a = coast[i], b = coast[(i + 1) % coast.length];
  area += a[0] * b[1] - b[0] * a[1];
}
area = Math.abs(area) / 2;
// Even-odd center scan plus all boundary-crossed cells. This conservatively
// includes cells touching the coast; no geographic area is inferred from pixels.
function count(level) {
  const span = 2048 / 2 ** level;
  const points = coast.map(p => p.map(v => v / span));
  const rows = new Map();
  const add = (r, lo, hi) => {
    if (hi < lo) return;
    if (!rows.has(r)) rows.set(r, []);
    rows.get(r).push([lo, hi]);
  };
  const minRow = Math.floor(Math.min(...points.map(p => p[1])));
  const maxRow = Math.floor(Math.max(...points.map(p => p[1])));
  for (let r = minRow; r <= maxRow; r++) {
    const y = r + 0.5, xs = [];
    for (let i = 0; i < points.length; i++) {
      const a = points[i], b = points[(i + 1) % points.length];
      if ((a[1] > y) !== (b[1] > y)) xs.push(a[0] + (y - a[1]) * (b[0] - a[0]) / (b[1] - a[1]));
    }
    xs.sort((a, b) => a - b);
    if (xs.length % 2) throw new Error('Unpaired coast intersections');
    for (let i = 0; i < xs.length; i += 2) add(r, Math.ceil(xs[i] - 0.5), Math.floor(xs[i + 1] - 0.5));
  }
  for (let i = 0; i < points.length; i++) {
    let a = points[i], b = points[(i + 1) % points.length];
    if (a[1] > b[1]) [a, b] = [b, a];
    for (let r = Math.floor(a[1]); r <= Math.floor(b[1]); r++) {
      const dy = b[1] - a[1];
      const x = y => dy === 0 ? a[0] : a[0] + (y - a[1]) / dy * (b[0] - a[0]);
      const ends = dy === 0 ? [a[0], b[0]] : [x(Math.max(r, a[1])), x(Math.min(r + 1, b[1]))];
      add(r, Math.floor(Math.min(...ends)), Math.floor(Math.max(...ends)));
    }
  }
  let total = 0;
  const intervals = [];
  for (const [r, ranges] of [...rows].sort((a, b) => a[0] - b[0])) {
    ranges.sort((a, b) => a[0] - b[0]);
    const merged = [];
    for (const [lo, hi] of ranges) {
      const last = merged.at(-1);
      if (last && lo <= last[1] + 1) last[1] = Math.max(last[1], hi);
      else merged.push([lo, hi]);
    }
    for (const [lo, hi] of merged) { total += hi - lo + 1; intervals.push([r, lo, hi]); }
  }
  const equivalentAreaTiles = area / span ** 2;
  if (total < equivalentAreaTiles) throw new Error('Coverage smaller than polygon area');
  return { level, tileCount: total, equivalentAreaTiles, intervals };
}
const levels = [0, 4, 6, 8, 10].map(count);
const tiles = plan.settlements.flatMap(s => s.tiles);
const sizes = tiles.map(t => fs.statSync(path.join(root, t.path)).size);
const meanBytes = sizes.reduce((a, b) => a + b, 0) / sizes.length;
const currentL10Equivalent = tiles.reduce((a, t) => a + 4 ** (10 - t.level), 0);
const l10 = levels.at(-1);
fs.writeFileSync(path.join(root, 'records/full-coverage-row-ranges.csv'), 'level,row,columnStartInclusive,columnEndInclusive\n' + l10.intervals.map(a => [10, ...a].join(',')).join('\n') + '\n');
const result = {
  status: 'capacity_audited_individual_png_coverage_not_feasible_here',
  requestedScope: 'all_ignis_land_at_existing_settlement_detail_density',
  terrainGeneratedByAudit: 0,
  source: '../WorldSectorPlan-v1/sector-plan.json',
  method: 'even-odd tile-center scan plus conservative coast boundary cells',
  polygonPoints: coast.length,
  levels: levels.map(({ intervals, ...v }) => v),
  current: { files: tiles.length, l10EquivalentAreaTiles: currentL10Equivalent, meanOutputBytes: meanBytes },
  l10: {
    rectanglePixels: source.ignis.pixelSize.map(v => v * 1024),
    rectangleTiles: source.counts.ignisRectangleSectors * 4 ** 10,
    landIncludingCoastTiles: l10.tileCount,
    currentAreaFractionUpperBound: currentL10Equivalent / l10.equivalentAreaTiles,
    estimatedOutputBytes: l10.tileCount * meanBytes,
    lowerBoundUniqueLandPixelRGBABytes: l10.equivalentAreaTiles * 2048 ** 2 * 4,
  },
  limitations: [
    'Counts apply to the existing drawn coast, not surveyed game geography.',
    'Coverage includes coast-touching cells conservatively.',
    'File size is extrapolated from the current 150 settlement cores, not measured wilderness output.',
    'Estimate excludes source images, overlap, repairs, mipmaps and LFS duplicates.',
    'L10 is the city density; L11 village density requires approximately four times the area tiles.',
    'No production time estimate or whole-continent completion claim is made.',
  ],
};
fs.writeFileSync(path.join(root, 'records/full-coverage-audit.json'), JSON.stringify(result, null, 2) + '\n');
console.log(JSON.stringify(result, null, 2));
