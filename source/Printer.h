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
#endif

#define PACKLEN        65536           // Length of data read buffer 64 K




typedef struct t_printdata {           // Print control structure
  int            step;                 // Next data printing step (0 - idle)
  std::string    infile;               // Name of input file
  std::string    outbmp;               // Name of output bitmap (empty: paper)
  #ifdef _WIN32
  HANDLE         hfile;                // Handle of input file
  FILETIME       modified;             // Time of last file modification
  HBITMAP        hbmp;                 // Handle of memory bitmap
  #elif __linux
  FILE           *hfile;
  time_t         modified;             // Time of last file modification
  #endif
  ulong          attributes;           // File attributes
  ulong          origsize;             // Original file size, bytes
  ulong          readsize;             // Amount of data read from file so far
  ulong          datasize;             // Size of (compressed) data
  ulong          alignedsize;          // Data size aligned to next 16 bytes
  ulong          pagesize;             // Size of (compressed) data on page
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
  int            dx,dy;                // Distance between dots, pixels
  int            px,py;                // Dot size, pixels
  int            nx,ny;                // Grid dimensions, blocks
  int            border;               // Border around the data grid, pixels
  uchar          *dibbits;             // Pointer to DIB bits
  uchar          *drawbits;            // Pointer to file bitmap bits
  uchar          bmi[sizeof(BITMAPINFO)+256*sizeof(RGBQUAD)]; // Bitmap info
  int            startdoc;             // Print job started
} t_printdata;


//uniques (should not have copies of)
extern int       resx,resy;            // Printer resolution, dpi (may be 0!)
extern t_printdata printdata;          // Print control structure

void   Printfile(const std::string &path, const std::string &bmp);
void   Preparefiletoprint(t_printdata *print);
//void   Initializeprinting(t_printdata *print);
void   Stopprinting(t_printdata *print);
void   Printnextpage(t_printdata *print);
static void   Drawblock(int index,t_data *block,uchar *bits,int width,int height,
                  int border,int nx,int ny,int dx,int dy,int px,int py,int black);
static void   Fillblock(int blockx,int blocky,uchar *bits,int width,int height,
                  int border,int nx,int ny,int dx,int dy,int px,int py,int black);
#endif
