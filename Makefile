EX=paperback-cli
SDIR=./source
CC=g++
CFLAGS=-std=c++11 -I"$(SDIR)" -I"$(SDIR)/cxxopts/include" -g

all: main

main: $(SDIR)/main.cpp $(SDIR)/Decoder.cpp $(SDIR)/Printer.cpp $(SDIR)/Fileproc.cpp
	$(CC) $^ $(CFLAGS) -o $(EX)

msys: $(SDIR)/main.cpp $(SDIR)/Decoder.cpp $(SDIR)/Printer.cpp $(SDIR)/Fileproc.cpp
	$(CC) $^ $(CFLAGS) -o $(EX) -mwin32


clean:
	rm $(EX) #*.o *.log 

