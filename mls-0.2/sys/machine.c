/*
 * machine.c -- virtual machine
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "machine.h"
#include "prims.h"
#include "objects.h"
#include "memory.h"
#include "ui.h"

#include "getline.h"


/**************************************************************/

/* global variables */


Machine machine;		/* an instance of the virtual machine */
Bool debugMachine = false;	/* operate VM in debug mode if set */
Bool runMachine = true;		/* while true, run the machine */


/**************************************************************/

/* string output */


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


/**************************************************************/

/* object inspector */


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
    printf("object @ 0x%08lX ", obj);
    showClassInfo(class);
    printf(" ");
    showRegisterCoincidence(obj);
  }
}


static void showObject(char *description, ObjPtr object) {
  ObjPtr class;
  int hash;
  int size, i;
  ObjPtr p;
  Word w;
  Byte b;

  /* show description (if it is given) */
  if (description != NULL) {
    printf("%s:\n", description);
  }
  /* show object pointer and class */
  class = getClass(object);
  printf("  addr = 0x%08lX ", object);
  showClassInfo(class);
  printf(" ");
  showRegisterCoincidence(object);
  printf("\n");
  printf("  class = ");
  showBrief(class);
  printf("\n");
  /* show hash, size, type, and contents */
  hash = getHash(object);
  size = getSize(object);
  printf("  hash = %d\n", hash);
  printf("  size = %d ", size);
  if (hasPtrs(object)) {
    printf("pointers\n");
    for (i = 0; i < size; i++) {
      p = getPtr(object, i);
      printf("  %4d: ", i);
      showBrief(p);
      printf("\n");
    }
  } else
  if (hasWords(object)) {
    printf("words\n");
    for (i = 0; i < size; i++) {
      w = getWord(object, i);
      printf("  %4d: ", i);
      printf("0x%08X", w);
      printf("\n");
    }
  } else
  if (hasBytes(object)) {
    printf("bytes\n");
    for (i = 0; i < size; i++) {
      b = getByte(object, i);
      printf("  %4d: ", i);
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


/**************************************************************/

/* debugger */


static void showLiteral(ObjPtr literal) {
  ObjPtr class;
  Byte c;

  class = getClass(literal);
  if (class == machine.ShortInteger) {
    printf("%d", getShortInteger(literal));
  } else
  if (class == machine.Float) {
    printf("%e", getFloat(literal));
  } else
  if (class == machine.Character) {
    c = getCharacter(literal);
    if (c >= 0x20 && c <= 0x7E) {
      printf("$%c", c);
    } else {
      printf("$.");
    }
  } else
  if (class == machine.String) {
    printf("'");
    showString(literal);
    printf("'");
  } else
  if (class == machine.Symbol) {
    printf("#");
    showString(literal);
  } else
  if (class == machine.Link) {
    showLiteral(getPtr(literal, KEY_IN_LINK));
    printf(" --> ");
    showLiteral(getPtr(literal, VALUE_IN_LINK));
  } else {
    showBrief(literal);
  }
}


static void showInstruction(Word ip,
                            ObjPtr code,
                            ObjPtr literals) {
  Word instr;
  Word opcode, operand1, operand2;

  printf("%04X    ", ip);
  instr = getWord(code, ip);
  printf("%08X    ", instr);
  opcode = (instr >> 24) & 0xFF;
  operand1 = (instr >> 16) & 0xFF;
  operand2 = instr & 0xFFFF;
  switch (opcode) {
    case OP_NOP:
      /* no operation */
      printf("NOP         ");
      break;
    case OP_PUSHSELF:
      /* push receiver */
      printf("PUSHSELF    ");
      break;
    case OP_PUSHNIL:
      /* push nil */
      printf("PUSHNIL     ");
      break;
    case OP_PUSHFALSE:
      /* push false */
      printf("PUSHFALSE   ");
      break;
    case OP_PUSHTRUE:
      /* push true */
      printf("PUSHTRUE    ");
      break;
    case OP_DUP:
      /* duplicate top of stack */
      printf("DUP         ");
      break;
    case OP_DROP:
      /* drop top of stack */
      printf("DROP        ");
      break;
    case OP_RETMSG:
      /* return top of stack from message */
      printf("RETMSG      ");
      break;
    case OP_RETBLK:
      /* return top of stack from block */
      printf("RETBLK      ");
      break;
    case OP_PUSHCONST:
      /* push constant */
      printf("PUSHCONST   %u\t  ; ", operand2);
      showLiteral(getPtr(literals, operand2));
      break;
    case OP_PUSHGLOB:
      /* push global */
      printf("PUSHGLOB    %u\t  ; ", operand2);
      showLiteral(getPtr(literals, operand2));
      break;
    case OP_STOREGLOB:
      /* store global */
      printf("STOREGLOB   %u\t  ; ", operand2);
      showLiteral(getPtr(literals, operand2));
      break;
    case OP_PUSHINST:
      /* push instance */
      printf("PUSHINST    %u", operand1);
      break;
    case OP_STOREINST:
      /* store instance */
      printf("STOREINST   %u", operand1);
      break;
    case OP_PUSHARG:
      /* push argument */
      printf("PUSHARG     %u", operand1);
      break;
    case OP_PUSHTEMP:
      /* push temporary */
      printf("PUSHTEMP    %u", operand1);
      break;
    case OP_STORETEMP:
      /* store temporary */
      printf("STORETEMP   %u", operand1);
      break;
    case OP_PUSHBLK:
      /* push block */
      printf("PUSHBLK     %u,%u", operand1, operand2);
      break;
    case OP_SEND:
      /* send message */
      printf("SEND        %u,%u\t  ; ", operand1, operand2);
      showLiteral(getPtr(literals, operand2));
      break;
    case OP_SENDSUPER:
      /* send message to super */
      printf("SENDSUPER   %u,%u\t  ; ", operand1, operand2);
      showLiteral(getPtr(literals, operand2));
      break;
    case OP_PRIM:
      /* call primitive */
      printf("PRIM        %u,%u", operand1, operand2);
      break;
    case OP_JUMP:
      /* jump */
      printf("JUMP        0x%04X", operand2);
      break;
    default:
      /* unknown opcode */
      printf("???         ");
      break;
  }
  printf("\n");
}


static void showInstructions(ObjPtr code,
                             ObjPtr literals) {
  int size, ip;

  size = getSize(code);
  for (ip = 0; ip < size; ip++) {
    showInstruction(ip, code, literals);
  }
}


static void showWhere(ObjPtr receiverClass,
                      ObjPtr methodClass,
                      ObjPtr selector) {
  showClassName(receiverClass);
  printf(" (");
  showClassName(methodClass);
  printf(")");
  printf(" >> ");
  showString(selector);
  printf("\n");
}


static void inspect(void) {
  char *line;
  char c;
  ObjPtr object;

  line = gl_getline("INSPECT: context, home, from, "
                    "method, rcvr, args, temps, stack, other? ");
  gl_histadd(line);
  c = line[0];
  switch (c) {
    case 'c':
      showObject("current active context",
                 machine.currentActiveContext);
      break;
    case 'h':
      if (machine.currentHomeContext ==
          machine.currentActiveContext) {
        printf("current home context = current active context\n");
      }
      showObject("current home context",
                 machine.currentHomeContext);
      break;
    case 'f':
      showObject("calling context (in current active context)",
                 getPtr(machine.currentActiveContext, CALLER_IN_CONTEXT));
      if (machine.currentHomeContext !=
          machine.currentActiveContext) {
        showObject("sending context (in current home context)",
                   getPtr(machine.currentHomeContext, CALLER_IN_CONTEXT));
      }
      break;
    case 'm':
      showObject("current method",
                 machine.currentMethod);
      break;
    case 'r':
      showObject("current receiver",
                 machine.currentReceiver);
      break;
    case 'a':
      showObject("current arguments",
                 machine.currentArgs);
      break;
    case 't':
      showObject("current temporaries",
                 machine.currentTemps);
      break;
    case 's':
      printf("current stack pointer = %u\n", machine.sp);
      showObject("current stack",
                 machine.currentStack);
      break;
    case 'o':
      line = gl_getline("object pointer? 0x");
      gl_histadd(line);
      object = strtoul(line, NULL, 16);
      showObject(NULL, object);
      break;
  }
}


static void debug(void) {
  Bool back;
  char *line;
  char c;

  back = false;
  while (!back) {
    showInstruction(machine.ip,
                    machine.currentCode,
                    machine.currentLiterals);
    do {
      line = gl_getline("DEBUG: inspect, where, list, step, run, quit? ");
      gl_histadd(line);
      c = line[0];
    } while (c != 'i' && c != 'w' && c != 'l' &&
             c != 's' && c != 'r' && c != 'q') ;
    switch (c) {
      case 'i':
        /* inspect */
        inspect();
        break;
      case 'w':
        /* where */
        /* nothing to do here */
        break;
      case 'l':
        /* list */
        showWhere(getClass(machine.currentReceiver),
                  getPtr(machine.currentMethod, CLASS_IN_METHOD),
                  getPtr(machine.currentMethod, SELECTOR_IN_METHOD));
        showInstructions(machine.currentCode,
                         machine.currentLiterals);
        break;
      case 's':
        /* step */
        back = true;
        break;
      case 'r':
        /* run */
        debugMachine = false;
        back = true;
        break;
      case 'q':
        /* quit */
        runMachine = false;
        back = true;
        break;
    }
    if (!back) {
      showWhere(getClass(machine.currentReceiver),
                getPtr(machine.currentMethod, CLASS_IN_METHOD),
                getPtr(machine.currentMethod, SELECTOR_IN_METHOD));
    }
  }
}


/**************************************************************/

/* auxiliary functions */


void push(ObjPtr object) {
  setPtr(machine.currentStack,
         machine.sp,
         object);
  machine.sp++;
  setPtr(machine.currentActiveContext,
         SP_IN_CONTEXT,
         newShortInteger(machine.sp));
}


ObjPtr pop(void) {
  ObjPtr object;

  machine.sp--;
  setPtr(machine.currentActiveContext,
         SP_IN_CONTEXT,
         newShortInteger(machine.sp));
  object = getPtr(machine.currentStack, machine.sp);
  setPtr(machine.currentStack,
         machine.sp,
         machine.nil);
  return object;
}


void activateContext(ObjPtr context) {
  machine.currentActiveContext = context;
  if (getClass(context) == machine.MethodContext) {
    /* the new context is a MethodContext */
    machine.currentHomeContext = context;
  } else {
    /* the new context is a BlockContext */
    machine.currentHomeContext = getPtr(context, HOME_IN_BLOCKCONTEXT);
  }
  machine.currentMethod =
    getPtr(machine.currentHomeContext, METHOD_IN_METHODCONTEXT);
  machine.currentReceiver =
    getPtr(machine.currentHomeContext, RECEIVER_IN_METHODCONTEXT);
  machine.currentArgs =
    getPtr(machine.currentHomeContext, ARGS_IN_METHODCONTEXT);
  machine.currentTemps =
    getPtr(machine.currentHomeContext, TEMPS_IN_METHODCONTEXT);
  machine.currentStack =
    getPtr(machine.currentActiveContext, STACK_IN_CONTEXT);
  machine.currentCode =
    getPtr(machine.currentMethod, CODE_IN_METHOD);
  machine.currentLiterals =
    getPtr(machine.currentMethod, LITERALS_IN_METHOD);
  machine.ip =
    getShortInteger(getPtr(machine.currentActiveContext, IP_IN_CONTEXT));
  machine.sp =
    getShortInteger(getPtr(machine.currentActiveContext, SP_IN_CONTEXT));
}


/**************************************************************/

/* instruction interpreter */


static void createBlockContext(int numArgs, int stackSize) {
  ObjPtr stack;

  machine.newContext =
    createObject(machine.BlockContext, SIZE_OF_BLOCKCONTEXT, true, false);
  if (stackSize != 0) {
    stack = createObject(machine.Array, stackSize, true, false);
    setPtr(machine.newContext,
           STACK_IN_BLOCKCONTEXT,
           stack);
  }
  setPtr(machine.newContext,
         ARGCOUNT_IN_BLOCKCONTEXT,
         newShortInteger(numArgs));
  setPtr(machine.newContext,
         IPSTART_IN_BLOCKCONTEXT,
         newShortInteger(machine.ip + 1));
  setPtr(machine.newContext,
         HOME_IN_BLOCKCONTEXT,
         machine.currentHomeContext);
}


static ObjPtr findMethod(ObjPtr initialClass, ObjPtr selector) {
  ObjPtr class;
  int hash;
  ObjPtr dictionary;
  ObjPtr hashTable;
  int size, bucket;
  ObjPtr link;

  class = initialClass;
  hash = getHash(selector);
  while (class != machine.nil) {
    /* try to find selector in the method dictionary of class */
    dictionary = getPtr(class, METHODS_IN_CLASS);
    hashTable = getPtr(dictionary, HASHTABLE_IN_DICTIONARY);
    size = getSize(hashTable);
    bucket = hash % size;
    link = getPtr(hashTable, bucket);
    while (link != machine.nil) {
      /* key equals selector? */
      if (getPtr(link, KEY_IN_LINK) == selector) {
        /* bingo, return method */
        return getPtr(link, VALUE_IN_LINK);
      }
      /* this is another symbol, try next */
      link = getPtr(link, NEXT_IN_LINK);
    }
    /* selector not found in this class, try superclass */
    class = getPtr(class, SUPERCLASS_IN_CLASS);
  }
  /* no method found */
  showClassName(initialClass);
  printf(" >> ");
  showString(selector);
  printf("\n");
  sysError("message not understood");
  /* never reached */
  return machine.nil;
}


static void executeNewMethod(void) {
  int stackSize;
  ObjPtr stack;
  int argSize;
  ObjPtr args;
  int tempSize;
  ObjPtr temps;
  int i;

  /* construct new context */
  machine.newContext =
    createObject(machine.MethodContext, SIZE_OF_METHODCONTEXT, true, false);
  setPtr(machine.newContext,
         CALLER_IN_METHODCONTEXT,
         machine.currentActiveContext);
  setPtr(machine.newContext,
         IP_IN_METHODCONTEXT,
         newShortInteger(0));
  stackSize =
    getShortInteger(getPtr(machine.newMethod, STACKSIZE_IN_METHOD));
  if (stackSize != 0) {
    stack = createObject(machine.Array, stackSize, true, false);
    setPtr(machine.newContext,
           STACK_IN_METHODCONTEXT,
           stack);
  }
  setPtr(machine.newContext,
         SP_IN_METHODCONTEXT,
         newShortInteger(0));
  setPtr(machine.newContext,
         METHOD_IN_METHODCONTEXT,
         machine.newMethod);
  argSize =
    getShortInteger(getPtr(machine.newMethod, ARGSIZE_IN_METHOD));
  if (argSize != 0) {
    args = createObject(machine.Array, argSize, true, false);
    setPtr(machine.newContext,
           ARGS_IN_METHODCONTEXT,
           args);
    for (i = 1; i <= argSize; i++) {
      setPtr(args, argSize - i, pop());
    }
  }
  setPtr(machine.newContext,
         RECEIVER_IN_METHODCONTEXT,
         pop());
  tempSize =
    getShortInteger(getPtr(machine.newMethod, TEMPSIZE_IN_METHOD));
  if (tempSize != 0) {
    temps = createObject(machine.Array, tempSize, true, false);
    setPtr(machine.newContext,
           TEMPS_IN_METHODCONTEXT,
           temps);
  }
  machine.newMethod = machine.nil;
  /* make the new context the current context */
  activateContext(machine.newContext);
  machine.newContext = machine.nil;
}


void run(void) {
  Word instr;
  Word opcode, operand1, operand2;
  ObjPtr selector;
  ObjPtr class;
  ObjPtr caller;
  ObjPtr retObj;

  while (runMachine) {
    /* test if debugging mode is on */
    if (debugMachine) {
      debug();
    }
    /* test run flag here again because debug() may have switched it off */
    if (!runMachine) {
      break;
    }
    /* now fetch and execute the next instruction */
    instr = getWord(machine.currentCode, machine.ip);
    machine.ip++;
    setPtr(machine.currentActiveContext,
           IP_IN_CONTEXT,
           newShortInteger(machine.ip));
    opcode = (instr >> 24) & 0xFF;
    operand1 = (instr >> 16) & 0xFF;
    operand2 = instr & 0xFFFF;
    switch (opcode) {
      case OP_NOP:
        /* no operation */
        break;
      case OP_PUSHSELF:
        /* push receiver */
        push(machine.currentReceiver);
        break;
      case OP_PUSHNIL:
        /* push nil */
        push(machine.nil);
        break;
      case OP_PUSHFALSE:
        /* push false */
        push(machine.false);
        break;
      case OP_PUSHTRUE:
        /* push true */
        push(machine.true);
        break;
      case OP_DUP:
        /* duplicate top of stack */
        push(getPtr(machine.currentStack, machine.sp - 1));
        break;
      case OP_DROP:
        /* drop top of stack */
        pop();
        break;
      case OP_RETMSG:
        /* return top of stack from message */
        /* get context to which we should return */
        caller =
          getPtr(machine.currentHomeContext, CALLER_IN_METHODCONTEXT);
        /* check that we do not try to return from a
           context that we have already returned from */
        if (caller == machine.nil) {
          sysError("cannot return");
        }
        /* set flag that we cannot return again */
        setPtr(machine.currentHomeContext,
               CALLER_IN_METHODCONTEXT,
               machine.nil);
        /* get object which should be returned */
        retObj = pop();
        /* change contexts */
        activateContext(caller);
        /* push returned object on stack */
        push(retObj);
        if (debugMachine) {
          showWhere(getClass(machine.currentReceiver),
                    getPtr(machine.currentMethod, CLASS_IN_METHOD),
                    getPtr(machine.currentMethod, SELECTOR_IN_METHOD));
        }
        break;
      case OP_RETBLK:
        /* return top of stack from block */
        /* get context to which we should return */
        caller =
          getPtr(machine.currentActiveContext, CALLER_IN_BLOCKCONTEXT);
        /* get object which should be returned */
        retObj = pop();
        /* change contexts */
        activateContext(caller);
        /* push returned object on stack */
        push(retObj);
        if (debugMachine) {
          showWhere(getClass(machine.currentReceiver),
                    getPtr(machine.currentMethod, CLASS_IN_METHOD),
                    getPtr(machine.currentMethod, SELECTOR_IN_METHOD));
        }
        break;
      case OP_PUSHCONST:
        /* push constant */
        push(getPtr(machine.currentLiterals, operand2));
        break;
      case OP_PUSHGLOB:
        /* push global */
        push(getPtr(getPtr(machine.currentLiterals, operand2),
                    VALUE_IN_LINK));
        break;
      case OP_STOREGLOB:
        /* store global */
        setPtr(getPtr(machine.currentLiterals, operand2),
               VALUE_IN_LINK,
               pop());
        break;
      case OP_PUSHINST:
        /* push instance */
        push(getPtr(machine.currentReceiver, operand1));
        break;
      case OP_STOREINST:
        /* store instance */
        setPtr(machine.currentReceiver, operand1, pop());
        break;
      case OP_PUSHARG:
        /* push argument */
        push(getPtr(machine.currentArgs, operand1));
        break;
      case OP_PUSHTEMP:
        /* push temporary */
        push(getPtr(machine.currentTemps, operand1));
        break;
      case OP_STORETEMP:
        /* store temporary */
        setPtr(machine.currentTemps, operand1, pop());
        break;
      case OP_PUSHBLK:
        /* push block */
        createBlockContext(operand1, operand2);
        push(machine.newContext);
        machine.newContext = machine.nil;
        break;
      case OP_SEND:
      case OP_SENDSUPER:
        /* send message */
        /* first, get selector of message */
        selector = getPtr(machine.currentLiterals, operand2);
        /* then, lookup method */
        if (opcode == OP_SEND) {
          /* SEND starts lookup in receiver's class */
          class = getClass(getPtr(machine.currentStack,
                                  machine.sp - operand1 - 1));
        } else {
          /* SENDSUPER starts lookup in superclass of method's class */
          class = getPtr(getPtr(machine.currentMethod, CLASS_IN_METHOD),
                         SUPERCLASS_IN_CLASS);
        }
        machine.newMethod = findMethod(class, selector);
        if (debugMachine) {
          showWhere(class,
                    getPtr(machine.newMethod, CLASS_IN_METHOD),
                    selector);
        }
        /* finally, check number of arguments and execute new method */
        if (operand1 !=
            getShortInteger(getPtr(machine.newMethod, ARGSIZE_IN_METHOD))) {
          sysError("wrong number of arguments in message send");
        }
        executeNewMethod();
        break;
      case OP_PRIM:
        /* call primitive */
        primitive(operand1, operand2);
        if (debugMachine) {
          showWhere(getClass(machine.currentReceiver),
                    getPtr(machine.currentMethod, CLASS_IN_METHOD),
                    getPtr(machine.currentMethod, SELECTOR_IN_METHOD));
        }
        break;
      case OP_JUMP:
        /* jump */
        machine.ip = operand2;
        setPtr(machine.currentActiveContext,
               IP_IN_CONTEXT,
               newShortInteger(machine.ip));
        break;
      default:
        /* unknown opcode */
        sysError("illegal opcode 0x%02X encountered", opcode);
        break;
    }
  }
}
