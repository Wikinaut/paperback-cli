////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// PaperBack -- high density backups on the plain paper                       //
//                                                                            //
// Copyright (c) 2007 Oleh Yuschuk                                            //
// ollydbg at t-online de (set Subject to 'paperback' or be filtered out!)    //
//                                                                            //
//                                                                            //
// This file is part of PaperBack.                                            //
//                                                                            //
// Paperback is free software; you can redistribute it and/or modify it under //
// the terms of the GNU General Public License as published by the Free       //
// Software Foundation; either version 3 of the License, or (at your option)  //
// any later version.                                                         //
//                                                                            //
// PaperBack is distributed in the hope that it will be useful, but WITHOUT   //
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or      //
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for   //
// more details.                                                              //
//                                                                            //
// You should have received a copy of the GNU General Public License along    //
// with this program. If not, see <http://www.gnu.org/licenses/>.             //
//                                                                            //
//                                                                            //
// Note that bzip2 compression/decompression library, which is the part of    //
// this project, is covered by different license, which, in my opinion, is    //
// compatible with GPL.                                                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include "bzlib.h"
#include "aes.h"
#include "Bitmap.h"

#include "paperbak.h"
#include "Resource.h"




// Processes data from the scanner.
int ProcessDIB(void *hdata,int offset) {
  int i,j,sizex,sizey,ncolor;
  uchar scale[256],*data,*pdata,*pbits;
  BITMAPINFO *pdib;
  pdib=(BITMAPINFO *)hdata;
  if (pdib==NULL)
    return -1;                         // Something is wrong with this DIB
  // Check that bitmap is more or less valid.
  if (pdib->bmiHeader.biSize!=sizeof(BITMAPINFOHEADER) ||
    pdib->bmiHeader.biPlanes!=1 ||
    (pdib->bmiHeader.biBitCount!=8 && pdib->bmiHeader.biBitCount!=24) ||
    (pdib->bmiHeader.biBitCount==24 && pdib->bmiHeader.biClrUsed!=0) ||
    pdib->bmiHeader.biCompression!=BI_RGB ||
    pdib->bmiHeader.biWidth<128 || pdib->bmiHeader.biWidth>32768 ||
    pdib->bmiHeader.biHeight<128 || pdib->bmiHeader.biHeight>32768
  ) {
    //GlobalUnlock(hdata);
    return -1; // Not a known bitmap!
  };                      
  sizex=pdib->bmiHeader.biWidth;
  sizey=pdib->bmiHeader.biHeight;
  ncolor=pdib->bmiHeader.biClrUsed;
  // Convert bitmap to 8-bit grayscale. Note that scan lines are DWORD-aligned.
  data=(uchar *)malloc(sizex*sizey);
  if (data==NULL) {
    //GlobalUnlock(hdata);
    return -1; };
  if (pdib->bmiHeader.biBitCount==8) {
    // 8-bit bitmap with palette.
    if (ncolor>0) {
      for (i=0; i<ncolor; i++) {
        scale[i]=(uchar)((pdib->bmiColors[i].rgbBlue+
        pdib->bmiColors[i].rgbGreen+pdib->bmiColors[i].rgbRed)/3);
      }; }
    else {
      for (i=0; i<256; i++) scale[i]=(uchar)i; };
    if (offset==0)
      offset=sizeof(BITMAPINFOHEADER)+ncolor*sizeof(RGBQUAD);
    pdata=data;
    for (j=0; j<sizey; j++) {
      offset=(offset+3) & 0xFFFFFFFC;
      pbits=((uchar *)(pdib))+offset;
      for (i=0; i<sizex; i++) {
        *pdata++=scale[*pbits++]; };
      offset+=sizex;
    }; }
  else {
    // 24-bit bitmap without palette.
    if (offset==0)
      offset=sizeof(BITMAPINFOHEADER)+ncolor*sizeof(RGBQUAD);
    pdata=data;
    for (j=0; j<sizey; j++) {
      offset=(offset+3) & 0xFFFFFFFC;
      pbits=((uchar *)(pdib))+offset;
      for (i=0; i<sizex; i++) {
        *pdata++=(uchar)((pbits[0]+pbits[1]+pbits[2])/3);
        pbits+=3; };
      offset+=sizex*3;
    };
  };
  // Decode bitmap. This is what we are for here.
  Startbitmapdecoding(&pb_procdata,data,sizex,sizey);
  // Free original bitmap and report success.
  //GlobalUnlock(hdata);
  return 0;
};



// Opens and decodes bitmap. Returns 0 on success and -1 on error.
int Decodebitmap(char *path) {
  int i,size;
  char s[TEXTLEN+MAXPATH],fil[MAXFILE],ext[MAXEXT];
  uchar *data,buf[sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)];
  FILE *f;
  BITMAPFILEHEADER *pbfh;
  BITMAPINFOHEADER *pbih;
  //HCURSOR prevcursor;
  // Ask for file name.
  //if (path==NULL || path[0]=='\0') {
  //  if (Selectinbmp()!=0) return -1; }
  //else {
  strncpy(pb_inbmp,path,sizeof(pb_inbmp));
  pb_inbmp[sizeof(pb_inbmp)-1]='\0';
  fnsplit(pb_inbmp,NULL,NULL,fil,ext);
  sprintf(s,"Reading %s%s...",fil,ext);
  Message(s,0);
  //Updatebuttons();
  // Open file and verify that this is the valid bitmap of known type.
  f=fopen(pb_inbmp,"rb");
  if (f==NULL) {                       // Unable to open file
    sprintf(s,"Unable to open %s%s",fil,ext);
    Reporterror(s);
    return -1; };
  // Reading 100-MB bitmap may take many seconds. Let's inform user by changing
  // mouse pointer.
  //prevcursor=SetCursor(LoadCursor(NULL,IDC_WAIT));
  i=fread(buf,1,sizeof(buf),f);
  //SetCursor(prevcursor);
  if (i!=sizeof(buf)) {                // Unable to read file
    sprintf(s,"Unable to read %s%s",fil,ext);
    Reporterror(s);
    fclose(f); 
    return -1; 
  };
  pbfh=(BITMAPFILEHEADER *)buf;
  pbih=(BITMAPINFOHEADER *)(buf+sizeof(BITMAPFILEHEADER));
  if (pbfh->bfType!=CHAR_BM ||
    pbih->biSize!=sizeof(BITMAPINFOHEADER) || pbih->biPlanes!=1 ||
    (pbih->biBitCount!=8 && pbih->biBitCount!=24) ||
    (pbih->biBitCount==24 && pbih->biClrUsed!=0) ||
    pbih->biCompression!=BI_RGB ||
    pbih->biWidth<128 || pbih->biWidth>32768 ||
    pbih->biHeight<128 || pbih->biHeight>32768
  ) {                                  // Invalid bitmap type
    sprintf(s,"Unsupported bitmap type: %s%s",fil,ext);
    Reporterror(s);
    fclose(f); return -1; };
  // Allocate buffer and read file.
  fseek(f,0,SEEK_END);
  size=ftell(f)-sizeof(BITMAPFILEHEADER);
  data=(uchar *)malloc(size);
  if (data==NULL) {                    // Unable to allocate memory
    Reporterror("Low memory");
    fclose(f); return -1; };
  fseek(f,sizeof(BITMAPFILEHEADER),SEEK_SET);
  i=fread(data,1,size,f);
  fclose(f);
  if (i!=size) {                       // Unable to read bitmap
    sprintf(s,"Unable to read %s%s",fil,ext);
    Reporterror(s);
    free(data);
    return -1; };
  // Process bitmap.
  ProcessDIB(data,pbfh->bfOffBits-sizeof(BITMAPFILEHEADER));
  free(data);
  return 0;
};

