#
# Makefile for Little Smalltalk
#

VERSION = 3.10

DIRS = doc system classes
BUILD = `pwd`/build

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
		 tar -cvf small-$(VERSION).tar small-$(VERSION)/* ; \
		 gzip -f small-$(VERSION).tar)
