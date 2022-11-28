/*
 * memory.c -- object memory
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "utils.h"
#include "machine.h"
#include "objects.h"
#include "memory.h"
#include "ui.h"


/**************************************************************/

/* macros */


/* three flags are coded in the size field of every object */

#define BROKEN_HEART		WORD_MSB
#define HAS_POINTERS		WORD_NSB
#define HAS_WORDS		WORD_TSB


/* macros to read and write data in memory */

#define readByte(x)		(*(Byte *)(memory + (x)))
#define writeByte(x, y)		(*(Byte *)(memory + (x)) = (y))
#define readWord(x)		(*(Word *)(memory + (x)))
#define writeWord(x, y)		(*(Word *)(memory + (x)) = (y))
#define readObjPtr(x)		(*(ObjPtr *)(memory + (x)))
#define writeObjPtr(x, y)	(*(ObjPtr *)(memory + (x)) = (y))


/* macros to read and write class and size fields of objects */

#define readClass(o)	    readObjPtr(o)
#define writeClass(o, c)    writeObjPtr(o, c)
#define readHash(o)	    readWord((o) + sizeof(ObjPtr))
#define writeHash(o, s)	    writeWord((o) + sizeof(ObjPtr), s)
#define readSize(o)	    readWord((o) + sizeof(ObjPtr) + sizeof(Word))
#define writeSize(o, s)	    writeWord((o) + sizeof(ObjPtr) + sizeof(Word), s)


/**************************************************************/

/* global variables */


Bool debugMemory = false;	/* debug flag, give statistics if set */
Bool enableGC = false;		/* enables garbage collections if set */

static Byte *memory;		/* object memory where all objects live */

static Address toStart;		/* base of "to" semispace in memory */
static Address toEnd;		/* top of "to" semispace in memory */
static Address fromStart;	/* base of "from" semispace in memory */
static Address fromEnd;		/* top of "from" semispace in memory */
static Address toFree;		/* address of first free byte in memory,
				   is always located in "to" semispace */

static Word numBytes;		/* number of bytes allocated since last GC,
				   also number of bytes copied during GC */
static Word numObjects;		/* number of objects allocated since last GC,
				   also number of objects copied during GC */


/**************************************************************/

/* garbage collector */


static ObjPtr copyObject(ObjPtr object) {
  Word length;
  ObjPtr copy;
  Address body;

  /* REMARK: short integers are provably never copied */
  /* read size of object */
  length = readSize(object);
  /* compute length of object dependent on pointer and word flags */
  if (length & HAS_POINTERS) {
    length &= ~(HAS_POINTERS | HAS_WORDS);
    length *= sizeof(ObjPtr);
  } else {
    if (length & HAS_WORDS) {
      length &= ~HAS_WORDS;
      length *= sizeof(Word);
    } else {
      length *= sizeof(Byte);
    }
  }
  length += sizeof(ObjPtr) + sizeof(Word) + sizeof(Word);
  /* if not enough space, something goes terribly wrong */
  if (toFree + length > toEnd) {
    sysError("copyObject has no space");
  }
  /* update collection statistics */
  if (debugMemory) {
    numBytes += length;
    numObjects++;
  }
  /* copy the object to free memory */
  copy = (ObjPtr) toFree;
  body = (Address) object;
  while (length--) {
    writeByte(toFree, readByte(body));
    toFree++;
    body++;
  }
  /* satisfy alignment restrictions */
  while (toFree & ALIGN_MASK) {
    toFree++;
  }
  /* return new address */
  return copy;
}


static ObjPtr updatePointer(ObjPtr object) {
  Word size;
  ObjPtr copy;

  /* a relocated short integer is the short integer itself */
  if (object & IS_SHORTINT) {
    return object;
  }
  /* read size and check the broken-heart flag */
  size = readSize(object);
  if (size & BROKEN_HEART) {
    /* object has already been copied, forward pointer is in class slot */
    return readClass(object);
  } else {
    /* object has not been copied yet, so do this now */
    copy = copyObject(object);
    /* in the original object: set broken-heart flag and forward pointer */
    writeSize(object, size | BROKEN_HEART);
    writeClass(object, copy);
    /* return pointer to copied object */
    return copy;
  }
}


#define UPDATE(reg)	reg = updatePointer(reg)


static void doGC(void) {
  Address tmp;
  Address toScan;
  Word size;
  int i;

  /* don't do collections if GC is disabled */
  if (!enableGC) {
    return;
  }
  /* print allocation statistics and init collection statistics */
  if (debugMemory) {
    printf("GC: %u bytes in %u objects allocated since last collection\n",
           numBytes, numObjects);
    numBytes = 0;
    numObjects = 0;
  }
  /* flip semispaces */
  tmp = toStart;
  toStart = fromStart;
  fromStart = tmp;
  tmp = toEnd;
  toEnd = fromEnd;
  fromEnd = tmp;
  /* set-up free and scan pointers */
  toFree = toStart;
  toScan = toFree;
  /* first relocate the roots of the world, i.e. all object registers */
  UPDATE(machine.nil);
  UPDATE(machine.false);
  UPDATE(machine.true);
  for (i = 0; i < 256; i++) {
    UPDATE(machine.character[i]);
  }
  UPDATE(machine.Smalltalk);
  UPDATE(machine.ShortInteger);
  UPDATE(machine.Float);
  UPDATE(machine.Character);
  UPDATE(machine.String);
  UPDATE(machine.Symbol);
  UPDATE(machine.Link);
  UPDATE(machine.Method);
  UPDATE(machine.Array);
  UPDATE(machine.WordArray);
  UPDATE(machine.MethodContext);
  UPDATE(machine.BlockContext);
  UPDATE(machine.Metaclass);
  UPDATE(machine.currentActiveContext);
  UPDATE(machine.currentHomeContext);
  UPDATE(machine.currentMethod);
  UPDATE(machine.currentReceiver);
  UPDATE(machine.currentArgs);
  UPDATE(machine.currentTemps);
  UPDATE(machine.currentStack);
  UPDATE(machine.currentCode);
  UPDATE(machine.currentLiterals);
  UPDATE(machine.newMethod);
  UPDATE(machine.newContext);
  UPDATE(machine.compilerMethod);
  UPDATE(machine.compilerLiteral);
  /* then relocate the rest of the world iteratively */
  while (toScan != toFree) {
    /* there is another object to scan */
    /* update class pointer of object */
    writeClass((ObjPtr) toScan, updatePointer(readClass((ObjPtr) toScan)));
    /* read size and let scan pointer point to object's body */
    size = readSize((ObjPtr) toScan);
    toScan += sizeof(ObjPtr) + sizeof(Word) + sizeof(Word);
    /* inspect pointer flag */
    if (size & HAS_POINTERS) {
      /* object has pointers, update them */
      size &= ~(HAS_POINTERS | HAS_WORDS);
      while (size--) {
        writeObjPtr(toScan, updatePointer(readObjPtr(toScan)));
        toScan += sizeof(ObjPtr);
      }
    } else {
      /* object has no pointers, skip it */
      if (size & HAS_WORDS) {
        size &= ~HAS_WORDS;
        toScan += size * sizeof(Word);
      } else {
        toScan += size * sizeof(Byte);
      }
    }
    /* satisfy alignment restrictions */
    while (toScan & ALIGN_MASK) {
      toScan++;
    }
  }
  /* print collection statistics and init allocation statistics */
  if (debugMemory) {
    printf("    %u bytes in %u objects copied during this collection\n",
           numBytes, numObjects);
    printf("    %lu of %lu bytes are now free\n",
           (Address) SEMI_SIZE * sizeof(Byte) - numBytes,
           (Address) SEMI_SIZE * sizeof(Byte));
    numBytes = 0;
    numObjects = 0;
  }
}


static void initGC(void) {
  /* "to" semispace initially starts at 0 */
  toStart = (Address) 0;
  toEnd = toStart + (Address) SEMI_SIZE * sizeof(Byte);
  /* "from" semispace starts where "to" semispace ends */
  fromStart = toEnd;
  fromEnd = fromStart + (Address) SEMI_SIZE * sizeof(Byte);
  /* first free byte depends on how much was loaded */
  toFree = toStart + (Address) machine.memorySize * sizeof(Byte);
  /* init allocation statistics */
  if (debugMemory) {
    numBytes = 0;
    numObjects = 0;
  }
}


static void exitGC(void) {
  /* do a collection to get objects compacted */
  doGC();
  /* check whether in lower semispace */
  if (toStart != (Address) 0) {
    /* it's the upper one, so collect again to switch semispaces */
    doGC();
  }
  /* compute total size of all objects in bytes */
  machine.memorySize = toFree - toStart;
}


/**************************************************************/

/* interface */


ObjPtr createObject(ObjPtr class, int size,
                    Bool hasPtrs, Bool hasWords) {
  static unsigned int fibHash = 314159265;
  Word length;
  ObjPtr object;
  int i;

  /* compute length of object in bytes */
  length = sizeof(ObjPtr) + sizeof(Word) + sizeof(Word);
  if (hasPtrs) {
    length += size * sizeof(ObjPtr);
  } else {
    if (hasWords) {
      length += size * sizeof(Word);
    } else {
      length += size * sizeof(Byte);
    }
  }
  /* check if remaining space is large enough */
  if (toFree + length > toEnd) {
    /* not large enough, do a collection */
    doGC();
    /* ATTENTION: don't forget to update the class pointer! Since */
    /* the class object must also be referenced elsewhere, we are */
    /* sure that no object is actually copied in the process. */
    class = updatePointer(class);
    /* check for object memory overflow */
    if (toFree + length > toEnd) {
      sysError("object memory exhausted");
    }
  }
  /* allocate the requested space */
  object = (ObjPtr) toFree;
  toFree += length;
  /* satisfy alignment restrictions */
  while (toFree & ALIGN_MASK) {
    toFree++;
  }
  /* update allocation statistics */
  if (debugMemory) {
    numBytes += length;
    numObjects++;
  }
  /* set class, hash and size; init fields */
  writeClass(object, class);
  writeHash(object, fibHash & ((1 << 30) - 1));
  fibHash += 0x9E3779B9;  /* Fibonacci hashing, see Knuth Vol. 3 */
  if (hasPtrs) {
    writeSize(object, size | HAS_POINTERS);
    /* ATTENTION: the following initialization is required! */
    for (i = 0; i < size; i++) {
      /* default object pointer value is nil */
      writeObjPtr(object + sizeof(ObjPtr) + sizeof(Word) +
                  sizeof(Word) + i * sizeof(ObjPtr), machine.nil);
    }
  } else {
    if (hasWords) {
      writeSize(object, size | HAS_WORDS);
      /* ATTENTION: the following initialization is optional! */
      for (i = 0; i < size; i++) {
        /* default word value is 0 */
        writeWord(object + sizeof(ObjPtr) + sizeof(Word) +
                  sizeof(Word) + i * sizeof(Word), 0);
      }
    } else {
      writeSize(object, size);
      /* ATTENTION: the following initialization is optional! */
      for (i = 0; i < size; i++) {
        /* default byte value is 0 */
        writeByte(object + sizeof(ObjPtr) + sizeof(Word) +
                  sizeof(Word) + i * sizeof(Byte), 0);
      }
    }
  }
  /* return the object created just now */
  return object;
}


ObjPtr getClass(ObjPtr object) {
  if (object & IS_SHORTINT) {
    return machine.ShortInteger;
  } else {
    return readClass(object);
  }
}


void setClass(ObjPtr object, ObjPtr class) {
  if (object & IS_SHORTINT) {
    sysError("setClass object is short integer");
  }
  writeClass(object, class);
}


int getHash(ObjPtr object) {
  if (object & IS_SHORTINT) {
    return object & ~IS_SHORTINT;
  } else {
    return readHash(object);
  }
}


void setHash(ObjPtr object, int hash) {
  if (object & IS_SHORTINT) {
    sysError("setHash object is short integer");
  }
  writeHash(object, hash);
}


int getSize(ObjPtr object) {
  if (object & IS_SHORTINT) {
    return 0;
  } else {
    return readSize(object) & ~(HAS_POINTERS | HAS_WORDS);
  }
}


Bool hasPtrs(ObjPtr object) {
  if (object & IS_SHORTINT) {
    return false;
  } else {
    return (readSize(object) & HAS_POINTERS) != 0;
  }
}


Bool hasWords(ObjPtr object) {
  if (object & IS_SHORTINT) {
    return false;
  } else {
    return (readSize(object) & HAS_WORDS) != 0;
  }
}


Bool hasBytes(ObjPtr object) {
  if (object & IS_SHORTINT) {
    return true;
  } else {
    return (readSize(object) & (HAS_POINTERS | HAS_WORDS)) == 0;
  }
}


void *body(ObjPtr object) {
  return memory + object + sizeof(ObjPtr) + sizeof(Word) + sizeof(Word);
}


ObjPtr getPtr(ObjPtr object, int index) {
  Word size;

  if (object & IS_SHORTINT) {
    sysError("getPtr object is short integer");
  }
  size = readSize(object);
  if ((size & HAS_POINTERS) == 0) {
    sysError("getPtr object has no pointers");
  }
  size &= ~HAS_POINTERS;
  if (index >= size) {
    sysError("getPtr index out of range");
  }
  return readObjPtr(object + sizeof(ObjPtr) + sizeof(Word) +
                    sizeof(Word) + index * sizeof(ObjPtr));
}


Word getWord(ObjPtr object, int index) {
  Word size;

  if (object & IS_SHORTINT) {
    sysError("getWord object is short integer");
  }
  size = readSize(object);
  if ((size & HAS_WORDS) == 0) {
    sysError("getWord object has no words");
  }
  size &= ~HAS_WORDS;
  if (index >= size) {
    sysError("getWord index out of range");
  }
  return readWord(object + sizeof(ObjPtr) + sizeof(Word) +
                  sizeof(Word) + index * sizeof(Word));
}


Byte getByte(ObjPtr object, int index) {
  Word size;

  if (object & IS_SHORTINT) {
    sysError("getByte object is short integer");
  }
  size = readSize(object);
  if ((size & HAS_POINTERS) || (size & HAS_WORDS)) {
    sysError("getByte object has no bytes");
  }
  if (index >= size) {
    sysError("getByte index out of range");
  }
  return readByte(object + sizeof(ObjPtr) + sizeof(Word) +
                  sizeof(Word) + index * sizeof(Byte));
}


void setPtr(ObjPtr object, int index, ObjPtr value) {
  Word size;

  if (object & IS_SHORTINT) {
    sysError("setPtr object is short integer");
  }
  size = readSize(object);
  if ((size & HAS_POINTERS) == 0) {
    sysError("setPtr object has no pointers");
  }
  size &= ~HAS_POINTERS;
  if (index >= size) {
    sysError("setPtr index out of range");
  }
  writeObjPtr(object + sizeof(ObjPtr) + sizeof(Word) +
              sizeof(Word) + index * sizeof(ObjPtr), value);
}


void setWord(ObjPtr object, int index, Word value) {
  Word size;

  if (object & IS_SHORTINT) {
    sysError("setWord object is short integer");
  }
  size = readSize(object);
  if ((size & HAS_WORDS) == 0) {
    sysError("setWord object has no words");
  }
  size &= ~HAS_WORDS;
  if (index >= size) {
    sysError("setWord index out of range");
  }
  writeWord(object + sizeof(ObjPtr) + sizeof(Word) +
            sizeof(Word) + index * sizeof(Word), value);
}


void setByte(ObjPtr object, int index, Byte value) {
  Word size;

  if (object & IS_SHORTINT) {
    sysError("setByte object is short integer");
  }
  size = readSize(object);
  if ((size & HAS_POINTERS) || (size & HAS_WORDS)) {
    sysError("setByte object has no bytes");
  }
  if (index >= size) {
    sysError("setByte index out of range");
  }
  writeByte(object + sizeof(ObjPtr) + sizeof(Word) +
            sizeof(Word) + index * sizeof(Byte), value);
}


void initMemory(char *imageFileName) {
  FILE *imageFile;

  /* allocate object memory */
  memory = allocate(MEMORY_SIZE * sizeof(Byte));
  /* open image file */
  imageFile = fopen(imageFileName, "rb");
  if (imageFile == NULL) {
    sysError("cannot open image file '%s' for read", imageFileName);
  }
  /* read machine state */
  if (fread(&machine, sizeof(Machine), 1, imageFile) != 1) {
    sysError("cannot read machine state from image file");
  }
  /* check image file signature */
  if (machine.signature_1 != SIGNATURE_1 ||
      machine.signature_2 != SIGNATURE_2) {
    sysError("file '%s' is not an image file", imageFileName);
  }
  /* check image file version number (major only, minor ignored) */
  if (machine.majorVersion != MAJOR_VNUM) {
    sysError("wrong image file version number");
  }
  /* load object memory */
  if (fread(memory, sizeof(Byte), machine.memorySize, imageFile) !=
      machine.memorySize) {
    sysError("cannot read objects from image file");
  }
  /* close image file */
  fclose(imageFile);
  /* init garbage collector */
  initGC();
}


void exitMemory(char *imageFileName) {
  FILE *imageFile;

  /* exit garbage collector */
  exitGC();
  /* open image file */
  imageFile = fopen(imageFileName, "wb");
  if (imageFile == NULL) {
    sysError("cannot open image file '%s' for write", imageFileName);
  }
  /* write machine state */
  if (fwrite(&machine, sizeof(Machine), 1, imageFile) != 1) {
    sysError("cannot write machine state to image file");
  }
  /* save object memory */
  if (fwrite(memory, sizeof(Byte), machine.memorySize, imageFile) !=
      machine.memorySize) {
    sysError("cannot write objects to image file");
  }
  /* close image file */
  fclose(imageFile);
  /* release object memory */
  release(memory);
}
