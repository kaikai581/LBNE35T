#=========================================================
# Makefile
#=========================================================

CC	= g++
COPTS	= -Wall -g -fPIC -DLINUX -O2

INCLUDE	= -I../FTDI-D2XX/
LIBS	= -L../FTDI-D2XX/build/x86_64 -lftd2xx

ROOT_LIBS   = $(shell root-config --libs --glibs)
ROOT_CFLAGS = $(shell root-config --cflags)

#=========================================================
# Compile
#=========================================================

FreeRun:	src/FreeRun.o src/LBNEWare.o src/Device.o src/ConfigBoards.o
	$(CC) $(ROOT_CFLAGS) -lrt -o FreeRun src/LBNEWare.o src/Device.o src/ConfigBoards.o src/FreeRun.o $(LIBS) $(ROOT_LIBS)

ConfigTest:	src/ConfigTest.o src/LBNEWare.o src/Device.o src/ConfigBoards.o
	$(CC) $(LIBS) $(ROOT_CFLAGS) $(ROOT_LIBS) -lrt -o ConfigTest src/ConfigTest.o src/LBNEWare.o src/Device.o src/ConfigBoards.o

LEDRun:	src/LEDRun.o src/LBNEWare.o src/Device.o src/ConfigBoards.o
	$(CC) $(LIBS) $(ROOT_CFLAGS) $(ROOT_LIBS) -lrt -o LEDRun src/LEDRun.o src/LBNEWare.o src/Device.o src/ConfigBoards.o

%.o :	%.c
	$(CC) -c $(COPTS) $(ROOT_CFLAGS) $(INCLUDE) -o $@ $<

%.o :	%.cpp
	$(CC) -c $(COPTS) $(ROOT_CFLAGS) $(INCLUDE) -o $@ $<
