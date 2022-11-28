/*
 * check.c -- semantic checks
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
#include "tree.h"
#include "check.h"
#include "ui.h"


#define MAX_NAME_SIZE		LINE_SIZE


/**************************************************************/


static Variable *allVariables;
static Variable *lastVariable;


static Variable *lookupVariable(char *name) {
  Variable *variable;

  variable = allVariables;
  while (variable != NULL) {
    if (strcmp(variable->name, name) == 0) {
      return variable;
    }
    variable = variable->next;
  }
  return NULL;
}


static Variable *enterVariable(VarType type, char *name) {
  Variable *variable;

  if (lookupVariable(name) != NULL) {
    return NULL;
  }
  variable = allocate(sizeof(Variable));
  variable->next = NULL;
  variable->type = type;
  variable->name = allocate(strlen(name) + 1);
  strcpy(variable->name, name);
  if (allVariables == NULL) {
    allVariables = variable;
  } else {
    lastVariable->next = variable;
  }
  lastVariable = variable;
  return variable;
}


static void initVariables(ObjPtr aClass) {
  ObjPtr lastClass, class;
  ObjPtr instVarArray;
  int numberInstVars, i;
  ObjPtr instVar;
  int nameSize, j;
  char name[MAX_NAME_SIZE];

  /* init the variable list */
  allVariables = NULL;
  /* enter pseudo variables */
  enterVariable(Self,  "self");
  enterVariable(Super, "super");
  enterVariable(Nil,   "nil");
  enterVariable(False, "false");
  enterVariable(True,  "true");
  /* enter instance variables of superclass chain, start with root class */
  lastClass = machine.nil;
  while (lastClass != aClass) {
    class = aClass;
    /* skip classes until the class before lastClass is reached */
    while (getPtr(class, SUPERCLASS_IN_CLASS) != lastClass) {
      class = getPtr(class, SUPERCLASS_IN_CLASS);
    }
    /* now add the instance variables of this class */
    instVarArray = getPtr(class, VARIABLES_IN_CLASS);
    numberInstVars = getSize(instVarArray);
    for (i = 0; i < numberInstVars; i++) {
      instVar = getPtr(instVarArray, i);
      nameSize = getSize(instVar);
      if (nameSize >= MAX_NAME_SIZE) {
        compWarning("instance variable name too long");
        nameSize = MAX_NAME_SIZE - 1;
      }
      for (j = 0; j < nameSize; j++) {
        name[j] = getByte(instVar, j);
      }
      name[j] = '\0';
      if (enterVariable(Instance, name) == NULL) {
        compError("variable '%s' is already defined", name);
        return;
      }
    }
    /* work towards aClass */
    lastClass = class;
  }
}


static void computeOffsets(Node *method) {
  int numInsts;
  int numArgs;
  int numTemps;
  Variable *variable;

  numInsts = 0;
  numArgs = 0;
  numTemps = 0;
  variable = allVariables;
  while (variable != NULL) {
    switch (variable->type) {
      case Self:
      case Super:
      case Nil:
      case False:
      case True:
        variable->offset = 0;
        break;
      case Instance:
        variable->offset = numInsts++;
        break;
      case Argument:
        variable->offset = numArgs++;
        break;
      case Temporary:
        variable->offset = numTemps++;
        break;
      case Global:
        variable->offset = 0;
        break;
      default:
        sysError("computeOffsets has illegal variable type");
        break;
    }
    variable = variable->next;
  }
  method->u.methodNode.numArgs = numArgs;
  method->u.methodNode.numTemps = numTemps;
}


/**************************************************************/


static Bool isAssignable(Variable *variable) {
  return variable->type == Instance ||
         variable->type == Temporary ||
         variable->type == Global;
}


static Bool isSuper(Node *node) {
  return node->type == Var &&
         node->u.varNode.var->type == Super;
}


static void checkNode(Node *node);


static void checkMethod(Node *node) {
  List *list;
  Node *varNode;
  Variable *variable;

  /* parameters */
  list = node->u.methodNode.parameters;
  while (list != NULL) {
    varNode = list->head;
    if (varNode == NULL || varNode->type != Var) {
      compError("line %d: checkMethod internal error 1",
                node->line);
      return;
    }
    if (isupper((int) varNode->u.varNode.name[0])) {
      compError("line %d: global name '%s' cannot be used as parameter",
                varNode->line, varNode->u.varNode.name);
      return;
    }
    variable = enterVariable(Argument, varNode->u.varNode.name);
    if (variable == NULL) {
      compError("line %d: parameter name '%s' already defined",
                varNode->line, varNode->u.varNode.name);
      return;
    }
    varNode->u.varNode.var = variable;
    list = list->tail;
  }
  /* temporaries */
  list = node->u.methodNode.temporaries;
  while (list != NULL) {
    varNode = list->head;
    if (varNode == NULL || varNode->type != Var) {
      compError("line %d: checkMethod internal error 2",
                node->line);
      return;
    }
    if (isupper((int) varNode->u.varNode.name[0])) {
      compError("line %d: global name '%s' cannot be used as temporary",
                varNode->line, varNode->u.varNode.name);
      return;
    }
    variable = enterVariable(Temporary, varNode->u.varNode.name);
    if (variable == NULL) {
      compError("line %d: temporary name '%s' already defined",
                varNode->line, varNode->u.varNode.name);
      return;
    }
    varNode->u.varNode.var = variable;
    list = list->tail;
  }
  /* statements */
  list = node->u.methodNode.statements;
  while (list != NULL) {
    checkNode(list->head);
    list = list->tail;
  }
}


static void checkReturn(Node *node) {
  checkNode(node->u.returnNode.expression);
}


static void checkAssign(Node *node) {
  Node *varNode;
  Variable *variable;

  /* check both sides */
  checkNode(node->u.assignNode.lhs);
  checkNode(node->u.assignNode.rhs);
  /* check for legal assignment */
  varNode = node->u.assignNode.lhs;
  if (varNode == NULL || varNode->type != Var) {
    compError("line %d: checkAssign internal error 1",
              node->line);
    return;
  }
  variable = varNode->u.varNode.var;
  if (variable == NULL) {
    compError("line %d: checkAssign internal error 2",
              node->line);
    return;
  }
  if (!isAssignable(variable)) {
    compError("line %d: illegal assignment to variable '%s'",
              node->line, variable->name);
    return;
  }
}


static void checkCascade(Node *node) {
  List *list;

  /* receiver */
  checkNode(node->u.cascadeNode.receiver);
  /* continuations */
  list = node->u.cascadeNode.continuations;
  while (list != NULL) {
    checkNode(list->head);
    list = list->tail;
  }
}


static void checkMessage(Node *node) {
  List *list;
  int numArgs;

  /* receiver, may be NULL in cascades */
  if (node->u.messageNode.receiver != NULL) {
    checkNode(node->u.messageNode.receiver);
    node->u.messageNode.superFlag = isSuper(node->u.messageNode.receiver);
  } else {
    node->u.messageNode.superFlag = false;
  }
  /* arguments */
  list = node->u.messageNode.arguments;
  numArgs = 0;
  while (list != NULL) {
    checkNode(list->head);
    list = list->tail;
    numArgs++;
  }
  node->u.messageNode.numArgs = numArgs;
}


static void checkVar(Node *node) {
  Variable *variable;

  variable = lookupVariable(node->u.varNode.name);
  if (variable == NULL) {
    if (!isupper((int) node->u.varNode.name[0])) {
      compError("line %d: unknown variable '%s'",
                node->line, node->u.varNode.name);
      return;
    }
    variable = enterVariable(Global, node->u.varNode.name);
  }
  node->u.varNode.var = variable;
}


static void checkInt(Node *node) {
  /* nothing to do here */
}


static void checkFloat(Node *node) {
  /* nothing to do here */
}


static void checkChar(Node *node) {
  /* nothing to do here */
}


static void checkString(Node *node) {
  /* nothing to do here */
}


static void checkSymbol(Node *node) {
  /* nothing to do here */
}


static void checkArray(Node *node) {
  List *elements;
  int numElems;

  /* elements */
  elements = node->u.arrayNode.elements;
  numElems = 0;
  while (elements != NULL) {
    checkNode(elements->head);
    elements = elements->tail;
    numElems++;
  }
  node->u.arrayNode.numElems = numElems;
}


static void checkBlock(Node *node) {
  List *list;
  Node *varNode;
  Variable *variable;
  int numArgs;

  /* block arguments */
  list = node->u.blockNode.arguments;
  numArgs = 0;
  while (list != NULL) {
    varNode = list->head;
    if (varNode == NULL || varNode->type != Var) {
      compError("line %d: checkBlock internal error",
                node->line);
      return;
    }
    if (isupper((int) varNode->u.varNode.name[0])) {
      compError("line %d: global name '%s' cannot be used as block argument",
                varNode->line, varNode->u.varNode.name);
      return;
    }
    variable = enterVariable(Temporary, varNode->u.varNode.name);
    if (variable == NULL) {
      compError("line %d: block argument name '%s' already defined",
                varNode->line, varNode->u.varNode.name);
      return;
    }
    varNode->u.varNode.var = variable;
    list = list->tail;
    numArgs++;
  }
  node->u.blockNode.numArgs = numArgs;
  /* statements */
  list = node->u.blockNode.statements;
  while (list != NULL) {
    checkNode(list->head);
    list = list->tail;
  }
}


static void checkPrim(Node *node) {
  List *list;
  int numArgs;

  /* arguments */
  list = node->u.primNode.arguments;
  numArgs = 0;
  while (list != NULL) {
    checkNode(list->head);
    list = list->tail;
    numArgs++;
  }
  node->u.primNode.numArgs = numArgs;
}


static void checkNode(Node *node) {
  if (node == NULL) {
    sysError("node pointer is NULL in checkNode");
  } else
  switch (node->type) {
    case Method:
      checkMethod(node);
      break;
    case Return:
      checkReturn(node);
      break;
    case Assign:
      checkAssign(node);
      break;
    case Cascade:
      checkCascade(node);
      break;
    case Message:
      checkMessage(node);
      break;
    case Var:
      checkVar(node);
      break;
    case Int:
      checkInt(node);
      break;
    case Float:
      checkFloat(node);
      break;
    case Char:
      checkChar(node);
      break;
    case String:
      checkString(node);
      break;
    case Symbol:
      checkSymbol(node);
      break;
    case Array:
      checkArray(node);
      break;
    case Block:
      checkBlock(node);
      break;
    case Prim:
      checkPrim(node);
      break;
    default:
      sysError("unknown node type %d in checkNode", node->type);
      break;
  }
}


void check(Node *method, ObjPtr class) {
  initVariables(class);
  checkNode(method);
  computeOffsets(method);
}


/**************************************************************/


static void showVariable(Variable *variable) {
  switch (variable->type) {
    case Self:
    case Super:
    case Nil:
    case False:
    case True:
      printf("P:");
      break;
    case Instance:
      printf("I%02d:", variable->offset);
      break;
    case Argument:
      printf("A%02d:", variable->offset);
      break;
    case Temporary:
      printf("T%02d:", variable->offset);
      break;
    case Global:
      printf("G:");
      break;
    default:
      sysError("showVariable has illegal variable type");
      break;
  }
  printf(" %s", variable->name);
}


void showVariables(void) {
  Variable *variable;

  variable = allVariables;
  printf("VARIABLES = {\n");
  while (variable != NULL) {
    printf("  ");
    showVariable(variable);
    printf("\n");
    variable = variable->next;
  }
  printf("}\n");
}


/**************************************************************/


void freeVariables(void) {
  Variable *next;

  while (allVariables != NULL) {
    release(allVariables->name);
    next = allVariables->next;
    release(allVariables);
    allVariables = next;
  }
}
