/*
 * compiler.h -- MLS compiler
 */


#ifndef _COMPILER_H_
#define _COMPILER_H_


extern Bool debugSource;	/* show source text if set */
extern Bool debugTokens;	/* show token stream if set */
extern Bool debugTree;		/* show syntax tree if set */
extern Bool debugVars;		/* show variable info if set */
extern Bool debugCode;		/* show generated code if set */


Bool compile(char *text, ObjPtr class, Bool valueNeeded);


#endif /* _COMPILER_H_ */
