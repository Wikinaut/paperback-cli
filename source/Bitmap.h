/*
 * =====================================================================================
 *
 *       Filename:  Bitmap.h
 *
 *    Description:  Bitmap structs and defines
 *
 *        Version:  1.0
 *        Created:  07/26/2017 09:38:18 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  suhrke@teknik.io
 *
 * =====================================================================================
 */

#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>

#define BI_RGB 16 //bitmap value for uncompressed color data




typedef struct BITMAPFILEHEADER {
  uint16_t bfType = 'BM';   //filetype, must be BM
  uint32_t bfSize;          //size in bytes of bitmap file 
  uint16_t bfReserved1 = 0; //unused, except to keep alignment
  uint16_t bfReserved2 = 0; //unused, except to keep alignment
  uint32_t bfOffBits;       //offset in bytes from beginning of 
                            // BITMAPFILEHEADER to the bitmap bits
} BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER {
  uint32_t biSize;         //# of bytes in struct
  int32_t biWidth;         //width of bitmap in pixels
  int32_t biHeight;        //height of bitmap in pixels
  uint16_t biPlanes = 1;   //# of planes for target device 
  uint16_t biBitCount;     //bitmap, bits per pixel
  uint32_t biCompression;  //bitmap, type of compression
  uint32_t biSizeImage;    //size of image, in bytes
                           //   0 if BI_RGB
  int32_t biXPelsPerMeter; //horizontal resolution of target device
  int32_t biYPelsPerMeter; //vertical resolution of target device
  uint32_t biClrUsed;      //# of color indices in color table
                           //  if 0, bitmap uses max # of colors that
                           //  correspond to biBitCount
  uint32_t biClrImportant; //# of color indices required for displaying bitmap
} BITMAPINFOHEADER; 

typedef struct RGBQUAD {
  unsigned char rgbBlue;
  unsigned char rgbGreen;
  unsigned char rgbRed;
  unsigned char rgbReserved = 0;
} RGBQUAD;

typedef struct BITMAPINFO {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[1];
} BITMAPINFO;


#endif //BITMAP_H

