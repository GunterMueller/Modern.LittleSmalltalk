/*
 * objects.h -- objects with known structure
 */


#ifndef _OBJECTS_H_
#define _OBJECTS_H_


#define KEY_IN_LINK			0
#define VALUE_IN_LINK			1
#define NEXT_IN_LINK			2
#define SIZE_OF_LINK			3

#define HASHTABLE_IN_DICTIONARY		0
#define SIZE_OF_DICTIONARY		1

#define NAME_IN_BEHAVIOR		0
#define INSTTYPE_IN_BEHAVIOR		1
#define INSTSIZE_IN_BEHAVIOR		2
#define METHODS_IN_BEHAVIOR		3
#define SUPERCLASS_IN_BEHAVIOR		4
#define VARIABLES_IN_BEHAVIOR		5
#define SIZE_OF_BEHAVIOR		6

#define NAME_IN_CLASS			NAME_IN_BEHAVIOR
#define INSTTYPE_IN_CLASS		INSTTYPE_IN_BEHAVIOR
#define INSTSIZE_IN_CLASS		INSTSIZE_IN_BEHAVIOR
#define METHODS_IN_CLASS		METHODS_IN_BEHAVIOR
#define SUPERCLASS_IN_CLASS		SUPERCLASS_IN_BEHAVIOR
#define VARIABLES_IN_CLASS		VARIABLES_IN_BEHAVIOR
#define SIZE_OF_CLASS			6

#define NAME_IN_METACLASS		NAME_IN_BEHAVIOR
#define INSTTYPE_IN_METACLASS		INSTTYPE_IN_BEHAVIOR
#define INSTSIZE_IN_METACLASS		INSTSIZE_IN_BEHAVIOR
#define METHODS_IN_METACLASS		METHODS_IN_BEHAVIOR
#define SUPERCLASS_IN_METACLASS		SUPERCLASS_IN_BEHAVIOR
#define VARIABLES_IN_METACLASS		VARIABLES_IN_BEHAVIOR
#define SIZE_OF_METACLASS		6

#define CALLER_IN_CONTEXT		0
#define IP_IN_CONTEXT			1
#define STACK_IN_CONTEXT		2
#define SP_IN_CONTEXT			3
#define SIZE_OF_CONTEXT			4

#define CALLER_IN_METHODCONTEXT		CALLER_IN_CONTEXT
#define IP_IN_METHODCONTEXT		IP_IN_CONTEXT
#define STACK_IN_METHODCONTEXT		STACK_IN_CONTEXT
#define SP_IN_METHODCONTEXT		SP_IN_CONTEXT
#define METHOD_IN_METHODCONTEXT		4
#define RECEIVER_IN_METHODCONTEXT	5
#define ARGS_IN_METHODCONTEXT		6
#define TEMPS_IN_METHODCONTEXT		7
#define SIZE_OF_METHODCONTEXT		8

#define CALLER_IN_BLOCKCONTEXT		CALLER_IN_CONTEXT
#define IP_IN_BLOCKCONTEXT		IP_IN_CONTEXT
#define STACK_IN_BLOCKCONTEXT		STACK_IN_CONTEXT
#define SP_IN_BLOCKCONTEXT		SP_IN_CONTEXT
#define ARGCOUNT_IN_BLOCKCONTEXT	4
#define IPSTART_IN_BLOCKCONTEXT		5
#define HOME_IN_BLOCKCONTEXT		6
#define SIZE_OF_BLOCKCONTEXT		7

#define TEXT_IN_METHOD			0
#define SELECTOR_IN_METHOD		1
#define CODE_IN_METHOD			2
#define LITERALS_IN_METHOD		3
#define ARGSIZE_IN_METHOD		4
#define TEMPSIZE_IN_METHOD		5
#define STACKSIZE_IN_METHOD		6
#define CLASS_IN_METHOD			7
#define SIZE_OF_METHOD			8


ObjPtr newShortInteger(int value);
int getShortInteger(ObjPtr object);
ObjPtr newFloat(double value);
double getFloat(ObjPtr object);
ObjPtr newCharacter(Byte value);
Byte getCharacter(ObjPtr object);
ObjPtr newString(char *string);
ObjPtr newSymbol(char *string);
ObjPtr lookupGlobal(char *string);


#endif /* _OBJECTS_H_ */
