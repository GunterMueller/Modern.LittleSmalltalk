/*
 * parser.h -- MLS parser interface
 */


#ifndef _PARSER_H_
#define _PARSER_H_


extern Node *method;


int yyparse(void);
void yyerror(char *msg);
char *concat(char *s1, char *s2);


#endif /* _PARSER_H_ */
