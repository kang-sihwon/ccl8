using System;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;
public static class SettlementAssembly {
 const int Core=2048,Margin=128,Work=2304;
 static byte[] Read(string file){using(var src=new Bitmap(file))using(var b=new Bitmap(Work,Work,PixelFormat.Format24bppRgb)){using(var g=Graphics.FromImage(b)){g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.PixelOffsetMode=PixelOffsetMode.HighQuality;g.DrawImage(src,new Rectangle(0,0,Work,Work));}var d=b.LockBits(new Rectangle(0,0,Work,Work),ImageLockMode.ReadOnly,PixelFormat.Format24bppRgb);var p=new byte[Work*Work*3];Marshal.Copy(d.Scan0,p,0,p.Length);b.UnlockBits(d);return p;}}
 static Bitmap BitmapOf(byte[] p,int w,int h){var b=new Bitmap(w,h,PixelFormat.Format24bppRgb);var d=b.LockBits(new Rectangle(0,0,w,h),ImageLockMode.WriteOnly,PixelFormat.Format24bppRgb);Marshal.Copy(p,0,d.Scan0,p.Length);b.UnlockBits(d);return b;}
 static float Smooth(float x){x=Math.Max(0,Math.Min(1,x));return x*x*(3-2*x);}
 static float Weight(int k){return k<0||k>=Work?0:Smooth(Math.Min((k+.5f)/256,(Work-k-.5f)/256));}
 static bool ApplyRepairs(string root,string sid,Bitmap atlas,Graphics target){
  string manifest=root+"/repairs/patches.csv";if(!File.Exists(manifest))return false;bool changed=false;
  foreach(string line in File.ReadAllLines(manifest)){
   if(line.StartsWith("#")||string.IsNullOrWhiteSpace(line))continue;var fields=line.Split(',');if(fields[0]!=sid)continue;
   int x=int.Parse(fields[2]),y=int.Parse(fields[3]),w=int.Parse(fields[4]),h=int.Parse(fields[5]),feather=int.Parse(fields[6]);
   if(x<0||y<0||x+w>atlas.Width||y+h>atlas.Height||feather<=0)throw new InvalidDataException("Repair bounds outside atlas.");
   using(var source=new Bitmap(root+"/repairs/"+fields[1]))using(var patch=new Bitmap(w,h,PixelFormat.Format32bppArgb)){
    using(var g=Graphics.FromImage(patch)){g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.DrawImage(source,new Rectangle(0,0,w,h));}
    var locked=patch.LockBits(new Rectangle(0,0,w,h),ImageLockMode.ReadWrite,PixelFormat.Format32bppArgb);var data=new byte[locked.Stride*h];Marshal.Copy(locked.Scan0,data,0,data.Length);
    for(int py=0;py<h;py++)for(int px=0;px<w;px++){float edge=Math.Min(Math.Min(px+.5f,w-px-.5f),Math.Min(py+.5f,h-py-.5f));data[py*locked.Stride+px*4+3]=(byte)Math.Round(255*Smooth(edge/feather));}
    Marshal.Copy(data,0,locked.Scan0,data.Length);patch.UnlockBits(locked);target.DrawImageUnscaled(patch,x,y);changed=true;
   }
  }
  target.Flush();return changed;
 }
 sealed class Cut { public int A,B; public bool Horizontal; public int[] Path; }
 static Cut FindCut(byte[][] plates,int a,int b,bool horizontal){
  const int overlap=2*Margin;
  var prior=new float[overlap];var next=new float[overlap];var back=new sbyte[Work*overlap];
  for(int along=0;along<Work;along++){
   for(int cross=0;cross<overlap;cross++){
    int pa=horizontal?(along*Work+Core+cross)*3:((Core+cross)*Work+along)*3;
    int pb=horizontal?(along*Work+cross)*3:(cross*Work+along)*3;
    float cost=1+Math.Abs(cross-Margin)*.002f;
    for(int ch=0;ch<3;ch++)cost+=Math.Abs(plates[a][pa+ch]-plates[b][pb+ch]);
    int best=cross;float score=along==0?0:prior[cross];
    if(along>0)for(int step=-1;step<=1;step++){int candidate=cross+step;if(candidate>=0&&candidate<overlap&&prior[candidate]+Math.Abs(step)*.5f<score){best=candidate;score=prior[candidate]+Math.Abs(step)*.5f;}}
    next[cross]=cost+score;back[along*overlap+cross]=(sbyte)(best-cross);
   }
   var swap=prior;prior=next;next=swap;
  }
  int end=0;for(int k=1;k<overlap;k++)if(prior[k]<prior[end])end=k;
  var path=new int[Work];for(int along=Work-1;along>=0;along--){path[along]=end;end+=back[along*overlap+end];}
  return new Cut{A=a,B=b,Horizontal=horizontal,Path=path};
 }
 static float CutWeight(int tile,int px,int py,int[] cols,int[] rows,Cut[] cuts){
  float weight=1;
  foreach(var cut in cuts){
   if(cut.A!=tile&&cut.B!=tile)continue;
   int ax=px+(cols[tile]-cols[cut.A])*Core,ay=py+(rows[tile]-rows[cut.A])*Core;
   int cross=(cut.Horizontal?ax:ay)-Core,along=cut.Horizontal?ay:ax;
   if(cross<0||cross>=2*Margin||along<0||along>=Work)continue;
   float preference=Smooth((cut.Path[along]+8-cross)/16f);
   weight*=cut.A==tile?preference:1-preference;
  }
  return weight;
 }
 public static void Build(string root,string sid,string[] ids,int[] cols,int[] rows,double[] contextRect,double[] atlasRect){
  int n=ids.Length,minC=int.MaxValue,minR=int.MaxValue,maxC=0,maxR=0;var plates=new byte[n][];
  for(int i=0;i<n;i++){plates[i]=Read(root+"/generated/"+ids[i]+".png");minC=Math.Min(minC,cols[i]);minR=Math.Min(minR,rows[i]);maxC=Math.Max(maxC,cols[i]);maxR=Math.Max(maxR,rows[i]);}
  var cutList=new System.Collections.Generic.List<Cut>();
  for(int i=0;i<n;i++)for(int j=i+1;j<n;j++){
   if(Math.Abs(cols[i]-cols[j])+Math.Abs(rows[i]-rows[j])!=1)continue;
   bool horizontal=rows[i]==rows[j];bool forward=horizontal?cols[i]<cols[j]:rows[i]<rows[j];
   cutList.Add(FindCut(plates,forward?i:j,forward?j:i,horizontal));
  }
  var cuts=cutList.ToArray();
  int width=(maxC-minC+1)*Core,height=(maxR-minR+1)*Core;
  using(var atlas=new Bitmap(width,height,PixelFormat.Format24bppRgb))using(var ga=Graphics.FromImage(atlas)){
   ga.Clear(Color.FromArgb(21,51,58));ga.InterpolationMode=InterpolationMode.HighQualityBicubic;
   using(var context=new Bitmap(root+"/contexts/"+sid+".png")){double sx=width/(atlasRect[2]-atlasRect[0]),sy=height/(atlasRect[3]-atlasRect[1]);ga.DrawImage(context,new RectangleF((float)((contextRect[0]-atlasRect[0])*sx),(float)((contextRect[1]-atlasRect[1])*sy),(float)((contextRect[2]-contextRect[0])*sx),(float)((contextRect[3]-contextRect[1])*sy)));}
   for(int i=0;i<n;i++){
    var guide=Read(root+"/guides/"+ids[i]+".png");var data=new byte[Core*Core*3];bool west=false,east=false,north=false,south=false;
    for(int j=0;j<n;j++){west|=cols[j]==cols[i]-1&&rows[j]==rows[i];east|=cols[j]==cols[i]+1&&rows[j]==rows[i];north|=cols[j]==cols[i]&&rows[j]==rows[i]-1;south|=cols[j]==cols[i]&&rows[j]==rows[i]+1;}
    for(int y=0;y<Core;y++)for(int x=0;x<Core;x++){
     float total=0,b=0,g=0,r=0;
     for(int j=0;j<n;j++){int px=x+(cols[i]-cols[j])*Core+Margin,py=y+(rows[i]-rows[j])*Core+Margin;float w=Weight(px)*Weight(py);if(w==0)continue;w*=CutWeight(j,px,py,cols,rows,cuts);if(w==0)continue;int at=(py*Work+px)*3;total+=w;b+=plates[j][at]*w;g+=plates[j][at+1]*w;r+=plates[j][at+2]*w;}
     if(total<1e-12f){total=b=g=r=0;for(int j=0;j<n;j++){int px=x+(cols[i]-cols[j])*Core+Margin,py=y+(rows[i]-rows[j])*Core+Margin;float w=Weight(px)*Weight(py);if(w==0)continue;int at=(py*Work+px)*3;total+=w;b+=plates[j][at]*w;g+=plates[j][at+1]*w;r+=plates[j][at+2]*w;}}
     float edge=1;if(!west)edge=Math.Min(edge,(x+.5f)/128);if(!east)edge=Math.Min(edge,(Core-x-.5f)/128);if(!north)edge=Math.Min(edge,(y+.5f)/128);if(!south)edge=Math.Min(edge,(Core-y-.5f)/128);edge=Smooth(edge);
     int dest=(y*Core+x)*3,orig=((y+Margin)*Work+x+Margin)*3;data[dest]=(byte)Math.Round(b/total*edge+guide[orig]*(1-edge));data[dest+1]=(byte)Math.Round(g/total*edge+guide[orig+1]*(1-edge));data[dest+2]=(byte)Math.Round(r/total*edge+guide[orig+2]*(1-edge));
    }
    using(var tile=BitmapOf(data,Core,Core)){tile.Save(root+"/tiles/"+ids[i]+".png",ImageFormat.Png);ga.DrawImageUnscaled(tile,(cols[i]-minC)*Core,(rows[i]-minR)*Core);}
   }
   if(ApplyRepairs(root,sid,atlas,ga))for(int i=0;i<n;i++)using(var tile=atlas.Clone(new Rectangle((cols[i]-minC)*Core,(rows[i]-minR)*Core,Core,Core),PixelFormat.Format24bppRgb))tile.Save(root+"/tiles/"+ids[i]+".png",ImageFormat.Png);
   atlas.Save(root+"/previews/"+sid+"-assembled.png",ImageFormat.Png);
   double ratio=Math.Min(1,2048.0/width);using(var thumb=new Bitmap((int)(width*ratio),(int)(height*ratio)))using(var g=Graphics.FromImage(thumb)){g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.DrawImage(atlas,new Rectangle(0,0,thumb.Width,thumb.Height));thumb.Save(root+"/previews/"+sid+"-overview.png",ImageFormat.Png);}
   int edges=0;for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)if(Math.Abs(cols[i]-cols[j])+Math.Abs(rows[i]-rows[j])==1)edges++;
   if(edges>0)using(var sheet=new Bitmap(1200,edges*260))using(var g=Graphics.FromImage(sheet))using(var f=new Font("Arial",16,GraphicsUnit.Pixel)){
    g.Clear(Color.FromArgb(30,32,32));g.InterpolationMode=InterpolationMode.HighQualityBicubic;int e=0;
    for(int i=0;i<n;i++)for(int j=i+1;j<n;j++){
     if(Math.Abs(cols[i]-cols[j])+Math.Abs(rows[i]-rows[j])!=1)continue;g.DrawString(ids[i]+" / "+ids[j],f,Brushes.White,8,e*260+4);Rectangle rect;
     if(rows[i]==rows[j]){rect=new Rectangle((Math.Max(cols[i],cols[j])-minC)*Core-160,(rows[i]-minR)*Core,320,Core);using(var strip=atlas.Clone(rect,PixelFormat.Format24bppRgb)){strip.RotateFlip(RotateFlipType.Rotate90FlipNone);g.DrawImage(strip,new Rectangle(0,e*260+28,1200,230));}}
     else{rect=new Rectangle((cols[i]-minC)*Core,(Math.Max(rows[i],rows[j])-minR)*Core-160,Core,320);g.DrawImage(atlas,new Rectangle(0,e*260+28,1200,230),rect,GraphicsUnit.Pixel);}e++;
    }sheet.Save(root+"/previews/"+sid+"-seams.png",ImageFormat.Png);
   }
  }
 }
}
