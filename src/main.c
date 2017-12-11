/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  Cross-platform command line version of Oleh Yuchuk's Paperbak, a 
 *                  (relatively) high-density paper backup solution
 *
 *        Version:  1.2
 *        Created:  07/27/2017 03:04:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  scuti@teknik.io
 *                  surkeh@protonmail.com
 *
 * =====================================================================================
 */


////////////////////////////////////////////////////////////////////////////////
//
// Data is kept in matrix 32x32 points. It consists of:
//
//   4-byte address (combined with redundancy count) or special marker;
//   90-byte compressed and encrypted data;
//   2-byte CRC of address and data (CCITT version);
//   32-byte Reed-Solomon error correction code of the previous 96 bytes.
//
// Top left point is the LSB of the low byte of address. Second point in the
// topmost row is the second bit, etc. I have selected horizontal orientation
// of bytes because jet printers in draft mode may shift rows in X. This may
// lead to the loss of 4 bytes, but not 32 at once.
//
// Even rows are XORed with 0x55555555 and odd with 0xAAAAAAAA to prevent long
// lines or columns of zeros or ones in uncompressed data.
//
// For each ngroup=redundancy data blocks, program creates one artificial block
// filled with data which is the XOR of ngroup blocks, additionally XORed with
// 0xFF. Blocks within the group are distributed through the sheet into the
// different rows and columns, thus increasing the probability of data recovery,
// even if some parts are completely missing. Redundancy blocks contain ngroup
// in the most significant 4 bits.
//
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>

#include "paperbak.h"
#include "Resource.h"

#define VERSIONHI 1
#define VERSIONLO 2


// Global forward declarations
t_fproc   pb_fproc[NFILE];        // Processed file
int       pb_resx, pb_resy;        // Printer resolution, dpi (may be 0!)
t_printdata pb_printdata;          // Print control structure
int       pb_orientation;          // Orientation of bitmap (-1: unknown)
t_procdata pb_procdata;            // Descriptor of processed data
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

// New globals
int       pb_npages;


// Function prototypes
int arguments (int ac, char **av);
void dhelp (const char *exe);
void dversion();
void nextBitmap (char *path);

// Enumerator types
enum Mode {
  MODE_ENCODE,
  MODE_DECODE,
  MODE_VERSION,
  MODE_HELP
};



int main (int argc, char ** argv) {
    // set values needed for cli version
    pb_autosave = 1;
    
    // set default values for vars affected by arg parsing
    pb_infile[0]   = '\0';
    pb_outfile[0]  = '\0';
    pb_outbmp[0]   = '\0';
    pb_npages      = 0;
    pb_dpi         = 200;
    pb_dotpercent  = 70;
    pb_redundancy  = 5;
    pb_printheader = 0;
    pb_printborder = 0;

    int mode = arguments (argc, argv);
    if (mode == MODE_ENCODE) {
        printf ("Encoding %s to create %s\n"
                "DPI: %d\n"
                "Dot percent: %d\n"
                "Redundancy: 1:%d\n"
                "Print header/footer: %d\n"
                "Print border: %d\n",
                pb_infile, pb_outbmp,
                pb_dpi, pb_dotpercent, pb_redundancy,
                pb_printheader, pb_printborder);

        Printfile (pb_infile, pb_outbmp);
        while (pb_printdata.step != 0) {
            Nextdataprintingstep (&pb_printdata);
        }
    }
    else if (mode == MODE_DECODE) {
        char drv[MAXDRIVE],dir[MAXDIR],nam[MAXFILE],ext[MAXEXT],path[MAXPATH+32];
        fnsplit (pb_infile, drv, dir, nam, ext);
        int i;
        if (pb_npages > 0) {
          for (int i = 0; i < pb_npages; i++) {
            sprintf(path,"%s%s%s_%04i%s",drv,dir,nam,i+1,ext);
            Decodebitmap (path);
            while (pb_procdata.step != 0) {
              nextBitmap (path);
            }
          }
        }
        else {
          sprintf(path,"%s%s%s%s",drv,dir,nam,ext);
          nextBitmap (path);
        }
    }
    else if (mode == MODE_VERSION) {
      dversion(argv[0]);
    }
    else {
      dhelp(argv[0]);
    }

    return 0;
}



inline void nextBitmap (char *path) {
  printf ("Decoding %s into %s\n", path, pb_outfile);
  Decodebitmap (path);
  while (pb_procdata.step != 0) {
    Nextdataprocessingstep (&pb_procdata);
  }
}



inline void dhelp (const char *exe) {
    printf("%s\n\n"
            "Usage:\n"
            "\t%s --encode -i [infile] -o [out].bmp [OPTION...]\n"
            "\t%s --decode -i [in].bmp -o [outfile]\n"
            "\t%s --decode -i [in].bmp -o [outfile] -p [nPages]\n"
            "\t--encode             Create a bitmap from the input file\n"
            "\t--decode             Decode an encoded bitmap/folder of bitmaps\n"
            "\t-i, --input          File to encode to or decode from\n"
            "\t-o, --output         Newly encoded bitmap or decoded file\n"
            "\t-p, --pages          Number of pages (e.g. bitmaps labeled 0001 through 0029)\n"
            "\t-d, --dpi            Dots per inch of the output bitmap (40 to 600)\n"
            "\t-s, --dotsize        Size of the dots in bitmap as percentage of maximum dot\n"
            "\t                     size in pixels, (50 to 100)\n"
            "\t-r, --redundancy     Data redundancy ratio of input or output bitmap as a\n"
            "\t                     reciprocal, (2 to 10)\n"
            "\t-n, --no-header      Disable printing of file name, last modify date and time,\n"
            "\t                     file size, and page number\n"
            "\t-b, --border         Print a black border around the page\n"
            "\t-v, --version        Display version and information about that version\n"
            "\t-h, --help           Display all arguments and program description\n\n",
            "\nEncodes or decodes high-density printable file backups.\n",
            exe,
            exe,
            exe);
}



inline void dversion() {
    printf("\nPaperBack v%d.%d\n"
            "Copyright © 2007 Oleh Yuschuk\n\n"
            "Parts copyright © 2013 Michael Mohr\n\n"
            "----- THIS SOFTWARE IS FREE -----\n"
            "Released under GNU Public License (GPL 3+)\n"
            "Full sources available\n\n"
            "Reed-Solomon ECC:\n"
            "Copyright © 2002 Phil Karn (GPL)\n\n"
            "Bzip2 data compression:\n"
            "Copyright © 1996-2010 Julian R. Seward (see sources)\n\n"
            "AES and SHA code:\n"
            "Copyright © 1998-2010, Brian Gladman (3-clause BSD)\n",
            VERSIONHI, VERSIONLO);
}



int arguments (int ac, char **av) {
    bool is_ok = true;
    int mode = MODE_HELP;
    struct option long_options[] = {
        // options that set flags
        {"encode",      no_argument, &mode, MODE_ENCODE},
        {"decode",      no_argument, &mode, MODE_DECODE},
        // options that assign values in switch
        {"input",       required_argument, NULL,  'i'},
        {"output",      required_argument, NULL,  'o'},
        {"pages",       required_argument, NULL,  'p'},
        {"dpi",         required_argument, NULL,  'd'},
        {"dotsize",     required_argument, NULL,  's'},
        {"redundancy",  required_argument, NULL,  'r'},
        {"no-header",   no_argument, NULL,        'n'},
        {"border",      no_argument, NULL,        'b'},
        {"version",     no_argument, NULL,        'v'},
        {"help",        no_argument, NULL,        'h'},
        {0, 0, 0, 0}
    };
    int c;
    while(is_ok) {
        int options_index = 0;
        c = getopt_long (ac, av, "i:o:p:f:d:s:r:nbvh", long_options, &options_index);
        if (c == -1) {
            break;
        }
        switch(c) {
            case 0:
                break;
            case 'i':
                if (optarg == NULL) {
                    fprintf(stderr, "error: arg is NULL ! \n");
                    is_ok = false;
                } else {
                    strcpy (pb_infile, optarg);
                }
                break;
            case 'o':
                if (optarg == NULL) {
                    fprintf(stderr, "error: outfile arg is null \n");
                    is_ok = false;
                } else {
                    strcpy (pb_outfile, optarg);
                    strcpy (pb_outbmp, optarg);
                }
                break;
            case 'p':
                if (optarg == NULL) {
                    fprintf(stderr, "error: pages arg is null \n");
                    is_ok = false;
                } else {
                   pb_npages     = atoi(optarg); 
                }
                break;
            case 'd':
                if (optarg != NULL)
                  pb_dpi         = atoi(optarg);
                break;
            case 's':
                if (optarg != NULL)
                  pb_dotpercent  = atoi(optarg);
                break;
            case 'r':
                if (optarg != NULL)
                  pb_redundancy  = atoi(optarg);
                break;
            case 'n':
                if (optarg != NULL)
                  pb_printheader = !(atoi(optarg));
                break;
            case 'b':
                if (optarg != NULL)
                  pb_printborder = atoi(optarg);
                break;
            case 'v':
                // as soon as -v encountered, return version mode
                return MODE_VERSION;
            case 'h':
                // as soon as -h encountered, return help mode
                return MODE_HELP;
            default:
                // as soon as unknown flag encountered, return help mode
                return MODE_HELP;
        }
    }
    if (strlen (pb_infile) == 0) {
        fprintf (stderr, "error: no input file given\n");
        return MODE_HELP;
    }
    if (strlen (pb_outfile) == 0) {
        fprintf (stderr, "error: no output file given\n");
        return MODE_HELP;
    }
    if (pb_npages < 0 || pb_npages > 9999) {
        fprintf (stderr, "error: invalid number of pages given\n");
        return MODE_HELP;
    }
    if (pb_dotpercent < 50 || pb_dotpercent > 100) {
        fprintf (stderr, "error: invalid dotsize given\n");
        return MODE_HELP;
    }
    if (pb_dpi < 40 || pb_dpi > 600) {
        fprintf (stderr, "error: invalid DPI given\n");
        return MODE_HELP;
    }
    if (pb_redundancy < 2 || pb_redundancy > 10) {
        fprintf (stderr, "error: invalid redundancy given\n");
        return MODE_HELP;
    }
    if (pb_printheader < 0 || pb_printheader > 1) {
        fprintf (stderr, "error: invalid header setting given\n");
        return MODE_HELP;
    }
    if (pb_printborder < 0 || pb_printborder > 1) {
        fprintf (stderr, "error: invalid border setting given\n");
        return MODE_HELP;
    }
    
    return mode;
}


