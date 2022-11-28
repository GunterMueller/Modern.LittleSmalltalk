/*
 * compiler.c -- MLS compiler
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "tree.h"
#include "code.h"
#include "check.h"
#include "parser.h"
#include "scanner.h"
#include "ui.h"


/**************************************************************/


Bool debugSource = false;	/* show source text if set */
Bool debugTokens = false;	/* show token stream if set */
Bool debugTree = false;		/* show syntax tree if set */
Bool debugVars = false;		/* show variable info if set */
Bool debugCode = false;		/* show generated code if set */


/**************************************************************/


Bool compile(char *text, ObjPtr class, Bool valueNeeded) {
  if (debugSource) {
    printf("----------- About To Compile The Following Text -----------\n");
    printf("%s", text);
    printf("-----------------------------------------------------------\n");
  }
  compilationOK = true;
  initScanner(text);
  yyparse();
  if (!compilationOK) {
    return false;
  }
  if (debugTree) {
    showTree(method);
  }
  check(method, class);
  if (!compilationOK) {
    return false;
  }
  if (debugVars) {
    showVariables();
  }
  code(text, method, class, valueNeeded);
  if (!compilationOK) {
    return false;
  }
  if (debugCode) {
    showCode();
  }
  freeTree(method);
  freeVariables();
  return true;
}
