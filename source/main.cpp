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
    if ((o["mode"].as<string>().compare("encode") != 0) && 
         o["mode"].as<string>().compare("decode") != 0) {
        cerr << "error: invalid mode given" << endl;
        is_ok = false;
    }
    return is_ok;
}

cxxopts::Options arguments(int ac, char **av) {
    cxxopts::Options o(av[0],
                       "Encodes or decodes high-density printable file backups.");
    vector<string> parg = {"mode", "input", "output"};
    o.add_options()
        ("h,help", "displays help")
        ("v,version", "Display version and information relevant to that version")
        ("mode", 
            "encode or decode, operation on input and output", 
            cxxopts::value<string>())
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
      cout << "\nPaperBack v" << VERSIONHI << "." << VERSIONLO << endl
           << "Copyright © 2007 Oleh Yuschuk" << endl << endl
           << "----- THIS SOFTWARE IS FREE -----" << endl
           << "Released under GNU Public License (GPL 3+)" << endl
           << "Full sources available" << endl << endl
           << "Reed-Solomon ECC:" << endl
           << "Copyright © 2002 Phil Karn (GPL)" << endl << endl;
    }else if (!validate(o)) {
      exit(EXIT_FAILURE);
    }
    return o;
}

int main(int argc, char ** argv) {
  try {
    cxxopts::Options options = arguments(argc, argv);

    // decode = !encode
    bool isEncode = options["mode"].as<string>().compare("encode") == 0;

    // around line 507 in original


    // externs (also have matching values in printdata and/or procdata)
    std::string infile = options["input"].as<string>();
    std::string outfile = options["output"].as<string>();

    if( isEncode ) {
      // begin the process to write the bitmap,
      // allocate memory for printdata
      // sets printdata.infile and printdata.outbmp
      // if second arg is not NULL, writes a bmp to outfile
      Printfile( infile, outfile );
      
      // Get more attributes
      // Opens buffer for arbitrary data
      Preparefiletoprint( &printdata );

      //Get more attributes
      // Construct superblock
      Initializeprinting( &printdata );
      //Create BMPs until all data has been written to BMP
      int currStep = printdata.step;
      while ( printdata.step == currStep ) {
      Printnextpage( &printdata );
      }
    }
    else {
      // Get attributes of the inputted bitmap
      Getgridposition(&procdata);
      Getgridintensity(&procdata);
      Getxangle(&procdata);
      Getyangle(&procdata);
      // Get more attributes and allocate memory for decoding
      Preparefordecoding(&procdata);
      // Decode block by block until step changes
      int currStep = procdata.step;
      while ( procdata.step == currStep ) {
        Decodenextblock(&procdata);
      }
      Finishdecoding(&procdata);

    }

    Freeprocdata(&procdata);
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
