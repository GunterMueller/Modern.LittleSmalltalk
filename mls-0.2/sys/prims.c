/*
 * prims.c -- the primitives
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "machine.h"
#include "prims.h"
#include "objects.h"
#include "memory.h"
#include "compiler.h"
#include "ui.h"


static void checkNumArgs(int required, int actual, int primNum) {
  if (actual != required) {
    sysError("primitive %d was called with %d argument(s) but needs %d",
             primNum, actual, required);
  }
}


static void illPrim(int numArgs, int primNum) {
  /* illegal primitive number, don't check arguments */
  sysError("illegal primitive %d called", primNum);
}


static void prim007(int numArgs, int primNum) {
  /* SystemDictionary >> shutDown */
  checkNumArgs(0, numArgs, primNum);
  runMachine = false;
  push(machine.true);
}


static void prim011(int numArgs, int primNum) {
  /* Object >> class */
  checkNumArgs(1, numArgs, primNum);
  push(getClass(pop()));
}


static void prim012(int numArgs, int primNum) {
  int size;

  /* Object >> size */
  checkNumArgs(1, numArgs, primNum);
  size = getSize(pop());
  push(newShortInteger(size));
}


static void prim013(int numArgs, int primNum) {
  int hash;

  /* Object >> hash */
  checkNumArgs(1, numArgs, primNum);
  hash = getHash(pop());
  push(newShortInteger(hash));
}


static void prim020(int numArgs, int primNum) {
  ObjPtr op1, op2;

  /* Object >> == */
  checkNumArgs(2, numArgs, primNum);
  op2 = pop();
  op1 = pop();
  push(op1 == op2 ? machine.true : machine.false);
}


static void prim021(int numArgs, int primNum) {
  ObjPtr class;
  int size;

  /* Behavior >> new, word object */
  checkNumArgs(2, numArgs, primNum);
  size = getShortInteger(pop());
  class = pop();
  push(createObject(class, size, false, true));
}


static void prim022(int numArgs, int primNum) {
  ObjPtr class;
  int size;

  /* Behavior >> new, byte object */
  checkNumArgs(2, numArgs, primNum);
  size = getShortInteger(pop());
  class = pop();
  push(createObject(class, size, false, false));
}


static void prim023(int numArgs, int primNum) {
  ObjPtr class;
  int size;

  /* Behavior >> new, pointer object */
  checkNumArgs(2, numArgs, primNum);
  size = getShortInteger(pop());
  class = pop();
  push(createObject(class, size, true, false));
}


static void prim024(int numArgs, int primNum) {
  ObjPtr string1, string2;
  int size1, size2, i, j;
  ObjPtr result;

  /* String >> , */
  checkNumArgs(2, numArgs, primNum);
  string2 = pop();
  size2 = getSize(string2);
  string1 = pop();
  size1 = getSize(string1);
  push(string2);
  push(string1);
  result = createObject(machine.String, size1 + size2, false, false);
  j = 0;
  string1 = pop();
  for (i = 0; i < size1; i++) {
    setByte(result, j, getByte(string1, i));
    j++;
  }
  string2 = pop();
  for (i = 0; i < size2; i++) {
    setByte(result, j, getByte(string2, i));
    j++;
  }
  push(result);
}


static void prim025(int numArgs, int primNum) {
  int index;
  ObjPtr object;

  /* WordArray >> at: */
  checkNumArgs(2, numArgs, primNum);
  index = getShortInteger(pop()) - 1;
  object = pop();
  push(newShortInteger(getWord(object, index)));
}


static void prim026(int numArgs, int primNum) {
  int index;
  ObjPtr object;

  /* ByteArray >> at: */
  checkNumArgs(2, numArgs, primNum);
  index = getShortInteger(pop()) - 1;
  object = pop();
  push(newShortInteger(getByte(object, index)));
}


static void prim027(int numArgs, int primNum) {
  int index;
  ObjPtr object;

  /* Object >> at: */
  checkNumArgs(2, numArgs, primNum);
  index = getShortInteger(pop()) - 1;
  object = pop();
  push(getPtr(object, index));
}


static void prim029(int numArgs, int primNum) {
  int index;
  ObjPtr object;

  /* String >> at: */
  checkNumArgs(2, numArgs, primNum);
  index = getShortInteger(pop()) - 1;
  object = pop();
  push(newCharacter(getByte(object, index)));
}


static void prim030(int numArgs, int primNum) {
  ObjPtr string, class, flag;
  Bool f;
  char s[METHOD_SIZE];
  int size, i;

  /* Compiler class >> compile:in:lastValueNeeded: */
  checkNumArgs(3, numArgs, primNum);
  flag = pop();
  if (flag == machine.false) {
    f = false;
  } else
  if (flag == machine.true) {
    f = true;
  } else {
    sysError("primitive 030 detected illegal flag value");
  }
  class= pop();
  string = pop();
  size = getSize(string);
  for (i = 0; i < size; i++) {
    s[i] = getByte(string, i);
  }
  s[i] = '\0';
  if (!compile(s, class, f)) {
    push(machine.nil);
  } else {
    push(machine.compilerMethod);
  }
}


static void prim035(int numArgs, int primNum) {
  int index;
  ObjPtr object;
  ObjPtr newObj;

  /* WordArray >> at:put: */
  checkNumArgs(3, numArgs, primNum);
  newObj = pop();
  index = getShortInteger(pop()) - 1;
  object = pop();
  setWord(object, index, getShortInteger(newObj));
  push(machine.true);
}


static void prim036(int numArgs, int primNum) {
  int index;
  ObjPtr object;
  ObjPtr newObj;

  /* ByteArray >> at:put: */
  checkNumArgs(3, numArgs, primNum);
  newObj = pop();
  index = getShortInteger(pop()) - 1;
  object = pop();
  setByte(object, index, getShortInteger(newObj));
  push(machine.true);
}


static void prim037(int numArgs, int primNum) {
  int index;
  ObjPtr object;
  ObjPtr newObj;

  /* Object >> at:put: */
  checkNumArgs(3, numArgs, primNum);
  newObj = pop();
  index = getShortInteger(pop()) - 1;
  object = pop();
  setPtr(object, index, newObj);
  push(machine.true);
}


static void prim039(int numArgs, int primNum) {
  int index;
  ObjPtr object;
  ObjPtr newObj;

  /* String >> at:put: */
  checkNumArgs(3, numArgs, primNum);
  newObj = pop();
  index = getShortInteger(pop()) - 1;
  object = pop();
  setByte(object, index, getCharacter(newObj));
  push(machine.true);
}


static void prim056(int numArgs, int primNum) {
  int value;

  /* Character >> asciiValue */
  checkNumArgs(1, numArgs, primNum);
  value = getCharacter(pop());
  push(newShortInteger(value));
}


static void prim057(int numArgs, int primNum) {
  char value;

  /* Character class >> withValue: */
  checkNumArgs(1, numArgs, primNum);
  value = getShortInteger(pop());
  push(newCharacter(value));
}


static void prim060(int numArgs, int primNum) {
  int op1, op2;

  /* ShortInteger >> + */
  checkNumArgs(2, numArgs, primNum);
  op2 = getShortInteger(pop());
  op1 = getShortInteger(pop());
  push(newShortInteger(op1 + op2));
}


static void prim061(int numArgs, int primNum) {
  int op1, op2;

  /* ShortInteger >> - */
  checkNumArgs(2, numArgs, primNum);
  op2 = getShortInteger(pop());
  op1 = getShortInteger(pop());
  push(newShortInteger(op1 - op2));
}


static void prim067(int numArgs, int primNum) {
  int op1, op2;

  /* ShortInteger >> \\ */
  checkNumArgs(2, numArgs, primNum);
  op2 = getShortInteger(pop());
  op1 = getShortInteger(pop());
  push(newShortInteger(op1 % op2));
}


static void prim068(int numArgs, int primNum) {
  int op1, op2;

  /* ShortInteger >> * */
  checkNumArgs(2, numArgs, primNum);
  op2 = getShortInteger(pop());
  op1 = getShortInteger(pop());
  push(newShortInteger(op1 * op2));
}


static void prim069(int numArgs, int primNum) {
  int op1, op2;

  /* ShortInteger >> // */
  checkNumArgs(2, numArgs, primNum);
  op2 = getShortInteger(pop());
  op1 = getShortInteger(pop());
  push(newShortInteger(op1 / op2));
}


static void prim090(int numArgs, int primNum) {
  ObjPtr blockContext;
  int numBlockArgs;
  ObjPtr blockStack;
  int i;

  /* BlockContext >> value */
  /* BlockContext >> value: */
  /* BlockContext >> value:value: */
  /* BlockContext >> value:value:value: */
  blockContext =
    getPtr(machine.currentStack, machine.sp - numArgs);
  numBlockArgs =
    getShortInteger(getPtr(blockContext, ARGCOUNT_IN_BLOCKCONTEXT));
  checkNumArgs(numBlockArgs, numArgs - 1, primNum);
  setPtr(blockContext,
         CALLER_IN_BLOCKCONTEXT,
         machine.currentActiveContext);
  setPtr(blockContext,
         IP_IN_BLOCKCONTEXT,
         getPtr(blockContext, IPSTART_IN_BLOCKCONTEXT));
  blockStack = getPtr(blockContext, STACK_IN_BLOCKCONTEXT);
  for (i = 0; i < numBlockArgs; i++) {
    setPtr(blockStack, i, pop());
  }
  setPtr(blockContext,
         SP_IN_BLOCKCONTEXT,
         newShortInteger(numBlockArgs));
  activateContext(pop());
}


static void prim130(int numArgs, int primNum) {
  char buffer[LINE_SIZE];

  /* String class >> input */
  checkNumArgs(0, numArgs, primNum);
  if (fgets(buffer, LINE_SIZE, stdin) == NULL) {
    sysError("terminated by EOF in stdin");
  }
  push(newString(buffer));
}


static void prim132(int numArgs, int primNum) {
  /* Object >> halt: */
  checkNumArgs(1, numArgs, primNum);
  printf("\n");
  showString(pop());
  printf("\n");
  debugMachine = true;
  push(machine.true);
}


static void prim133(int numArgs, int primNum) {
  ObjPtr string;
  int size, i;
  char c;

  /* String >> output */
  checkNumArgs(1, numArgs, primNum);
  string = pop();
  size = getSize(string);
  for (i = 0; i < size; i++) {
    c = getByte(string, i);
    fputc(c, stdout);
  }
  push(machine.true);
}


static void prim134(int numArgs, int primNum) {
  char c;

  /* Character >> output */
  checkNumArgs(1, numArgs, primNum);
  c = getCharacter(pop());
  fputc(c, stdout);
  push(machine.true);
}


static void prim251(int numArgs, int primNum) {
  int op1, op2;

  /* ShortInteger >> <= */
  checkNumArgs(2, numArgs, primNum);
  op2 = getShortInteger(pop());
  op1 = getShortInteger(pop());
  push(op1 < op2 ? machine.true : machine.false);
}


static void prim252(int numArgs, int primNum) {
  int op1, op2;

  /* ShortInteger >> <= */
  checkNumArgs(2, numArgs, primNum);
  op2 = getShortInteger(pop());
  op1 = getShortInteger(pop());
  push(op1 <= op2 ? machine.true : machine.false);
}


static void prim253(int numArgs, int primNum) {
  int op1, op2;

  /* ShortInteger >> bitAnd: */
  checkNumArgs(2, numArgs, primNum);
  op2 = getShortInteger(pop());
  op1 = getShortInteger(pop());
  push(newShortInteger(op1 & op2));
}


static void prim254(int numArgs, int primNum) {
  int op1, op2;

  /* ShortInteger >> bitOr: */
  checkNumArgs(2, numArgs, primNum);
  op2 = getShortInteger(pop());
  op1 = getShortInteger(pop());
  push(newShortInteger(op1 | op2));
}


static void prim255(int numArgs, int primNum) {
  int op1, op2;

  /* ShortInteger >> bitXor: */
  checkNumArgs(2, numArgs, primNum);
  op2 = getShortInteger(pop());
  op1 = getShortInteger(pop());
  push(newShortInteger(op1 ^ op2));
}


typedef void (*Prim)(int numArgs, int primNum);


static Prim primTbl[256] = {
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, prim007,
  illPrim, illPrim, illPrim, prim011, prim012, prim013, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, prim020, prim021, prim022, prim023,
  prim024, prim025, prim026, prim027, illPrim, prim029, prim030, illPrim,
  illPrim, illPrim, illPrim, prim035, prim036, prim037, illPrim, prim039,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  prim056, prim057, illPrim, illPrim, prim060, prim061, illPrim, illPrim,
  illPrim, illPrim, illPrim, prim067, prim068, prim069, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, prim090, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, prim130, illPrim, prim132, prim133, prim134, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim, illPrim,
  illPrim, illPrim, illPrim, prim251, prim252, prim253, prim254, prim255
};


void primitive(int numArgs, int primNum) {
  (*primTbl[primNum & 0xFF])(numArgs, primNum);
}
