'use strict';
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const base = path.resolve(__dirname, '../../Docs/ArtDirection/EtherWorldMap/Atlas/VelesTarnelRoden-v1');
const context = vm.createContext({});
for (const name of ['geography', 'lore', 'layouts', 'hydrology']) {
  const file = path.join(base, 'TernBasin377', name+'.js');
  vm.runInContext(fs.readFileSync(file, 'utf8'), context, {filename:file});
}
const result = vm.runInContext('({regions, localPlans, crossings, habitats, ways:ways.map(({id,kind,width,name,points})=>({id,kind,width,name,points}))})', context);
process.stdout.write(JSON.stringify(result));

