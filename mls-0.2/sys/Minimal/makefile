#
# Makefile for minimal class library
#

CLS = Object.mls UndefinedObject.mls Boolean.mls Magnitude.mls \
      Collection.mls Link.mls Behavior.mls Method.mls Context.mls \
      Compiler.mls

all:
	../mkimage --memory --filein --source --tree --vars --code $(CLS)

report:
	@grep '<!' *.mls

clean:
	rm -f *~ *.img *.lst
