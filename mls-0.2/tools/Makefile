#
# Makefile for MLS tools
#

EXTOBJS = ../sys/utils.o ../sys/objects.o \
          ../sys/memory.o ../sys/ui-tty/ttyprim.o

GETLINE = -L../sys/getline -lgetline

all:		showimg

install:	showimg

showimg:	showimg.c
		gcc -Wall -g -o showimg showimg.c $(EXTOBJS) $(GETLINE)

clean:
		rm -f *~ showimg
