/*
 * filein.c -- interpret a class file
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "filein.h"
#include "compiler.h"
#include "ui.h"


#define MAX_TOKENS	(LINE_SIZE / 2)

#define IS_VARIABLE	0x04
#define IS_POINTERS	0x02
#define IS_WORDS	0x01


/**************************************************************/


Bool debugFileIn = false;	/* debug flag, show details if set */


/**************************************************************/


static Bool fileInClassDef(int lineNumber,
                           char *tokens[], int numTokens) {
  int type;
  int numInst;
  int i;

  if (numTokens < 3) {
    sysWarning("line %d: too few tokens in line", lineNumber);
    return false;
  }
  if (strcmp(tokens[0], "Class") == 0) {
    type = IS_POINTERS;
  } else
  if (strcmp(tokens[0], "VarByteClass") == 0) {
    type = IS_VARIABLE;
  } else
  if (strcmp(tokens[0], "VarWordClass") == 0) {
    type = IS_VARIABLE | IS_WORDS;
  } else
  if (strcmp(tokens[0], "VarClass") == 0) {
    type = IS_VARIABLE | IS_POINTERS;
  } else {
    sysWarning("line %d: illegal class type specification", lineNumber);
    return false;
  }
  numInst = numTokens - 3;
  if (debugFileIn) {
    printf("class definition of '%s' as ", tokens[1]);
    if (type & IS_VARIABLE) {
      printf("variable");
    } else {
      printf("fixed");
    }
    printf(" ");
    if (type & IS_POINTERS) {
      printf("pointer");
    } else {
      if (type & IS_WORDS) {
        printf("word");
      } else {
        printf("byte");
      }
    }
    printf(" subclass of '%s'\n", tokens[2]);
    if (numInst > 0) {
      printf("  instvars: ");
      for (i = 0; i < numInst; i++) {
        printf("%s ", tokens[i + 3]);
      }
      printf("\n");
    }
  }
  return true;
}


static Bool fileInMethods(int lineNumber,
                          char *tokens[], int numTokens,
                          FILE *classFile) {
  char methodLine[LINE_SIZE];
  char methodSource[METHOD_SIZE];
  char *p;
  int n;

  if (numTokens < 2) {
    sysWarning("line %d: too few tokens in line", lineNumber);
    return false;
  }
  if (numTokens > 2) {
    sysWarning("line %d: too many tokens in line", lineNumber);
    return false;
  }
  if (debugFileIn) {
    printf("compiling methods for class '%s'\n",
           tokens[1]);
  }
  while (1) {
    /* file-in a single method */
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
    if (!compile(methodSource, 0, false)) {
      return false;
    }
    if (methodLine[0] == ']') {
      break;
    }
  }
  return true;
}


/**************************************************************/


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


Bool fileIn(FILE *classFile) {
  int lineNumber;
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
    if (strcmp(tokens[0], "Class") == 0 ||
        strcmp(tokens[0], "VarByteClass") == 0 ||
        strcmp(tokens[0], "VarWordClass") == 0 ||
        strcmp(tokens[0], "VarClass") == 0) {
      if (!fileInClassDef(lineNumber, tokens, n)) {
        return false;
      }
      continue;
    }
    if (strcmp(tokens[0], "Methods") == 0) {
      if (!fileInMethods(lineNumber, tokens, n, classFile)) {
        return false;
      }
      continue;
    }
    sysWarning("line %d: unrecognized line", lineNumber);
    return false;
  }
  return true;
}
