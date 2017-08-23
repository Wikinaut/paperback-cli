EX=paperback-cli
SDIR=./src
CC=g++
PORTDIR=$(SDIR)/PortLibCpp/src
CFLAGS=-std=c++11 -I"$(SDIR)" -I"$(PORTDIR)" -I"$(SDIR)/cxxopts/include" -I"$(SDIR)/AES" -I"$(SDIR)/BZLIB"

all: main

main: $(SDIR)/main.cpp $(SDIR)/Printer.cpp $(SDIR)/Scanner.cpp $(SDIR)/Fileproc.cpp $(SDIR)/Decoder.cpp $(SDIR)/Fileproc.cpp $(SDIR)/Crc16.cpp $(SDIR)/Ecc.cpp $(SDIR)/BZLIB/bz_lib.cpp $(SDIR)/BZLIB/bz_blocksort.cpp $(SDIR)/BZLIB/bz_compress.cpp $(SDIR)/BZLIB/bz_crctable.cpp $(SDIR)/BZLIB/bz_decompress.cpp $(SDIR)/BZLIB/bz_huffman.cpp $(SDIR)/BZLIB/bz_randtable.cpp $(SDIR)/AES/ae_aes.cpp $(PORTDIR)/FileAttributes.cpp #$(SDIR)/Decoder.cpp
	$(CC) $^ $(CFLAGS) -o $(EX)

msys: $(SDIR)/main.cpp $(SDIR)/Printer.cpp $(SDIR)/Fileproc.cpp $(SDIR)/Decoder.cpp 
	$(CC) $^ $(CFLAGS) -o $(EX) -mwin32


clean:
	rm $(EX) #*.o *.log 

