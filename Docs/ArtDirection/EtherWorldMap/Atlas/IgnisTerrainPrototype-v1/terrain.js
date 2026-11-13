// Deterministic shared-coordinate surface synthesis. This is not a DEM.
class SharedTerrain {
  constructor(materialUrl, macroUrl) {
    this.canvas = document.createElement('canvas');
    const gl = this.gl = this.canvas.getContext('webgl2', {alpha:false, preserveDrawingBuffer:true});
    if (!gl) throw new Error('WebGL2 unavailable');
    const compile = (type, text) => {
      const shader = gl.createShader(type); gl.shaderSource(shader, text); gl.compileShader(shader);
      if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) throw new Error(gl.getShaderInfoLog(shader));
      return shader;
    };
    const vs = compile(gl.VERTEX_SHADER, `#version 300 es
      in vec2 position; out vec2 uv;
      void main(){uv=vec2(position.x*.5+.5,.5-position.y*.5);gl_Position=vec4(position,0,1);}`);
    const fs = compile(gl.FRAGMENT_SHADER, `#version 300 es
      precision highp float;
      in vec2 uv; out vec4 color;
      uniform sampler2D materials,macro;
      uniform vec2 origin,span;
      uniform vec4 bounds;
      uniform float detail;
      uniform int roadCount,riverCount;
      uniform vec4 roads[64],rivers[64];
      uniform float riverWidths[64];
      uniform int joinCount;
      uniform vec4 joins[4],joinValues[4];
      float hash(vec2 p){vec3 p3=fract(vec3(p.xyx)*.1031);p3+=dot(p3,p3.yzx+33.33);return fract((p3.x+p3.y)*p3.z);}
      float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1)),f.x),f.y);}
      float fbm(vec2 p){return .55*noise(p)+.28*noise(p*2.03+19.)+.12*noise(p*4.01-7.)+.05*noise(p*8.03);}
      vec3 samplePatch(vec2 p, vec2 cell, vec2 quadrant,float lod){
        vec2 q=(p-cell)*.46+.5;
        float r=hash(cell+quadrant*93.);
        if(r<.25)q=vec2(q.y,1.-q.x);else if(r<.5)q=1.-q;else if(r<.75)q=vec2(1.-q.y,q.x);
        return textureLod(materials,(quadrant+.018+.964*clamp(q,0.,1.))*.5,lod).rgb;
      }
      vec3 material(vec2 p,vec2 quadrant){
        vec2 i=floor(p),f=fract(p);vec2 a=pow(1.-f,vec2(4)),b=pow(f,vec2(4));
        vec4 w=vec4(a.x*a.y,b.x*a.y,a.x*b.y,b.x*b.y);w/=dot(w,vec4(1));
        float lod=max(0.,log2(max(length(dFdx(p)),length(dFdy(p)))*300.));
        return samplePatch(p,i,quadrant,lod)*w.x+samplePatch(p,i+vec2(1,0),quadrant,lod)*w.y+samplePatch(p,i+vec2(0,1),quadrant,lod)*w.z+samplePatch(p,i+vec2(1),quadrant,lod)*w.w;
      }
      float segmentDistance(vec2 p,vec4 s){vec2 d=s.zw-s.xy;float t=clamp(dot(p-s.xy,d)/max(dot(d,d),.001),0.,1.);return length(p-s.xy-t*d);}
      void main(){
        // Coordinates relative to one fixed global origin avoid camera-seeded noise.
        vec2 p=origin+uv*span;
        vec2 global=p+vec2(33554432.,8388608.);
        vec2 buv=(global-bounds.xy)/(bounds.zw-bounds.xy);
        vec3 base=texture(macro,clamp(buv,0.,1.)).rgb;
        p.x*=.58778525;
        float roadDistance=1e9,riverEdge=1e9;
        for(int i=0;i<64;i++){if(i>=roadCount)break;roadDistance=min(roadDistance,segmentDistance(p,roads[i]));}
        for(int i=0;i<64;i++){if(i>=riverCount)break;riverEdge=min(riverEdge,segmentDistance(p,rivers[i])-riverWidths[i]);}
        float halfRoad=5.,riverExtra=0.;
        for(int i=0;i<4;i++){if(i>=joinCount)break;float blend=1.-smoothstep(joins[i].z,joins[i].w,length(p-joins[i].xy));halfRoad=max(halfRoad,mix(5.,joinValues[i].x,blend));riverExtra=max(riverExtra,joinValues[i].y*blend);}
        riverEdge-=riverExtra;
        float green=base.g-(base.r+base.b)*.5;
        float lum=dot(base,vec3(.299,.587,.114));
        float landscape=fbm(p/8192.);
        float grove=fbm(p/900.+vec2(42,13));
        float forest=smoothstep(.39,.66,landscape*.35+grove*.65+green*.7);
        forest*=smoothstep(halfRoad+6.,halfRoad+25.,roadDistance)*smoothstep(4.,28.,riverEdge);
        float rock=smoothstep(.64,.83,fbm(p/1600.+vec2(123,7))+(lum-.3)*.35-green*.7);
        vec2 warp=vec2(noise(p/180.),noise(p/180.+44.))*8.;
        vec3 grass=material((p+warp)/160.,vec2(0,0));
        vec3 trees=material(p/220.,vec2(1,0));
        vec3 stones=material(p/130.,vec2(0,1));
        vec3 fine=mix(mix(grass,trees,forest),stones,rock*.85);
        float relief=fbm(p/2200.);
        float shade=.83+.27*relief+.12*(fbm((p+vec2(22,-30))/2200.)-relief)*30.;
        vec3 tint=clamp(base/(lum+.001),vec3(.7),vec3(1.3));
        fine*=mix(vec3(1),tint,.22)*clamp(shade,.68,1.15);
        // Snow and arid regions inherit the macro reference but remain proposals.
        fine=mix(fine,vec3(.75,.77,.74)*(shade*.9),smoothstep(.64,.85,lum));
        float arid=smoothstep(.03,.13,base.r-base.g)*(1.-forest);
        fine=mix(fine,material(p/140.,vec2(1,1))*vec3(1.12,.99,.85),arid*.65);
        float rough=noise(p/5.)*.7+noise(p/19.)*1.8;
        float roadMask=1.-smoothstep(halfRoad-1.+rough,halfRoad+1.+rough,roadDistance);
        vec3 dirt=material(p/65.,vec2(1,1))*vec3(1.05,1.02,.91);
        fine=mix(fine,dirt,roadMask*.94);
        float ruts=1.-smoothstep(.35,1.05,abs(roadDistance-2.2));
        fine*=1.-ruts*.15*roadMask;
        float bank=1.-smoothstep(0.,11.,riverEdge+rough*2.);
        fine=mix(fine,dirt*vec3(.87,.91,.86),bank);
        float water=1.-smoothstep(-3.,1.,riverEdge+rough*2.);
        vec3 waterColor=mix(vec3(.105,.21,.215),vec3(.21,.33,.31),fbm(p/35.)*.6+noise(p/2.)*.15);
        fine=mix(fine,waterColor,water);
        color=vec4(mix(base,fine,detail),1);
      }`);
    const program = this.program = gl.createProgram(); gl.attachShader(program,vs);gl.attachShader(program,fs);gl.linkProgram(program);
    if(!gl.getProgramParameter(program,gl.LINK_STATUS))throw new Error(gl.getProgramInfoLog(program));
    gl.useProgram(program);
    const buffer=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,buffer);gl.bufferData(gl.ARRAY_BUFFER,new Float32Array([-1,-1,1,-1,-1,1,-1,1,1,-1,1,1]),gl.STATIC_DRAW);
    const attr=gl.getAttribLocation(program,'position');gl.enableVertexAttribArray(attr);gl.vertexAttribPointer(attr,2,gl.FLOAT,false,0,0);
    this.uniforms={};for(const name of ['origin','span','bounds','detail','roadCount','riverCount','roads','rivers','riverWidths','joinCount','joins','joinValues'])this.uniforms[name]=gl.getUniformLocation(program,name);
    this.ready=Promise.all([this.texture(materialUrl,0,'materials'),this.texture(macroUrl,1,'macro')]).then(()=>this.loaded=true);
  }
  texture(url,unit,name){return new Promise((resolve,reject)=>{
    const im=new Image();im.onerror=()=>reject(new Error('Texture load failed: '+name));im.onload=()=>{
      const gl=this.gl;gl.activeTexture(gl.TEXTURE0+unit);const tex=gl.createTexture();gl.bindTexture(gl.TEXTURE_2D,tex);
      gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,im);
      gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);
      gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR_MIPMAP_LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);gl.generateMipmap(gl.TEXTURE_2D);
      gl.useProgram(this.program);gl.uniform1i(gl.getUniformLocation(this.program,name),unit);resolve();};im.src=url;
  });}
  render(w,h,view,bounds,amount=1,features=[]){
    if(!this.loaded)return null;const gl=this.gl;
    if(this.canvas.width!==w||this.canvas.height!==h){this.canvas.width=w;this.canvas.height=h;}
    gl.viewport(0,0,w,h);gl.useProgram(this.program);
    gl.uniform2f(this.uniforms.origin,view[0]-33554432,view[1]-8388608);
    gl.uniform2f(this.uniforms.span,view[2]-view[0],view[3]-view[1]);
    const road=[],river=[],widths=[];
    for(const f of features){if(f.kind!=='river'&&!(f.kind==='route'&&f.mode==='road'))continue;
      for(let i=1;i<f.worldPixelPoints.length;i++){const a=f.worldPixelPoints[i-1],b=f.worldPixelPoints[i];
        if(Math.max(a[0],b[0])<view[0]-1024||Math.min(a[0],b[0])>view[2]+1024||Math.max(a[1],b[1])<view[1]-1024||Math.min(a[1],b[1])>view[3]+1024)continue;
        const target=f.kind==='river'?river:road;if(target.length>=256)continue;
        target.push((a[0]-33554432)*.58778525,a[1]-8388608,(b[0]-33554432)*.58778525,b[1]-8388608);
        if(f.kind==='river')widths.push((f.id==='H01'?180:55)/.596/2);
      }
    }
    gl.uniform1i(this.uniforms.roadCount,road.length/4);gl.uniform1i(this.uniforms.riverCount,river.length/4);
    if(road.length)gl.uniform4fv(this.uniforms.roads,road);if(river.length){gl.uniform4fv(this.uniforms.rivers,river);gl.uniform1fv(this.uniforms.riverWidths,widths);}
    const joins=(this.joins||[]).slice(0,4);gl.uniform1i(this.uniforms.joinCount,joins.length);
    if(joins.length){gl.uniform4fv(this.uniforms.joins,joins.flatMap(j=>[(j.center[0]-33554432)*.58778525,j.center[1]-8388608,j.inner,j.outer]));gl.uniform4fv(this.uniforms.joinValues,joins.flatMap(j=>[j.roadHalf,j.riverExtra,0,0]));}
    gl.uniform4fv(this.uniforms.bounds,bounds);gl.uniform1f(this.uniforms.detail,amount);gl.drawArrays(gl.TRIANGLES,0,6);
    if(gl.getError()!==gl.NO_ERROR)throw new Error('Terrain render error');return this.canvas;
  }
}
