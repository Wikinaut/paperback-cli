#ifndef COMPRESS_H
#define COMPRESS_H




#include "Global.h"



#define DBITLEN        16              // Max. dictionary size is 2**DBITLEN
#define NBITS          8               // We deal with bytes, aren't we?



typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;



typedef struct t_centry {              // Entry in encoding dictionary
  ushort         character;
  ushort         code;
  struct t_centry *lptr;
  struct t_centry *rptr;
} t_centry;

typedef struct t_dentry {              // Entry in decoding dictionary
  ushort         character;
  struct t_dentry *prev;
} t_dentry;



static t_centry  *cdict;               // Pointer to encoding dictionary
static t_dentry  *ddict;               // Pointer to decoding dictionary

static uint      ndict;                // Current number of words in dictionary
static uchar     *bout;                // Pointer to output buffer
static ulong     nbout;                // Size of output buffer
static ulong     nout;                 // Actual number of output bytes
static uchar     *bin;                 // Pointer to input buffer
static ulong     nbin;                 // Size of input buffer
static ulong     nin;                  // Actual number of input bytes
static ulong     wrvalue;
static ulong     rdvalue;
static uint      wrbits;
static uint      rdbits;
static ulong     codelen;              // Current number of bits in output code



void Writecode(uint value);
int Compress(uchar *bufin,ulong nbufin,uchar *bufout,ulong nbufout);
void Writelink(t_dentry *chain,uint *character);
uint Writestring(uint prevcode,uint currcode,uint firstchar);
uint Readcode(void);
int Decompress(uchar *bufin,ulong nbufin,uchar *bufout,ulong nbufout);



#endif //COMPRESS_H

