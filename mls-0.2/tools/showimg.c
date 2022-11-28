/*
 * showimg.c -- show contents of an MLS image file
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../sys/common.h"
#include "../sys/machine.h"
#include "../sys/objects.h"
#include "../sys/memory.h"
#include "../sys/ui.h"


#define MAX_OBJECTS	10000


Machine machine;


static struct {
  ObjPtr where;
  ObjPtr class;
  int hash;
  Bool hasPtrs;
  Bool hasWords;
  Bool hasBytes;
  int size;
} objTbl[MAX_OBJECTS];

static int numObjects;


static int find(ObjPtr obj) {
  int i;

  for (i = 0; i < numObjects; i++) {
    if (objTbl[i].where == obj) {
      return i;
    }
  }
  sysError("object at addr 0x%08X not found in table", obj);
  /* never reached */
  return -1;
}


static void fillObjectTable(void) {
  ObjPtr addr;
  int size;

  numObjects = 0;
  addr = 0;
  while (addr != machine.memorySize) {
    if (numObjects == MAX_OBJECTS) {
      sysError("too many objects in image");
    }
    objTbl[numObjects].where = addr;
    objTbl[numObjects].class = getClass(addr);
    objTbl[numObjects].hash = getHash(addr);
    objTbl[numObjects].hasPtrs = hasPtrs(addr);
    objTbl[numObjects].hasWords = hasWords(addr);
    objTbl[numObjects].hasBytes = hasBytes(addr);
    if (objTbl[numObjects].hasPtrs) {
      if (objTbl[numObjects].hasWords) {
        sysError("object at addr 0x%08lX has pointers as well as words",
                 addr);
      }
      if (objTbl[numObjects].hasBytes) {
        sysError("object at addr 0x%08lX has pointers as well as bytes",
                 addr);
      }
    } else
    if (objTbl[numObjects].hasWords) {
      if (objTbl[numObjects].hasBytes) {
        sysError("object at addr 0x%08lX has words as well as bytes",
                 addr);
      }
    } else
    if (!objTbl[numObjects].hasBytes) {
      sysError("object at addr 0x%08lX has no pointers, words, or bytes",
               addr);
    }
    size = getSize(addr);
    objTbl[numObjects].size = size;
    addr += sizeof(ObjPtr) + sizeof(Word) + sizeof(Word);
    if (objTbl[numObjects].hasPtrs) {
      addr += size * sizeof(ObjPtr);
    } else {
      if (objTbl[numObjects].hasWords) {
        addr += size * sizeof(Word);
      } else {
        addr += size * sizeof(Byte);
      }
    }
    while (addr & ALIGN_MASK) {
      addr++;
    }
    numObjects++;
  }
}


void showString(ObjPtr stringObj) {
  int size, i;
  Byte c;

  size = getSize(stringObj);
  for (i = 0; i < size; i++) {
    c = getByte(stringObj, i);
    if (c >= 0x20 && c <= 0x7E) {
      printf("%c", c);
    } else {
      printf(".");
    }
  }
}


static char *article(char firstCharOfNoun) {
  if (firstCharOfNoun == 'A' || firstCharOfNoun == 'a' ||
      firstCharOfNoun == 'E' || firstCharOfNoun == 'e' ||
      firstCharOfNoun == 'I' || firstCharOfNoun == 'i' ||
      firstCharOfNoun == 'O' || firstCharOfNoun == 'o' ||
      firstCharOfNoun == 'U' || firstCharOfNoun == 'u') {
    return "an";
  } else {
    return "a";
  }
}


static void showClassName(ObjPtr class) {
  ObjPtr name;

  name = getPtr(class, NAME_IN_CLASS);
  showString(name);
  if (getClass(class) == machine.Metaclass) {
    printf(" class");
  }
}


static void showClassInfo(ObjPtr class) {
  ObjPtr name;

  name = getPtr(class, NAME_IN_CLASS);
  printf("[%s ", article(getByte(name, 0)));
  showClassName(class);
  printf("]");
}


static void showRegisterCoincidence(ObjPtr obj) {
  int i;

  if (obj == machine.nil) {
    printf("(nil)");
  } else
  if (obj == machine.false) {
    printf("(false)");
  } else
  if (obj == machine.true) {
    printf("(true)");
  } else
  if (obj == machine.Smalltalk) {
    printf("(Smalltalk)");
  } else
  if (obj == machine.ShortInteger) {
    printf("(ShortInteger)");
  } else
  if (obj == machine.Float) {
    printf("(Float)");
  } else
  if (obj == machine.Character) {
    printf("(Character)");
  } else
  if (obj == machine.String) {
    printf("(String)");
  } else
  if (obj == machine.Symbol) {
    printf("(Symbol)");
  } else
  if (obj == machine.Link) {
    printf("(Link)");
  } else
  if (obj == machine.Method) {
    printf("(Method)");
  } else
  if (obj == machine.Array) {
    printf("(Array)");
  } else
  if (obj == machine.WordArray) {
    printf("(WordArray)");
  } else
  if (obj == machine.MethodContext) {
    printf("(MethodContext)");
  } else
  if (obj == machine.BlockContext) {
    printf("(BlockContext)");
  } else
  if (obj == machine.Metaclass) {
    printf("(Metaclass)");
  } else {
    for (i = 0; i < 256; i++) {
      if (obj == machine.character[i]) {
        if (i >= 0x20 && i <= 0x7E) {
          printf("(%c)", i);
        } else {
          printf("(.)");
        }
        break;
      }
    }
  }
}


static void showBrief(ObjPtr obj) {
  ObjPtr class;

  class = getClass(obj);
  if (class == machine.ShortInteger) {
    printf("integer %d", getShortInteger(obj));
  } else {
    printf("object # %d ", find(obj));
    showClassInfo(class);
    printf(" ");
    showRegisterCoincidence(obj);
  }
}


static void showObject(int n) {
  int i;
  ObjPtr p;
  Word w;
  Byte b;

  printf("# %4d: addr = 0x%08lX ", n, objTbl[n].where);
  showClassInfo(objTbl[n].class);
  printf(" ");
  showRegisterCoincidence(objTbl[n].where);
  printf("\n");
  printf("        class = ");
  showBrief(objTbl[n].class);
  printf("\n");
  printf("        hash = %d\n", objTbl[n].hash);
  printf("        size = %d ", objTbl[n].size);
  if (objTbl[n].hasPtrs) {
    printf("pointers\n");
    for (i = 0; i < objTbl[n].size; i++) {
      p = getPtr(objTbl[n].where, i);
      printf("        %4d: ", i);
      showBrief(p);
      printf("\n");
    }
  } else
  if (objTbl[n].hasWords) {
    printf("words\n");
    for (i = 0; i < objTbl[n].size; i++) {
      w = getWord(objTbl[n].where, i);
      printf("        %4d: ", i);
      printf("0x%08X", w);
      printf("\n");
    }
  } else
  if (objTbl[n].hasBytes) {
    printf("bytes\n");
    for (i = 0; i < objTbl[n].size; i++) {
      b = getByte(objTbl[n].where, i);
      printf("        %4d: ", i);
      printf("0x%02X", b);
      if (b >= 0x20 && b <= 0x7E) {
        printf(" (%c)", b);
      } else {
        printf(" (.)");
      }
      printf("\n");
    }
  }
}


static void showObjects(void) {
  int i;

  for (i = 0; i < numObjects; i++) {
    showObject(i);
  }
}


static void showRegisters(void) {
  int i;

  printf("machine.nil                  = ");
  showBrief(machine.nil);
  printf("\n");

  printf("machine.false                = ");
  showBrief(machine.false);
  printf("\n");

  printf("machine.true                 = ");
  showBrief(machine.true);
  printf("\n");

  for (i = 0; i < 256; i++) {
    printf("machine.character[%3d]       = ", i);
    showBrief(machine.character[i]);
    printf("\n");
  }

  printf("machine.Smalltalk            = ");
  showBrief(machine.Smalltalk);
  printf("\n");

  printf("machine.ShortInteger         = ");
  showBrief(machine.ShortInteger);
  printf("\n");

  printf("machine.Float                = ");
  showBrief(machine.Float);
  printf("\n");

  printf("machine.Character            = ");
  showBrief(machine.Character);
  printf("\n");

  printf("machine.String               = ");
  showBrief(machine.String);
  printf("\n");

  printf("machine.Symbol               = ");
  showBrief(machine.Symbol);
  printf("\n");

  printf("machine.Link                 = ");
  showBrief(machine.Link);
  printf("\n");

  printf("machine.Method               = ");
  showBrief(machine.Method);
  printf("\n");

  printf("machine.Array                = ");
  showBrief(machine.Array);
  printf("\n");

  printf("machine.WordArray            = ");
  showBrief(machine.WordArray);
  printf("\n");

  printf("machine.MethodContext        = ");
  showBrief(machine.MethodContext);
  printf("\n");

  printf("machine.BlockContext         = ");
  showBrief(machine.BlockContext);
  printf("\n");

  printf("machine.Metaclass            = ");
  showBrief(machine.Metaclass);
  printf("\n");

  printf("machine.currentActiveContext = ");
  showBrief(machine.currentActiveContext);
  printf("\n");

  printf("machine.currentHomeContext   = ");
  showBrief(machine.currentHomeContext);
  printf("\n");

  printf("machine.currentMethod        = ");
  showBrief(machine.currentMethod);
  printf("\n");

  printf("machine.currentReceiver      = ");
  showBrief(machine.currentReceiver);
  printf("\n");

  printf("machine.currentArgs          = ");
  showBrief(machine.currentArgs);
  printf("\n");

  printf("machine.currentTemps         = ");
  showBrief(machine.currentTemps);
  printf("\n");

  printf("machine.currentStack         = ");
  showBrief(machine.currentStack);
  printf("\n");

  printf("machine.currentCode          = ");
  showBrief(machine.currentCode);
  printf("\n");

  printf("machine.currentLiterals      = ");
  showBrief(machine.currentLiterals);
  printf("\n");

  printf("machine.newMethod            = ");
  showBrief(machine.newMethod);
  printf("\n");

  printf("machine.newContext           = ");
  showBrief(machine.newContext);
  printf("\n");

  printf("machine.ip                   = %d\n", machine.ip);
  printf("machine.sp                   = %d\n", machine.sp);
}


int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <image file>\n", argv[0]);
    exit(1);
  }
  initMemory(argv[1]);
  fillObjectTable();
  showObjects();
  printf("\n");
  showRegisters();
  return 0;
}
