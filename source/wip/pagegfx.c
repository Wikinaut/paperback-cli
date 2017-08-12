
#include "pagegfx.h"

#include <math.h>
#include <stdio.h>


#define NDOT           32              // Block X and Y size, dots

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
