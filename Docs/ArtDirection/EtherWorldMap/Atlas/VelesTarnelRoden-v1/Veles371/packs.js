'use strict';
const blocks=new Map(), loading=new Map(), drawings=new Map();
const decoder=new TextDecoder();
function blockId(i,j){return [Math.floor((i+world.radius)/world.block),Math.floor((j+world.radius)/world.block)].join('_')}
function plateBlock(id,text){
  const raw=Uint8Array.from(atob(text),c=>c.charCodeAt(0));
  const view=new DataView(raw.buffer),end=raw.length-22;
  if(view.getUint32(end,true)!==0x06054b50)throw Error('ZIP 끝을 읽을 수 없어.');
  let at=view.getUint32(end+16,true);
  const files=new Map();
  for(let n=0;n<view.getUint16(end+10,true);n++){
    if(view.getUint32(at,true)!==0x02014b50)throw Error('ZIP 색인을 읽을 수 없어.');
    const len=view.getUint16(at+28,true),name=decoder.decode(raw.subarray(at+46,at+46+len));
    const start=view.getUint32(at+42,true);
    const offset=start+30+view.getUint16(start+26,true)+view.getUint16(start+28,true);
    files.set(name,{offset,bytes:view.getUint32(at+20,true),size:view.getUint32(at+24,true)});
    at+=46+len+view.getUint16(at+30,true)+view.getUint16(at+32,true);
  }
  blocks.set(id,{raw,files});
  if(blocks.size>12)blocks.delete(blocks.keys().next().value);
}
function getBlock(id){
  if(blocks.has(id))return Promise.resolve(blocks.get(id));
  if(loading.has(id))return loading.get(id);
  const promise=new Promise((resolve,reject)=>{
    const script=document.createElement('script');
    script.onload=()=>{script.remove();loading.delete(id);if(blocks.has(id))resolve(blocks.get(id));else reject(Error('구역 색인을 확인해 줘.'))};
    script.onerror=()=>{script.remove();loading.delete(id);reject(Error('구역 파일을 열 수 없어.'))};
    script.src='blocks/'+id+'.js';document.head.append(script);
  });
  loading.set(id,promise);return promise;
}
async function loadDrawing(i,j){
  const k=i+'_'+j;
  if(drawings.has(k))return drawings.get(k);
  const {raw,files}=await getBlock(blockId(i,j)),f=files.get(k+'.svg');
  if(!f)throw Error('타일 색인을 확인해 줘.');
  const input=new Blob([raw.subarray(f.offset,f.offset+f.bytes)]);
  const bytes=await new Response(input.stream().pipeThrough(new DecompressionStream('deflate-raw'))).arrayBuffer();
  if(bytes.byteLength!==f.size)throw Error('도면 크기가 색인과 달라.');
  if(drawings.has(k))return drawings.get(k);
  const text=decoder.decode(new Uint8Array(bytes,0,Math.min(bytes.byteLength,620)));
  const dimensions=text.match(/viewBox="0 0 ([\d.]+) ([\d.]+)"/);
  const url=URL.createObjectURL(new Blob([bytes],{type:'image/svg+xml'}));
  const item={url,w:Number(dimensions[1]),h:Number(dimensions[2])};drawings.set(k,item);
  if(drawings.size>90){
    const oldest=drawings.keys().next().value;URL.revokeObjectURL(drawings.get(oldest).url);drawings.delete(oldest);
  }
  return item;
}

