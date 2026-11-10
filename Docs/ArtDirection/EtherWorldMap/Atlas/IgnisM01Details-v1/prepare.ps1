$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$root = $PSScriptRoot
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../../../../..')).Path
foreach ($dir in @('guides','generated','records','previews','tiles')) { New-Item -ItemType Directory -Force -Path "$root/$dir" | Out-Null }
Add-Type -ReferencedAssemblies @('System.Drawing.Common','System.Drawing.Primitives','System.Runtime','System.Collections','System.Runtime.InteropServices','System.Private.Windows.GdiPlus','System.Private.Windows.Core') -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class DetailGrid {
 public const double W=67108864.0, H=33554432.0, R=6366.197723675814;
 public static double[] World(double x,double y) {
  double p0=54*Math.PI/180,l0=-Math.PI/180,r=Math.Sqrt(x*x+y*y),c=r/R;
  double p=Math.Asin(Math.Cos(c)*Math.Sin(p0)+y*Math.Sin(c)*Math.Cos(p0)/r);
  double l=l0+Math.Atan2(x*Math.Sin(c),r*Math.Cos(p0)*Math.Cos(c)-y*Math.Sin(p0)*Math.Sin(c));
  return new[]{(l+Math.PI)/(2*Math.PI)*W,(Math.PI/2-p)/Math.PI*H};
 }
 public static double[] Local(double wx,double wy) {
  double p0=54*Math.PI/180,l0=-Math.PI/180,p=Math.PI/2-wy/H*Math.PI,l=wx/W*2*Math.PI-Math.PI;
  double c=Math.Acos(Math.Min(1,Math.Max(-1,Math.Sin(p0)*Math.Sin(p)+Math.Cos(p0)*Math.Cos(p)*Math.Cos(l-l0))));
  double k=c==0?1:c/Math.Sin(c);
  return new[]{R*k*Math.Cos(p)*Math.Sin(l-l0),R*k*(Math.Cos(p0)*Math.Sin(p)-Math.Sin(p0)*Math.Cos(p)*Math.Cos(l-l0))};
 }
 public static void Guides(string src,string dir) {
  using(var input=new Bitmap(src)) using(var clean=new Bitmap(input.Width,input.Height,PixelFormat.Format24bppRgb)) {
   using(var g=Graphics.FromImage(clean)) g.DrawImageUnscaled(input,0,0);
   var sd=clean.LockBits(new Rectangle(0,0,clean.Width,clean.Height),ImageLockMode.ReadOnly,PixelFormat.Format24bppRgb);
   byte[] source=new byte[sd.Stride*sd.Height];Marshal.Copy(sd.Scan0,source,0,source.Length);clean.UnlockBits(sd);
   for(int row=3286;row<=3287;row++) for(int col=16316;col<=16321;col++) {
    using(var b=new Bitmap(2304,2304,PixelFormat.Format24bppRgb)) {
     var bd=b.LockBits(new Rectangle(0,0,2304,2304),ImageLockMode.WriteOnly,PixelFormat.Format24bppRgb);
     byte[] data=new byte[bd.Stride*2304];
     for(int y=0;y<2304;y++) for(int x=0;x<2304;x++) {
      double[] xy=Local(col*2048.0-128+x+.5,row*2048.0-128+y+.5);
      double sx=(xy[0]-14.5)/7*clean.Width-.5,sy=(-10.35-xy[1])/3.5*clean.Height-.5;
      sx=Math.Max(0,Math.Min(clean.Width-1.001,sx));sy=Math.Max(0,Math.Min(clean.Height-1.001,sy));
      int ix=(int)sx,iy=(int)sy;double fx=sx-ix,fy=sy-iy;
      for(int ch=0;ch<3;ch++) data[y*bd.Stride+x*3+ch]=(byte)Math.Round((1-fy)*((1-fx)*source[iy*sd.Stride+ix*3+ch]+fx*source[iy*sd.Stride+(ix+1)*3+ch])+fy*((1-fx)*source[(iy+1)*sd.Stride+ix*3+ch]+fx*source[(iy+1)*sd.Stride+(ix+1)*3+ch]));
     }
     Marshal.Copy(data,0,bd.Scan0,data.Length);b.UnlockBits(bd);b.Save(dir+"/L10-C"+col+"-R"+row+".png",ImageFormat.Png);
    }
   }
  }
 }
}
'@
if (!(Test-Path "$root/guides/L10-C16321-R3287.png")) { [DetailGrid]::Guides("$repo/Docs/ArtDirection/EtherWorldMap/Atlas/Ignis-v1/M01-terrain.png","$root/guides") }
$m01 = (Get-Content "$repo/Docs/ArtDirection/EtherWorldMap/Atlas/Ignis-v1/provenance/detail-plans.json" -Raw | ConvertFrom-Json)[0]
$tiles = @(); for($r=3286;$r -le 3287;$r++){ for($c=16316;$c -le 16321;$c++){ $tiles += [ordered]@{id="L10-C$c-R$r";column=$c;row=$r;worldPixelBounds=@(($c*2048),($r*2048),(($c+1)*2048),(($r+1)*2048));parentL0="L0-C$([math]::Floor($c/1024))-R$([math]::Floor($r/1024))"} } }
$points=@(); foreach($p in $m01.details){ $v=[DetailGrid]::World($p.xy_km[0],$p.xy_km[1]); $points += [ordered]@{id=$p.id;name=$p.name;xy_km=$p.xy_km;worldPixel=$v;atlasPixel=@(($v[0]-16316*2048),($v[1]-3286*2048));status='existing_detail_candidate_not_canon'} }
$plan=[ordered]@{version=1;date='2026-11-11';subject='M01 central city and approaches';status='visual_candidate_not_gameplay_asset';level=10;worldSize=@(67108864,33554432);core=2048;margin=128;workingSize=2304;columns=@(16316,16321);rows=@(3286,3287);atlasSize=@(12288,4096);worldPixelBounds=@((16316*2048),(3286*2048),(16322*2048),(3288*2048));northWestLocalKm=[DetailGrid]::Local(16316*2048,3286*2048);southEastLocalKm=[DetailGrid]::Local(16322*2048,3288*2048);tiles=$tiles;points=$points;source='../Ignis-v1/M01-terrain.png';sourceSize=@(1774,887);sourceBoundsLocalKm=$m01.bounds_xy_km;projection='World equirectangular L10; source spherical AEQD at lon -1 lat 54 radius 6366.197723675814 km';limitations=@('M01 drawing is a layout candidate; not measured building dimensions','L0 parent has no settlement detail to validate building continuity against','Equirectangular chart stretches east-west near latitude 54');}
$plan | ConvertTo-Json -Depth 12 | Set-Content "$root/records/plan.json" -Encoding utf8
$plan | Select-Object atlasSize,northWestLocalKm,southEastLocalKm | ConvertTo-Json
