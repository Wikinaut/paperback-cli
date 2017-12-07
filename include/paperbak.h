////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                          THIS SOFTWARE IS FREE!                            //
//                                                                            //
// This program is free software; you can redistribute it and/or modify it    //
// under the terms of the GNU General Public License as published by the Free //
// Software Foundation; either version 2 of the License, or (at your option)  //
// any later version. This program is distributed in the hope that it will be //
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of     //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General  //
// Public License (http://www.fsf.org/copyleft/gpl.html) for more details.    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#endif

#if defined max
#undef max
#endif

#if defined min
#undef min
#endif

#include "bzlib.h"
#include "Bitmap.h"
#include "FileAttributes.h"
#include "Borland.h"




////////////////////////////////////////////////////////////////////////////////
///////////////////////////// GENERAL DEFINITIONS //////////////////////////////

// Size required by Reed-Solomon ECC
#define ECC_SIZE 32

// Oleh's magic numbers
#define FILENAME_SIZE 64

#define VERSIONHI      1               // Major version
#define VERSIONLO      2               // Minor version

#define MAINDX         800             // Max width of the main window, pixels
#define MAINDY         600             // Max height of the main window, pixels

#define TEXTLEN        256             // Maximal length of strings
#define PASSLEN        33              // Maximal length of password, incl. 0
#define USE_SHA1       1
#define AESKEYLEN      24              // AES key length in bytes (16, 24, or 32)

typedef unsigned char  uchar;
typedef uint16_t       ushort;
typedef unsigned int   uint;

#ifdef ulong
#undef ulong
#endif
typedef unsigned long  ulong;

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// DATA PROPERTIES ////////////////////////////////

// Don't change the definitions below! Program may crash if any modified!
#define NDOT           32              // Block X and Y size, dots
#define NDATA          90              // Number of data bytes in a block
#define MAXSIZE        0x0FFFFF80      // Maximal (theoretical) length of file
#define SUPERBLOCK     0xFFFFFFFF      // Address of superblock

#define NGROUP         5               // For NGROUP blocks (1..15), 1 recovery
#define NGROUPMIN      2
#define NGROUPMAX      10

typedef struct __attribute__ ((packed)) t_data { // Block on paper
  uint32_t          addr;                 // Offset of the block or special code
  uchar          data[NDATA];          // Useful data
  ushort         crc;                  // Cyclic redundancy of addr and data
  uchar          ecc[32];              // Reed-Solomon's error correction code
} t_data;
#ifdef __linux__
_Static_assert(sizeof(t_data)==128, "t_data not 128 bytes long");
#endif

#define PBM_COMPRESSED 0x01            // Paper backup is compressed
#define PBM_ENCRYPTED  0x02            // Paper backup is encrypted

// FILETIME is 64-bit data type, time_t typically 64-bit, but was 32-bit in
// older *NIX versions.  Assertion failure is likely due to this.  128 bytes
// is necessary for ECC to work properly (and multiples of 16 for CRC)
typedef struct __attribute__ ((packed)) t_superdata { // Id block on paper
  uint32_t       addr;                 // Expecting SUPERBLOCK
  uint32_t       datasize;             // Size of (compressed) data
  uint32_t       pagesize;             // Size of (compressed) data on page
  uint32_t       origsize;             // Size of original (uncompressed) data
  uchar          mode;                 // Special mode bits, set of PBM_xxx
  uchar          attributes;           // Basic file attributes
  ushort         page;                 // Actual page (1-based)
  FileTimePortable modified;           // last modify time
  ushort         filecrc;              // CRC of compressed decrypted file
  char           name[FILENAME_SIZE];  // File name - may have all 64 chars
  ushort         crc;                  // Cyclic redundancy of previous fields
  uchar          ecc[ECC_SIZE];        // Reed-Solomon's error correction code
} t_superdata;
#ifdef __linux__
_Static_assert(sizeof(t_superdata)==sizeof(t_data), 
                      "t_superdata is not the same size as t_data");
#endif

typedef struct t_block {               // Block in memory
  uint32_t       addr;                 // Offset of the block
  uint32_t       recsize;              // 0 for data, or length of covered data
  uchar          data[NDATA];          // Useful data
} t_block;

typedef struct t_superblock {          // Identification block in memory
  uint32_t       addr;                 // Expecting SUPERBLOCK
  uint32_t       datasize;             // Size of (compressed) data
  uint32_t       pagesize;             // Size of (compressed) data on page
  uint32_t       origsize;             // Size of original (uncompressed) data
  uint32_t       mode;                 // Special mode bits, set of PBM_xxx
  ushort         page;                 // Actual page (1-based)
  FileTimePortable modified;           // last modify time
  uint32_t       attributes;           // Basic file attributes
  uint32_t       filecrc;              // 16-bit CRC of decrypted packed file
  char           name[FILENAME_SIZE];  // File name - may have all 64 chars
  int            ngroup;               // Actual NGROUP on the page
} t_superblock;


////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// CRC //////////////////////////////////////

ushort Crc16(uchar *data,int length);


////////////////////////////////////////////////////////////////////////////////
////////////////////////// REED-SOLOMON ECC ROUTINES ///////////////////////////

void   Encode8(uchar *data,uchar *parity,int pad);
int    Decode8(uchar *data, int *eras_pos, int no_eras,int pad);


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// PRINTER ////////////////////////////////////

#define PACKLEN        65536           // Length of data read buffer 64 K

typedef struct t_printdata {           // Print control structure
  int            step;                 // Next data printing step (0 - idle)
  char           infile[MAXPATH];      // Name of input file
  char           outbmp[MAXPATH];      // Name of output bitmap (empty: paper)
  FILE           *hfile;               // (Formerly HANDLE) file pointer
  FileTimePortable modified;           // last modify time
  uint32_t       attributes;           // File attributes
  uint32_t       origsize;             // Original file size, bytes
  uint32_t       readsize;             // Amount of data read from file so far
  uint32_t       datasize;             // Size of (compressed) data
  uint32_t       alignedsize;          // Data size aligned to next 16 bytes
  uint32_t       pagesize;             // Size of (compressed) data on page
  int            compression;          // 0: none, 1: fast, 2: maximal
  int            encryption;           // 0: none, 1: encrypt
  int            printheader;          // Print header and footer
  int            printborder;          // Print border around bitmap
  int            redundancy;           // Redundancy
  uchar          *buf;                 // Buffer for compressed file
  uint32_t       bufsize;              // Size of buf, bytes
  uchar          *readbuf;             // Read buffer, PACKLEN bytes long
  bz_stream      bzstream;             // Compression control structure
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
  //FIXME bitmap file pointer needed?
  //HBITMAP        hbmp;                 // Handle of memory bitmap
  uchar          *dibbits;             // Pointer to DIB bits
  uchar          *drawbits;            // Pointer to file bitmap bits
  uchar          bmi[sizeof(BITMAPINFO)+256*sizeof(RGBQUAD)]; // Bitmap info
  int            startdoc;             // Print job started
} t_printdata;


int       pb_resx, pb_resy;            // Printer resolution, dpi (may be 0!)
t_printdata pb_printdata;          // Print control structure

void   Initializeprintsettings(void);
void   Closeprintsettings(void);
void   Setuppage(void);
void   Stopprinting(t_printdata *print);
void   Nextdataprintingstep(t_printdata *print);
void   Printfile(const char *path, const char *bmp);


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// DECODER ////////////////////////////////////

#define M_BEST         0x00000001      // Search for best possible quality

typedef struct t_procdata {            // Descriptor of processed data
  int            step;                 // Next data processing step (0 - idle)
  int            mode;                 // Set of M_xxx
  uchar          *data;                // Pointer to bitmap
  int            sizex;                // X bitmap size, pixels
  int            sizey;                // Y bitmap size, pixels
  int            gridxmin,gridxmax;    // Rought X grid limits, pixels
  int            gridymin,gridymax;    // Rought Y grid limits, pixels
  int            searchx0,searchx1;    // X grid search limits, pixels
  int            searchy0,searchy1;    // Y grid search limits, pixels
  int            cmean;                // Mean grid intensity (0..255)
  int            cmin,cmax;            // Minimal and maximal grid intensity
  float          sharpfactor;          // Estimated sharpness correction factor
  float          xpeak;                // Base X grid line, pixels
  float          xstep;                // X grid step, pixels
  float          xangle;               // X tilt, radians
  float          ypeak;                // Base Y grid line, pixels
  float          ystep;                // Y grid step, pixels
  float          yangle;               // Y tilt, radians
  float          blockborder;          // Relative width of border around block
  int            bufdx,bufdy;          // Dimensions of block buffers, pixels
  uchar          *buf1,*buf2;          // Rotated and sharpened block
  int            *bufx,*bufy;          // Block grid data finders
  uchar          *unsharp;             // Either buf1 or buf2
  uchar          *sharp;               // Either buf1 or buf2
  float          blockxpeak,blockypeak;// Exact block position in unsharp
  float          blockxstep,blockystep;// Exact block dimensions in unsharp
  int            nposx;                // Number of blocks to scan in X
  int            nposy;                // Number of blocks to scan in X
  int            posx,posy;            // Next block to scan
  t_data         uncorrected;          // Data before ECC for block display
  t_block        *blocklist;           // List of blocks recognized on page
  t_superblock   superblock;           // Page header
  int            maxdotsize;           // Maximal size of the data dot, pixels
  int            orientation;          // Data orientation (-1: unknown)
  int            ngood;                // Page statistics: good blocks
  int            nbad;                 // Page statistics: bad blocks
  int            nsuper;               // Page statistics: good superblocks
  int            nrestored;            // Page statistics: restored bytes
} t_procdata;

int       pb_orientation;          // Orientation of bitmap (-1: unknown)
t_procdata pb_procdata;            // Descriptor of processed data

void   Nextdataprocessingstep(t_procdata *pdata);
void   Freeprocdata(t_procdata *pdata);
void   Startbitmapdecoding(t_procdata *pdata,uchar *data,int sizex,int sizey);
void   Stopbitmapdecoding(t_procdata *pdata);
int    Decodeblock(t_procdata *pdata,int posx,int posy,t_data *result);


////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// FILE PROCESSOR ////////////////////////////////

#define NFILE          5               // Max number of simultaneous files

typedef struct t_fproc {               // Descriptor of processed file
  int            busy;                 // In work
  // General file data.
  char           name[64];             // File name - may have all 64 chars
  FileTimePortable modified;           // last modify time
  uint32_t       attributes;           // Basic file attrributes
  uint32_t       datasize;             // Size of (compressed) data
  uint32_t       pagesize;             // Size of (compressed) data on page
  uint32_t       origsize;             // Size of original (uncompressed) data
  uint32_t       mode;                 // Special mode bits, set of PBM_xxx
  int            npages;               // Total number of pages
  uint32_t       filecrc;              // 16-bit CRC of decrypted packed file
  // Properties of currently processed page.
  int            page;                 // Currently processed page
  int            ngroup;               // Actual NGROUP on the page
  uint32_t       minpageaddr;          // Minimal address of block on page
  uint32_t       maxpageaddr;          // Maximal address of block on page
  // Gathered data.
  int            nblock;               // Total number of data blocks
  int            ndata;                // Number of decoded blocks so far
  uchar          *datavalid;           // 0:data invalid, 1:valid, 2:recovery
  uchar          *data;                // Gathered data
  // Statistics.
  int            goodblocks;           // Total number of good blocks read
  int            badblocks;            // Total number of unreadable blocks
  uint32_t       restoredbytes;        // Total number of bytes restored by ECC
  int            recoveredblocks;      // Total number of recovered blocks
  int            rempages[8];          // 1-based list of remaining pages
} t_fproc;

t_fproc   pb_fproc[NFILE];             // Processed file

void   Closefproc(int slot);
int    Startnextpage(t_superblock *superblock);
int    Addblock(t_block *block,int slot);
int    Finishpage(int slot,int ngood,int nbad,uint32_t nrestored);
int    Saverestoredfile(int slot,int force);


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// SCANNER ////////////////////////////////////

int    Decodebitmap(char *path);


////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// USER INTERFACE ////////////////////////////////

char      pb_infile[MAXPATH];      // Last selected file to read
char      pb_outbmp[MAXPATH];      // Last selected bitmap to save
char      pb_inbmp[MAXPATH];       // Last selected bitmap to read
char      pb_outfile[MAXPATH];     // Last selected data file to save
 
char      pb_password[PASSLEN];    // Encryption password
 
int       pb_dpi;                  // Dot raster, dots per inch
int       pb_dotpercent;           // Dot size, percent of dpi
int       pb_compression;          // 0: none, 1: fast, 2: maximal
int       pb_redundancy;           // Redundancy (NGROUPMIN..NGROUPMAX)
int       pb_printheader;          // Print header and footer
int       pb_printborder;          // Border around bitmap
int       pb_autosave;             // Autosave completed files
int       pb_bestquality;          // Determine best quality
int       pb_encryption;           // Encrypt data before printing
int       pb_opentext;             // Enter passwords in open text
  
int       pb_marginunits;          // 0:undef, 1:inches, 2:millimeters
int       pb_marginleft;           // Left printer page margin
int       pb_marginright;          // Right printer page margin
int       pb_margintop;            // Top printer page margin
int       pb_marginbottom;         // Bottom printer page margin
  

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// SERVICE FUNCTIONS ///////////////////////////////


void Reporterror(const char *input);

void Message(const char *input, int progress);

// Formerly standard case insentitive cstring compare
int strnicmp (const char *str1, const char *str2, size_t len);

// returns 0 on success, -1 on failure
int Getpassword();

int max (int a, int b);

int min (int a, int b);


////////////////////////////////////////////////////////////////////////////////
////////////////////////// WINDOWS SERVICE FUNCTIONS ///////////////////////////

#if defined(_WIN32) || defined(__CYGWIN__)
// Converts file date and time into the text according to system defaults and
// places into the string s of length n. Returns number of characters in s.
int Filetimetotext(FILETIME *fttime,char *s,int n);

void print_filetime(FILETIME ftime);

#endif

