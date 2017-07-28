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
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "cxxopts.hpp"
#include "Decoder.h"
#include "Printer.h"

using namespace std;

#define VERSIONHI 1
#define VERSIONLO 2




// redundancy 1 to 10
// dot size 50 to 100
// dpi 40 to 300
bool validate(cxxopts::Options &o) {
    bool is_ok = true;
    if (o["i"].as<string>().empty()) {
        cerr << "error: no input file given" << endl;
        is_ok = false;
    }
    if (o["o"].as<string>().empty()) {
        cerr << "error: no output file given" << endl;
        is_ok = false;
    }
    if (o["s"].as<int>() < 50 || o["s"].as<int>() > 100) {
        cerr << "error: invalid value for dot size" << endl;
        is_ok = false;
    }
    if (o["d"].as<int>() < 40 || o["d"].as<int>() > 300) {
        cerr << "error: invalid value for dpi" << endl;
        is_ok = false;
    }
    if (o["r"].as<int>() < 2 || o["r"].as<int>() > 10) {
        cerr << "error: invalid value for redundancy" << endl;
        is_ok = false;
    }
    return is_ok;
}

cxxopts::Options arguments(int ac, char **av, bool &isEncode, bool &isDecode) {
    cxxopts::Options o(av[0],
                       "Encodes or decodes high-density printable file backups.");
    vector<string> parg = {"input", "output"};
    o.add_options()
        ("h,help", "displays help")
        ("v,version", "Display version and information relevant to that version")
        ("encode", 
            "action to encode data to bitmap", 
            cxxopts::value<bool>(isEncode))
        ("decode", 
            "action to decode data from bitmap", 
            cxxopts::value<bool>(isDecode))
        ("i,input", 
            "file to encode to or decode from",
            cxxopts::value<string>(),
            "FILE")
        ("o,output",
            "file as a result of program",
            cxxopts::value<string>(),
            "FILE")
        ("d,dpi", 
            "dots per inch of input or output bitmap, between 40 and 300", 
            cxxopts::value<int>() -> default_value("200"))
        ("s,dotsize",
            "size of the dots in bitmap as percentage of maximum dot size in pixels, between 50 and 100",
            cxxopts::value<int>() -> default_value("70"))
        ("r,redundancy", 
            "data redundancy ratio of input or output bitmap as a reciprocal, between 2 and 10", 
            cxxopts::value<int>() -> default_value("5"))
        ;
    o.parse_positional(parg);
    o.parse(ac, av);
    if (o.count("help")) {
        cout << o.help() << endl;
    }else if (o.count("version")) {
      char s[1024];
      sprintf(s,"\nPaperBack v%i.%02i\n"
          "Copyright © 2007 Oleh Yuschuk\n\n"
          "----- THIS SOFTWARE IS FREE -----\n"
          "Released under GNU Public License (GPL 3+)\n"
          "Full sources available\n\n"
          "Reed-Solomon ECC:\n"
          "Copyright © 2002 Phil Karn (GPL)\n\n",
          VERSIONHI,VERSIONLO);
      cout << s;
    }else if (!validate(o)) {
      exit(EXIT_FAILURE);
    }
    return o;
}

int main(int argc, char ** argv) {
  try {
    bool isEncode = false;
    bool isDecode = false;
    cxxopts::Options options = arguments(argc, argv, isEncode, isDecode);


    // around line 507 in original


    // externs (also have matching values in printdata and/or procdata)
    std::string infileString = options["input"].as<string>();
    const char * infile = infileString.c_str();
    const char * outfile = options["output"].as<string>().c_str();

    if( isEncode ) {
      // Accepts arbitrary data, no need to check if data is good

      // ?Set struct printdata values
      printdata.step = 0;
      printdata.infile = infile; //memcpy?
      printdata.outbmp = outfile; //memcpy?
#ifdef _WIN32
      //hfile = GET HANDLE
      //FILETIME = GET FILETIME FROM HANDLE
#elif __linux__
      hfile = infileString;
      //modified = GET TIME FROM STAT
#endif 
      //!!! what other data needed?

      // ?begin the process to write the bitmap
      // if second arg is not NULL, writes a bmp to outfile
      Printfile( outfile, outfile );
      
      // printdata.step drives control flow to write bitmap
      do {
        Nextdataprintingstep(&printdata);
      } while (printdata.step != 0);


      //!!!
    }
    else if( isDecode ) {
      // Input file must be a valid bitmap 
      // Verify the input file has valid bitmap header
      //!!!

      // procdata.step drives control flow, value of 0 starts encoding
      do {
        Nextdataprocessingstep(&procdata);
      } while (procdata.step != 0);

      //!!!
    }

    Freeprocdata(&procdata);
    Stopprinting(&printdata);

  } 
  catch (const cxxopts::OptionException& e) {
    cerr << "error parsing options: " << e.what() << endl;
    exit(1);
  }
  catch (const std::exception& e) {
    cerr << "An unexpected error occurred: " << e.what() << endl;
    exit(1);
  }

  return 0;
}
