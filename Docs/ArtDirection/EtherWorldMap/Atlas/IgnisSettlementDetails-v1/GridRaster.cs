using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;
public static class GridRaster {
 const double W=67108864.0,H=33554432.0,R=6366.197723675814;
 public static double[] Local(double wx,double wy){double p0=54*Math.PI/180,l0=-Math.PI/180,p=Math.PI/2-wy/H*Math.PI,l=wx/W*2*Math.PI-Math.PI,c=Math.Acos(Math.Min(1,Math.Max(-1,Math.Sin(p0)*Math.Sin(p)+Math.Cos(p0)*Math.Cos(p)*Math.Cos(l-l0)))),k=c<1e-10?1:c/Math.Sin(c);return new[]{R*k*Math.Cos(p)*Math.Sin(l-l0),R*k*(Math.Cos(p0)*Math.Sin(p)-Math.Sin(p0)*Math.Cos(p)*Math.Cos(l-l0))};}
 public static void Warp(string input,string output,double[] bounds,double[] world,int width,int height){
  using(var src=new Bitmap(input))using(var clean=new Bitmap(src.Width,src.Height,PixelFormat.Format32bppArgb))using(var dst=new Bitmap(width,height,PixelFormat.Format32bppArgb)){
   using(var g=Graphics.FromImage(clean))g.DrawImageUnscaled(src,0,0);
   var sd=clean.LockBits(new Rectangle(0,0,clean.Width,clean.Height),ImageLockMode.ReadOnly,PixelFormat.Format32bppArgb);var source=new byte[sd.Stride*sd.Height];Marshal.Copy(sd.Scan0,source,0,source.Length);clean.UnlockBits(sd);
   var dd=dst.LockBits(new Rectangle(0,0,width,height),ImageLockMode.WriteOnly,PixelFormat.Format32bppArgb);var data=new byte[dd.Stride*height];
   for(int y=0;y<height;y++)for(int x=0;x<width;x++){
    var xy=Local(world[0]+(x+.5)/width*(world[2]-world[0]),world[1]+(y+.5)/height*(world[3]-world[1]));double sx=(xy[0]-bounds[0])/(bounds[2]-bounds[0])*clean.Width-.5,sy=(bounds[3]-xy[1])/(bounds[3]-bounds[1])*clean.Height-.5;
    if(sx<0||sy<0||sx>clean.Width-1||sy>clean.Height-1)continue;
    int ix=Math.Min((int)sx,clean.Width-2),iy=Math.Min((int)sy,clean.Height-2);double fx=sx-ix,fy=sy-iy;
    for(int ch=0;ch<3;ch++)data[y*dd.Stride+x*4+ch]=(byte)Math.Round((1-fy)*((1-fx)*source[iy*sd.Stride+ix*4+ch]+fx*source[iy*sd.Stride+(ix+1)*4+ch])+fy*((1-fx)*source[(iy+1)*sd.Stride+ix*4+ch]+fx*source[(iy+1)*sd.Stride+(ix+1)*4+ch]));data[y*dd.Stride+x*4+3]=255;
   }Marshal.Copy(data,0,dd.Scan0,data.Length);dst.UnlockBits(dd);dst.Save(output,ImageFormat.Png);
  }
 }
 public static void Contact(string[] paths,string[] labels,string output,int columns,int size){Gallery(paths,labels,output,columns,size,size);}
 public static void Gallery(string[] paths,string[] labels,string output,int columns,int width,int height){
  using(var b=new Bitmap(columns*width,(int)Math.Ceiling(paths.Length/(double)columns)*(height+28)))using(var g=Graphics.FromImage(b))using(var f=new Font("Malgun Gothic",16,GraphicsUnit.Pixel)){
   g.Clear(Color.FromArgb(30,32,32));g.InterpolationMode=InterpolationMode.HighQualityBicubic;
   for(int i=0;i<paths.Length;i++){
    int x=i%columns*width,y=i/columns*(height+28);g.DrawString(labels[i],f,Brushes.White,x+5,y+4);
    using(var s=new Bitmap(paths[i])){double scale=Math.Min(width/(double)s.Width,height/(double)s.Height);int w=(int)(s.Width*scale),h=(int)(s.Height*scale);g.DrawImage(s,new Rectangle(x+(width-w)/2,y+28+(height-h)/2,w,h));}
   }
   b.Save(output,ImageFormat.Png);
  }
 }
}
