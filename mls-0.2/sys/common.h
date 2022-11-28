/*
 * common.h -- common definitions
 */


#ifndef _COMMON_H_
#define _COMMON_H_


/* version number */

#define MAJOR_VNUM	0		/* MLS major version number */
#define MINOR_VNUM	2		/* MLS minor version number */


/* default image name */

#define DFLT_IMG_NAME	"mls.img"


/* some sizes */

#define K		1024
#define M		(K * K)

#define ALIGN		(1 << 2)	/* architecture dependant alignment */
#define ALIGN_MASK	(ALIGN - 1)	/* alignment mask */

#define SEMI_SIZE	(300 * K)	/* size of a single semispace */
#define MEMORY_SIZE	(2 * SEMI_SIZE)	/* total size of object memory */

#define LINE_SIZE	200		/* used for line buffer */
#define METHOD_SIZE	20000		/* used for method buffer */


/* boolean type */

typedef enum { false, true } Bool;


/* unsigned types, address type, and object pointer type */

typedef unsigned char Byte;
typedef unsigned int Word;
typedef unsigned long Address;

typedef Address ObjPtr;


/* most, next, and third significant bit of words */

#define WORD_MSB	(((Word) 1) << (8 * sizeof(Word) - 1))
#define WORD_NSB	(((Word) 1) << (8 * sizeof(Word) - 2))
#define WORD_TSB	(((Word) 1) << (8 * sizeof(Word) - 3))


/* most and next significant bit of object pointers */

#define OBJPTR_MSB	(((ObjPtr) 1) << (8 * sizeof(ObjPtr) - 1))
#define OBJPTR_NSB	(((ObjPtr) 1) << (8 * sizeof(ObjPtr) - 2))


#endif /* _COMMON_H_ */
