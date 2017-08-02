/*
 * =====================================================================================
 *
 *       Filename:  Global.h
 *
 *    Description:  As paperback was designed around global memory management, some 
 *                  must remain global until serious refactoring
 *
 *        Version:  1.2
 *        Created:  07/25/2017 09:23:02 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Oleh Yuschuk
 *    Modified By:  suhrke@teknik.io
 *
 * =====================================================================================
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <ctime>
#include <iostream>
#include <string>
#ifdef __WIN32
#include <windows.h>
#endif





////////////////////////////////////////////////////////////////////////////////
///////////////////////////// GENERAL DEFINITIONS //////////////////////////////

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

#define TEXTLEN        256             // Maximal length of strings

//Arbitrary values set by Borland compiler
#define MAXPATH 80
#define MAXFILE 9 
#define MAXEXT 5    
#define MAXDIR 66 
#define MAXDRIVE 3

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

typedef struct t_data {                // Block on paper
  ulong          addr;                 // Offset of the block or special code
  uchar          data[NDATA];          // Useful data
  ushort         crc;                  // Cyclic redundancy of addr and data
  uchar          ecc[32];              // Reed-Solomon's error correction code
} t_data;
//static_assert(sizeof(t_data)==128);

#define PBM_COMPRESSED 0x01            // Paper backup is compressed
#define PBM_ENCRYPTED  0x02            // Paper backup is encrypted

// FILETIME is 64-bit data type, time_t typically 64-bit, but was 32-bit in
// older *NIX versions.  long and pointers are 64-bit in *NIX while only
// 32-bit in Windows.  Depending on implementation, alignment issues possible
typedef struct t_superdata {           // Identification block on paper
  ulong          addr;                 // Expecting SUPERBLOCK
  ulong          datasize;             // Size of (compressed) data
  ulong          pagesize;             // Size of (compressed) data on page
  ulong          origsize;             // Size of original (uncompressed) data
  uchar          mode;                 // Special mode bits, set of PBM_xxx
  uchar          attributes;           // Basic file attributes
  ushort         page;                 // Actual page (1-based)
#ifdef _WIN32
  FILETIME       modified;             // Time of last file modification
#elif __linux__
  time_t         modified;
#endif
  ushort         filecrc;              // CRC of compressed decrypted file
  char           name[64];             // File name - may have all 64 chars
  ushort         crc;                  // Cyclic redundancy of previous fields
  uchar          ecc[32];              // Reed-Solomon's error correction code
} t_superdata;
//static_assert(sizeof(t_superdata)==sizeof(t_data));

typedef struct t_block {               // Block in memory
  ulong          addr;                 // Offset of the block
  ulong          recsize;              // 0 for data, or length of covered data
  uchar          data[NDATA];          // Useful data
} t_block;

typedef struct t_superblock {          // Identification block in memory
  ulong          addr;                 // Expecting SUPERBLOCK
  ulong          datasize;             // Size of (compressed) data
  ulong          pagesize;             // Size of (compressed) data on page
  ulong          origsize;             // Size of original (uncompressed) data
  ulong          mode;                 // Special mode bits, set of PBM_xxx
  ushort         page;                 // Actual page (1-based)
#ifdef _WIN32
  FILETIME       modified;             // Time of last file modification
#elif __linux__
  time_t         modified;
#endif
  ulong          attributes;           // Basic file attributes
  ulong          filecrc;              // 16-bit CRC of decrypted packed file
  char           name[64];             // File name - may have all 64 chars
  int            ngroup;               // Actual NGROUP on the page
} t_superblock;


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////     OPTIONS     ///////////////////////////////

// All unique
extern char      infile[MAXPATH];      // Last selected file to read
extern char      outbmp[MAXPATH];      // Last selected bitmap to save
extern char      inbmp[MAXPATH];       // Last selected bitmap to read
extern char      outfile[MAXPATH];     // Last selected data file to save

// All unique
extern int       dpi;                  // Dot raster, dots per inch
extern int       dotpercent;           // Dot size, percent of dpi
extern int       redundancy;           // Redundancy (NGROUPMIN..NGROUPMAX)
extern int       printheader;          // Print header and footer
extern int       printborder;          // Border around bitmap
extern int       autosave;             // Autosave completed files
extern int       bestquality;          // Determine best quality

// All unique
extern int       marginunits;          // 0:undef, 1:inches, 2:millimeters
extern int       marginleft;           // Left printer page margin
extern int       marginright;          // Right printer page margin
extern int       margintop;            // Top printer page margin
extern int       marginbottom;         // Bottom printer page margin

//void   Options(void);



inline void Reporterror(const std::string &input) {
  std::cerr << input << std::endl;
}


inline void Message(const std::string &input, int progress) {
  std::cout << input << " @ " << progress <<  "%" << std::endl;
}


// Converts file date and time into the text according to system defaults and
// places into the string s of length n. Returns number of characters in s.
#ifdef _WIN32
inline int Filetimetotext(FILETIME *fttime,char *s,int n) {
  int l;
  SYSTEMTIME sttime;
  FileTimeToSystemTime(fttime,&sttime);
  l=GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&sttime,NULL,s,n);
  s[l-1]=' ';                          // Yuck, that's Windows
  l+=GetTimeFormat(LOCALE_USER_DEFAULT,TIME_NOSECONDS,&sttime,NULL,s+l,n-l);
  return l;
};
#endif

//prints because debugging is not so effective with giant structs
inline void print_superblock(t_superblock x) {
    std::cout 
        << "addr: " << x.addr
        << "datasize: " << x.datasize
        << "pagesize: " << x.pagesize
        << "origsize: " << x.origsize
        << "mode: " << x.mode
        << "page: " << x.page
//        << "modified: " << x.modified
        << "attributes: " << x.attributes
        << "filecrc: " << x.filecrc
        << "name: " << x.name
        << "ngroup: " << x.ngroup
        << std::endl;
}

inline void print_superdata(t_superdata x) {
    std::cout 
        << "addr: " << x.addr
        << "datasize: " << x.datasize
        << "pagesize: " << x.pagesize
        << "origsize: " << x.origsize
        << "mode: " << x.mode
        << "attributes: " << x.attributes
        << "page: " << x.page
//       << "modified: " << x.modified
        << "filecrc: " << x.filecrc
        << "name: " << x.name
        << "crc: " << x.crc
        << "ecc: " << x.ecc
        << std::endl;
}

#endif //GLOBAL_H

