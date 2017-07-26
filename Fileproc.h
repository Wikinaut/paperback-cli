/*
 * =====================================================================================
 *
 *       Filename:  Fileproc.h
 *
 *    Description:  
 *
 *        Version:  1.2
 *        Created:  07/26/2017 05:32:51 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Oleh Yuschuk
 *    Modified By:  suhrke@teknik.io
 *
 * =====================================================================================
 */




#include <string>



typedef struct t_fproc {               // Descriptor of processed file
  int            busy;                 // In work
  // General file data.
  char           name[64];             // File name - may have all 64 chars
  //FILETIME       modified;             // Time of last file modification
  time_t         modified;
  ulong          attributes;           // Basic file attrributes
  ulong          datasize;             // Size of (compressed) data
  ulong          pagesize;             // Size of (compressed) data on page
  ulong          origsize;             // Size of original (uncompressed) data
  ulong          mode;                 // Special mode bits, set of PBM_xxx
  int            npages;               // Total number of pages
  ulong          filecrc;              // 16-bit CRC of decrypted packed file
  // Properties of currently processed page.
  int            page;                 // Currently processed page
  int            ngroup;               // Actual NGROUP on the page
  ulong          minpageaddr;          // Minimal address of block on page
  ulong          maxpageaddr;          // Maximal address of block on page
  // Gathered data.
  int            nblock;               // Total number of data blocks
  int            ndata;                // Number of decoded blocks so far
  uchar          *datavalid;           // 0:data invalid, 1:valid, 2:recovery
  uchar          *data;                // Gathered data
  // Statistics.
  int            goodblocks;           // Total number of good blocks read
  int            badblocks;            // Total number of unreadable blocks
  ulong          restoredbytes;        // Total number of bytes restored by ECC
  int            recoveredblocks;      // Total number of recovered blocks
  int            rempages[8];          // 1-based list of remaining pages
} t_fproc;

//unique 
t_fproc   fproc;         // Processed file

void   Closefproc();
int    Startnextpage(t_superblock *superblock);
int    Addblock(t_block *block);
int    Finishpage(int ngood,int nbad,ulong nrestored);
int    Saverestoredfile(int force);
#ifdef _WIN32
int    Filetimetotext(FILETIME *fttime,char *s,int n);
#endif

