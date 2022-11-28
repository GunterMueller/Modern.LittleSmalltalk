/*
 * tree.h -- abstract syntax tree
 */


#ifndef _TREE_H_
#define _TREE_H_


typedef enum {
  Self, Super, Nil, False, True,
  Instance, Argument, Temporary, Global
} VarType;

typedef struct variable {
  struct variable *next;
  VarType type;
  char *name;
  int offset;
} Variable;


typedef struct list {
  struct node *head;
  struct list *tail;
} List;


typedef enum {
  Method, Return, Assign, Cascade, Message, Var,
  Int, Float, Char, String, Symbol, Array, Block, Prim
} NodeType;

typedef struct node {
  NodeType type;
  int line;
  union {
    struct {
      char *selector;
      struct list *parameters;
      struct list *temporaries;
      struct list *statements;
      /* following fields filled in by semantic analysis */
      int numArgs;
      int numTemps;
    } methodNode;
    struct {
      struct node *expression;
    } returnNode;
    struct {
      struct node *lhs;
      struct node *rhs;
    } assignNode;
    struct {
      struct node *receiver;
      struct list *continuations;
    } cascadeNode;
    struct {
      char *selector;
      struct node *receiver;		/* may be NULL in cascades */
      struct list *arguments;
      /* following fields filled in by semantic analysis */
      Bool superFlag;
      int numArgs;
    } messageNode;
    struct {
      char *name;
      /* following fields filled in by semantic analysis */
      Variable *var;
    } varNode;
    struct {
      int val;
    } intNode;
    struct {
      double val;
    } floatNode;
    struct {
      char val;
    } charNode;
    struct {
      char *val;
    } stringNode;
    struct {
      char *name;
    } symbolNode;
    struct {
      struct list *elements;
      /* following fields filled in by semantic analysis */
      int numElems;
    } arrayNode;
    struct {
      struct list *arguments;
      struct list *statements;
      /* following fields filled in by semantic analysis */
      int numArgs;
    } blockNode;
    struct {
      int number;
      struct list *arguments;
      /* following fields filled in by semantic analysis */
      int numArgs;
    } primNode;
  } u;
} Node;


List *mkList(Node *head, List *tail);

Node *mkMethod(int line, char *selector, List *parameters,
               List *temporaries, List *statements);
Node *mkReturn(int line, Node *expression);
Node *mkAssign(int line, Node *lhs, Node *rhs);
Node *mkCascade(int line, Node *receiver, List *continuations);
Node *mkMessage(int line, char *selector,
                Node *receiver, List *arguments);
Node *mkVar(int line, char *name);
Node *mkInt(int line, int val);
Node *mkFloat(int line, double val);
Node *mkChar(int line, char val);
Node *mkString(int line, char *val);
Node *mkSymbol(int line, char *name);
Node *mkArray(int line, List *elements);
Node *mkBlock(int line, List *arguments, List *statements);
Node *mkPrim(int line, int number, List *arguments);

void showTree(Node *tree);
void freeTree(Node *tree);


#endif /* _TREE_H_ */
