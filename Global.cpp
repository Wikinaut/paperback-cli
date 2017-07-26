#include "Global.h"
#include <iostream>
using namespace std;

// placeholders for old GUI functions

void Reporterror(const string &input) {
    cerr << input << endl;
}

void Message(const string &input, int progress) {
    cout << input << " @ " << progress << endl;
}

