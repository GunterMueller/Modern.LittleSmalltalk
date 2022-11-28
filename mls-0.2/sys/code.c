/*
 * code.c -- code generator
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "machine.h"
#include "objects.h"
#include "memory.h"
#include "tree.h"
#include "code.h"
#include "ui.h"


/**************************************************************/


static int maxStacksize;
static int currentStacksize;


static void updateStack(int stackChange) {
  currentStacksize += stackChange;
  if (currentStacksize < 0) {
    sysError("coder tried to code a pop from an empty stack");
  }
  if (currentStacksize > maxStacksize) {
    maxStacksize = currentStacksize;
  }
}


/**************************************************************/


#define MAX_LITERALS		1000

static Node *literalArray[MAX_LITERALS];
static int literalSize;


static Word makeLiteral(Node *literal) {
  if (literalSize == MAX_LITERALS) {
    sysError("too many literals in method");
  }
  literalArray[literalSize] = literal;
  return literalSize++;
}


/**************************************************************/


#define MAX_INSTRS		20000

static Word instrArray[MAX_INSTRS];
static int instrSize;


static void codeInstr(Byte opcode,
                      Byte arg1,
                      Word arg2,
                      int stackChange) {
  if (instrSize == MAX_INSTRS) {
    sysError("too many instructions in method");
  }
  instrArray[instrSize++] = ((Word) opcode << 24) |
                            ((Word) arg1 << 16) |
                            ((Word) arg2 << 0);
  updateStack(stackChange);
}


static int getCurrentLocation(void) {
  return instrSize;
}


static void patchOperand2(int where, int value) {
  instrArray[where] |= value;
}


/**************************************************************/


static void codeLoad(Node *varNode) {
  Variable *variable;

  variable = varNode->u.varNode.var;
  switch (variable->type) {
    case Self:
      codeInstr(OP_PUSHSELF, 0, 0, 1);
      break;
    case Super:
      codeInstr(OP_PUSHSELF, 0, 0, 1);
      break;
    case Nil:
      codeInstr(OP_PUSHNIL, 0, 0, 1);
      break;
    case False:
      codeInstr(OP_PUSHFALSE, 0, 0, 1);
      break;
    case True:
      codeInstr(OP_PUSHTRUE, 0, 0, 1);
      break;
    case Instance:
      codeInstr(OP_PUSHINST, variable->offset, 0, 1);
      break;
    case Argument:
      codeInstr(OP_PUSHARG, variable->offset, 0, 1);
      break;
    case Temporary:
      codeInstr(OP_PUSHTEMP, variable->offset, 0, 1);
      break;
    case Global:
      codeInstr(OP_PUSHGLOB, 0, makeLiteral(varNode), 1);
      break;
    default:
      sysError("codeLoad has illegal variable type %d", variable->type);
      break;
  }
}


static void codeStore(Node *varNode) {
  Variable *variable;

  variable = varNode->u.varNode.var;
  switch (variable->type) {
    case Instance:
      codeInstr(OP_STOREINST, variable->offset, 0, -1);
      break;
    case Temporary:
      codeInstr(OP_STORETEMP, variable->offset, 0, -1);
      break;
    case Global:
      codeInstr(OP_STOREGLOB, 0, makeLiteral(varNode), -1);
      break;
    default:
      sysError("codeStore has illegal variable type %d", variable->type);
      break;
  }
}


static void codeNode(Node *node, Bool valueNeeded);


static void codeMethod(Node *node, Bool valueNeeded) {
  List *statements;
  Node *statement;

  /* If valueNeeded is true, then return the value of the last
     expression (instead of returning self). This is needed in
     case of interactively evaluating an expression. */
  statements = node->u.methodNode.statements;
  if (statements == NULL) {
    codeInstr(OP_PUSHSELF, 0, 0, 1);
    codeInstr(OP_RETMSG, 0, 0, -1);
  } else {
    while (statements->tail != NULL) {
      statement = statements->head;
      codeNode(statement, false);
      statements = statements->tail;
    }
    statement = statements->head;
    if (statement->type == Return) {
      codeNode(statement, false);
    } else {
      if (valueNeeded) {
        codeNode(statement, true);
        codeInstr(OP_RETMSG, 0, 0, -1);
      } else {
        codeNode(statement, false);
        codeInstr(OP_PUSHSELF, 0, 0, 1);
        codeInstr(OP_RETMSG, 0, 0, -1);
      }
    }
  }
}


static void codeReturn(Node *node, Bool valueNeeded) {
  if (valueNeeded) {
    sysError("valueNeeded in codeReturn");
  }
  codeNode(node->u.returnNode.expression, true);
  codeInstr(OP_RETMSG, 0, 0, -1);
}


static void codeAssign(Node *node, Bool valueNeeded) {
  codeNode(node->u.assignNode.rhs, true);
  if (valueNeeded) {
    codeInstr(OP_DUP, 0, 0, 1);
  }
  codeStore(node->u.assignNode.lhs);
}


static void codeCascade(Node *node, Bool valueNeeded) {
  List *continuations;

  codeNode(node->u.cascadeNode.receiver, true);
  continuations = node->u.cascadeNode.continuations;
  while (continuations->tail != NULL) {
    codeInstr(OP_DUP, 0, 0, 1);
    codeNode(continuations->head, false);
    continuations = continuations->tail;
  }
  if (valueNeeded) {
    codeInstr(OP_DUP, 0, 0, 1);
  }
  codeNode(continuations->head, false);
}


static void codeMessage(Node *node, Bool valueNeeded) {
  List *arguments;

  /* ATTENTION: if we have a null receiver (a cascaded message)
     then do nothing since the receiver has already been pushed */
  if (node->u.messageNode.receiver != NULL) {
    codeNode(node->u.messageNode.receiver, true);
  }
  arguments = node->u.messageNode.arguments;
  while (arguments != NULL) {
    codeNode(arguments->head, true);
    arguments = arguments->tail;
  }
  if (node->u.messageNode.superFlag) {
    codeInstr(OP_SENDSUPER,
              node->u.messageNode.numArgs,
              makeLiteral(node),
              -node->u.messageNode.numArgs);
  } else {
    codeInstr(OP_SEND,
              node->u.messageNode.numArgs,
              makeLiteral(node),
              -node->u.messageNode.numArgs);
  }
  if (!valueNeeded) {
    codeInstr(OP_DROP, 0, 0, -1);
  }
}


static void codeVar(Node *node, Bool valueNeeded) {
  if (valueNeeded) {
    codeLoad(node);
  }
}


static void codeInt(Node *node, Bool valueNeeded) {
  if (valueNeeded) {
    codeInstr(OP_PUSHCONST, 0, makeLiteral(node), 1);
  }
}


static void codeFloat(Node *node, Bool valueNeeded) {
  if (valueNeeded) {
    codeInstr(OP_PUSHCONST, 0, makeLiteral(node), 1);
  }
}


static void codeChar(Node *node, Bool valueNeeded) {
  if (valueNeeded) {
    codeInstr(OP_PUSHCONST, 0, makeLiteral(node), 1);
  }
}


static void codeString(Node *node, Bool valueNeeded) {
  if (valueNeeded) {
    codeInstr(OP_PUSHCONST, 0, makeLiteral(node), 1);
  }
}


static void codeSymbol(Node *node, Bool valueNeeded) {
  if (valueNeeded) {
    codeInstr(OP_PUSHCONST, 0, makeLiteral(node), 1);
  }
}


static void codeArray(Node *node, Bool valueNeeded) {
  if (valueNeeded) {
    codeInstr(OP_PUSHCONST, 0, makeLiteral(node), 1);
  }
}


static void codeBlock(Node *node, Bool valueNeeded) {
  List *arguments;
  Node *argument;
  List *statements;
  Node *statement;
  int location1, location2;
  int saveMaxStacksize, saveCurrentStacksize;

  if (valueNeeded) {
    location1 = getCurrentLocation();
    codeInstr(OP_PUSHBLK, node->u.blockNode.numArgs, 0, 1);
    location2 = getCurrentLocation();
    codeInstr(OP_JUMP, 0, 0, 0);
    saveMaxStacksize = maxStacksize;
    saveCurrentStacksize = currentStacksize;
    maxStacksize = 0;
    currentStacksize = 0;
    updateStack(node->u.blockNode.numArgs);
    arguments = node->u.blockNode.arguments;
    while (arguments != NULL) {
      argument = arguments->head;
      codeStore(argument);
      arguments = arguments->tail;
    }
    statements = node->u.blockNode.statements;
    if (statements == NULL) {
      codeInstr(OP_PUSHNIL, 0, 0, 1);
      codeInstr(OP_RETBLK, 0, 0, -1);
    } else {
      while (statements->tail != NULL) {
        statement = statements->head;
        codeNode(statement, false);
        statements = statements->tail;
      }
      statement = statements->head;
      if (statement->type == Return) {
        codeNode(statement, false);
      } else {
        codeNode(statement, true);
        codeInstr(OP_RETBLK, 0, 0, -1);
      }
    }
    patchOperand2(location1, maxStacksize);
    maxStacksize = saveMaxStacksize;
    currentStacksize = saveCurrentStacksize;
    patchOperand2(location2, getCurrentLocation());
  }
}


static void codePrim(Node *node, Bool valueNeeded) {
  List *arguments;

  arguments = node->u.primNode.arguments;
  while (arguments != NULL) {
    codeNode(arguments->head, true);
    arguments = arguments->tail;
  }
  codeInstr(OP_PRIM,
            node->u.primNode.numArgs,
            node->u.primNode.number,
            1 - node->u.primNode.numArgs);
  if (!valueNeeded) {
    codeInstr(OP_DROP, 0, 0, -1);
  }
}


static void codeNode(Node *node, Bool valueNeeded) {
  if (node == NULL) {
    sysError("node pointer is NULL in codeNode");
  } else
  switch (node->type) {
    case Method:
      codeMethod(node, valueNeeded);
      break;
    case Return:
      codeReturn(node, valueNeeded);
      break;
    case Assign:
      codeAssign(node, valueNeeded);
      break;
    case Cascade:
      codeCascade(node, valueNeeded);
      break;
    case Message:
      codeMessage(node, valueNeeded);
      break;
    case Var:
      codeVar(node, valueNeeded);
      break;
    case Int:
      codeInt(node, valueNeeded);
      break;
    case Float:
      codeFloat(node, valueNeeded);
      break;
    case Char:
      codeChar(node, valueNeeded);
      break;
    case String:
      codeString(node, valueNeeded);
      break;
    case Symbol:
      codeSymbol(node, valueNeeded);
      break;
    case Array:
      codeArray(node, valueNeeded);
      break;
    case Block:
      codeBlock(node, valueNeeded);
      break;
    case Prim:
      codePrim(node, valueNeeded);
      break;
    default:
      sysError("unknown node type %d in codeNode", node->type);
      break;
  }
}


static ObjPtr codeLiteral(Node *node) {
  ObjPtr literal;
  Variable *variable;
  ObjPtr link, array;
  List *elements;
  int i;

  if (node == NULL) {
    sysError("node pointer is NULL in codeLiteral");
  } else
  switch (node->type) {
    case Message:
      /* here, the message's selector is to be coded */
      literal = newSymbol(node->u.messageNode.selector);
      break;
    case Var:
      /* only global variables can appear here */
      variable = node->u.varNode.var;
      if (variable->type != Global) {
        sysError("non-global variable in codeLiteral");
      }
      literal = lookupGlobal(variable->name);
      if (literal == machine.nil) {
        compError("global variable '%s' not found", variable->name);
      }
      break;
    case Int:
      literal = newShortInteger(node->u.intNode.val);
      break;
    case Float:
      literal = newFloat(node->u.floatNode.val);
      break;
    case Char:
      literal = newCharacter(node->u.charNode.val);
      break;
    case String:
      literal = newString(node->u.stringNode.val);
      break;
    case Symbol:
      literal = newSymbol(node->u.symbolNode.name);
      break;
    case Array:
      /* Arrays must be constructed recursively. Use
         machine.compilerLiteral as temporary stack. */
      link = createObject(machine.Link, SIZE_OF_LINK, true, false);
      setPtr(link, NEXT_IN_LINK, machine.compilerLiteral);
      machine.compilerLiteral = link;
      array =
        createObject(machine.Array, node->u.arrayNode.numElems, true, false);
      setPtr(machine.compilerLiteral, VALUE_IN_LINK, array);
      elements = node->u.arrayNode.elements;
      i = 0;
      while (elements != NULL) {
        literal = codeLiteral(elements->head);
        setPtr(getPtr(machine.compilerLiteral, VALUE_IN_LINK), i, literal);
        elements = elements->tail;
        i++;
      }
      literal =
        getPtr(machine.compilerLiteral, VALUE_IN_LINK);
      machine.compilerLiteral =
        getPtr(machine.compilerLiteral, NEXT_IN_LINK);
      break;
    default:
      sysError("unknown node type %d in codeLiteral", node->type);
      break;
  }
  return literal;
}


void code(char *text, Node *method, ObjPtr class, Bool valueNeeded) {
  ObjPtr aux;
  int i;
  ObjPtr literal;

  instrSize = 0;
  literalSize = 0;
  maxStacksize = 0;
  currentStacksize = 0;
  codeNode(method, valueNeeded);
  machine.compilerMethod = class;
  aux = createObject(machine.Method, SIZE_OF_METHOD, true, false);
  setPtr(aux, CLASS_IN_METHOD, machine.compilerMethod);
  machine.compilerMethod = aux;
  aux = newString(text);
  setPtr(machine.compilerMethod, TEXT_IN_METHOD, aux);
  aux = newSymbol(method->u.methodNode.selector);
  setPtr(machine.compilerMethod, SELECTOR_IN_METHOD, aux);
  if (instrSize != 0) {
    aux = createObject(machine.WordArray, instrSize, false, true);
    setPtr(machine.compilerMethod, CODE_IN_METHOD, aux);
    for (i = 0; i < instrSize; i++) {
      setWord(aux, i, instrArray[i]);
    }
  } else {
    setPtr(machine.compilerMethod, CODE_IN_METHOD, machine.nil);
  }
  if (literalSize != 0) {
    aux = createObject(machine.Array, literalSize, true, false);
    setPtr(machine.compilerMethod, LITERALS_IN_METHOD, aux);
    for (i = 0; i < literalSize; i++) {
      literal = codeLiteral(literalArray[i]);
      aux = getPtr(machine.compilerMethod, LITERALS_IN_METHOD);
      setPtr(aux, i, literal);
    }
  } else {
    setPtr(machine.compilerMethod, LITERALS_IN_METHOD, machine.nil);
  }
  aux = newShortInteger(method->u.methodNode.numArgs);
  setPtr(machine.compilerMethod, ARGSIZE_IN_METHOD, aux);
  aux = newShortInteger(method->u.methodNode.numTemps);
  setPtr(machine.compilerMethod, TEMPSIZE_IN_METHOD, aux);
  aux = newShortInteger(maxStacksize);
  setPtr(machine.compilerMethod, STACKSIZE_IN_METHOD, aux);
}


/**************************************************************/


static void showInstr(int instrNum) {
  Word instr;
  Byte opcode;
  Byte operand1;
  Word operand2;

  instr = instrArray[instrNum];
  printf("%04X    %08X    ", instrNum, instr);
  opcode = (instr >> 24) & 0xFF;
  operand1 = (instr >> 16) & 0xFF;
  operand2 = (instr >> 0) & 0xFFFF;
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
      printf("PUSHCONST   %u", operand2);
      break;
    case OP_PUSHGLOB:
      /* push global */
      printf("PUSHGLOB    %u", operand2);
      break;
    case OP_STOREGLOB:
      /* store global */
      printf("STOREGLOB   %u", operand2);
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
      printf("SEND        %u,%u", operand1, operand2);
      break;
    case OP_SENDSUPER:
      /* send message to super */
      printf("SENDSUPER   %u,%u", operand1, operand2);
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


static void showLiteral(int litNum) {
  Node *node;
  char c;
  char *p;
  Variable *variable;

  printf("%04d    ", litNum);
  node = literalArray[litNum];
  switch (node->type) {
    case Message:
      printf("#%s\n", node->u.messageNode.selector);
      break;
    case Var:
      variable = node->u.varNode.var;
      if (variable->type != Global) {
        sysError("illegal variable type in showLiteral");
      }
      printf("Global(#%s)\n", variable->name);
      break;
    case Int:
      printf("%d\n", node->u.intNode.val);
      break;
    case Float:
      printf("%e\n", node->u.floatNode.val);
      break;
    case Char:
      c = node->u.charNode.val;
      if (c >= 0x20 && c <= 0x7E) {
        printf("$%c", c);
      } else {
        printf(".");
      }
      printf("\n");
      break;
    case String:
      p = node->u.stringNode.val;
      printf("'");
      while (*p != '\0') {
        c = *p++;
        if (c >= 0x20 && c <= 0x7E) {
          printf("%c", c);
        } else {
          printf(".");
        }
      }
      printf("'\n");
      break;
    case Symbol:
      printf("#%s\n", node->u.symbolNode.name);
      break;
    case Array:
      printf("#(...)\n");
      break;
    default:
      sysError("unknown node type %d in showLiteral", node->type);
      break;
  }
}


void showCode(void) {
  int i;

  printf("instructions:\n");
  for (i = 0; i < instrSize; i++) {
    showInstr(i);
  }
  printf("literals:\n");
  for (i = 0; i < literalSize; i++) {
    showLiteral(i);
  }
}
