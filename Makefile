EX=paperback-cli
SDIR=src
PDIR=lib/PortLibC
BZDIR=lib/bzip2
AESDIR=lib/minizip/aes
CC=gcc
LDFLAGS=#-lcrypto -lssl
CFLAGS=-Iinclude -Ilib/PortLibC/include -Ilib/cxxopts/include -I$(PDIR)/include -I$(BZDIR) -I$(AESDIR) #-std=c++11 -DUSE_SHA1 

all: main

main: $(SDIR)/main.c $(SDIR)/paperbak.c $(SDIR)/Printer.c $(SDIR)/Scanner.c $(SDIR)/Fileproc.c $(SDIR)/Decoder.c $(SDIR)/Fileproc.c $(SDIR)/Crc16.c $(SDIR)/Ecc.c $(PDIR)/src/FileAttributes.c $(PDIR)/src/Borland.c $(BZDIR)/bzlib.c $(BZDIR)/blocksort.c $(BZDIR)/compress.c $(BZDIR)/crctable.c $(BZDIR)/decompress.c $(BZDIR)/huffman.c $(BZDIR)/randtable.c $(AESDIR)/pwd2key.c $(AESDIR)/hmac.c $(AESDIR)/sha1.c $(AESDIR)/aescrypt.c $(AESDIR)/aeskey.c $(AESDIR)/aes_ni.c $(AESDIR)/aestab.c $(AESDIR)/fileenc.c $(AESDIR)/prng.c lib/aes_modes.c
	$(CC) $^ $(LDFLAGS) $(CFLAGS) -o $(EX)


clean:
	rm $(EX) *.o *.log 

