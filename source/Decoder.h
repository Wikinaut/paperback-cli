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

#define M_BEST         0x00000000      // Search for best possible quality




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

