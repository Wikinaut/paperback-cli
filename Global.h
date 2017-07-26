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

#include <ctime>


////////////////////////////////////////////////////////////////////////////////
///////////////////////////// GENERAL DEFINITIONS //////////////////////////////
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

#define TEXTLEN        256             // Maximal length of strings


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

