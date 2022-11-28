#
# Makefile for Little Smalltalk system
#

BUILD = ../build
CLS = ../classes

STDWIN_TOP = ../../stdwin-0.9.7

CC = gcc
CFLAGS = -Wall -g -DLINUX $(UI_FLG) $(UI_INC)
LDFLAGS = -g
LDLIBS = $(UI_LIB) -lm

SRCS = memory.c names.c news.c interp.c primitive.c \
       filein.c lex.c parser.c unixio.c
OBJS = $(patsubst %.c,%.o,$(SRCS))

ifeq ($(UI), tty)
  UI_FLG =
  UI_INC =
  UI_LIB =
  UI_SRC = ui-tty/ttyprim.c
endif
ifeq ($(UI), win)
  UI_FLG = -DSTDWIN
  UI_INC = -I$(STDWIN_TOP)/H
  UI_LIB = -L$(STDWIN_TOP) -lstdwin -ltermcap -L/usr/X11R6/lib -lX11
  UI_SRC = ui-win/winprim.c
endif
UI_OBJ = $(patsubst %.c,%.o,$(UI_SRC))

MKIMG = mkimg
ST = st
IMAGE = systemImage

BASIC_ST = $(CLS)/std/basic.st $(CLS)/std/mag.st $(CLS)/std/collect.st
UNIX_ST = $(CLS)/unix/file.st $(CLS)/unix/mult.st

ifeq ($(UI), tty)
  UI_ST = $(CLS)/tty/tty.st
endif
ifeq ($(UI), win)
  UI_ST = $(CLS)/win/graphics.st $(CLS)/win/stdwin.st
endif

.PHONY:		all install clean

all:		$(MKIMG) $(ST)

install:	$(MKIMG) $(ST)
		./$(MKIMG) $(BASIC_ST) $(UNIX_ST) $(UI_ST)
		mkdir -p $(BUILD)
		cp $(ST) $(BUILD)
		cp $(IMAGE) $(BUILD)

$(MKIMG):	$(OBJS) mkimg.o $(UI_OBJ)
		$(CC) $(LDFLAGS) -o $(MKIMG) $(OBJS) $(UI_OBJ) mkimg.o $(LDLIBS)

$(ST):		$(OBJS) st.o $(UI_OBJ)
		$(CC) $(LDFLAGS) -o $(ST) $(OBJS) $(UI_OBJ) st.o $(LDLIBS)

%.o:		%.c
		$(CC) $(CFLAGS) -o $@ -c $<

depend.mak:	$(SRCS) mkimg.c
		$(CC) -MM -MG $(CFLAGS) $(SRCS) $(UI_SRC) mkimg.c st.c > depend.mak

-include depend.mak

clean:
		rm -f *~ ui-tty/*~ ui-tty/*.o ui-win/*~ ui-win/*.o
		rm -f $(OBJS)
		rm -f mkimg.o $(MKIMG)
		rm -f st.o $(ST)
		rm -f systemImage depend.mak
