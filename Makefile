EX=paperback-cli
SDIR=src
BZDIR=lib/BZLIB
AESDIR=lib/AES
CC=g++
CFLAGS=-Wall -std=c++11 -I"include" -I"lib/PortLibC/include" -I"lib/cxxopts/include" -I"lib/AES" -I"lib/BZLIB"

all: main

main: $(SDIR)/main.cpp $(SDIR)/Printer.cpp $(SDIR)/Scanner.cpp $(SDIR)/Fileproc.cpp $(SDIR)/Decoder.cpp $(SDIR)/Fileproc.cpp $(SDIR)/Crc16.cpp $(SDIR)/Ecc.cpp $(BZDIR)/bz_lib.cpp $(BZDIR)/bz_blocksort.cpp $(BZDIR)/bz_compress.cpp $(BZDIR)/bz_crctable.cpp $(BZDIR)/bz_decompress.cpp $(BZDIR)/bz_huffman.cpp $(BZDIR)/bz_randtable.cpp $(AESDIR)/ae_aes.cpp
	$(CC) $^ $(CFLAGS) -o $(EX)


clean:
	rm $(EX) *.o *.log 

