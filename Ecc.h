
#ifndef ECC_H
#define ECC_H

void   Encode8(unsigned char *data, unsigned char *parity, int pad);

int    Decode8(unsigned char *data, int *eras_pos, int no_eras, int pad);

#endif