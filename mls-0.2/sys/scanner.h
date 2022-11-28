/*
 * scanner.h -- MLS scanner interface
 */


#ifndef _SCANNER_H_
#define _SCANNER_H_


typedef struct {
  int line;
} NoVal;

typedef struct {
  int line;
  int val;
} IntVal;

typedef struct {
  int line;
  double val;
} FloatVal;

typedef struct {
  int line;
  char val;
} CharVal;

typedef struct {
  int line;
  char *val;
} StringVal;


void initScanner(char *source);
int yylex(void);
void showToken(int token);


#endif /* _SCANNER_H_ */
