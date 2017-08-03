/*
 * =====================================================================================
 *
 *       Filename:  Decoder.h
 *
 *    Description:  Decode paperbak data obtained from bitmap file
 *
 *        Version:  1.2
 *        Created:  07/26/2017 05:43:57 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Oleh Yuschuk
 *    Modified By:  suhrke@teknik.io
 *
 * =====================================================================================
 */

#ifndef DECODER_H
#define DECODER_H

#include <string>
#include "Global.h"

#define M_BEST         0x00000001      // Search for best possible quality




typedef struct t_procdata {            // Descriptor of processed data
  int            step;                 // Next data processing step (0 - idle)
  int            mode;                 // Set of M_xxx
  uchar          *data;                // Pointer to bitmap
  int            sizex;                // X bitmap size, pixels
  int            sizey;                // Y bitmap size, pixels
  int            gridxmin;             // Rought X grid limits, pixels
  int            gridxmax;             // Rought X grid limits, pixels
  int            gridymin;             // Rought Y grid limits, pixels
  int            gridymax;             // Rought Y grid limits, pixels
  int            searchx0;             // X grid search limits, pixels
  int            searchx1;             // X grid search limits, pixels
  int            searchy0;             // Y grid search limits, pixels
  int            searchy1;             // Y grid search limits, pixels
  int            cmean;                // Mean grid intensity (0..255)
  int            cmin;                 // Minimal and maximal grid intensity
  int            cmax;                 // Minimal and maximal grid intensity
  float          sharpfactor;          // Estimated sharpness correction factor
  float          xpeak;                // Base X grid line, pixels
  float          xstep;                // X grid step, pixels
  float          xangle;               // X tilt, radians
  float          ypeak;                // Base Y grid line, pixels
  float          ystep;                // Y grid step, pixels
  float          yangle;               // Y tilt, radians
  float          blockborder;          // Relative width of border around block
  int            bufdx;                // Dimensions of block buffers, pixels
  int            bufdy;                // Dimensions of block buffers, pixels
  uchar          *buf1;                // Rotated and sharpened block
  uchar          *buf2;                // Rotated and sharpened block
  int            *bufx;                // Block grid data finders
  int            *bufy;                // Block grid data finders
  uchar          *unsharp;             // Either buf1 or buf2
  uchar          *sharp;               // Either buf1 or buf2
  float          blockxpeak;           // Exact block position in unsharp
  float          blockypeak;           // Exact block position in unsharp
  float          blockxstep;           // Exact block dimensions in unsharp
  float          blockystep;           // Exact block dimensions in unsharp
  int            nposx;                // Number of blocks to scan in X
  int            nposy;                // Number of blocks to scan in X
  int            posx;                 // Next block to scan
  int            posy;                 // Next block to scan
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


inline void print_procdata(t_procdata &pd) {
  using namespace std;
  cout << "==============================================================================" << endl;
  cout << "step: " << pd.step << endl;
  cout << "mode: " << pd.mode << endl;
  cout << "*data: " << *pd.data << endl;
  cout << "sizex: " << pd.sizex << endl;
  cout << "sizey: " << pd.sizey << endl;
  cout << "gridxmin: " << pd.gridxmin << endl;
  cout << "gridxmax: " << pd.gridxmax << endl;
  cout << "gridymin: " << pd.gridymin << endl;
  cout << "gridymax: " << pd.gridymax << endl;
  cout << "searchx0: " << pd.searchx0 << endl;
  cout << "searchx1: " << pd.searchx1 << endl;
  cout << "searchy0: " << pd.searchy0 << endl;
  cout << "searchy1: " << pd.searchy1 << endl;
  cout << "cmean: " << pd.cmean << endl;
  cout << "cmin: " << pd.cmin << endl;
  cout << "cmax: " << pd.cmax << endl;
  cout << "sharpfactor: " << pd.sharpfactor << endl;
  cout << "xpeak: " << pd.xpeak << endl;
  cout << "xstep: " << pd.xstep << endl;
  cout << "xangle: " << pd.xangle << endl;
  cout << "ypeak: " << pd.ypeak << endl;
  cout << "ystep: " << pd.ystep << endl;
  cout << "yangle: " << pd.yangle << endl;
  cout << "blockborder: " << pd.blockborder << endl;
  cout << "bufdx: " << pd.bufdx << endl;
  cout << "bufdy: " << pd.bufdy << endl;
  cout << "*buf1: " << *pd.buf1 << endl;
  cout << "*buf2: " << *pd.buf2 << endl;
  cout << "*bufx: " << *pd.bufx << endl;
  cout << "*bufy: " << *pd.bufy << endl;
  if( pd.unsharp == NULL )
    cout << "*unsharp is NULL " << endl;
  else
    cout << "*unsharp: " << *pd.unsharp << endl;
  if( pd.sharp == NULL )
    cout << "*sharp is NULL" << endl;
  else
    cout << "*sharp: " << *pd.sharp << endl;
  cout << "blockxpeak: " << pd.blockxpeak << endl;
  cout << "blockypeak: " << pd.blockypeak << endl;
  cout << "blockxstep: " << pd.blockxstep << endl;
  cout << "blockystep: " << pd.blockystep << endl;
  cout << "nposx: " << pd.nposx << endl;
  cout << "nposy: " << pd.nposy << endl;
  cout << "posx: " << pd.posx << endl;
  cout << "posy: " << pd.posy << endl;
  print_data(pd.uncorrected);
  cout << "*blocklist is non-NULL: " << (pd.blocklist!=0) << endl;
  print_superblock(pd.superblock);
  cout << "maxdotsize: " << pd.maxdotsize << endl;
  cout << "orientation: " << pd.orientation << endl;
  cout << "ngood: " << pd.ngood << endl;
  cout << "nbad: " << pd.nbad << endl;
  cout << "nsuper: " << pd.nsuper << endl;
  cout << "nrestored: " << pd.nrestored << endl;
  cout << "==============================================================================" << endl;
}



//unique 
extern int       orientation;          // Orientation of bitmap (-1: unknown)
//unique 
extern t_procdata procdata;            // Descriptor of processed data

//void   Nextdataprocessingstep(t_procdata *pdata);
int    Decodebitmap(const std::string &fileName);
int    Getgridposition(t_procdata *pdata);
int    Getgridintensity(t_procdata *pdata);
int    Getxangle(t_procdata *pdata);
int    Getyangle(t_procdata *pdata);
void   Preparefordecoding(t_procdata *pdata);
void   Decodenextblock(t_procdata *pdata);
void   Finishdecoding(t_procdata *pdata);
void   Freeprocdata(t_procdata *pdata);
void   Startbitmapdecoding(t_procdata *pdata,uchar *data,int sizex,int sizey);
void   Stopbitmapdecoding(t_procdata *pdata);
int    Decodeblock(t_procdata *pdata,int posx,int posy,t_data *result);
int    ProcessDIB(void *hdata,int offset);

#endif //DECODER_H

