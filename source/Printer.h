/*
 * =====================================================================================
 *
 *       Filename:  Printer.h
 *
 *    Description:  Functions to create a bitmap
 *
 *        Version:  1.2
 *        Created:  07/26/2017 11:44:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Oleh Yuschuk
 *    Modified By:  suhrke@teknik.io
 *
 * =====================================================================================
 */
#ifndef PRINTER_H
#define PRINTER_H

#include <cstring>
#include <string>
#include "Global.h"
#ifdef __linux__
#include "Bitmap.h"
#include <sys/stat.h>
#endif

#define PACKLEN        65536           // Length of data read buffer 64 K
#define HEIGHT_A4      11.7            // Printable area in inches
#define WIDTH_A4       8.3             // Printable area in inches




typedef struct t_printdata {           // Print control structure
  int            step;                 // Next data printing step (0 - idle)
  std::string    infile;               // Name of input file
  std::string    outbmp;               // Name of output bitmap (empty: paper)
  #ifdef _WIN32
  HANDLE         hfile;                // Handle of input file
  FILETIME       modified;             // Time of last file modification
  HBITMAP        hbmp;                 // Handle of memory bitmap
  ulong          attributes;           // File attributes
  #elif __linux
  FILE           *hfile;
  struct stat    attributes;
  time_t         modified;
  #endif
  ulong          origsize;             // Original file size, bytes
  ulong          readsize;             // Amount of data read from file so far
  //ulong          datasize;             // Size of (compressed) data
  //ulong          alignedsize;          // Data size aligned to next 16 bytes
  ulong          pagesize;             // Size of data on page
  int            compression;          // 0: none, 1: fast, 2: maximal
  int            encryption;           // 0: none, 1: encrypt
  int            printheader;          // Print header and footer
  int            printborder;          // Print border around bitmap
  int            redundancy;           // Redundancy
  uchar          *buf;                 // Buffer for compressed file
  ulong          bufsize;              // Size of buf, bytes
  uchar          *readbuf;             // Read buffer, PACKLEN bytes long
  //bz_stream      bzstream;             // Compression control structure
  int            bufcrc;               // 16-bit CRC of (packed) data in buf
  t_superdata    superdata;            // Identification block on paper
  //HDC            dc;                   // Printer device context
  int            frompage;             // First page to print (0-based)
  int            topage;               // Last page (0-based, inclusive)
  int            ppix;                 // Printer X resolution, pixels per inch
  int            ppiy;                 // Printer Y resolution, pixels per inch
  int            width;                // Page width, pixels
  int            height;               // Page height, pixels
  //HFONT          hfont6;               // Font 1/6 inch high
  //HFONT          hfont10;              // Font 1/10 inch high
  int            extratop;             // Height of title line, pixels
  int            extrabottom;          // Height of info line, pixels
  int            black;                // Palette index of dots colour
  int            borderleft;           // Left page border, pixels
  int            borderright;          // Right page border, pixels
  int            bordertop;            // Top page border, pixels
  int            borderbottom;         // Bottom page border, pixels
  int            dx;                   // Distance between dots, pixels
  int            dy;                   // Distance between dots, pixels
  int            px;                   // Dot size, pixels
  int            py;                   // Dot size, pixels
  int            nx;                   // Grid dimensions, blocks
  int            ny;                   // Grid dimensions, blocks
  int            border;               // Border around the data grid, pixels
  uchar          *dibbits;             // Pointer to DIB bits
  uchar          *drawbits;            // Pointer to file bitmap bits
  uchar          bmi[sizeof(BITMAPINFO)+256*sizeof(RGBQUAD)]; // Bitmap info
  int            startdoc;             // Print job started
} t_printdata;


inline void print_printdata(t_printdata& pd) {
  using namespace std;
  cout << "step: " << pd.step << endl;
  cout << "infile: " << pd.infile << endl;
  cout << "outbmp: " << pd.outbmp << endl;
#ifdef _WIN32
  cout << "hfile: " << pd.hfile << endl;
//  cout << "modified: " << pd.modified << endl;
  {
      char str[30];
      int ok = Filetimetotext(&pd.modified, str, 30);
      if (ok) {
          cout << str << endl;
      }
  }
  cout << "hbmp: " << pd.hbmp << endl;
  cout << "attributes: " << pd.attributes << endl;
#elif __linux__
  cout << "*hfile is non-NULL: " << (pd.hfile != 0) << endl;
  cout << "attributes is non-NULL: " << (&pd.attributes != 0) << endl;
  cout << "modified: " << pd.modified << endl;
#endif
  cout << "origsize: " << pd.origsize << endl;
  cout << "readsize: " << pd.readsize << endl;
  cout << "pagesize: " << pd.pagesize << endl;
  cout << "printheader: " << pd.printheader << endl;
  cout << "printborder: " << pd.printborder << endl;
  cout << "redundancy: " << pd.redundancy << endl;
  cout << "*buf: " << *pd.buf << endl;
  cout << "bufsize: " << pd.bufsize << endl;
  cout << "*readbuf: " << *pd.readbuf << endl;
  cout << "bufcrc: " << pd.bufcrc << endl;
  print_superdata(pd.superdata);
  cout << "frompage: " << pd.frompage << endl;
  cout << "topage: " << pd.topage << endl;
  cout << "ppix: " << pd.ppix << endl;
  cout << "ppiy: " << pd.ppiy << endl;
  cout << "width: " << pd.width << endl;
  cout << "height: " << pd.height << endl;
  cout << "extratop: " << pd.extratop << endl;
  cout << "extrabottom: " << pd.extrabottom << endl;
  cout << "black: " << pd.black << endl;
  cout << "borderleft: " << pd.borderleft << endl;
  cout << "borderright: " << pd.borderright << endl;
  cout << "bordertop: " << pd.bordertop << endl;
  cout << "borderbottom: " << pd.borderbottom << endl;
  cout << "dx: " << pd.dx << endl;
  cout << "dy: " << pd.dy << endl;
  cout << "px: " << pd.px << endl;
  cout << "py: " << pd.py << endl;
  cout << "nx: " << pd.nx << endl;
  cout << "ny: " << pd.ny << endl;
  cout << "border: " << pd.border << endl;
  cout << "*dibbits: " << *pd.dibbits << endl;
  cout << "*drawbits: " << *pd.drawbits << endl;
  cout << "*bmi: " << *pd.bmi << endl;
  cout << "startdoc: " << pd.startdoc << endl;
}


//uniques (should not have copies of)
extern int       resx,resy;            // Printer resolution, dpi (may be 0!)
extern t_printdata printdata;          // Print control structure

t_printdata   Printfile(const std::string &path, const std::string &bmp);
int    Preparefiletoprint(t_printdata *print);
int    Initializeprinting(t_printdata *print, uint pageWidth, uint pageHeight);
void   Stopprinting(t_printdata *print);
void   Printnextpage(t_printdata *print);
static void   Drawblock(int index,t_data *block,uchar *bits,int width,int height,
    int border,int nx,int ny,int dx,int dy,int px,int py,int black);
static void   Fillblock(int blockx,int blocky,uchar *bits,int width,int height,
    int border,int nx,int ny,int dx,int dy,int px,int py,int black);
#endif
