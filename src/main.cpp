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
#include <stdlib.h>
#include <iostream>
#include <string>
#include "cxxopts.hpp"
#include "paperbak.h"
#include "Resource.h"

using namespace std;

#define VERSIONHI 1
#define VERSIONLO 2




inline bool isSwitchValid(int value)
{
  return ( value < 0 || value > 1 );
}



// redundancy 1 to 10
// dot size 50 to 100
// dpi 40 to 300
bool validate(cxxopts::Options &o) 
{
    bool is_ok = true;
    if ((o["mode"].as<string>().compare("encode") != 0) && 
         o["mode"].as<string>().compare("decode") != 0) {
        cerr << "error: invalid mode given" << endl;
        is_ok = false;
    }
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
    if ( isSwitchValid(o["no-header"].as<int>()) ) {
        cerr << "error: invalid value given for no-header switch" << endl;
        is_ok = false;
    }
    if ( isSwitchValid(o["border"].as<int>()) ) {
        cerr << "error: invalid value given for border switch" << endl;
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
        ("n,no-header", 
            "disable printing of file name, last modify date and time, file size, and page number as a header",
            cxxopts::value<int>() -> default_value("0")-> implicit_value("1"))
        ("b,border", 
            "print a black border around the block", 
            cxxopts::value<int>() -> default_value("0")-> implicit_value("1"))
        ;
    o.parse_positional(parg);
    o.parse(ac, av);
    if (o.count("help")) {
        cout << o.help() << endl;
        exit(EXIT_SUCCESS);
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
    // set arguments to extern (global) variables
    dpi = options["dpi"].as<int>();
    dotpercent = options["dotsize"].as<int>();
    redundancy = options["redundancy"].as<int>();
    printheader = ( ! options["no-header"].as<int>() );
    printborder = options["border"].as<int>(); 
    // decode = !encode
    bool isEncode = options["mode"].as<string>().compare("encode") == 0;

    // externs (also have matching values in printdata and/or procdata)
    std::string infile = options["input"].as<string>();
    std::string outfile = options["output"].as<string>();

    if( isEncode ) {
      // begin the process to write the bitmap,
      // if second arg is not NULL, writes a bmp to outfile
      t_printdata printdata = Printfile( infile.c_str(), outfile.c_str() );
        while (printdata.step != 0) {
          Nextdataprintingstep (&printdata);
      }
    }
    else {
      if (Decodebitmap (infile.c_str()) == 0) {
          while (procdata.step != 0) {
            Nextdataprocessingstep (&procdata);
          }
      }
    }
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