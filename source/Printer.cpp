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
#include <iostream>
#include <time.h>

#include "Printer.h"
#include "Crc16.h"
#include "Ecc.h"


// command line program is stripped down to only
// output bitmap files thus a lot of printer functions are unnecessary


// externs/global that were causing linker and all other errors
int       dpi;                  // Dot raster, dots per inch
int       dotpercent;           // Dot size, percent of dpi
int       resx,resy;            // Printer resolution, dpi (may be 0!)
int       redundancy;           // Redundancy (NGROUPMIN..NGROUPMAX)
int       printheader;          // Print header and footer
int       printborder;          // Border around bitmap
t_printdata printdata;          // extern



// Sends specified file to printer (bmp=NULL) or to bitmap file.
t_printdata Printfile(const std::string &path, const std::string &bmp)
{
  // Prepare descriptor.
  // memset(&printdata,0,sizeof(printdata));
  t_printdata printdata = {};
  printdata.infile = path;
  if ( bmp.length() > 0 ) {
    printdata.outbmp = bmp;
  }
  // Start printing.
  printdata.step=1;
  //  Updatebuttons();
  return printdata;
};



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



// Opens input file and allocates memory buffers.
void Preparefiletoprint(t_printdata *print) {
  ulong l;
#ifdef _WIN32
  FILETIME created, accessed, modified;
  // Get file attributes.
  print->attributes=GetFileAttributes(print->infile.c_str());
  if (print->attributes==0xFFFFFFFF)
    print->attributes=FILE_ATTRIBUTE_NORMAL;
  // Open input file.
  print->hfile = CreateFile(print->infile.c_str(),GENERIC_READ,FILE_SHARE_READ,
    NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if (print->hfile==INVALID_HANDLE_VALUE) {
    Reporterror("Unable to open file");
    Stopprinting(print);
    return; 
  };
  // Get time of last file modification.
  GetFileTime(print->hfile,&created,&accessed,&modified);
  if (modified.dwHighDateTime==0)
    print->modified=created;
  else
    print->modified=modified;
  // Get original (uncompressed) file size.
  print->origsize=GetFileSize(print->hfile, (LPDWORD)&l);
  if (print->origsize==0 || print->origsize>MAXSIZE || l!=0) {
    Reporterror("Invalid file size");
    Stopprinting(print);
    return; 
  };
#elif __linux
  //!!!TEST struct mem allocation
  // Get file attributes
  if ( stat(print->infile.c_str(), &(print->attributes)) != 0 ) {
    Reporterror("Unable to get input file attributes");
    Stopprinting(print);
    return;
  }
  // Open input file.
  print->hfile = fopen( print->infile.c_str(), "rb" );
  if (print->hfile == NULL) {
    Reporterror("Unable to open file");
    Stopprinting(print);
    return; 
  }
  // Get time of last file modification.
  print->modified = print->attributes.st_mtime;
  // Get original (uncompressed) file size.
  print->origsize = print->attributes.st_size;
  if (print->origsize==0 || print->origsize>MAXSIZE) {
    Reporterror("Invalid file size");
    Stopprinting(print);
    return;
  }
#endif
  print->readsize=0;

  // Allocate buffer for compressed file. (If compression is off, buffer will
  // contain uncompressed data). As AES encryption works on 16-byte records,
  // buffer is aligned to next 16-bit border.
  print->bufsize=(print->origsize+15) & 0xFFFFFFF0;
  print->buf=(uchar *)malloc(print->bufsize);
  if (print->buf==NULL) {
    Reporterror("Not enough memory for output file");
    Stopprinting(print);
    return; 
  };
  // Allocate read buffer. Because compression may take significant time, I
  // pack data in pieces of PACKLEN bytes.
  print->readbuf=(uchar *)malloc(PACKLEN);
  if (print->readbuf==NULL) {
    Reporterror("Not enough memory for read buffer");
    Stopprinting(print);
    return; 
  };
  // Set options.
//  print->compression=compression;
//  print->encryption=encryption;
  print->printheader=printheader;
  print->printborder=printborder;
  print->redundancy=redundancy;
  // Step finished.
  print->step++;
};

int Initializeprinting(t_printdata *print, uint pageWidth, uint pageHeight) {
  int i,dx,dy,px,py,width,height,success,rastercaps;
  long nx, ny; // Number of blocks in each row and column
  char fil[MAXPATH],nam[MAXFILE],ext[MAXEXT],jobname[TEXTLEN];
  BITMAPINFO *pbmi;
  //SIZE extent;
  //PRINTDLG printdlg;
  //DOCINFO dinfo;
  //DEVNAMES *pdevnames;
  // Prepare superdata.
  print->superdata.addr=SUPERBLOCK;
  print->superdata.datasize=print->alignedsize;
  print->superdata.origsize=print->origsize;
  if (print->compression)
    print->superdata.mode|=PBM_COMPRESSED;
  if (print->encryption)
    print->superdata.mode|=PBM_ENCRYPTED;
#ifdef __WIN32
  print->superdata.attributes=(uchar)(print->attributes &
    (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|
    FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_ARCHIVE|
    FILE_ATTRIBUTE_NORMAL));
  print->superdata.modified=print->modified;
#elif __linux__
  //!!! get attributes needed to recreate file from stat
  //set print->modified
#endif
  print->superdata.filecrc=(ushort)print->bufcrc;
  //fnsplit(print->infile,NULL,NULL,nam,ext);
  //fnmerge(fil,NULL,NULL,nam,ext);
  // Note that name in superdata may be not null-terminated.
  strncpy(print->superdata.name,fil,32); // don't overwrite the salt and iv at the end of this buffer
  print->superdata.name[31] = '\0'; // ensure that later string operations don't overflow into binary data

  // If printing to paper, ask user to select printer and, if necessary, adjust
  // parameters. I do not enforce high quality or high resolution - the user is
  // the king (well, a sort of).
  

  //if (print->outbmp[0]=='\0') {
  //  // Open standard Print dialog box.
  //  memset(&printdlg,0,sizeof(PRINTDLG));
  //  printdlg.lStructSize=sizeof(PRINTDLG);
  //  printdlg.hwndOwner=hwmain;
  //  printdlg.hDevMode=pagesetup.hDevMode;
  //  printdlg.hDevNames=pagesetup.hDevNames;
  //  printdlg.hDC=NULL;                 // Returns DC
  //  printdlg.Flags=PD_ALLPAGES|PD_RETURNDC|PD_NOSELECTION|PD_PRINTSETUP;
  //  printdlg.nFromPage=1;              // It's hard to calculate the number of
  //  printdlg.nToPage=9999;             // pages in advance.
  //  printdlg.nMinPage=1;
  //  printdlg.nMaxPage=9999;
  //  printdlg.nCopies=1;
  //  printdlg.hInstance=hinst;
  //  success=PrintDlg(&printdlg);
  //  // Save important information.
  //  print->dc=printdlg.hDC;
  //  print->frompage=printdlg.nFromPage-1;
  //  print->topage=printdlg.nToPage-1;
  //  // Clean up to prevent memory leaks.
  //  if (pagesetup.hDevMode==NULL)
  //    pagesetup.hDevMode=printdlg.hDevMode;
  //  else if (printdlg.hDevMode!=pagesetup.hDevMode)
  //    GlobalFree(printdlg.hDevMode);
  //  if (pagesetup.hDevNames==NULL)
  //    pagesetup.hDevNames=printdlg.hDevNames;
  //  else if (printdlg.hDevNames!=pagesetup.hDevNames)
  //    GlobalFree(printdlg.hDevNames);
  //  // Analyse results.
  //  if (success==0) {                  // User cancelled printing
  //    Message("",0);
  //    Stopprinting(print);
  //    return; };

  //   if (print->dc==NULL) {             // Printer DC is unavailable
  //    Reporterror("Unable to access printer");
  //    Stopprinting(print);
  //    return; };

  //    // Assure that printer is capable of displaying bitmaps.
  //  rastercaps=GetDeviceCaps(print->dc,RASTERCAPS);
  //  if ((rastercaps & RC_DIBTODEV)==0) {
  //    Reporterror("The selected printer can't print bitmaps");
  //    Stopprinting(print);
  //    return; };
  //  // Get resolution and size of print area in pixels.
  //  print->ppix=GetDeviceCaps(print->dc,LOGPIXELSX);
  //  print->ppiy=GetDeviceCaps(print->dc,LOGPIXELSY);
  //  width=GetDeviceCaps(print->dc,HORZRES);
  //  height=GetDeviceCaps(print->dc,VERTRES); 


  //  // Create fonts to draw title and comment. If system is unable to create
  //  // any font, I get standard one. Of course, standard font will be almost
  //  // invisible with printer's resolution.
  //  if (print->printheader) {
  //    print->hfont6=CreateFont(print->ppiy/6,0,0,0,FW_LIGHT,0,0,0,
  //      ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,
  //      PROOF_QUALITY,FF_SWISS,NULL);
  //    print->hfont10=CreateFont(print->ppiy/10,0,0,0,FW_LIGHT,0,0,0,
  //      ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,
  //      PROOF_QUALITY,FF_SWISS,NULL);
  //    if (print->hfont6==NULL)
  //      print->hfont6=(HFONT)GetStockObject(SYSTEM_FONT);
  //    if (print->hfont10==NULL)
  //      print->hfont10=(HFONT)GetStockObject(SYSTEM_FONT);
  //    // Set text color (gray) and alignment (centered).
  //    SetTextColor(print->dc,RGB(128,128,128));
  //    SetTextAlign(print->dc,TA_TOP|TA_CENTER);
  //    // Calculate height of title and info lines on the paper.
  //    SelectObject(print->dc,print->hfont6);
  //    if (GetTextExtentPoint32(print->dc,"Page",4,&extent)==0)
  //      print->extratop=print->ppiy/4;
  //    else
  //      print->extratop=extent.cy+print->ppiy/16;
  //    SelectObject(print->dc,print->hfont10);
  //    if (GetTextExtentPoint32(print->dc,"Page",4,&extent)==0)
  //      print->extrabottom=print->ppiy/6;
  //    else
  //      print->extrabottom=extent.cy+print->ppiy/24;
  //    ; }
  //  else {
  //    print->hfont6=NULL;
  //    print->hfont10=NULL;
  //    print->extratop=print->extrabottom=0; };



  //  // Dots on paper are black (palette index 0 in the memory bitmap that will
  //  // be created later in this subroutine).
  //  print->black=0; }
  // I treat printing to bitmap as a debugging feature and set some more or
  // less sound defaults.

//    print->dc=NULL;
    print->frompage=0;
    print->topage=9999;
    if (resx==0 || resy==0) {
      print->ppix=300; print->ppiy=300; 
    } else {
      print->ppix=resx; print->ppiy=resy; 
    }
//    if (pagesetup.Flags & PSD_INTHOUSANDTHSOFINCHES) {
//      width=pagesetup.ptPaperSize.x*print->ppix/1000;
//      height=pagesetup.ptPaperSize.y*print->ppiy/1000; 
//    } else if (pagesetup.Flags & PSD_INHUNDREDTHSOFMILLIMETERS) {
//      width=pagesetup.ptPaperSize.x*print->ppix/2540;
//      height=pagesetup.ptPaperSize.y*print->ppiy/2540; 
//    } else {                             // Use default A4 size (210x292 mm)*/
//      width=print->ppix*8270/1000;
//      height=print->ppiy*11690/1000; 
//    };
//        print->hfont6=NULL;
//        print->hfont10=NULL;
//    print->extratop=print->extrabottom=0;
//    // To simplify recognition of grid on high-contrast bitmap, dots on the
//    // bitmap are dark gray.
//    print->black=64; 



    // Calculate page borders in the pixels of printer's resolution.
//     if (pagesetup.Flags & PSD_INTHOUSANDTHSOFINCHES) {
//        print->borderleft=pagesetup.rtMargin.left*print->ppix/1000;
//        print->borderright=pagesetup.rtMargin.right*print->ppix/1000;
//        print->bordertop=pagesetup.rtMargin.top*print->ppiy/1000;
//        print->borderbottom=pagesetup.rtMargin.bottom*print->ppiy/1000; 
//    }
//    else if (pagesetup.Flags & PSD_INHUNDREDTHSOFMILLIMETERS) {
//        print->borderleft=pagesetup.rtMargin.left*print->ppix/2540;
//        print->borderright=pagesetup.rtMargin.right*print->ppix/2540;
//        print->bordertop=pagesetup.rtMargin.top*print->ppiy/2540;
//        print->borderbottom=pagesetup.rtMargin.bottom*print->ppiy/2540; 
//    } else {  
      print->borderleft=print->ppix/2; 
      print->borderright=print->ppix/2;
      print->bordertop=print->ppiy/2;
      print->borderbottom=print->ppiy/2; 
//    } 


  // Calculate size of printable area, in the pixels of printer's resolution.
  // Truncation of pageWidth * ppix and pageHeight * ppiy is INTENDED behavior
  width  = pageWidth * print->ppix  - print->borderleft+print->borderright;
  height = pageHeight * print->ppiy - print->bordertop+print->borderbottom+print->extratop+print->extrabottom;
  // Calculate data point raster (dx,dy) and size of the point (px,py) in the
  // pixels of printer's resolution. Note that pixels, at least in theory, may
  // be non-rectangular.
  dx=std::max(print->ppix/dpi,2);
  px=std::max((dx*dotpercent)/100,1);
  dy=std::max(print->ppiy/dpi,2);
  py=std::max((dy*dotpercent)/100,1);
  // Calculate width of the border around the data grid.
  if (print->printborder)
    print->border=dx*16;
  else if ( print->outbmp.length() > 0 )
    print->border=25;
  else
    print->border=0;
  // Calculate the number of data blocks that fit onto the single page. Single
  // page must contain at least redundancy data blocks plus 1 recovery checksum,
  // and redundancy+1 superblocks with name and size of the data. Data and
  // recovery blocks should be placed into different columns.
  nx=(width-px-2*print->border)/(NDOT*dx+3*dx);
  ny=(height-py-2*print->border)/(NDOT*dy+3*dy);
  std::cout << "nx: " << nx << std::endl;
  std::cout << "ny: " << ny << std::endl;
  std::cout << "width: " << width << std::endl;
  std::cout << "height: " << height << std::endl;
  std::cout << "print->ppix: " << print->ppix << std::endl;
  std::cout << "print->ppiy: " << print->ppiy << std::endl;
  std::cout << "print->bordertop: " << print->bordertop << std::endl;
  std::cout << "print->borderbottom: " << print->borderbottom << std::endl;
  std::cout << "print->borderleft: " << print->borderleft << std::endl;
  std::cout << "print->borderright: " << print->borderright << std::endl;
  std::cout << "print->border: " << print->border << std::endl;
  std::cout << "px: " << px << std::endl;
  std::cout << "py: " << py << std::endl;
  std::cout << "NDOT: " << NDOT << std::endl;
  std::cout << "dx: " << dx <<std::endl;
  std::cout << "dy: " << dy << std::endl;
  long multResult = nx*ny;
  if ( nx > 0 && ny > 0 && multResult < 0 ) {
    std::cerr << "Input file is too large to back up.  Please break the file apart" << std::endl;
    return -1;
  }
  if (nx<print->redundancy+1 || ny<3 || nx*ny<2*print->redundancy+2) {
    Reporterror("Printable area is too small, reduce borders or block size");
    Stopprinting(print);
    return -1; 
  };
  // Calculate final size of the bitmap where I will draw the image.
  width=(nx*(NDOT+3)*dx+px+2*print->border+3) & 0xFFFFFFFC;
  height=ny*(NDOT+3)*dy+py+2*print->border;
  // Fill in bitmap header. To simplify processing, I use 256-color bitmap
  // (1 byte per pixel).
  pbmi=(BITMAPINFO *)print->bmi;
  memset(pbmi,0,sizeof(BITMAPINFOHEADER));
  pbmi->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  pbmi->bmiHeader.biWidth=width;
  pbmi->bmiHeader.biHeight=height;
  pbmi->bmiHeader.biPlanes=1;
  pbmi->bmiHeader.biBitCount=8;
  pbmi->bmiHeader.biCompression=BI_RGB;
  pbmi->bmiHeader.biSizeImage=0;
  pbmi->bmiHeader.biXPelsPerMeter=0;
  pbmi->bmiHeader.biYPelsPerMeter=0;
  pbmi->bmiHeader.biClrUsed=256;
  pbmi->bmiHeader.biClrImportant=256;
  for (i=0; i<256; i++) {
    pbmi->bmiColors[i].rgbBlue=(uchar)i;
    pbmi->bmiColors[i].rgbGreen=(uchar)i;
    pbmi->bmiColors[i].rgbRed=(uchar)i;
    pbmi->bmiColors[i].rgbReserved=0; };
  // Create bitmap. Direct drawing is faster than tens of thousands of API
  // calls.
  if ( print->outbmp.empty() ) {
    Reporterror("Outbmp unspecified, can not create BMP");
    Stopprinting(print);
    return -1;
  }
  else {                               // Save to bitmap
    print->drawbits=(uchar *)malloc(width*height);
    if (print->drawbits==NULL) {
      Reporterror("Low memory, can't create bitmap");
      return -1;
    };
  };
  // Calculate the total size of useful data, bytes, that fits onto the page.
  // For each redundancy blocks, I create one recovery block. For each chain, I
  // create one superblock that contains file name and size, plus at least one
  // superblock at the end of the page.
  print->pagesize=((multResult-print->redundancy-2)/(print->redundancy+1))*
    print->redundancy*NDATA;
  print->superdata.pagesize=print->pagesize;
  // Save calculated parameters.
  print->width=width;
  print->height=height;
  print->dx=dx;
  print->dy=dy;
  print->px=px;
  print->py=py;
  print->nx=nx;
  print->ny=ny;

  // Start printing. -> not actually printing - don't need
//   if (print->outbmp[0]=='\0') {
//    if (pagesetup.hDevNames!=NULL)
//      pdevnames=(DEVNAMES *)GlobalLock(pagesetup.hDevNames);
//    else
//      pdevnames=NULL;
//    memset(&dinfo,0,sizeof(DOCINFO));
//    dinfo.cbSize=sizeof(DOCINFO);
//    sprintf(jobname,"PaperBack - %.64s",print->superdata.name);
//    dinfo.lpszDocName=jobname;
//    if (pdevnames==NULL)
//      dinfo.lpszOutput=NULL;
//    else
//      dinfo.lpszOutput=(char *)pdevnames+pdevnames->wOutputOffset;
//    success=StartDoc(print->dc,&dinfo);
//    if (pdevnames!=NULL)
//      GlobalUnlock(pagesetup.hDevNames);
//    if (success<=0) {
//      Reporterror("Unable to print");
//      Stopprinting(print);
//      return; };
//    print->startdoc=1;
//  };

  // Step finished.
  print->step++;
} 


#ifdef _WIN32
// Stops printing and cleans print descriptor.
void Stopprinting(t_printdata *print) {
  // Finish compression.
//  if (print->compression!=0) {
//    BZ2_bzCompressEnd(&print->bzstream);
//    print->compression=0; };
  // Close input file.
  if (print->hfile!=NULL && print->hfile!=INVALID_HANDLE_VALUE) {
    CloseHandle(print->hfile); print->hfile=NULL; };
  // Deallocate memory.
  if (print->buf!=NULL) {
    free(print->buf); print->buf=NULL; };
  if (print->readbuf!=NULL) {
    free(print->readbuf); print->readbuf=NULL; };
  if (print->drawbits!=NULL) {
    free(print->drawbits); print->drawbits=NULL; };
  // Free other resources.
  //if (print->startdoc!=0) {
//    EndDoc(print->dc); 
//    print->startdoc=0; };
//  if (print->dc!=NULL) {
//    DeleteDC(print->dc); print->dc=NULL; };
//  if (print->hfont6!=NULL && print->hfont6!=GetStockObject(SYSTEM_FONT))
//    DeleteObject(print->hfont6);
//  print->hfont6=NULL;
//  if (print->hfont10!=NULL && print->hfont10!=GetStockObject(SYSTEM_FONT))
//    DeleteObject(print->hfont10);
//  print->hfont10=NULL;
//  if (print->hbmp!=NULL) {
//    DeleteObject(print->hbmp); print->hbmp=NULL; print->dibbits=NULL; };
  // Stop printing.
  print->step=0;
};



#elif __linux__
void Stopprinting(t_printdata *print) {
    // close input flie.
    if (print -> hfile != NULL) {
        fclose(print->hfile);
    }
    // deallocate memory
    if (print -> buf != NULL) {
        free(print -> buf);
        print -> buf = NULL;
    }
    if (print -> readbuf != NULL) {
        free(print -> readbuf);
        print -> readbuf = NULL;
    }
    if (print -> drawbits != NULL) {
        free(print -> drawbits);
        print -> drawbits = NULL;
    }
    print -> startdoc = 0;
    print -> step = 0;
}
#endif



// Prints one complete page or saves one bitmap.
void Printnextpage(t_printdata *print) {
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
  if (print->outbmp.empty() )
    bits=print->dibbits;
  else
    bits=print->drawbits;
  // Start new page.
  //if (print->outbmp[0]=='\0') {
  //  success=StartPage(print->dc);
  //  if (success<=0) {
  //    Reporterror("Unable to print");
  //    Stopprinting(print);
  //    return;
  //  };
  //}; 
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
    // Print title at the top of the page and info text at the bottom.
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
    //SetDIBitsToDevice(print->dc,
    //  print->borderleft,print->bordertop+print->extratop,
    //  width,height,0,0,0,height,bits,
    //  (BITMAPINFO *)print->bmi,DIB_RGB_COLORS);
    //  EndPage(print->dc);

    // Save bitmap to file. First, get file name.
    //     fnsplit(print->outbmp,drv,dir,nam,ext);
    //       if (ext[0]=='\0') strcpy(ext,".bmp");
    //       if (npages>1)
    //       sprintf(path,"%s%s%s_%04i%s",drv,dir,nam,print->frompage+1,ext);
    //       else
    //       sprintf(path,"%s%s%s%s",drv,dir,nam,ext);

    cout << print->outbmp << "#" << (print->frompage)+1;
    FILE *f = fopen( print->outbmp.c_str(), "wb");
    if (f == NULL) {
      Reporterror("Can not open file for writing");
      Stopprinting(print);
      return;
    }

    // Create and save bitmap file header.
    success=1;
    n=sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD);
    bmfh.bfType=19778; //First two bytes are 'BM' (19778)
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
      if (fwrite(pbmi, sizeof(char), n, f) > 0 || u!=(ulong)n) 
        success=0; 
    };
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
      Stopprinting(print);
      return;
    };

    // Page printed, proceed with next.
    print->frompage++;
};

