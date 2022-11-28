#
# Makefile for Modern Little Smalltalk
#

VERSION = 0.2

DIRS = doc cls sys tools
BUILD = ./build

.PHONY:		help tty win all clean dist

help:
		@echo "***********************************"
		@echo "* use 'make tty' or 'make win'    *"
		@echo "* to make the command-line or     *"
		@echo "* windowing version, respectively *"
		@echo "***********************************"

tty:
		$(MAKE) all UI=tty

win:
		$(MAKE) all UI=win

all:
		for i in $(DIRS) ; do $(MAKE) -C $$i install ; done

clean:
		for i in $(DIRS) ; do $(MAKE) -C $$i clean ; done
		rm -f *~
		rm -rf $(BUILD)

dist:		clean
		(cd .. ; \
		 tar -cvf mls-$(VERSION).tar mls-$(VERSION)/* ; \
		 gzip -f mls-$(VERSION).tar)
