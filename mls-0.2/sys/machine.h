/*
 * machine.h -- virtual machine
 */


#ifndef _MACHINE_H_
#define _MACHINE_H_


#define SIGNATURE_1	0x37A2F90B
#define SIGNATURE_2	0x1E84C56D


typedef struct {
  /* image file signature */
  Word signature_1;		/* must be SIGNATURE_1 */
  Word signature_2;		/* must be SIGNATURE_2 */
  /* image file version number */
  int majorVersion;		/* same as main program's major version */
  int minorVersion;		/* main's minor version, is not checked */
  /* image file structure */
  long memoryStart;		/* byte offset of object memory in file */
  long memorySize;		/* total size of object memory in bytes */
  /* known objects */
  ObjPtr nil;			/* the single instance of UndefinedObject */
  ObjPtr false;			/* the single instance of False */
  ObjPtr true;			/* the single instance of True */
  ObjPtr character[256];	/* the 256 instances of Character */
  ObjPtr Smalltalk;		/* the single instance of SystemDictionary */
  /* known classes */
  ObjPtr ShortInteger;		/* class object of class ShortInteger */
  ObjPtr Float;			/* class object of class Float */
  ObjPtr Character;		/* class object of class Character */
  ObjPtr String;		/* class object of class String */
  ObjPtr Symbol;		/* class object of class Symbol */
  ObjPtr Link;			/* class object of class Link */
  ObjPtr Method;		/* class object of class Method */
  ObjPtr Array;			/* class object of class Array */
  ObjPtr WordArray;		/* class object of class WordArray */
  ObjPtr MethodContext;		/* class object of class MethodContext */
  ObjPtr BlockContext;		/* class object of class BlockContext */
  ObjPtr Metaclass;		/* class object of class Metaclass */
  /* machine registers which hold objects */
  ObjPtr currentActiveContext;	/* anchor of all current... objects */
  ObjPtr currentHomeContext;	/* currently used home context */
  ObjPtr currentMethod;		/* currently used method */
  ObjPtr currentReceiver;	/* currently used receiver */
  ObjPtr currentArgs;		/* currently used argument array */
  ObjPtr currentTemps;		/* currently used temporary array */
  ObjPtr currentStack;		/* currently used stack array */
  ObjPtr currentCode;		/* currently used code array */
  ObjPtr currentLiterals;	/* currently used literal frame */
  ObjPtr newMethod;		/* new method to be executed */
  ObjPtr newContext;		/* new context to be executed */
  /* machine registers which hold non-objects */
  Word ip;			/* instruction pointer into code */
  Word sp;			/* stack pointer into stack */
  /* compiler related objects */
  ObjPtr compilerMethod;
  ObjPtr compilerLiteral;
} Machine;


#define OP_NOP		0x00	/* no operation: -,- */
#define OP_PUSHSELF	0x01	/* push receiver: -,- */
#define OP_PUSHNIL	0x02	/* push nil: -,- */
#define OP_PUSHFALSE	0x03	/* push false: -,- */
#define OP_PUSHTRUE	0x04	/* push true: -,- */
#define OP_DUP		0x05	/* duplicate top of stack: -,- */
#define OP_DROP		0x06	/* drop top of stack: -,- */
#define OP_RETMSG	0x07	/* return top of stack from message: -,- */
#define OP_RETBLK	0x08	/* return top of stack from block: -,- */
#define OP_PUSHCONST	0x09	/* push constant: -,constnum */
#define OP_PUSHGLOB	0x0A	/* push global: -,globalnum */
#define OP_STOREGLOB	0x0B	/* store global: -,globalnum */
#define OP_PUSHINST	0x0C	/* push instance: instnum,- */
#define OP_STOREINST	0x0D	/* store instance: instnum,- */
#define OP_PUSHARG	0x0E	/* push argument: argnum,- */
#define OP_PUSHTEMP	0x0F	/* push temporary: tempnum,- */
#define OP_STORETEMP	0x10	/* store temporary: tempnum,- */
#define OP_PUSHBLK	0x11	/* push block: numargs,stacksize */
#define OP_SEND		0x12	/* send message: numargs,selector */
#define OP_SENDSUPER	0x13	/* send message to super: numargs,selector */
#define OP_PRIM		0x14	/* call primitive: numargs,primnum */
#define OP_JUMP		0x15	/* jump: -,target */


extern Machine machine;		/* an instance of the virtual machine */
extern Bool debugMachine;	/* operate VM in debug mode if set */
extern Bool runMachine;		/* while true, run the machine */


void showString(ObjPtr stringObj);
void push(ObjPtr object);
ObjPtr pop(void);
void activateContext(ObjPtr context);
void run(void);


#endif /* _MACHINE_H_ */
