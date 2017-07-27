#include "Global.h"
#include <iostream>




void Reporterror(const std::string &input) {
  std::cerr << input << std::endl;
}



void Message(const std::string &input, int progress) {
  std::cout << input << " @ " << progress << std::endl;
}



// Converts file date and time into the text according to system defaults and
// places into the string s of length n. Returns number of characters in s.
#ifdef _WIN32
int Filetimetotext(FILETIME *fttime,char *s,int n) {
  int l;
  SYSTEMTIME sttime;
  FileTimeToSystemTime(fttime,&sttime);
  l=GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&sttime,NULL,s,n);
  s[l-1]=' ';                          // Yuck, that's Windows
  l+=GetTimeFormat(LOCALE_USER_DEFAULT,TIME_NOSECONDS,&sttime,NULL,s+l,n-l);
  return l;
};
#endif

