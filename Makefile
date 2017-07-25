EX=paperback-cli
CC=g++
CFLAGS=-std=c++11 

all: main

main: Compress.o Decode.o
	$(CC) $^ main.cpp $(CFLAGS) -o $(EX) 2>error8.log

Compress.o: Compress.cpp
	$(CC) -c $^ $(CFLAGS)

Decode.o: Decode.cpp
	$(CC) -c $^ $(CFLAGS)

Crc16.o: Crc16.cpp
	$(CC) -c $^ $(CFLAGS)

Ecc.o: Ecc.cpp
	$(CC) -c $^ $(CFLAGS)


clean:
	rm *.o *.log $(EX)

