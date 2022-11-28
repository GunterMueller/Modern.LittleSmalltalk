/*
 * mkimage.c -- main program of MLS image generator
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "utils.h"
#include "machine.h"
#include "objects.h"
#include "memory.h"
#include "compiler.h"
#include "ui.h"


/**************************************************************/


#define MAX_CLASS_FILES	20			/* max # of class files */
#define MAX_CLASSES	(10 * MAX_CLASS_FILES)	/* max # of classes */
#define MAX_VARS	25			/* max # of instvars */
#define MAX_TOKENS	(LINE_SIZE / 2)		/* max # of tokens */

#define IS_VARIABLE	0x04
#define IS_POINTERS	0x02
#define IS_WORDS	0x01


/**************************************************************/


static void createImageFile(char *imageFileName) {
  FILE *imageFile;

  /* try to create image file */
  imageFile = fopen(imageFileName, "wb");
  if (imageFile == NULL) {
    sysError("cannot create image file '%s'", imageFileName);
  }
  /* write initial machine state */
  machine.signature_1 = SIGNATURE_1;
  machine.signature_2 = SIGNATURE_2;
  machine.majorVersion = MAJOR_VNUM;
  machine.minorVersion = MINOR_VNUM;
  machine.memoryStart = sizeof(Machine);
  machine.memorySize = 0;
  if (fwrite(&machine, sizeof(Machine), 1, imageFile) != 1) {
    sysError("cannot write initial machine state");
  }
  /* close image file */
  fclose(imageFile);
}


/**************************************************************/


static struct {
  int type;
  char *name;
  char *super;
  char *vars[MAX_VARS];
  int numVars;
  char *classVars[MAX_VARS];
  int numClassVars;
  ObjPtr classObject;
  ObjPtr metaclassObject;
} allClasses[MAX_CLASSES];

static int numClasses = 0;


static void showClass(int classNum) {
  int i;

  printf("%2d: name = %s\n", classNum, allClasses[classNum].name);
  printf("    ");
  if (allClasses[classNum].type & IS_VARIABLE) {
    printf("variable");
  } else {
    printf("fixed");
  }
  printf(" ");
  if (allClasses[classNum].type & IS_POINTERS) {
    printf("pointer");
  } else {
    if (allClasses[classNum].type & IS_WORDS) {
      printf("word");
    } else {
      printf("byte");
    }
  }
  printf(" subclass of %s\n", allClasses[classNum].super);
  if (allClasses[classNum].numVars > 0) {
    printf("    instVars = ");
    for (i = 0; i < allClasses[classNum].numVars; i++) {
      printf("%s ", allClasses[classNum].vars[i]);
    }
    printf("\n");
  }
  if (allClasses[classNum].numClassVars > 0) {
    printf("    classVars = ");
    for (i = 0; i < allClasses[classNum].numClassVars; i++) {
      printf("%s ", allClasses[classNum].classVars[i]);
    }
    printf("\n");
  }
  printf("    ObjPtr of class = 0x%08lX\n",
         allClasses[classNum].classObject);
  printf("    ObjPtr of metaclass = 0x%08lX\n",
         allClasses[classNum].metaclassObject);
}


static int findClass(char *name) {
  int i;

  for (i = 0; i < numClasses; i++) {
    if (strcmp(allClasses[i].name, name) == 0) {
      break;
    }
  }
  if (i == numClasses) {
    sysError("essential class '%s' not present in class library", name);
  }
  return i;
}


static ObjPtr findClassObject(char *name) {
  return allClasses[findClass(name)].classObject;
}


static ObjPtr findMetaclassObject(char *name) {
  return allClasses[findClass(name)].metaclassObject;
}


/**************************************************************/


static void bigBangPart1(void) {
  int i;

  /* create nil: this will allow to work createObject() correctly */
  machine.nil = createObject((ObjPtr) 0, 0, true, false);
  /* now false and true can be created */
  machine.false = createObject(machine.nil, 0, true, false);
  machine.true = createObject(machine.nil, 0, true, false);
  /* then, the characters */
  for (i = 0; i < 256; i++) {
    machine.character[i] = createObject(machine.nil, 1, false, false);
    setByte(machine.character[i], 0, i);
  }
}


static void bigBangPart2(void) {
  int i;

  /* patch classes of objects created in part 1 */
  setClass(machine.nil, findClassObject("UndefinedObject"));
  setClass(machine.false, findClassObject("False"));
  setClass(machine.true, findClassObject("True"));
  for (i = 0; i < 256; i++) {
    setClass(machine.character[i], findClassObject("Character"));
  }
}


/**************************************************************/


static ObjPtr WordArray(int n) {
  ObjPtr class;
  ObjPtr wordArray;

  class = findClassObject("WordArray");
  wordArray = createObject(class, n, false, true);
  return wordArray;
}


static ObjPtr Method(ObjPtr text,
                     ObjPtr selector,
                     ObjPtr code,
                     ObjPtr literals,
                     ObjPtr argSize,
                     ObjPtr tempSize,
                     ObjPtr stackSize,
                     ObjPtr whereClass) {
  ObjPtr class;
  ObjPtr method;

  class = findClassObject("Method");
  method = createObject(class, SIZE_OF_METHOD, true, false);
  setPtr(method, TEXT_IN_METHOD, text);
  setPtr(method, SELECTOR_IN_METHOD, selector);
  setPtr(method, CODE_IN_METHOD, code);
  setPtr(method, LITERALS_IN_METHOD, literals);
  setPtr(method, ARGSIZE_IN_METHOD, argSize);
  setPtr(method, TEMPSIZE_IN_METHOD, tempSize);
  setPtr(method, STACKSIZE_IN_METHOD, stackSize);
  setPtr(method, CLASS_IN_METHOD, whereClass);
  return method;
}


static ObjPtr MethodContext(ObjPtr caller,
                            ObjPtr ip,
                            ObjPtr stack,
                            ObjPtr sp,
                            ObjPtr method,
                            ObjPtr receiver,
                            ObjPtr args,
                            ObjPtr temps) {
  ObjPtr class;
  ObjPtr methodContext;

  class = findClassObject("MethodContext");
  methodContext = createObject(class, SIZE_OF_METHODCONTEXT, true, false);
  setPtr(methodContext, CALLER_IN_METHODCONTEXT, caller);
  setPtr(methodContext, IP_IN_METHODCONTEXT, ip);
  setPtr(methodContext, STACK_IN_METHODCONTEXT, stack);
  setPtr(methodContext, SP_IN_METHODCONTEXT, sp);
  setPtr(methodContext, METHOD_IN_METHODCONTEXT, method);
  setPtr(methodContext, RECEIVER_IN_METHODCONTEXT, receiver);
  setPtr(methodContext, ARGS_IN_METHODCONTEXT, args);
  setPtr(methodContext, TEMPS_IN_METHODCONTEXT, temps);
  return methodContext;
}


static ObjPtr Link(ObjPtr key, ObjPtr value, ObjPtr next) {
  ObjPtr class;
  ObjPtr link;

  class = findClassObject("Link");
  link = createObject(class, SIZE_OF_LINK, true, false);
  setPtr(link, KEY_IN_LINK, key);
  setPtr(link, VALUE_IN_LINK, value);
  setPtr(link, NEXT_IN_LINK, next);
  return link;
}


static ObjPtr Array(int n) {
  ObjPtr class;
  ObjPtr array;

  class = findClassObject("Array");
  array = createObject(class, n, true, false);
  return array;
}


static ObjPtr Dictionary(void) {
  ObjPtr class;
  ObjPtr dictionary;
  ObjPtr hashTable;

  class = findClassObject("Dictionary");
  dictionary = createObject(class, SIZE_OF_DICTIONARY, true, false);
  hashTable = Array(7);
  setPtr(dictionary, HASHTABLE_IN_DICTIONARY, hashTable);
  return dictionary;
}


static ObjPtr SystemDictionary(void) {
  ObjPtr class;
  ObjPtr systemDictionary;
  ObjPtr hashTable;

  class = findClassObject("SystemDictionary");
  systemDictionary = createObject(class, SIZE_OF_DICTIONARY, true, false);
  hashTable = Array(7);
  setPtr(systemDictionary, HASHTABLE_IN_DICTIONARY, hashTable);
  return systemDictionary;
}


static ObjPtr String(char *s) {
  int n, i;
  ObjPtr class;
  ObjPtr string;

  n = strlen(s);
  class = findClassObject("String");
  string = createObject(class, n, false, false);
  for (i = 0; i < n; i++) {
    setByte(string, i, s[i]);
  }
  return string;
}


static ObjPtr Symbol(char *s) {
  int n, i;
  int h;
  ObjPtr hashTable;
  int size, bucket;
  ObjPtr link;
  ObjPtr class;
  ObjPtr symbol;

  n = strlen(s);
  h = hash(s, n);
  hashTable = getPtr(machine.Smalltalk, HASHTABLE_IN_DICTIONARY);
  size = getSize(hashTable);
  bucket = h % size;
  link = getPtr(hashTable, bucket);
  while (link != machine.nil) {
    symbol = getPtr(link, KEY_IN_LINK);
    if (h == getHash(symbol) && n == getSize(symbol)) {
      /* hash and size are equal - must check the characters */
      for (i = 0; i < n; i++) {
        if (s[i] != getByte(symbol, i)) {
          break;
        }
      }
      if (i == n) {
        /* symbol found, return it */
        return symbol;
      }
    }
    link = getPtr(link, NEXT_IN_LINK);
  }
  /* symbol not found, must create a new one */
  class = findClassObject("Symbol");
  symbol = createObject(class, n, false, false);
  setHash(symbol, h);
  for (i = 0; i < n; i++) {
    setByte(symbol, i, s[i]);
  }
  /* enter the new symbol into the table */
  link = Link(symbol, machine.nil, getPtr(hashTable, bucket));
  setPtr(hashTable, bucket, link);
  /* finally return the new symbol */
  return symbol;
}


/**************************************************************/


Bool debugFileIn = false;	/* debug flag, show details if set */

static int lineNumber;


static int tokenize(char *line, char *tokens[], int maxTokens) {
  int n;
  char *p;

  n = 0;
  p = strtok(line, " \t\r\n");
  while (p != NULL) {
    if (n < maxTokens) {
      tokens[n++] = p;
    }
    p = strtok(NULL, " \t\r\n");
  }
  return n;
}


/**************************************************************/


static Bool readClassDef(char *tokens[], int numTokens) {
  int i, j;

  if (numTokens < 4) {
    sysWarning("line %d: too few tokens in line", lineNumber);
    return false;
  }
  /* type of class */
  if (strcmp(tokens[2], "SUBCLASSOF") == 0) {
    allClasses[numClasses].type = IS_POINTERS;
  } else
  if (strcmp(tokens[2], "VARBYTESUBCLASSOF") == 0) {
    allClasses[numClasses].type = IS_VARIABLE;
  } else
  if (strcmp(tokens[2], "VARWORDSUBCLASSOF") == 0) {
    allClasses[numClasses].type = IS_VARIABLE | IS_WORDS;
  } else
  if (strcmp(tokens[2], "VARSUBCLASSOF") == 0) {
    allClasses[numClasses].type = IS_VARIABLE | IS_POINTERS;
  } else {
    sysWarning("line %d: illegal class type specification", lineNumber);
    return false;
  }
  /* class name */
  if (!isupper((int) *tokens[1])) {
    sysWarning("line %d: class name '%s' must start with upper case",
               lineNumber, tokens[1]);
    return false;
  }
  allClasses[numClasses].name = allocate(strlen(tokens[1]) + 1);
  strcpy(allClasses[numClasses].name, tokens[1]);
  /* superclass name */
  if (!isupper((int) *tokens[3]) && strcmp(tokens[3], "nil") != 0) {
    sysWarning("line %d: class name '%s' must start with upper case",
               lineNumber, tokens[3]);
    return false;
  }
  allClasses[numClasses].super = allocate(strlen(tokens[3]) + 1);
  strcpy(allClasses[numClasses].super, tokens[3]);
  /* rest of line */
  i = 4;
  /* instance variables */
  j = 0;
  if (i < numTokens && strcmp(tokens[i], "VARS") == 0) {
    i++;
    while (i < numTokens && islower((int) *tokens[i])) {
      allClasses[numClasses].vars[j] = allocate(strlen(tokens[i]) + 1);
      strcpy(allClasses[numClasses].vars[j], tokens[i]);
      j++;
      i++;
    }
  }
  allClasses[numClasses].numVars = j;
  /* class variables */
  j = 0;
  if (i < numTokens && strcmp(tokens[i], "CLASSVARS") == 0) {
    i++;
    while (i < numTokens && islower((int) *tokens[i])) {
      allClasses[numClasses].classVars[j] = allocate(strlen(tokens[i]) + 1);
      strcpy(allClasses[numClasses].classVars[j], tokens[i]);
      j++;
      i++;
    }
  }
  allClasses[numClasses].numClassVars = j;
  /* anything left? */
  if (i < numTokens) {
    sysWarning("line %d: excess tokens in CLASS definition", lineNumber);
    return false;
  }
  /* allocate class and metaclass object */
  allClasses[numClasses].classObject =
    createObject(machine.nil, SIZE_OF_CLASS, true, false);
  allClasses[numClasses].metaclassObject =
    createObject(machine.nil, SIZE_OF_METACLASS, true, false);
  /* done */
  if (debugFileIn) {
    showClass(numClasses);
  }
  numClasses++;
  return true;
}


static Bool createClassesFrom(FILE *classFile) {
  char line[LINE_SIZE];
  char *tokens[MAX_TOKENS];
  int n;

  lineNumber = 0;
  while (fgets(line, LINE_SIZE, classFile) != NULL) {
    lineNumber++;
    n = tokenize(line, tokens, MAX_TOKENS);
    if (n == 0) {
      /* line is empty */
      continue;
    }
    if (*tokens[0] == '*') {
      /* line is a comment line */
      continue;
    }
    if (strcmp(tokens[0], "CLASS") == 0) {
      if (!readClassDef(tokens, n)) {
        return false;
      }
      continue;
    }
    if (strcmp(tokens[0], "METHODS") == 0 ||
        strcmp(tokens[0], "CLASSMETHODS") == 0) {
      /* skip method definitions */
      while (1) {
        if (fgets(line, LINE_SIZE, classFile) == NULL) {
          sysError("unexpected end of class file");
        }
        lineNumber++;
        if (line[0] == '\0') {
          /* line is empty */
          continue;
        }
        if (line[0] == ']') {
          /* end of method definitions */
          break;
        }
      }
      continue;
    }
    sysWarning("line %d: unrecognized line", lineNumber);
    return false;
  }
  return true;
}


/**************************************************************/


static ObjPtr enter(ObjPtr dictionary, ObjPtr key, ObjPtr value) {
  int h;
  ObjPtr hashTable;
  int size, bucket;
  ObjPtr link;

  h = getHash(key);
  hashTable = getPtr(dictionary, HASHTABLE_IN_DICTIONARY);
  size = getSize(hashTable);
  bucket = h % size;
  link = getPtr(hashTable, bucket);
  while (link != machine.nil) {
    if (key == getPtr(link, KEY_IN_LINK)) {
      /* key found, store value and return link */
      setPtr(link, VALUE_IN_LINK, value);
      return link;
    }
    link = getPtr(link, NEXT_IN_LINK);
  }
  /* key not found, create a new link */
  link = Link(key, value, getPtr(hashTable, bucket));
  /* insert it into table */
  setPtr(hashTable, bucket, link);
  /* and return it */
  return link;
}


static void createGlobalDictionary(void) {
  machine.Smalltalk = SystemDictionary();
  enter(machine.Smalltalk, Symbol("Smalltalk"), machine.Smalltalk);
}


/**************************************************************/


static void initClasses(void) {
  int i, j, n;
  ObjPtr name;
  ObjPtr instType;
  ObjPtr instSize;
  ObjPtr methods;
  ObjPtr superClass;
  ObjPtr variables;
  ObjPtr varName;

  /* iterate over all classes */
  for (i = 0; i < numClasses; i++) {
    /* set class of class object to its metaclass */
    setClass(allClasses[i].classObject, allClasses[i].metaclassObject);
    /* set name of class */
    name = Symbol(allClasses[i].name);
    setPtr(allClasses[i].classObject, NAME_IN_CLASS, name);
    /* set instance type */
    instType = newShortInteger(allClasses[i].type);
    setPtr(allClasses[i].classObject, INSTTYPE_IN_CLASS, instType);
    /* set number of instance variables,
       sum over class and all superclasses */
    j = i;
    n = 0;
    while (1) {
      n += allClasses[j].numVars;
      if (strcmp(allClasses[j].super, "nil") == 0) {
        break;
      }
      j = findClass(allClasses[j].super);
    }
    instSize = newShortInteger(n);
    setPtr(allClasses[i].classObject, INSTSIZE_IN_CLASS, instSize);
    /* install an empty method dictionary */
    methods = Dictionary();
    setPtr(allClasses[i].classObject, METHODS_IN_CLASS, methods);
    /* set superclass */
    if (strcmp(allClasses[i].super, "nil") == 0) {
      superClass = machine.nil;
    } else {
      superClass = findClassObject(allClasses[i].super);
    }
    setPtr(allClasses[i].classObject, SUPERCLASS_IN_CLASS, superClass);
    /* install an array of instance variable names */
    variables = Array(allClasses[i].numVars);
    setPtr(allClasses[i].classObject, VARIABLES_IN_CLASS, variables);
    for (j = 0; j < allClasses[i].numVars; j++) {
      varName = String(allClasses[i].vars[j]);
      setPtr(variables, j, varName);
    }
    /* install class in global dictionary */
    enter(machine.Smalltalk, name, allClasses[i].classObject);
  }
  /* iterate over all metaclasses */
  for (i = 0; i < numClasses; i++) {
    /* set class of metaclass object to 'Metaclass' */
    setClass(allClasses[i].metaclassObject, findClassObject("Metaclass"));
    /* set name of metaclass (same as class' name) */
    name = Symbol(allClasses[i].name);
    setPtr(allClasses[i].metaclassObject, NAME_IN_METACLASS, name);
    /* set instance type */
    instType = newShortInteger(IS_POINTERS);
    setPtr(allClasses[i].metaclassObject, INSTTYPE_IN_METACLASS, instType);
    /* set number of instance variables,
       sum over class and all superclasses */
    //j = i;
    j = findClass("Class");
    n = 0;
    while (1) {
      n += allClasses[j].numVars;
      if (strcmp(allClasses[j].super, "nil") == 0) {
        break;
      }
      j = findClass(allClasses[j].super);
    }
    instSize = newShortInteger(n);
    setPtr(allClasses[i].metaclassObject, INSTSIZE_IN_METACLASS, instSize);
    /* install an empty method dictionary */
    methods = Dictionary();
    setPtr(allClasses[i].metaclassObject, METHODS_IN_METACLASS, methods);
    /* set superclass */
    if (strcmp(allClasses[i].super, "nil") == 0) {
      superClass = findClassObject("Class");
    } else {
      superClass = findMetaclassObject(allClasses[i].super);
    }
    setPtr(allClasses[i].metaclassObject, SUPERCLASS_IN_METACLASS, superClass);
    /* install an array of class variable names */
    //variables = Array(allClasses[i].numClassVars);
    //setPtr(allClasses[i].metaclassObject, VARIABLES_IN_METACLASS, variables);
    //for (j = 0; j < allClasses[i].numClassVars; j++) {
    //  varName = String(allClasses[i].classVars[j]);
    //  setPtr(variables, j, varName);
    //}
  }
}


/**************************************************************/


static struct {
  char *name;
  ObjPtr *regPtr;
} knownClasses[] = {
  { "ShortInteger",  &machine.ShortInteger  },
  { "Float",         &machine.Float         },
  { "Character",     &machine.Character     },
  { "String",        &machine.String        },
  { "Symbol",        &machine.Symbol        },
  { "Link",          &machine.Link          },
  { "Method",        &machine.Method        },
  { "Array",         &machine.Array         },
  { "WordArray",     &machine.WordArray     },
  { "MethodContext", &machine.MethodContext },
  { "BlockContext",  &machine.BlockContext  },
  { "Metaclass",     &machine.Metaclass     },
};


static void storeKnownClasses(void) {
  int i;
  ObjPtr class;

  for (i = 0; i < sizeof(knownClasses)/sizeof(knownClasses[0]); i++) {
    class = findClassObject(knownClasses[i].name);
    *knownClasses[i].regPtr = class;
  }
}


/**************************************************************/


static Bool readMethodDefs(char *tokens[], int numTokens,
                           FILE *classFile, Bool forMetaclass) {
  char methodSource[METHOD_SIZE];
  char *p;
  char methodLine[LINE_SIZE];
  int n;
  ObjPtr targetClass;

  if (numTokens < 2) {
    sysWarning("line %d: too few tokens in line", lineNumber);
    return false;
  }
  if (numTokens > 2) {
    sysWarning("line %d: too many tokens in line", lineNumber);
    return false;
  }
  if (debugFileIn) {
    if (!forMetaclass) {
      printf("compiling methods for class '%s'\n", tokens[1]);
    } else {
      printf("compiling methods for metaclass of class '%s'\n", tokens[1]);
    }
  }
  while (1) {
    /* read a single method */
    /* first, get text of method */
    p = methodSource;
    while (1) {
      if (fgets(methodLine, LINE_SIZE, classFile) == NULL) {
        sysWarning("unexpected end of file");
        return false;
      }
      if (methodLine[0] == '|' || methodLine[0] == ']') {
        break;
      }
      n = strlen(methodLine);
      if (n >= &methodSource[METHOD_SIZE] - p) {
        sysWarning("method text too long");
        return false;
      }
      strcpy(p, methodLine);
      p += n;
    }
    /* then, compile method */
    if (!forMetaclass) {
      targetClass = findClassObject(tokens[1]);
    } else {
      targetClass = findMetaclassObject(tokens[1]);
    }
    if (!compile(methodSource, targetClass, false)) {
      return false;
    }
    /* finally, install method */
    enter(getPtr(targetClass, METHODS_IN_BEHAVIOR),
          getPtr(machine.compilerMethod, SELECTOR_IN_METHOD),
          machine.compilerMethod);
    /* check for another method */
    if (methodLine[0] == ']') {
      break;
    }
  }
  return true;
}


static Bool createMethodsFrom(FILE *classFile) {
  char line[LINE_SIZE];
  char *tokens[MAX_TOKENS];
  int n;

  lineNumber = 0;
  while (fgets(line, LINE_SIZE, classFile) != NULL) {
    lineNumber++;
    n = tokenize(line, tokens, MAX_TOKENS);
    if (n == 0) {
      /* line is empty */
      continue;
    }
    if (*tokens[0] == '*') {
      /* line is a comment line */
      continue;
    }
    if (strcmp(tokens[0], "CLASS") == 0) {
      /* skip class definitions */
      continue;
    }
    if (strcmp(tokens[0], "METHODS") == 0) {
      if (!readMethodDefs(tokens, n, classFile, false)) {
        return false;
      }
      continue;
    }
    if (strcmp(tokens[0], "CLASSMETHODS") == 0) {
      if (!readMethodDefs(tokens, n, classFile, true)) {
        return false;
      }
      continue;
    }
    sysWarning("line %d: unrecognized line", lineNumber);
    return false;
  }
  return true;
}


/**************************************************************/


static Word bootCode[] = {
  OP_PUSHSELF << 24 | 0x00 << 16 | 0x0000,
  OP_SEND     << 24 | 0x00 << 16 | 0x0000,
  OP_DROP     << 24 | 0x00 << 16 | 0x0000,
  OP_JUMP     << 24 | 0x00 << 16 | 0x0000,
};

static int bootSize = sizeof(bootCode)/sizeof(bootCode[0]);


static void createInitialContext(void) {
  ObjPtr code;
  ObjPtr literals;
  ObjPtr method;
  ObjPtr context;
  int i;

  /*
   * This is the somewhat tricky startup of Modern Little Smalltalk.
   * The goal is to have as few things fixed as possible. So the actual
   * startup procedure is programmed in MLS itself, only the invocation
   * of this procedure is done here (and therefore fixed).
   *
   * We set up a method in execution by installing a MethodContext.
   * The method (which is hand-coded and actually never installed in
   * any class) has the selector 'boot' and pretends to be found in
   * class 'SystemDictionary', so that it could have been invoked by
   * the expression 'Smalltalk boot' (because Smalltalk is an instance
   * of SystemDictionary). Its definition would look like this:
   *
   * boot
   *    "Boot the Smalltalk system."
   *    [true] whileTrue: [self startUp]
   *
   * The loop is included only as precaution against coding errors in
   * the startUp method, which should never return to the boot method.
   * The method with selector 'startUp' in class 'SystemDictionary'
   * then takes the resposibility for actually starting-up Smalltalk.
   */
  /* first, the method */
  code = WordArray(bootSize);
  for (i = 0; i < bootSize; i++) {
    setWord(code, i, bootCode[i]);
  }
  literals = Array(1);
  setPtr(literals, 0, Symbol("startUp"));
  method = Method(String("boot\n"
                         "    \"Boot the Smalltalk system.\"\n"
                         "    [true] whileTrue: [self startUp]\n"),
                  Symbol("boot"),
                  code,
                  literals,
                  newShortInteger(0),
                  newShortInteger(0),
                  newShortInteger(1),
                  findClassObject("SystemDictionary"));
  /* then, the context */
  context = MethodContext(machine.nil,
                          newShortInteger(0),
                          Array(1),
                          newShortInteger(0),
                          method,
                          machine.Smalltalk,
                          machine.nil,
                          machine.nil);
  /* finally, the machine registers */
  machine.currentActiveContext = context;
  machine.currentHomeContext = context;
  machine.currentMethod = getPtr(context, METHOD_IN_METHODCONTEXT);
  machine.currentReceiver = getPtr(context, RECEIVER_IN_METHODCONTEXT);
  machine.currentArgs = getPtr(context, ARGS_IN_METHODCONTEXT);
  machine.currentTemps = getPtr(context, TEMPS_IN_METHODCONTEXT);
  machine.currentStack = getPtr(context, STACK_IN_METHODCONTEXT);
  machine.currentCode = getPtr(method, CODE_IN_METHOD);
  machine.currentLiterals = getPtr(method, LITERALS_IN_METHOD);
  machine.ip = getShortInteger(getPtr(context, IP_IN_METHODCONTEXT));
  machine.sp = getShortInteger(getPtr(context, SP_IN_METHODCONTEXT));
}


/**************************************************************/


static void version(char *myself) {
  printf("%s version %d.%d (compiled %s)\n",
         myself, MAJOR_VNUM, MINOR_VNUM, __DATE__);
}


static void help(char *myself) {
  printf("Usage: %s [options] [<class file> <class file> ...]\n", myself);
  printf("Options:\n");
  printf("  --image <image file>    set image file name\n");
  printf("  --memory                show memory statistics\n");
  printf("  --filein                show file-in details\n");
  printf("  --source                show source given to compiler\n");
  printf("  --tokens                show token stream within compiler\n");
  printf("  --tree                  show syntax tree within compiler\n");
  printf("  --vars                  show variable info within compiler\n");
  printf("  --code                  show code generated by compiler\n");
  printf("  --version               show version and terminate\n");
  printf("  --help                  show this help and terminate\n");
}


int main(int argc, char *argv[]) {
  int i;
  char *imageFileName;
  char *classFileName[MAX_CLASS_FILES];
  int numClassFiles;
  FILE *classFile;

  printf("Modern Little Smalltalk %d.%d image generator\n",
         MAJOR_VNUM, MINOR_VNUM);
  imageFileName = DFLT_IMG_NAME;
  numClassFiles = 0;
  for (i = 1; i < argc; i++) {
    if (*argv[i] == '-') {
      /* option */
      if (strcmp(argv[i], "--image") == 0) {
        if (i == argc - 1) {
          sysError("no image file name specified");
        }
        imageFileName = argv[++i];
      } else
      if (strcmp(argv[i], "--memory") == 0) {
        debugMemory = true;
      } else
      if (strcmp(argv[i], "--filein") == 0) {
        debugFileIn = true;
      } else
      if (strcmp(argv[i], "--source") == 0) {
        debugSource = true;
      } else
      if (strcmp(argv[i], "--tokens") == 0) {
        debugTokens = true;
      } else
      if (strcmp(argv[i], "--tree") == 0) {
        debugTree = true;
      } else
      if (strcmp(argv[i], "--vars") == 0) {
        debugVars = true;
      } else
      if (strcmp(argv[i], "--code") == 0) {
        debugCode = true;
      } else
      if (strcmp(argv[i], "--version") == 0) {
        version(argv[0]);
        exit(0);
      } else
      if (strcmp(argv[i], "--help") == 0) {
        help(argv[0]);
        exit(0);
      } else {
        sysError("unknown option '%s', try '%s --help'",
                 argv[i], argv[0]);
      }
    } else {
      /* file */
      if (numClassFiles == MAX_CLASS_FILES) {
        sysError("too many class files specified");
      }
      classFileName[numClassFiles++] = argv[i];
    }
  }
  /* create an empty image file */
  createImageFile(imageFileName);
  /* init object memory */
  initMemory(imageFileName);
  /* create the world */
  bigBangPart1();
  /* create the standard library class objects */
  for (i = 0; i < numClassFiles; i++) {
    printf("creating classes from '%s'\n", classFileName[i]);
    classFile = fopen(classFileName[i], "r");
    if (classFile == NULL) {
      sysError("cannot open class file '%s'", classFileName[i]);
    }
    if (!createClassesFrom(classFile)) {
      sysError("could not create classes from '%s' properly",
               classFileName[i]);
    }
    fclose(classFile);
  }
  /* patch the world */
  bigBangPart2();
  /* create the global system dictionary */
  createGlobalDictionary();
  /* initialize the class objects */
  initClasses();
  /* store known classes in machine structure */
  storeKnownClasses();
  /* file-in the standard library classes */
  for (i = 0; i < numClassFiles; i++) {
    printf("creating methods from '%s'\n", classFileName[i]);
    classFile = fopen(classFileName[i], "r");
    if (classFile == NULL) {
      sysError("cannot open class file '%s'", classFileName[i]);
    }
    if (!createMethodsFrom(classFile)) {
      sysError("could not create methods from '%s' properly",
               classFileName[i]);
    }
    fclose(classFile);
  }
  /* create the initial context */
  createInitialContext();
  /* exit object memory */
  enableGC = true;
  exitMemory(imageFileName);
  /* that's it */
  printf("Image generated.\n");
  return 0;
}
