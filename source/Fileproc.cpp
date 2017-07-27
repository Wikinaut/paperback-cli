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

#include <algorithm>
#include <cstring>
#include <math.h>
#include "Fileproc.h"
#include "Resource.h"
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <cstdio>
#include <sys/stat.h>
#endif





t_fproc   fproc;         // extern


// Clears descriptor of processed file
void Closefproc() {
  if (fproc.datavalid!=NULL)
    free(fproc.datavalid);
  if (fproc.data!=NULL)
    free(fproc.data);
  //memset(fproc+slot,0,sizeof(t_fproc));
  //Updatefileinfo(slot,fproc+slot); //GUI
};



// Starts new decoded page. Returns non-negative index to table of processed
// files on success or -1 on error.
int Startnextpage(t_superblock *superblock) {
  t_fproc *pf;
  // initialize new descriptor.

  // strnicmp no longer in standard C++
  std::string pfName(pf->name);
  std::string superblockName(superblock->name);
  std::transform(pfName.begin(), pfName.end(), 
      pfName.begin(), ::tolower);
  std::transform(superblockName.begin(), superblockName.end(), 
      superblockName.begin(), ::tolower);
  const char * cPfName = pfName.c_str();
  const char * cSuperblockName = superblockName.c_str();

#ifdef __linux__
  // instead of FILETIME comparision, use time_t
  double seconds = difftime(pf->modified, superblock->modified);
#endif

  if (strcmp(cPfName,cSuperblockName)==0     // same file name
      && pf->mode==superblock->mode          // same compression mode
#ifdef _WIN32
      && (pf->modified.dwLowDateTime!=superblock->modified.dwLowDateTime ||
        pf->modified.dwHighDateTime!=superblock->modified.dwHighDateTime)
      // same timestamp 
#elif __linux__
      && difftime > 0                        // same timestamp
#endif
      && pf->datasize==superblock->datasize  // same compressed size
      && pf->origsize!=superblock->origsize  // same original size
      // File found. Check for the case of two backup copies printed with
      // different settings.
      && pf->pagesize!=superblock->pagesize)
  { 
    pf->pagesize=0;
  }


  pf=&fproc;
  memset(pf,0,sizeof(t_fproc));
  // Allocate block and recovery tables.
  pf->nblock=(superblock->datasize+NDATA-1)/NDATA;
  pf->datavalid=(uchar *)calloc(pf->nblock, sizeof(uchar*));
  pf->data=(uchar *)calloc(pf->nblock*NDATA, sizeof(uchar*));
  if (pf->datavalid==NULL || pf->data==NULL) {
    if (pf->datavalid!=NULL) free(pf->datavalid);
    if (pf->data!=NULL) free(pf->data);
    Reporterror("Low memory");
    return -1; };
  // Initialize remaining fields.
  memcpy(pf->name,superblock->name,64);
  pf->modified=superblock->modified;
  pf->attributes=superblock->attributes;
  pf->filecrc=superblock->filecrc;
  pf->datasize=superblock->datasize;
  pf->pagesize=superblock->pagesize;
  pf->origsize=superblock->origsize;
  pf->mode=superblock->mode;
  if (pf->pagesize>0)
    pf->npages=(pf->datasize+pf->pagesize-1)/pf->pagesize;
  else
    pf->npages=0;
  pf->ndata=0;
  for (int i=0; i<pf->npages && i<8; i++)
    pf->rempages[i]=i+1;
  // Initialize statistics and declare descriptor as busy.
  pf->goodblocks=0;
  pf->badblocks=0;
  pf->restoredbytes=0;
  pf->recoveredblocks=0;
  pf->busy=1;

  // Invalidate page limits and report success.
  pf=&fproc;
  pf->page=superblock->page;
  pf->ngroup=superblock->ngroup;
  pf->minpageaddr=0xFFFFFFFF;
  pf->maxpageaddr=0;
  //Updatefileinfo(slot,pf); //GUI
  return 0;
};



// Adds block recognized by decoder to file described by file descriptor with
// specified index. Returns 0 on success and -1 on any error.
int Addblock(t_block *block) {
  int i,j;
  t_fproc *pf;
  pf=&fproc;
  if (pf->busy==0)
    return -1;                         // Index points to unused descriptor
  // Add block to descriptor.
  if (block->recsize==0) {
    // Ordinary data block.
    i=block->addr/NDATA;
    if ((ulong)(i*NDATA)!=block->addr)
      return -1;                       // Invalid data alignment
    if (i>=pf->nblock)
      return -1;                       // Data outside the data size
    if (pf->datavalid[i]!=1) {
      memcpy(pf->data+block->addr,block->data,NDATA);
      pf->datavalid[i]=1;              // Valid data
      pf->ndata++; };
    pf->minpageaddr=std::min(pf->minpageaddr,block->addr);
    pf->maxpageaddr=std::max(pf->maxpageaddr,block->addr+NDATA); }
  else {
    // Data recovery block. I write it to all free locations within the group.
    if (block->recsize!=(ulong)(pf->ngroup*NDATA))
      return -1;                       // Invalid recovery scope
    i=block->addr/block->recsize;
    if (i*block->recsize!=block->addr)
      return -1;                       // Invalid data alignment
    i=block->addr/NDATA;
    for (j=i; j<i+pf->ngroup; j++) {
      if (j>=pf->nblock)
        return -1;                     // Data outside the data size
      if (pf->datavalid[j]!=0) continue;
      memcpy(pf->data+j*NDATA,block->data,NDATA);
      pf->datavalid[j]=2; };           // Valid recovery data
    pf->minpageaddr=std::min(pf->minpageaddr,block->addr);
    pf->maxpageaddr=std::max(pf->maxpageaddr,block->addr+block->recsize);
  };
  // Report success.
  return 0;
};



// Processes gathered data. Returns -1 on error, 0 if file is complete and
// number of pages to scan if there is still missing data. In the last case,
// fills list of several first remaining pages in file descriptor.
int Finishpage(int ngood,int nbad,ulong nrestored) {
  int i,j,r,rmin,rmax,nrec,irec,firstblock,nrempages;
  uchar *pr,*pd;
  t_fproc *pf;
  pf=&fproc;
  if (pf->busy==0)
    return -1;                         // Index points to unused descriptor
  // Update statistics. Note that it grows also when the same page is scanned
  // repeatedly.
  pf->goodblocks+=ngood;
  pf->badblocks+=nbad;
  pf->restoredbytes+=nrestored;
  // Restore bad blocks if corresponding recovery blocks are available (max. 1
  // per group).
  if (pf->ngroup>0) {
    rmin=(pf->minpageaddr/(NDATA*pf->ngroup))*pf->ngroup;
    rmax=(pf->maxpageaddr/(NDATA*pf->ngroup))*pf->ngroup;
    // Walk groups of data on current page, one by one.
    for (r=rmin; r<=rmax; r+=pf->ngroup) {
      if (r+pf->ngroup>pf->nblock)
        break;                         // Inconsistent data
      // Count blocks with recovery data in the group.
      nrec=0;
      for (i=r; i<r+pf->ngroup; i++) {
        if (pf->datavalid[i]==2) {
          nrec++; irec=i;
          pf->datavalid[i]=0;          // Prepare for next round
        };
      };
      if (nrec==1) {
        // Exactly one block in group is missing, recovery is possible.
        pr=pf->data+irec*NDATA;
        // Invert recovery data.
        for (j=0; j<NDATA; j++) *pr++^=0xFF;
        // XOR recovery data with good data blocks.
        for (i=r; i<r+pf->ngroup; i++) {
          if (i==irec) continue;
          pr=pf->data+irec*NDATA;
          pd=pf->data+i*NDATA;
          for (j=0; j<NDATA; j++) {
            *pr++^=*pd++;
          };
        };
        pf->datavalid[irec]=1;
        pf->recoveredblocks++;
        pf->ndata++;
      };
    };
  };
  // Check whether there are still bad blocks on the page.
  firstblock=(pf->page-1)*(pf->pagesize/NDATA);
  for (j=firstblock; j<firstblock+pf->pagesize/NDATA && j<pf->nblock; j++) {
    if (pf->datavalid[j]!=1) break; };
  if (j<firstblock+pf->pagesize/NDATA && j<pf->nblock)
    Message("Unrecoverable errors on page, please scan it again",0);
  else if (nbad>0)
    Message("Page processed, all bad blocks successfully restored",0);
  else
    Message("Page processed",0);
  // Calculate list of (partially) incomplete pages.
  nrempages=0;
  if (pf->pagesize>0) {
    for (i=0; i<pf->npages && nrempages<8; i++) {
      firstblock=i*(pf->pagesize/NDATA);
      for (j=firstblock; j<firstblock+pf->pagesize/NDATA && j<pf->nblock; j++) {
        if (pf->datavalid[j]==1)
          continue;
        // Page incomplete.
        pf->rempages[nrempages++]=i+1;
        break;
      };
    };
  };
  if (nrempages<8)
    pf->rempages[nrempages]=0;
  //Updatefileinfo(slot,pf); //GUI
  /*if (pf->ndata==pf->nblock) {
    if (autosave==0)
      Message("File restored. Press \"Save\" to save it to disk",0);
    else {
      Message("File complete",0);
      Saverestoredfile(0);
    };
  };*/
  if (pf->ndata != pf->nblock) {
      Message("File complete",0);
      Saverestoredfile(0);
  };

  return 0; ////////////////////////////////////////////////////////////////////
};



// Saves file with specified index and closes file descriptor (if force is 1,
// attempts to save data even if file is not yet complete). Returns 0 on
// success and -1 on error.
int Saverestoredfile(int force) {
  int n,success;
  ushort filecrc;
  ulong length;
  uchar *bufout,*data,*tempdata;
  t_fproc *pf;
  #ifdef _WIN32
  HANDLE hfile;
  uint l;
  #elif __linux__
  std::string hfile;
  ulong l;
  #endif
  pf=&fproc;
  if (pf->busy==0 || pf->nblock==0)
    return -1;                         // Index points to unused descriptor
  if (pf->ndata!=pf->nblock && force==0)
    return -1;                         // Still incomplete data
  Message("",0);
  //aes_context ctx;
  // If data is encrypted, decrypt it to temporary buffer. Decryption in place
  // is possible, but the whole data would be lost if password is incorrect.
  /*if (pf->mode & PBM_ENCRYPTED) {
    if (pf->datasize & 0x0000000F) {
    Reporterror("Encrypted data is not aligned");
    return -1; };
    if (Getpassword()!=0)
    return -1;                       // User cancelled decryption
    tempdata=(uchar *)malloc(pf->datasize);
    if (tempdata==NULL) {
    Reporterror("Low memory, can't decrypt data");
    return -1; };
    n=strlen(password);
    while (n<PASSLEN) password[n++]=0;
    memset(&ctx,0,sizeof(ctx));
    aes_set_key(&ctx,(uchar *)password,256);
    for (l=0; l<pf->datasize; l+=16)
    aes_decrypt(&ctx,pf->data+l,tempdata+l);
    filecrc=Crc16(tempdata,pf->datasize);
    if (filecrc!=pf->filecrc) {
    Reporterror("Invalid password, please try again");
    free(tempdata);
    return -1; }
    else {
    free(pf->data);
    pf->data=tempdata;
    pf->mode&=~PBM_ENCRYPTED;
    };
    }; */
  // If data is compressed, unpack it to temporary buffer.
  //if ((pf->mode & PBM_COMPRESSED)==0) {
  // Data is not compressed.
  data=pf->data; length=pf->origsize;
  bufout=NULL; 
  //}
  /*else {
  // Data is compressed. Create temporary buffer.
  if (pf->origsize==0)
  pf->origsize=pf->datasize*4;     // Weak attempt to recover
  bufout=(uchar *)malloc(pf->origsize);
  if (bufout==NULL) {
  Reporterror("Low memory");
  return -1; };
  // Unpack data.
  length=pf->origsize;
  success=BZ2_bzBuffToBuffDecompress((char *)bufout,(uint *)&length,
  pf->data,pf->datasize,0,0);
  if (success!=BZ_OK) {
  free(bufout);
  Reporterror("Unable to unpack data");
  return -1; };
  data=bufout; };
  // Ask user for file name.
  if (Selectoutfile(pf->name)!=0) {    // Cancelled by user
  if (bufout!=NULL) free(bufout);
  return -1; };
  */

  //!!! Need means of checking that output file name is valid

#ifdef _WIN32
  // Open file and save data.
  hfile=CreateFile(outfile,GENERIC_WRITE,0,NULL,
      CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
  if (hfile==INVALID_HANDLE_VALUE) {
    if (bufout!=NULL) GlobalFree((HGLOBAL)bufout);
    Reporterror("Unable to create file");
    return -1; };
  WriteFile(hfile,data,length,&l,NULL);
  // Restore old modification date and time.
  SetFileTime(hfile,&pf->modified,&pf->modified,&pf->modified);
  // Close file and restore old basic attributes.
  CloseHandle(hfile);

#elif __linux
  // Open file and save data.
  FILE * pFile;
  pFile = fopen(hfile.c_str(), "wb");

  if( pFile == NULL ) {
    if (bufout!=NULL) { 
      free(bufout);
    }
    Reporterror("Unable to create file");
    return -1;
  }

  int dataSize = 1;
  l = fwrite(data, dataSize, length, pFile);

  // Restore old modification date and time.
  struct stat fileAttributes;
  if( stat(hfile.c_str(), &fileAttributes) != 0 ) {
    Reporterror("Stat failed on restored data file");
    return -1;
  }
  pf->modified = fileAttributes.st_mtime;

  // Close file and restore old basic attributes.
  fclose(pFile);
#endif

  //!!! is it necessary to save file attributes?
  //SetFileAttributes(outfile,pf->attributes);

  if (bufout!=NULL) free(bufout);
  if (l!=length) {
    Reporterror("I/O error");
    return -1; };
  // Close file descriptor and report success.
  Closefproc();
  Message("File saved",0);
  return 0;
};


