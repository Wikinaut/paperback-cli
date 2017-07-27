/*
 * =====================================================================================
 *
 *       Filename:  Ecc.h
 *
 *    Description:  
 *
 *        Version:  1.2
 *        Created:  07/26/2017 08:42:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Oleh Yuschuk
 *    Modified By:  scuti@teknik.io
 *
 * =====================================================================================
 */

#ifndef ECC_H
#define ECC_H

void   Encode8(unsigned char *data, unsigned char *parity, int pad);
int    Decode8(unsigned char *data, int *eras_pos, int no_eras, int pad);

#endif //ECC_H

