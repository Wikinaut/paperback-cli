
#include "pagegfx.h"

#include <stdio.h>
#include <string.h>

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define NDOT 32              // Block X and Y size, dots

// Service function, clips regular 32x32-dot raster to bitmap in the position
// with given block coordinates (may be outside the bitmap).
void Fillblock(int blockx,
               int blocky,
               unsigned char *bits,
               int width,
               int height,
               int border,
               int nx,
               int ny,
               int dx,
               int dy,
               int px,
               int py,
               int black) {
typedef unsigned char uchar;
#define black 0
  int i,j,x0,y0,x,y,m,n;
  ulong t;
  // Convert cell coordinates into the X-Y bitmap coordinates.
  x0=blockx*(NDOT+3)*dx+2*dx+border;
  y0=blocky*(NDOT+3)*dy+2*dy+border;
  // Print raster.
  for (j=0; j<32; j++) {
    if ((j & 1)==0)
      t=0x55555555;
    else {
      if (blocky<0 && j<=24) t=0;
      else if (blocky>=ny && j>8) t=0;
      else if (blockx<0) t=0xAA000000;
      else if (blockx>=nx) t=0x000000AA;
      else t=0xAAAAAAAA; };
    for (i=0; i<32; i++) {
      if (t & 1) {
        for (m=0; m<py; m++) {
          for (n=0; n<px; n++) {
            x=x0+i*dx+n;
            y=y0+j*dy+m;
            if (x<0 || x>=width || y<0 || y>=height)
              continue;
            bits[(height-y-1)*width+x]=(uchar)black;
          };
        };
      };
      t>>=1;
    };
  };
};


int Getprintablewidth(unsigned int pwidth, int ppix) {
    int leftborder  = ppix/2;
    int rightborder = ppix/2;
    return pwidth * ppix - (leftborder + rightborder);
}

int Getprintableheight (unsigned int pheight, 
                        int ppiy, 
                        int extratop, 
                        int extrabottom) {
    int topborder    = ppiy/2;
    int bottomborder = ppiy/2;
    return pheight * ppiy - (topborder + bottomborder + extratop + extrabottom);
}

int Getdatapointraster(int pointsize, int dpi) {
    return max(pointsize/dpi, 2);
}

int Getpointsize(int dpr, int dc) {
    return max((dpr * dc)/100, 1);
}

int Getprintableaxiscount (unsigned int axis,
                           int dpi,
                           int dotpercent,
                           int border,
                           int ppi) {
   int dpr = Getdatapointraster(ppi, dpi);
   int p   = Getpointsize(dpr, dotpercent);
   return (axis - p - 2 * border) / NDOT * dpr + 3 * dpr;
}

struct pagegfx init_pagegfx(int ppix, 
                            int ppiy, 
                            int dpi, 
                            int dotpercent, 
                            int redundancy, 
                            int pagewidth, 
                            int pageheight,
                            bool has_border) {
    bool is_ok;
    int dx = Getdatapointraster(ppix, dpi);
    int dy = Getdatapointraster(ppiy, dpi);

    int px = Getpointsize(dx, dotpercent);
    int py = Getpointsize(dy, dotpercent);

    int b;
    if (has_border) {
        b = dx * 16;
    } else {
        b = 25;
    }

    int nx = Getprintableaxiscount(Getprintablewidth(pagewidth, ppix), 
                                   dpi,
                                   dotpercent,
                                   b,
                                   ppix);
    int ny = Getprintableaxiscount(Getprintableheight(pageheight, ppiy, 0, 0), 
                                   dpi, 
                                   dotpercent,
                                   b,
                                   ppiy);

    if (nx > 0 && ny > 0 && (long)(nx * ny) < 0) {
        fprintf(stderr, "error: input file is too large to back up" 
                        "- please break the file apart.");
        is_ok = false;
    }
    if (nx < redundancy + 1 || ny < 3 || nx * ny < 2 * redundancy + 2) {
        fprintf(stderr, "printable area is too small, reduce borders or blocksize");
        is_ok = false;
    }
    int drawwidth = nx * (NDOT + 3) * dx + px + 2 * b + 3 & 0xFFFFFFFC;
    int drawheight = ny * (NDOT + 3) * dy + py + 2 * b;

    struct pagegfx p = {};
    p.dx = dx;
    p.dy = dy;
    p.px = py;
    p.py = py;
    p.nx = nx;
    p.ny = ny;
    p.width = drawwidth;
    p.height = drawheight;
    return p;
}

void draw_grid_vlines (unsigned char *bits, 
                       struct pagegfx p,
                       bool has_border, 
                       int border) {
#define START 0
#define CLR_BLACK 0
    int i, j, k;
    int basex;
    for (i = START; i <= p.nx; i++) {
        if (has_border) {
            basex = i*(NDOT + 3) * p.dx + border;
            for (j = START; j < p.ny *(NDOT + 3) * p.dy + p.py + 2 * border; j++, basex += p.width) {
                for (k = START; k < p.px; k++) { 
                    bits[basex+k] = CLR_BLACK;
                }
            }
        } else {
            basex = i*(NDOT + 3) * p.dx + p.width * border + border;
            for (j = START; j < p.ny *(NDOT + 3) * p.dy; j++, basex += p.width) {
                for (k = START; k < p.px; k++) { 
                    bits[basex+k] = CLR_BLACK;
                }
            }
        }
    }
}

void draw_grid_hline(unsigned char *bits,
                     struct pagegfx p,
                     bool has_border,
                     int border) {
#define START 0
#define CLR_BLACK 0
    int i, j, k;
    for (j = START; j <= p.ny; j++) {
        if (has_border) {
            for (k = START; k < p.py; k++) {
                memset(bits + (j*(NDOT+3) * p.dy + k + border) * p.width, 
                       CLR_BLACK, 
                       p.width);
            }
        } else {
            for (k = START; k < p.py; k++) {
                memset(bits + (j * (NDOT+3) * p.dy + k + border) * p.width + border, 
                       CLR_BLACK, 
                       p.nx * (NDOT+3) * p.dx + p.px);
            }
        }
    }
}

void fillraster(unsigned char *bits,
                struct pagegfx p,
                int border,
                bool has_border) {
#define CLR_BLACK 0
    unsigned int i, j;
    if (has_border) {
        for (j = -1; j <= p.ny; j++) {
            Fillblock(-1,
                      j,
                      bits,
                      p.width,
                      p.height,
                      border,
                      p.nx,
                      p.ny,
                      p.dx,
                      p.dy,
                      p.px,
                      p.py,
                      CLR_BLACK);
            Fillblock(p.nx,
                      j,
                      bits,
                      p.width,
                      p.height,
                      border,
                      p.nx,
                      p.ny,
                      p.dx,
                      p.dy,
                      p.px,
                      p.py,
                      CLR_BLACK);
        }
        for (i = 0; i < p.nx; i++) {
            Fillblock(i,
                      -1,
                      bits,
                      p.width,
                      p.height,
                      border,
                      p.nx,
                      p.ny,
                      p.dx,
                      p.dy,
                      p.px,
                      p.py,
                      CLR_BLACK);
            Fillblock(i,
                      p.ny,
                      bits,
                      p.width,
                      p.height,
                      border,
                      p.nx,
                      p.ny,
                      p.dx,
                      p.dy,
                      p.px,
                      p.py,
                      CLR_BLACK);
        }
    }
}

