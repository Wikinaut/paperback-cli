EX=paperback-cli
SDIR=./src
CC=g++
CFLAGS=-std=c++11 -I"$(SDIR)" -I"$(SDIR)/PortLibCpp/src" -I"$(SDIR)/cxxopts/include" -I"$(SDIR)/BZLIB/" -I"$(SDIR)/AES" -g

all: main

main: $(SDIR)/main.cpp $(SDIR)/Decoder.cpp $(SDIR)/Printer.cpp $(SDIR)/Fileproc.cpp
	$(CC) $^ $(CFLAGS) -o $(EX)

msys: $(SDIR)/main.cpp $(SDIR)/Decoder.cpp $(SDIR)/Printer.cpp $(SDIR)/Fileproc.cpp
	$(CC) $^ $(CFLAGS) -o $(EX) -mwin32


clean:
	rm $(EX) #*.o *.log 

