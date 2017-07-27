/*
 * =====================================================================================
 *
 *       Filename:  Crc16.h
 *
 *    Description:  Cyclic redundancy checks for file integrity
 *
 *        Version:  1.2
 *        Created:  07/26/2017 08:42:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Oleh Yuschuk
 *    Modified By:  scuti@teknik.io
 *
 * =====================================================================================
 */

#ifndef CRC16_H
#define CRC16_H

unsigned short Crc16(unsigned char *data, int length);
#endif //CRC16_H

