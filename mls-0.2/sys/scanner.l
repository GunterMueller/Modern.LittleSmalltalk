%{

/*
 * scanner.l -- MLS scanner specification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "utils.h"
#include "tree.h"
#include "scanner.h"
#include "parser.tab.h"
#include "ui.h"

#define YY_DECL int getToken(void)

#define YY_INPUT(buf,result,max_size) { \
  char c = (*src == '\0' ? '\0' : *src++); \
  result = (c == '\0' ? YY_NULL : (buf[0] = c, 1)); \
}

extern Bool debugTokens;	/* show token stream if set */

static char *src;		/* pointer into source */
static int line;		/* current line number */

%}

L		[A-Za-z]
D		[0-9]
SPCL		(\~|\=|\<|\>|\+|\-|\*|\/|\\|\,|\@)

INT		(\-?{D}+)
FLT1		(\-?{D}+\.{D}+)
FLT2		(\-?{D}+e\-?{D}+)
FLT3		(\-?{D}+\.{D}+e\-?{D}+)
FLT		({FLT1}|{FLT2}|{FLT3})
STR		(\'([^']|\'\')*\')
BIN		({SPCL}|{SPCL}{SPCL})
ID		({L}({L}|{D})*)
KW		({ID}\:)
CV		(\:{ID})


%%


[\ \t]		{
		  /* white space: nothing returned */
		}

\n		{
		  /* newline: nothing returned */
		  line++;
		}

\"[^"]*\"	{
		  /* comment: nothing returned */
		  char *p;
		  p = yytext;
		  while (*p != '\0') {
		    if (*p == '\n') {
		      line++;
		    }
		    p++;
		  }
		}

\(		{
		  yylval.noVal.line = line;
		  return LPAREN;
		}

\)		{
		  yylval.noVal.line = line;
		  return RPAREN;
		}

\[		{
		  yylval.noVal.line = line;
		  return LBRACK;
		}

\]		{
		  yylval.noVal.line = line;
		  return RBRACK;
		}

\<\-		{
		  yylval.noVal.line = line;
		  return ASSIGN;
		}

\|		{
		  yylval.noVal.line = line;
		  return BAR;
		}

\^		{
		  yylval.noVal.line = line;
		  return CARET;
		}

\#		{
		  yylval.noVal.line = line;
		  return HASH;
		}

\;		{
		  yylval.noVal.line = line;
		  return SEMIC;
		}

\.		{
		  yylval.noVal.line = line;
		  return DOT;
		}

\<\!		{
		  yylval.noVal.line = line;
		  return PRIMBGN;
		}

\!\>		{
		  yylval.noVal.line = line;
		  return PRIMEND;
		}

{INT}		{
		  yylval.intVal.line = line;
		  yylval.intVal.val = strtoul(yytext, NULL, 10);
		  return INTLIT;
		}

{FLT}		{
		  yylval.floatVal.line = line;
		  yylval.floatVal.val = strtod(yytext, NULL);
		  return FLTLIT;
		}

\$\n		{
		  yylval.charVal.line = line++;
		  yylval.charVal.val = yytext[1];
		  return CHRLIT;
		}

\$.		{
		  yylval.charVal.line = line;
		  yylval.charVal.val = yytext[1];
		  return CHRLIT;
		}

{STR}		{
		  char *p;
		  yytext[yyleng - 1] = '\0';
		  strcpy(yytext, yytext + 1);
		  p = yytext;
		  while (*p != '\0') {
		    if (*p == '\'') {
		      strcpy(p + 1, p + 2);
		    } else
		    if (*p == '\n') {
		      line++;
		    }
		    p++;
		  }
		  yylval.stringVal.line = line;
		  yylval.stringVal.val = allocate(strlen(yytext) + 1);
		  strcpy(yylval.stringVal.val, yytext);
		  return STRLIT;
		}

{BIN}		{
		  yylval.stringVal.line = line;
		  yylval.stringVal.val = allocate(strlen(yytext) + 1);
		  strcpy(yylval.stringVal.val, yytext);
		  return BINSEL;
		}

{ID}		{
		  yylval.stringVal.line = line;
		  yylval.stringVal.val = allocate(strlen(yytext) + 1);
		  strcpy(yylval.stringVal.val, yytext);
		  return IDENT;
		}

{KW}		{
		  yylval.stringVal.line = line;
		  yylval.stringVal.val = allocate(strlen(yytext) + 1);
		  strcpy(yylval.stringVal.val, yytext);
		  return KEYWORD;
		}

{KW}{KW}+	{
		  yylval.stringVal.line = line;
		  yylval.stringVal.val = allocate(strlen(yytext) + 1);
		  strcpy(yylval.stringVal.val, yytext);
		  return KEYWORDS;
		}

{CV}		{
		  yylval.stringVal.line = line;
		  yylval.stringVal.val = allocate(strlen(yytext + 1) + 1);
		  strcpy(yylval.stringVal.val, yytext + 1);
		  return COLONVAR;
		}

.		{
		  if (yytext[0] >= 0x20 && yytext[0] <= 0x7E) {
		    compError("illegal character '%c' (0x%02X) in line %d",
		              yytext[0], yytext[0], line);
		  } else {
		    compError("illegal character '%c' (0x%02X) in line %d",
		              '.', yytext[0], line);
		  }
		}


%%


int yywrap(void) {
  return 1;
}


void initScanner(char *source) {
  src = source;
  line = 1;
}


int yylex(void) {
  int token;

  token = getToken();
  if (debugTokens) {
    showToken(token);
  }
  return token;
}


void showToken(int token) {
  printf("TOKEN = ");
  switch (token) {
    case 0:
      printf("-- EOT --");
      break;
    case LPAREN:
      printf("LPAREN in line %d", yylval.noVal.line);
      break;
    case RPAREN:
      printf("RPAREN in line %d", yylval.noVal.line);
      break;
    case LBRACK:
      printf("LBRACK in line %d", yylval.noVal.line);
      break;
    case RBRACK:
      printf("RBRACK in line %d", yylval.noVal.line);
      break;
    case ASSIGN:
      printf("ASSIGN in line %d", yylval.noVal.line);
      break;
    case BAR:
      printf("BAR in line %d", yylval.noVal.line);
      break;
    case CARET:
      printf("CARET in line %d", yylval.noVal.line);
      break;
    case HASH:
      printf("HASH in line %d", yylval.noVal.line);
      break;
    case SEMIC:
      printf("SEMIC in line %d", yylval.noVal.line);
      break;
    case DOT:
      printf("DOT in line %d", yylval.noVal.line);
      break;
    case PRIMBGN:
      printf("PRIMBGN in line %d", yylval.noVal.line);
      break;
    case PRIMEND:
      printf("PRIMEND in line %d", yylval.noVal.line);
      break;
    case INTLIT:
      printf("INTLIT in line %d, value = %d (0x%08X)",
             yylval.intVal.line, yylval.intVal.val, yylval.intVal.val);
      break;
    case FLTLIT:
      printf("FLTLIT in line %d, value = %e",
             yylval.floatVal.line, yylval.floatVal.val);
      break;
    case CHRLIT:
      if (yylval.charVal.val >= 0x20 && yylval.charVal.val <= 0x7E) {
        printf("CHRLIT in line %d, value = %c (0x%02X)",
               yylval.charVal.line, yylval.charVal.val, yylval.charVal.val);
      } else {
        printf("CHRLIT in line %d, value = %c (0x%02X)",
               yylval.charVal.line, '.', yylval.charVal.val);
      }
      break;
    case STRLIT:
      printf("STRLIT in line %d, value = '%s'",
             yylval.stringVal.line, yylval.stringVal.val);
      break;
    case BINSEL:
      printf("BINSEL in line %d, value = '%s'",
             yylval.stringVal.line, yylval.stringVal.val);
      break;
    case IDENT:
      printf("IDENT in line %d, value = '%s'",
             yylval.stringVal.line, yylval.stringVal.val);
      break;
    case KEYWORD:
      printf("KEYWORD in line %d, value = '%s'",
             yylval.stringVal.line, yylval.stringVal.val);
      break;
    case KEYWORDS:
      printf("KEYWORDS in line %d, value = '%s'",
             yylval.stringVal.line, yylval.stringVal.val);
      break;
    case COLONVAR:
      printf("COLONVAR in line %d, value = '%s'",
             yylval.stringVal.line, yylval.stringVal.val);
      break;
    default:
      /* this should never happen */
      sysError("unknown token %d in showToken", token);
      break;
  }
  printf("\n");
}
