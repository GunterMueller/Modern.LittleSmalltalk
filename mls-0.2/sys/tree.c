/*
 * tree.c -- abstract syntax tree
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "utils.h"
#include "tree.h"
#include "ui.h"


/**************************************************************/


List *mkList(Node *head, List *tail) {
  List *list;

  list = allocate(sizeof(List));
  list->head = head;
  list->tail = tail;
  return list;
}


Node *mkMethod(int line, char *selector, List *parameters,
               List *temporaries, List *statements) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Method;
  node->line = line;
  node->u.methodNode.selector = selector;
  node->u.methodNode.parameters = parameters;
  node->u.methodNode.temporaries = temporaries;
  node->u.methodNode.statements = statements;
  return node;
}


Node *mkReturn(int line, Node *expression) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Return;
  node->line = line;
  node->u.returnNode.expression = expression;
  return node;
}


Node *mkAssign(int line, Node *lhs, Node *rhs) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Assign;
  node->line = line;
  node->u.assignNode.lhs = lhs;
  node->u.assignNode.rhs = rhs;
  return node;
}


Node *mkCascade(int line, Node *receiver, List *continuations) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Cascade;
  node->line = line;
  node->u.cascadeNode.receiver = receiver;
  node->u.cascadeNode.continuations = continuations;
  return node;
}


Node *mkMessage(int line, char *selector,
                Node *receiver, List *arguments) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Message;
  node->line = line;
  node->u.messageNode.selector = selector;
  node->u.messageNode.receiver = receiver;
  node->u.messageNode.arguments = arguments;
  return node;
}


Node *mkVar(int line, char *name) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Var;
  node->line = line;
  node->u.varNode.name = name;
  return node;
}


Node *mkInt(int line, int val) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Int;
  node->line = line;
  node->u.intNode.val = val;
  return node;
}


Node *mkFloat(int line, double val) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Float;
  node->line = line;
  node->u.floatNode.val = val;
  return node;
}


Node *mkChar(int line, char val) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Char;
  node->line = line;
  node->u.charNode.val = val;
  return node;
}


Node *mkString(int line, char *val) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = String;
  node->line = line;
  node->u.stringNode.val = val;
  return node;
}


Node *mkSymbol(int line, char *name) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Symbol;
  node->line = line;
  node->u.symbolNode.name = name;
  return node;
}


Node *mkArray(int line, List *elements) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Array;
  node->line = line;
  node->u.arrayNode.elements = elements;
  return node;
}


Node *mkBlock(int line, List *arguments, List *statements) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Block;
  node->line = line;
  node->u.blockNode.arguments = arguments;
  node->u.blockNode.statements = statements;
  return node;
}


Node *mkPrim(int line, int number, List *arguments) {
  Node *node;

  node = allocate(sizeof(Node));
  node->type = Prim;
  node->line = line;
  node->u.primNode.number = number;
  node->u.primNode.arguments = arguments;
  return node;
}


/**************************************************************/


static void indent(int n) {
  int i;

  for (i = 0; i < n; i++) {
    printf("  ");
  }
}


static void say(char *s) {
  printf("%s", s);
}


static void sayInt(int i) {
  printf("%d", i);
}


static void sayFloat(double d) {
  printf("%e", d);
}


static void sayChar(char c) {
  printf("$%c", c);
}


static void sayString(char *s) {
  printf("'%s'", s);
}


static void saySymbol(char *s) {
  printf("#%s", s);
}


static void showNode(Node *node, int indent);


static void showList(List *list, int n) {
  indent(n);
  if (list == NULL) {
    say("--empty--");
  } else {
    say("List(\n");
    do {
      showNode(list->head, n + 1);
      list = list->tail;
      if (list != NULL) {
        say(",\n");
      } else {
        say(")");
      }
    } while (list != NULL);
  }
}


static void showMethod(Node *node, int n) {
  indent(n);
  say("Method(\n");
  indent(n + 1);
  sayString(node->u.methodNode.selector);
  say(",\n");
  showList(node->u.methodNode.parameters, n + 1);
  say(",\n");
  showList(node->u.methodNode.temporaries, n + 1);
  say(",\n");
  showList(node->u.methodNode.statements, n + 1);
  say(")");
}


static void showReturn(Node *node, int n) {
  indent(n);
  say("Return(\n");
  showNode(node->u.returnNode.expression, n + 1);
  say(")");
}


static void showAssign(Node *node, int n) {
  indent(n);
  say("Assign(\n");
  showNode(node->u.assignNode.lhs, n + 1);
  say(",\n");
  showNode(node->u.assignNode.rhs, n + 1);
  say(")");
}


static void showCascade(Node *node, int n) {
  indent(n);
  say("Cascade(\n");
  showNode(node->u.cascadeNode.receiver, n + 1);
  say(",\n");
  showList(node->u.cascadeNode.continuations, n + 1);
  say(")");
}


static void showMessage(Node *node, int n) {
  indent(n);
  say("Message(\n");
  indent(n + 1);
  sayString(node->u.messageNode.selector);
  say(",\n");
  if (node->u.messageNode.receiver == NULL) {
    indent(n + 1);
    say("--none--");
  } else {
    showNode(node->u.messageNode.receiver, n + 1);
  }
  say(",\n");
  showList(node->u.messageNode.arguments, n + 1);
  say(")");
}


static void showVar(Node *node, int n) {
  indent(n);
  say("Var(");
  sayString(node->u.varNode.name);
  say(")");
}


static void showInt(Node *node, int n) {
  indent(n);
  say("Int(");
  sayInt(node->u.intNode.val);
  say(")");
}


static void showFloat(Node *node, int n) {
  indent(n);
  say("Float(");
  sayFloat(node->u.floatNode.val);
  say(")");
}


static void showChar(Node *node, int n) {
  indent(n);
  say("Char(");
  sayChar(node->u.charNode.val);
  say(")");
}


static void showString(Node *node, int n) {
  indent(n);
  say("String(");
  sayString(node->u.stringNode.val);
  say(")");
}


static void showSymbol(Node *node, int n) {
  indent(n);
  say("Symbol(");
  saySymbol(node->u.symbolNode.name);
  say(")");
}


static void showArray(Node *node, int n) {
  indent(n);
  say("Array(\n");
  showList(node->u.arrayNode.elements, n + 1);
  say(")");
}


static void showBlock(Node *node, int n) {
  indent(n);
  say("Block(\n");
  showList(node->u.blockNode.arguments, n + 1);
  say(",\n");
  showList(node->u.blockNode.statements, n + 1);
  say(")");
}


static void showPrim(Node *node, int n) {
  indent(n);
  say("Prim(\n");
  indent(n + 1);
  sayInt(node->u.primNode.number);
  say(",\n");
  showList(node->u.primNode.arguments, n + 1);
  say(")");
}


static void showNode(Node *node, int indent) {
  if (node == NULL) {
    sysError("node pointer is NULL in showNode");
  } else
  switch (node->type) {
    case Method:
      showMethod(node, indent);
      break;
    case Return:
      showReturn(node, indent);
      break;
    case Assign:
      showAssign(node, indent);
      break;
    case Cascade:
      showCascade(node, indent);
      break;
    case Message:
      showMessage(node, indent);
      break;
    case Var:
      showVar(node, indent);
      break;
    case Int:
      showInt(node, indent);
      break;
    case Float:
      showFloat(node, indent);
      break;
    case Char:
      showChar(node, indent);
      break;
    case String:
      showString(node, indent);
      break;
    case Symbol:
      showSymbol(node, indent);
      break;
    case Array:
      showArray(node, indent);
      break;
    case Block:
      showBlock(node, indent);
      break;
    case Prim:
      showPrim(node, indent);
      break;
    default:
      sysError("unknown node type %d in showNode", node->type);
      break;
  }
}


void showTree(Node *tree) {
  showNode(tree, 0);
  printf("\n");
}


/**************************************************************/


static void freeList(List *list) {
  List *next;

  while (list != NULL) {
    freeTree(list->head);
    next = list->tail;
    release(list);
    list = next;
  }
}


void freeTree(Node *node) {
  if (node == NULL) {
    sysError("node pointer is NULL in freeTree");
  } else
  switch (node->type) {
    case Method:
      release(node->u.methodNode.selector);
      freeList(node->u.methodNode.parameters);
      freeList(node->u.methodNode.temporaries);
      freeList(node->u.methodNode.statements);
      break;
    case Return:
      freeTree(node->u.returnNode.expression);
      break;
    case Assign:
      freeTree(node->u.assignNode.lhs);
      freeTree(node->u.assignNode.rhs);
      break;
    case Cascade:
      freeTree(node->u.cascadeNode.receiver);
      freeList(node->u.cascadeNode.continuations);
      break;
    case Message:
      release(node->u.messageNode.selector);
      if (node->u.messageNode.receiver != NULL) {
        freeTree(node->u.messageNode.receiver);
      }
      freeList(node->u.messageNode.arguments);
      break;
    case Var:
      release(node->u.varNode.name);
      /* variable records proper are freed separately */
      break;
    case Int:
      /* nothing to do here */
      break;
    case Float:
      /* nothing to do here */
      break;
    case Char:
      /* nothing to do here */
      break;
    case String:
      release(node->u.stringNode.val);
      break;
    case Symbol:
      release(node->u.stringNode.val);
      break;
    case Array:
      freeList(node->u.arrayNode.elements);
      break;
    case Block:
      freeList(node->u.blockNode.arguments);
      freeList(node->u.blockNode.statements);
      break;
    case Prim:
      freeList(node->u.primNode.arguments);
      break;
    default:
      sysError("unknown node type %d in freeTree", node->type);
      break;
  }
  release(node);
}
