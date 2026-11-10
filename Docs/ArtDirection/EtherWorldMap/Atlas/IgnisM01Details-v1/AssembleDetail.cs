using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class AssembleDetail {
 const int Core=2048, Margin=128, Work=2304, FirstC=16316, FirstR=3286, Cols=6, Rows=2;
 static string Id(int c,int r) { return "L10-C"+c+"-R"+r; }
 static byte[] Read(string path) {
  using(var src=new Bitmap(path)) using(var b=new Bitmap(Work,Work,PixelFormat.Format24bppRgb)) {
   using(var g=Graphics.FromImage(b)) {g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.PixelOffsetMode=PixelOffsetMode.HighQuality;g.DrawImage(src,new Rectangle(0,0,Work,Work));}
   var d=b.LockBits(new Rectangle(0,0,Work,Work),ImageLockMode.ReadOnly,PixelFormat.Format24bppRgb);
   var bytes=new byte[Work*Work*3];Marshal.Copy(d.Scan0,bytes,0,bytes.Length);b.UnlockBits(d);return bytes;
  }
 }
 static Bitmap BitmapOf(byte[] pixels,int width,int height) {
  var b=new Bitmap(width,height,PixelFormat.Format24bppRgb);
  var d=b.LockBits(new Rectangle(0,0,width,height),ImageLockMode.WriteOnly,PixelFormat.Format24bppRgb);Marshal.Copy(pixels,0,d.Scan0,pixels.Length);b.UnlockBits(d);return b;
 }
 static float Weight(int pos) {if(pos<0||pos>=Work)return 0;float t=Math.Min(1f,Math.Min((pos+.5f)/256,(Work-pos-.5f)/256));return t*t*(3-2*t);}
 public static void Build(string root) {
  var plates=new byte[Rows*Cols][];
  for(int r=FirstR;r<FirstR+Rows;r++)for(int c=FirstC;c<FirstC+Cols;c++)plates[(r-FirstR)*Cols+c-FirstC]=Read(root+"/generated/"+Id(c,r)+".png");
  using(var atlas=new Bitmap(Cols*Core,Rows*Core,PixelFormat.Format24bppRgb)) using(var ga=Graphics.FromImage(atlas)) {
   for(int r=FirstR;r<FirstR+Rows;r++)for(int c=FirstC;c<FirstC+Cols;c++) {
    var data=new byte[Core*Core*3];
    for(int y=0;y<Core;y++)for(int x=0;x<Core;x++) {
     float sum=0,b=0,g=0,red=0;
     for(int rr=Math.Max(FirstR,r-1);rr<=Math.Min(FirstR+Rows-1,r+1);rr++) for(int cc=Math.Max(FirstC,c-1);cc<=Math.Min(FirstC+Cols-1,c+1);cc++) {
      int px=x+(c-cc)*Core+Margin,py=y+(r-rr)*Core+Margin;float w=Weight(px)*Weight(py);if(w==0)continue;
      byte[] p=plates[(rr-FirstR)*Cols+cc-FirstC];int si=(py*Work+px)*3;b+=w*p[si];g+=w*p[si+1];red+=w*p[si+2];sum+=w;
     }
     int di=(y*Core+x)*3;data[di]=(byte)Math.Round(b/sum);data[di+1]=(byte)Math.Round(g/sum);data[di+2]=(byte)Math.Round(red/sum);
    }
    using(var tile=BitmapOf(data,Core,Core)) {tile.Save(root+"/tiles/"+Id(c,r)+".png",ImageFormat.Png);ga.DrawImageUnscaled(tile,(c-FirstC)*Core,(r-FirstR)*Core);}
   }
   atlas.Save(root+"/previews/m01-assembled.png",ImageFormat.Png);
   using(var p=new Bitmap(3072,1024)) {using(var gp=Graphics.FromImage(p)){gp.InterpolationMode=InterpolationMode.HighQualityBicubic;gp.DrawImage(atlas,0,0,3072,1024);}p.Save(root+"/previews/m01-overview.png",ImageFormat.Png);}
  }
 }
 public static void Comparison(string root) {
  var src=Read(root+"/guides/L10-C16318-R3286.png");
  using(var before=BitmapOf(src,Work,Work)) using(var after=new Bitmap(root+"/generated/L10-C16318-R3286.png")) using(var b=new Bitmap(1672,941))using(var g=Graphics.FromImage(b)) {
   g.Clear(Color.FromArgb(30,32,32));g.InterpolationMode=InterpolationMode.HighQualityBicubic;
   g.DrawImage(before,new Rectangle(0,65,830,830));g.DrawImage(after,new Rectangle(842,65,830,830));
   using(var font=new Font("Malgun Gothic",18,FontStyle.Regular,GraphicsUnit.Pixel)) {g.DrawString("기존 M01 같은 위치 확대",font,Brushes.White,25,20);g.DrawString("새 상세 도안 · 수직 시점 보정",font,Brushes.White,866,20);g.DrawString("배치 참고 시안 · 개별 건물 치수와 실제 게임 좌표 미확정",font,Brushes.White,25,910);}
   b.Save(root+"/previews/m01-detail-comparison.png",ImageFormat.Png);
  }
 }
 public static void Repair(string root) {
  byte[] pixels;
  using(var src=new Bitmap(root+"/previews/m01-assembled.png")) {
   var d=src.LockBits(new Rectangle(0,0,12288,4096),ImageLockMode.ReadOnly,PixelFormat.Format24bppRgb);pixels=new byte[12288*4096*3];Marshal.Copy(d.Scan0,pixels,0,pixels.Length);src.UnlockBits(d);
  }
  foreach(int center in new[]{4096,6144,8192}) using(var src=new Bitmap(root+"/repairs/seam-"+center+"-after.png")) using(var patch=new Bitmap(1024,2048,PixelFormat.Format24bppRgb)) {
   using(var g=Graphics.FromImage(patch)){g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.DrawImage(src,new Rectangle(0,0,1024,2048));}
   var d=patch.LockBits(new Rectangle(0,0,1024,2048),ImageLockMode.ReadOnly,PixelFormat.Format24bppRgb);var p=new byte[1024*2048*3];Marshal.Copy(d.Scan0,p,0,p.Length);patch.UnlockBits(d);
   for(int y=0;y<2048;y++)for(int x=0;x<1024;x++) {
    double a=Math.Max(0,Math.Min(1,Math.Min((x+.5)/128,(1023.5-x)/128)));a=a*a*(3-2*a);double v=Math.Min(1,(2047.5-y)/128);a*=v*v*(3-2*v);
    int dest=(y*12288+center-512+x)*3,from=(y*1024+x)*3;
    for(int ch=0;ch<3;ch++)pixels[dest+ch]=(byte)Math.Round(pixels[dest+ch]*(1-a)+p[from+ch]*a);
   }
  }
  using(var atlas=BitmapOf(pixels,12288,4096)) {
   atlas.Save(root+"/previews/m01-assembled.png",ImageFormat.Png);
   for(int r=0;r<Rows;r++)for(int c=0;c<Cols;c++)using(var tile=atlas.Clone(new Rectangle(c*Core,r*Core,Core,Core),PixelFormat.Format24bppRgb))tile.Save(root+"/tiles/"+Id(FirstC+c,FirstR+r)+".png",ImageFormat.Png);
   using(var preview=new Bitmap(3072,1024))using(var g=Graphics.FromImage(preview)){g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.DrawImage(atlas,new Rectangle(0,0,3072,1024));preview.Save(root+"/previews/m01-overview.png",ImageFormat.Png);}
  }
 }
 public static void Seams(string root) {
  using(var atlas=new Bitmap(root+"/previews/m01-assembled.png")) {
   int n=0;using(var contact=new Bitmap(1600,2400))using(var g=Graphics.FromImage(contact))using(var f=new Font("Arial",17,GraphicsUnit.Pixel)) {
    g.Clear(Color.FromArgb(30,32,32));g.InterpolationMode=InterpolationMode.HighQualityBicubic;
    for(int r=0;r<Rows;r++)for(int c=1;c<Cols;c++){int x=n%2*800,y=n/2*300;g.DrawString("V C"+(FirstC+c)+" R"+(FirstR+r),f,Brushes.White,x+8,y+4);using(var strip=atlas.Clone(new Rectangle(c*Core-200,r*Core,400,Core),PixelFormat.Format24bppRgb)){strip.RotateFlip(RotateFlipType.Rotate90FlipNone);g.DrawImage(strip,new Rectangle(x,y+30,800,250));}n++;}
    for(int c=0;c<Cols;c++){int x=n%2*800,y=n/2*300;g.DrawString("H C"+(FirstC+c)+" R3287",f,Brushes.White,x+8,y+4);g.DrawImage(atlas,new Rectangle(x,y+30,800,250),new Rectangle(c*Core,Core-200,Core,400),GraphicsUnit.Pixel);n++;}
    contact.Save(root+"/previews/m01-seams.png",ImageFormat.Png);
   }
  }
 }
}
