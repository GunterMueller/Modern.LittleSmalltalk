#
# Makefile for MLS system
#

BUILD = ../build
CLS = ../cls

CC = gcc
CFLAGS = -Wall -g -I./getline
LDFLAGS = -g -L./getline
LDLIBS = -lgetline -lm

SRCS = utils.c machine.c prims.c objects.c memory.c \
       compiler.c check.c code.c tree.c parser.tab.c lex.yy.c
OBJS = $(patsubst %.c,%.o,$(SRCS))

ifeq ($(UI), tty)
  UI_SRC = ui-tty/ttyprim.c
  UI_MLS = $(CLS)/tty/tty.mls
endif
ifeq ($(UI), win)
  UI_SRC = ui-win/winprim.c
  UI_MLS = $(CLS)/win/graphics.mls $(CLS)/win/stdwin.mls
endif
UI_OBJ = $(patsubst %.c,%.o,$(UI_SRC))

BASIC_MLS = $(CLS)/std/basic.mls $(CLS)/std/mag.mls $(CLS)/std/collect.mls \
            $(CLS)/std/file.mls $(CLS)/std/mult.mls

IMAGE = mls.img

MKIMAGE = mkimage
MKIMAGE_SRC = mkimage.c
MKIMAGE_OBJ = $(patsubst %.c,%.o,$(MKIMAGE_SRC))

MLS = mls
MLS_SRC = mls.c filein.c
MLS_OBJ = $(patsubst %.c,%.o,$(MLS_SRC))

.PHONY:		all install tests clean

all:		$(MKIMAGE) $(MLS)

install:	$(MKIMAGE) $(MLS)
		./$(MKIMAGE) --image $(IMAGE) $(BASIC_MLS) $(UI_MLS)
		mkdir -p $(BUILD)
		cp $(MLS) $(BUILD)
		cp $(IMAGE) $(BUILD)

tests:		$(MKIMAGE)
		@for i in Tests/test???.mls ; do \
		  ./$(MKIMAGE) --image $(IMAGE) $$i ; \
		done

getline/libgetline.a:
		$(MAKE) -C getline

$(MKIMAGE):	$(OBJS) $(MKIMAGE_OBJ) $(UI_OBJ) getline/libgetline.a
		$(CC) $(LDFLAGS) -o $(MKIMAGE) $(OBJS) $(UI_OBJ) \
		  $(MKIMAGE_OBJ) $(LDLIBS)

$(MLS):		$(OBJS) $(MLS_OBJ) $(UI_OBJ) getline/libgetline.a
		$(CC) $(LDFLAGS) -o $(MLS) $(OBJS) $(UI_OBJ) \
		  $(MLS_OBJ) $(LDLIBS)

parser.tab.c:	parser.y
		bison -d parser.y

lex.yy.c:	scanner.l
		flex scanner.l

lex.yy.o:	lex.yy.c
		$(CC) $(CFLAGS) -Wno-unused-function -o lex.yy.o -c lex.yy.c

%.o:		%.c
		$(CC) $(CFLAGS) -o $@ -c $<

depend.mak:	$(SRCS) $(UI_SRC) $(MKIMAGE_SRC) $(MLS_SRC)
		$(CC) -MM -MG $(CFLAGS) $(SRCS) $(UI_SRC) \
		  $(MKIMAGE_SRC) $(MLS_SRC) > depend.mak

-include depend.mak

clean:
		$(MAKE) -C getline clean
		rm -f *~ *.o $(MKIMAGE) $(MLS)
		rm -f parser.tab.c parser.tab.h lex.yy.c
		rm -f ui-tty/*~ ui-tty/*.o
		rm -f ui-win/*~ ui-win/*.o
		rm -f depend.mak $(IMAGE)
