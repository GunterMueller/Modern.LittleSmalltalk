%{

/*
 * parser.y -- MLS parser specification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "utils.h"
#include "tree.h"
#include "parser.h"
#include "scanner.h"
#include "ui.h"

Node *method;

%}

%union {
  NoVal noVal;
  IntVal intVal;
  FloatVal floatVal;
  CharVal charVal;
  StringVal stringVal;
  List *list;
  Node *node;
}

%token	<noVal>		LPAREN RPAREN LBRACK RBRACK
%token	<noVal>		ASSIGN BAR CARET HASH SEMIC DOT
%token	<noVal>		PRIMBGN PRIMEND
%token	<intVal>	INTLIT
%token	<floatVal>	FLTLIT
%token	<charVal>	CHRLIT
%token	<stringVal>	STRLIT
%token	<stringVal>	BINSEL
%token	<stringVal>	IDENT
%token	<stringVal>	KEYWORD
%token	<stringVal>	KEYWORDS
%token	<stringVal>	COLONVAR

%type	<node>		messagePattern
%type	<node>		keywordPattern binaryPattern unaryPattern
%type	<list>		temporaries tempList statements
%type	<node>		expression cascadedExpr
%type	<list>		cascade
%type	<node>		keywordCont keywordContAux binaryCont unaryCont
%type	<node>		keywordExpr keywordExprAux binaryExpr unaryExpr
%type	<node>		primary literal symbol array
%type	<list>		elements
%type	<node>		block
%type	<list>		blockArgs blockArgList
%type	<node>		primitive
%type	<list>		primaries

%start			method


%%


method			: messagePattern temporaries statements
			  {
			    $1->u.methodNode.temporaries = $2;
			    $1->u.methodNode.statements = $3;
			    method = $1;
			  }
			;

messagePattern		: keywordPattern
			  {
			    $$ = $1;
			  }
			| binaryPattern
			  {
			    $$ = $1;
			  }
			| unaryPattern
			  {
			    $$ = $1;
			  }
			;

keywordPattern		: KEYWORD IDENT
			  {
			    $$ = mkMethod($1.line,
			                  $1.val,
			                  mkList(mkVar($2.line, $2.val),
			                         NULL),
			                  NULL,
			                  NULL);
			  }
			| KEYWORD IDENT keywordPattern
			  {
			    $3->line = $1.line;
			    $3->u.methodNode.selector =
			      concat($1.val, $3->u.methodNode.selector);
			    $3->u.methodNode.parameters =
			      mkList(mkVar($2.line, $2.val),
			             $3->u.methodNode.parameters);
			    $$ = $3;
			  }
			;

binaryPattern		: BINSEL IDENT
			  {
			    $$ = mkMethod($1.line,
			                  $1.val,
			                  mkList(mkVar($2.line, $2.val),
			                         NULL),
			                  NULL,
			                  NULL);
			  }
			;

unaryPattern		: IDENT
			  {
			    $$ = mkMethod($1.line,
			                  $1.val,
			                  NULL,
			                  NULL,
			                  NULL);
			  }
			;

temporaries		: /* empty */
			  {
			    $$ = NULL;
			  }
			| BAR tempList BAR
			  {
			    $$ = $2;
			  }
			;

tempList		: /* empty */
			  {
			    $$ = NULL;
			  }
			| IDENT tempList
			  {
			    $$ = mkList(mkVar($1.line, $1.val), $2);
			  }
			;

statements		: /* empty */
			  {
			    $$ = NULL;
			  }
			| CARET expression
			  {
			    $$ = mkList(mkReturn($1.line, $2), NULL);
			  }
			| expression
			  {
			    $$ = mkList($1, NULL);
			  }
			| expression DOT statements
			  {
			    $$ = mkList($1, $3);
			  }
			;

expression		: IDENT ASSIGN expression
			  {
			    $$ = mkAssign($2.line,
			                  mkVar($1.line, $1.val),
			                  $3);
			  }
			| cascadedExpr
			  {
			    $$ = $1;
			  }
			;

cascadedExpr		: keywordExpr
			  {
			    $$ = $1;
			  }
			| keywordExpr cascade
			  {
			    $$ = mkCascade($1->line, $1, $2);
			  }
			;

cascade			: SEMIC keywordCont
			  {
			    $$ = mkList($2, NULL);
			  }
			| SEMIC keywordCont cascade
			  {
			    $$ = mkList($2, $3);
			  }
			;

keywordCont		: binaryCont
			  {
			    $$ = $1;
			  }
			| binaryCont keywordContAux
			  {
			    $2->u.messageNode.receiver = $1;
			    $$ = $2;
			  }
			;

keywordContAux		: KEYWORD binaryExpr
			  {
			    $$ = mkMessage($1.line,
			                   $1.val,
			                   NULL,
			                   mkList($2, NULL));
			  }
			| KEYWORD binaryExpr keywordContAux
			  {
			    $3->line = $1.line;
			    $3->u.messageNode.selector =
			      concat($1.val, $3->u.messageNode.selector);
			    $3->u.messageNode.arguments =
			      mkList($2, $3->u.messageNode.arguments);
			    $$ = $3;
			  }
			;

binaryCont		: unaryCont
			  {
			    $$ = $1;
			  }
			| binaryCont BINSEL unaryExpr
			  {
			    $$ = mkMessage($2.line,
			                   $2.val,
			                   $1,
			                   mkList($3, NULL));
			  }
			;

unaryCont		: /* empty */
			  {
			    $$ = NULL;
			  }
			| unaryCont IDENT
			  {
			    $$ = mkMessage($2.line,
			                   $2.val,
			                   $1,
			                   NULL);
			  }
			;

keywordExpr		: binaryExpr
			  {
			    $$ = $1;
			  }
			| binaryExpr keywordExprAux
			  {
			    $2->u.messageNode.receiver = $1;
			    $$ = $2;
			  }
			;

keywordExprAux		: KEYWORD binaryExpr
			  {
			    $$ = mkMessage($1.line,
			                   $1.val,
			                   NULL,
			                   mkList($2, NULL));
			  }
			| KEYWORD binaryExpr keywordExprAux
			  {
			    $3->line = $1.line;
			    $3->u.messageNode.selector =
			      concat($1.val, $3->u.messageNode.selector);
			    $3->u.messageNode.arguments =
			      mkList($2, $3->u.messageNode.arguments);
			    $$ = $3;
			  }
			;

binaryExpr		: unaryExpr
			  {
			    $$ = $1;
			  }
			| binaryExpr BINSEL unaryExpr
			  {
			    $$ = mkMessage($2.line,
			                   $2.val,
			                   $1,
			                   mkList($3, NULL));
			  }
			;

unaryExpr		: primary
			  {
			    $$ = $1;
			  }
			| unaryExpr IDENT
			  {
			    $$ = mkMessage($2.line,
			                   $2.val,
			                   $1,
			                   NULL);
			  }
			;

primary			: IDENT
			  {
			    $$ = mkVar($1.line, $1.val);
			  }
			| literal
			  {
			    $$ = $1;
			  }
			| block
			  {
			    $$ = $1;
			  }
			| primitive
			  {
			    $$ = $1;
			  }
			| LPAREN expression RPAREN
			  {
			    $$ = $2;
			  }
			;

literal			: INTLIT
			  {
			    $$ = mkInt($1.line, $1.val);
			  }
			| FLTLIT
			  {
			    $$ = mkFloat($1.line, $1.val);
			  }
			| CHRLIT
			  {
			    $$ = mkChar($1.line, $1.val);
			  }
			| STRLIT
			  {
			    $$ = mkString($1.line, $1.val);
			  }
			| HASH symbol
			  {
			    $2->line = $1.line;
			    $$ = $2;
			  }
			| HASH array
			  {
			    $2->line = $1.line;
			    $$ = $2;
			  }
			;

symbol			: IDENT
			  {
			    $$ = mkSymbol($1.line, $1.val);
			  }
			| BINSEL
			  {
			    $$ = mkSymbol($1.line, $1.val);
			  }
			| KEYWORD
			  {
			    $$ = mkSymbol($1.line, $1.val);
			  }
			| KEYWORDS
			  {
			    $$ = mkSymbol($1.line, $1.val);
			  }
			;

array			: LPAREN elements RPAREN
			  {
			    $$ = mkArray($1.line, $2);
			  }
			;

elements		: /* empty */
			  {
			    $$ = NULL;
			  }
			| INTLIT elements
			  {
			    $$ = mkList(mkInt($1.line, $1.val), $2);
			  }
			| FLTLIT elements
			  {
			    $$ = mkList(mkFloat($1.line, $1.val), $2);
			  }
			| CHRLIT elements
			  {
			    $$ = mkList(mkChar($1.line, $1.val), $2);
			  }
			| STRLIT elements
			  {
			    $$ = mkList(mkString($1.line, $1.val), $2);
			  }
			| symbol elements
			  {
			    $$ = mkList($1, $2);
			  }
			| array elements
			  {
			    $$ = mkList($1, $2);
			  }
			;

block			: LBRACK blockArgs statements RBRACK
			  {
			    $$ = mkBlock($1.line, $2, $3);
			  }
			;

blockArgs		: /* empty */
			  {
			    $$ = NULL;
			  }
			| blockArgList BAR
			  {
			    $$ = $1;
			  }
			;

blockArgList		: /* empty */
			  {
			    $$ = NULL;
			  }
			| COLONVAR blockArgList
			  {
			    $$ = mkList(mkVar($1.line, $1.val), $2);
			  }
			;

primitive		: PRIMBGN INTLIT primaries PRIMEND
			  {
			    $$ = mkPrim($1.line, $2.val, $3);
			  }
			;

primaries		: /* empty */
			  {
			    $$ = NULL;
			  }
			| primary primaries
			  {
			    $$ = mkList($1, $2);
			  }
			;


%%


void yyerror(char *msg) {
  compError("%s in line %d", msg, yylval.noVal.line);
}


char *concat(char *s1, char *s2) {
  char *s;

  s = allocate(strlen(s1) + strlen(s2) + 1);
  strcpy(s, s1);
  strcat(s, s2);
  release(s1);
  release(s2);
  return s;
}
