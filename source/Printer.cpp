/*
 * =====================================================================================
 *
 *       Filename:  Printer.cpp
 *
 *    Description:  Functions to create a bitmap from data
 *
 *        Version:  1.2 
 *        Created:  07/26/2017 11:43:54 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Oleh Yuschuk
 *    Modified By:  scuti@teknik.io 
 *                  surhke@teknik.io
 *
 * =====================================================================================
 */
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <time.h>

#include "Printer.h"
#include "Crc16.h"
#include "Ecc.h"


// command line program is stripped down to only
// output bitmap files thus a lot of printer functions are unnecessary



t_printdata printdata;          // extern

// Service function, puts block of data to bitmap as a grid of 32x32 dots in
// the position with given index. Bitmap is treated as a continuous line of
// cells, where end of the line is connected to the start of the next line.
static void Drawblock(int index,t_data *block,uchar *bits,int width,int height,
    int border,int nx,int ny,int dx,int dy,int px,int py,int black
    ) {
  int i,j,x,y,m,n;
  ulong t;
  // Convert cell index into the X-Y bitmap coordinates.
  x=(index%nx)*(NDOT+3)*dx+2*dx+border;
  y=(index/nx)*(NDOT+3)*dy+2*dy+border;
  bits+=(height-y-1)*width+x;
  // Add CRC.
  block->crc=(ushort)(Crc16((uchar *)block,NDATA+sizeof(ulong))^0x55AA);
  // Add error correction code.
  Encode8((uchar *)block,block->ecc,127);
  // Print block. To increase the reliability of empty or half-empty blocks
  // and close-to-0 addresses, I XOR all data with 55 or AA.
  for (j=0; j<32; j++) {
    t=((ulong *)block)[j];
    if ((j & 1)==0)
      t^=0x55555555;
    else
      t^=0xAAAAAAAA;
    x=0;
    for (i=0; i<32; i++) {
      if (t & 1) {
        for (m=0; m<py; m++) {
          for (n=0; n<px; n++) {
            bits[x-m*width+n]=(uchar)black;
          };
        };
      };
      t>>=1;
      x+=dx;
    };
    bits-=dy*width;
  };
};



// Service function, clips regular 32x32-dot raster to bitmap in the position
// with given block coordinates (may be outside the bitmap).
static void Fillblock(int blockx,int blocky,uchar *bits,int width,int height,
    int border,int nx,int ny,int dx,int dy,int px,int py,int black
    ) {
  int i,j,x0,y0,x,y,m,n;
  ulong t;
  // Convert cell coordinates into the X-Y bitmap coordinates.
  x0=blockx*(NDOT+3)*dx+2*dx+border;
  y0=blocky*(NDOT+3)*dy+2*dy+border;
  // Print raster.
  for (j=0; j<32; j++) {
    if ((j & 1)==0)
      t=0x55555555;
    else {
      if (blocky<0 && j<=24) t=0;
      else if (blocky>=ny && j>8) t=0;
      else if (blockx<0) t=0xAA000000;
      else if (blockx>=nx) t=0x000000AA;
      else t=0xAAAAAAAA; };
    for (i=0; i<32; i++) {
      if (t & 1) {
        for (m=0; m<py; m++) {
          for (n=0; n<px; n++) {
            x=x0+i*dx+n;
            y=y0+j*dy+m;
            if (x<0 || x>=width || y<0 || y>=height)
              continue;
            bits[(height-y-1)*width+x]=(uchar)black;
          };
        };
      };
      t>>=1;
    };
  };
};



// Prints one complete page or saves one bitmap.
static void Printnextpage(t_printdata *print) {
  using namespace std;
  int dx,dy,px,py,nx,ny,width,height,border,redundancy,black;
  int i,j,k,l,n,success,basex,nstring,npages,rot;
  char s[TEXTLEN],ts[TEXTLEN/2];
  char drv[MAXDRIVE],dir[MAXDIR],nam[MAXFILE],ext[MAXEXT],path[MAXPATH+32];
  uchar *bits;
  ulong u,size,pagesize,offset;
  t_data block,cksum;
  BITMAPFILEHEADER bmfh;
  BITMAPINFO *pbmi;
  // Calculate offset of this page in data.
  offset=print->frompage*print->pagesize;
  if (offset>=print->datasize || print->frompage>print->topage) {
    // All requested pages are printed, finish this step.
    print->step++;
    return; };
  // Report page.
  npages=(print->datasize+print->pagesize-1)/print->pagesize;
  sprintf(s,"Processing page %i of %i...",print->frompage+1,npages);
  Message(s,0);
  // Get frequently used variables.
  dx=print->dx;
  dy=print->dy;
  px=print->px;
  py=print->py;
  nx=print->nx;
  ny=print->ny;
  width=print->width;
  border=print->border;
  size=print->alignedsize;
  pagesize=print->pagesize;
  redundancy=print->redundancy;
  black=print->black;
  if (print->outbmp[0]=='\0')
    bits=print->dibbits;
  else
    bits=print->drawbits;
  // Start new page.
  /*if (print->outbmp[0]=='\0') {
    success=StartPage(print->dc);
  //!!! instead of print, create bmp file
  if (success<=0) {
  Reporterror("Unable to print");
  Stopprinting(print);
  return;
  };
  }; */
  // Check if we can reduce the vertical size of the table on the last page.
  // To assure reliable orientation, I request at least 3 rows.
  l=min(size-offset,pagesize);
  n=(l+NDATA-1)/NDATA;                 // Number of pure data blocks on page
  nstring=                             // Number of groups (length of string)
    (n+redundancy-1)/redundancy;
  n=(nstring+1)*(redundancy+1)+1;      // Total number of blocks to print
  n=max((n+nx-1)/nx,3);                // Number of rows (at least 3)
  if (ny>n) ny=n;
  height=ny*(NDOT+3)*dy+py+2*border;
  // Initialize bitmap to all white.
  memset(bits,255,height*width);
  // Draw vertical grid lines.
  for (i=0; i<=nx; i++) {
    if (print->printborder) {
      basex=i*(NDOT+3)*dx+border;
      for (j=0; j<ny*(NDOT+3)*dy+py+2*border; j++,basex+=width) {
        for (k=0; k<px; k++) bits[basex+k]=0;
      }; }
    else {
      basex=i*(NDOT+3)*dx+width*border+border;
      for (j=0; j<ny*(NDOT+3)*dy; j++,basex+=width) {
        for (k=0; k<px; k++) bits[basex+k]=0;
      };
    };
  };
  // Draw horizontal grid lines.
  for (j=0; j<=ny; j++) {
    if (print->printborder) {
      for (k=0; k<py; k++) {
        memset(bits+(j*(NDOT+3)*dy+k+border)*width,0,width);
      }; }
    else {
      for (k=0; k<py; k++) {
        memset(bits+(j*(NDOT+3)*dy+k+border)*width+border,0,
            nx*(NDOT+3)*dx+px);
      };
    };
  };
  // Fill borders with regular raster.
  if (print->printborder) {
    for (j=-1; j<=ny; j++) {
      Fillblock(-1,j,bits,width,height,border,nx,ny,dx,dy,px,py,black);
      Fillblock(nx,j,bits,width,height,border,nx,ny,dx,dy,px,py,black); };
    for (i=0; i<nx; i++) {
      Fillblock(i,-1,bits,width,height,border,nx,ny,dx,dy,px,py,black);
      Fillblock(i,ny,bits,width,height,border,nx,ny,dx,dy,px,py,black);
    };
  };
  // Update superblock.
  print->superdata.page=
    (ushort)(print->frompage+1);       // Page number is 1-based
  // First block in every string (including redundancy string) is a superblock.
  // To improve redundancy, I avoid placing blocks belonging to the same group
  // in the same column (consider damaged diode in laser printer).
  for (j=0; j<=redundancy; j++) {
    k=j*(nstring+1);
    if (nstring+1>=nx)
      k+=(nx/(redundancy+1)*j-k%nx+nx)%nx;
    Drawblock(k,(t_data *)&print->superdata,
        bits,width,height,border,nx,ny,dx,dy,px,py,black); };
    // Now the most important part - encode and draw data, group by group!
    for (i=0; i<nstring; i++) {
      // Prepare redundancy block.
      cksum.addr=offset ^ (redundancy<<28);
      memset(cksum.data,0xFF,NDATA);
      // Process data group.
      for (j=0; j<redundancy; j++) {
        // Fill block with data.
        block.addr=offset;
        if (offset<size) {
          l=size-offset;
          if (l>NDATA) l=NDATA;
          memcpy(block.data,print->buf+offset,l); }
        else
          l=0;
        // Bytes beyond the data are set to 0.
        while (l<NDATA)
          block.data[l++]=0;
        // Update redundancy block.
        for (l=0; l<NDATA; l++) cksum.data[l]^=block.data[l];
        // Find cell where block will be placed on the paper. The first block in
        // every string is the superblock.
        k=j*(nstring+1);
        if (nstring+1<nx)
          k+=i+1;
        else {
          // Optimal shift between the first columns of the strings is
          // nx/(redundancy+1). Next line calculates how I must rotate the j-th
          // string. Best understandable after two bottles of Weissbier.
          rot=(nx/(redundancy+1)*j-k%nx+nx)%nx;
          k+=(i+1+rot)%(nstring+1); };
        Drawblock(k,&block,bits,width,height,border,nx,ny,dx,dy,px,py,black);
        offset+=NDATA;
      };
      // Process redundancy block in the similar way.
      k=redundancy*(nstring+1);
      if (nstring+1<nx)
        k+=i+1;
      else {
        rot=(nx/(redundancy+1)*redundancy-k%nx+nx)%nx;
        k+=(i+1+rot)%(nstring+1); };
      Drawblock(k,&cksum,bits,width,height,border,nx,ny,dx,dy,px,py,black);
    };
    // Print superblock in all remaining cells.
    for (k=(nstring+1)*(redundancy+1); k<nx*ny; k++) {
      Drawblock(k,(t_data *)&print->superdata,
          bits,width,height,border,nx,ny,dx,dy,px,py,black); };
    // When printing to paper, print title at the top of the page and info text
    // at the bottom.
    if (print->outbmp[0]=='\0') {
      if (print->printheader) {
        // Print title at the top of the page.
      #ifdef _WIN32
        Filetimetotext(&print->modified,ts,sizeof(ts));
      #elif __linux
        struct tm * timeinfo;
        timeinfo = localtime (&print->modified); 
      strftime(ts, strlen(ts), "%D %l:%M %p", timeinfo);
      if(ts[0] == '0') {
        memmove(ts, ts+1, strlen(ts));
      }
      #endif

      n=sprintf(s,"%.64s [%s, %i bytes] - page %i of %i",
        print->superdata.name,ts,print->origsize,print->frompage+1,npages);
      //SelectObject(print->dc,print->hfont6); //!!! bitmap output instead of printing
      //TextOut(print->dc,print->borderleft+width/2,print->bordertop,s,n);
      // Print info at the bottom of the page.
      n=sprintf(s,"Recommended scanner resolution %i dots per inch",
        max(print->ppix*3/dx,print->ppiy*3/dy));
      //SelectObject(print->dc,print->hfont10); //!!! bitmap output instead of printing
      //TextOut(print->dc,
      //  print->borderleft+width/2,
      //  print->bordertop+print->extratop+height+print->ppiy/24,s,n);
      ;
    };
    // Transfer bitmap to paper and send page to printer.
    /*SetDIBitsToDevice(print->dc,
      print->borderleft,print->bordertop+print->extratop,
      width,height,0,0,0,height,bits,
      (BITMAPINFO *)print->bmi,DIB_RGB_COLORS);
      EndPage(print->dc); */
    }
    else {
      // Save bitmap to file. First, get file name.
      /*     fnsplit(print->outbmp,drv,dir,nam,ext);
             if (ext[0]=='\0') strcpy(ext,".bmp");
             if (npages>1)
             sprintf(path,"%s%s%s_%04i%s",drv,dir,nam,print->frompage+1,ext);
             else
             sprintf(path,"%s%s%s%s",drv,dir,nam,ext); */

      sprintf("%s#%i", print -> outbmp, print -> frompage+1);
      FILE *f = fopen(print -> outbmp, "wb");
      if (f == NULL) {
        Reporterror("can not open file for writing");
        //Stopprinting(print);
        return;
      }

      // Create and save bitmap file header.
      success=1;
      n=sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD);
      bmfh.bfType='BM';
      bmfh.bfSize=sizeof(bmfh)+n+width*height;
      bmfh.bfReserved1=bmfh.bfReserved2=0;
      bmfh.bfOffBits=sizeof(bmfh)+n;
      u = fwrite(&bmfh, sizeof(char), sizeof(bmfh), f);
      if (u != sizeof(bmfh))
        success=0;
      // Update and save bitmap info header and palette.
      if (success) {
        pbmi=(BITMAPINFO *)print->bmi;
        pbmi->bmiHeader.biWidth=width;
        pbmi->bmiHeader.biHeight=height;
        pbmi->bmiHeader.biXPelsPerMeter=(print->ppix*10000)/254;
        pbmi->bmiHeader.biYPelsPerMeter=(print->ppiy*10000)/254;
        if (fwrite(pbmi, sizeof(char), n, f) > 0 || u!=(ulong)n) success=0; };
      // Save bitmap data.
      if (success) {
        u = fwrite(bits, sizeof(char), width*height, f);
        if (u != (ulong)(width*height))
          success=0;
        ;  
      };
      fclose(f);
      if (success==0) {
        Reporterror("Unable to save bitmap");
        //Stopprinting(print);
        return;
      };
    };
    // Page printed, proceed with next.
    print->frompage++;
};



// Sends specified file to printer (bmp=NULL) or to bitmap file.
void Printfile(char *path,char *bmp) {
  // Stop printing of previous file, if any.
  //Stopprinting(&printdata);
  // Prepare descriptor.
  memset(&printdata,0,sizeof(printdata));
  strncpy(printdata.infile,path,MAXPATH-1);
  if (bmp!=NULL)
    strncpy(printdata.outbmp,bmp,MAXPATH-1);
  // Start printing.
  printdata.step=1;
  //  Updatebuttons();
};

