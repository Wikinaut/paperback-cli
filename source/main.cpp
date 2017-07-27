#include "cxxopts.hpp"
#include <iostream>
using namespace std;

cxxopts::Options arguments(int ac, char **av) {
    cxxopts::Options o(av[0],
                       "Encodes or decodes high-density printable file backups.");
    vector<string> parg = {"input", "output"};
    o.add_options()
        ("h,help", "displays help")
        ("encode", 
            "action to encode data to bitmap", 
            cxxopts::value<bool>())
        ("decode", 
            "action to decode data from bitmap", 
            cxxopts::value<bool>())
        ("i,input", 
            "file to encode to or decode from",
            cxxopts::value<string>(),
            "FILE")
        ("o,output",
            "file as a result of program",
            cxxopts::value<string>(),
            "FILE")
        ("d,dpi", 
            "dots per inch of input or output bitmap", 
            cxxopts::value<int>() -> default_value("200"))
        ("s,dotsize",
            "size of the dots in bitmap as percentage of maximum dot size in pixels",
            cxxopts::value<int>() -> default_value("70"))
        ("r,redundancy", 
            "data redundancy ratio of input or output bitmap as a reciprocal", 
            cxxopts::value<int>() -> implicit_value("5"))
        ;
    o.parse_positional(parg);
    o.parse(ac, av);
    if (o.count("help")) {
        cout << o.help() << endl;
    }
    // if there is no given input or output, then invalid
    if ((o.count("i") < 1 || o.count("o") < 1) ||
        (o.count("encode") < 1 && o.count("decode") < 1)) {
        cerr << "error: no input or output file - exiting" << endl;
        exit(EXIT_FAILURE);
    }
    return o;
}

int main(int argc, char ** argv) {
    cxxopts::Options p = arguments(argc, argv);
    return 0;
}
