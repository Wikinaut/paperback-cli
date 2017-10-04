/*
 * =====================================================================================
 *
 *       Filename:  paperbak.c
 *
 *    Description:  
 *
 *        Version:  0.1 
 *        Created:  10/04/2017 02:53:12 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  surkeh@protonmail.com
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "paperbak.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// SERVICE FUNCTIONS ///////////////////////////////


void Reporterror(const char *input) 
{
  printf("%s\n", input);
}



void Message(const char *input, int progress) 
{
  //printf("%s @ %d\%\n", input, progress);
  printf("%s\n", input);
}



// Formerly standard case insentitive cstring compare
int strnicmp (const char *str1, const char *str2, size_t len)
{
  char s1[len], s2[len];
  strcpy (s1, str1);
  strcpy (s2, str2);
  for (int i = 0; i < len; i++) {
      s1[i] = tolower(s1[i]);
      s2[i] = tolower(s1[i]);
      if (s1[i] < s2[i])      //s1 less than s2, return negative
        return -1;
      else if (s1[i] > s2[i]) //s1 more than s2, return positive
        return 1;
  }

  // if all characters are the same, return 0
  return 0;
}



// returns 0 on success, -1 on failure
int Getpassword()
{
  // LINUX-ONLY, deprecated, and only gets 8 character long password
  //char * pw = getpass("Enter encryption password: ");
  //int pwLength = strlen(pw);

  // Crossplatform
  printf ("Enter encryption password: ");
  char pw[PASSLEN];
  int pwLength = 0;
  char ch = '\0';
  printf ("\033[8m"); //set terminal to hide typing
  while (pwLength < PASSLEN) {
    ch = getchar();
    if (ch == '\r' || ch == '\n' || ch == EOF)
      break;

    if (pwLength < (PASSLEN - 1)) {
      pw[pwLength] = ch;
      pw[pwLength + 1] = '\0';
    }

    ++pwLength;
  }
  printf ("\033[28m"); //set terminal to display typing
 
  int status = -1;
  printf ("strlen(pw): %i\n", strlen(pw));
  fwrite (pw, PASSLEN, 1, stdout);
  printf ("\n");
  if (pwLength > 0 && pwLength <= (PASSLEN - 1) ) {
    // put password into global password variable
    memcpy (pb_password, pw, PASSLEN);
    status = 0; //success
  }
  else {
    Reporterror("Password must be 32 characters or less");
    status = -1; //failure
  }
  
  printf ("strlen(pb_password): %i\n", strlen(pb_password));
  fwrite (pb_password, PASSLEN, 1, stdout);
  printf ("\n");
  // overwrite pw for security FIXME with random data
  memset (pw, 0, PASSLEN);
  return status;
}

int max (int a, int b) 
{
  return a > b ? a : b;
}

int min (int a, int b) 
{
  return a < b ? a : b;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////// WINDOWS SERVICE FUNCTIONS ///////////////////////////

#if defined(_WIN32) || defined(__CYGWIN__)
// Converts file date and time into the text according to system defaults and
// places into the string s of length n. Returns number of characters in s.
int Filetimetotext(FILETIME *fttime,char *s,int n) {
  int l;
  SYSTEMTIME sttime;
  FileTimeToSystemTime(fttime,&sttime);
  l=GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&sttime,NULL,s,n);
  s[l-1]=' ';                          // Yuck, that's Windows
  l+=GetTimeFormat(LOCALE_USER_DEFAULT,TIME_NOSECONDS,&sttime,NULL,s+l,n-l);
  return l;
};

void print_filetime(FILETIME ftime) {
    char str[30];
    int ok = Filetimetotext(&ftime, str, 30);
    if (ok) {
      printf("%s\n", str);
    }
}

#endif

