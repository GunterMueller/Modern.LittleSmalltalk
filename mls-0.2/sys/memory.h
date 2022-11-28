/*
 * memory.h -- object memory
 */


#ifndef _MEMORY_H_
#define _MEMORY_H_


/* two bits of the object pointer are used in coding short integers */

#define IS_SHORTINT		OBJPTR_MSB
#define IS_NEGATIVE		OBJPTR_NSB


extern Bool debugMemory;	/* debug flag, give statistics if set */
extern Bool enableGC;		/* enables garbage collections if set */


ObjPtr createObject(ObjPtr class, int size,
                    Bool hasPtrs, Bool hasWords);

ObjPtr getClass(ObjPtr object);
void setClass(ObjPtr object, ObjPtr class);

int getHash(ObjPtr object);
void setHash(ObjPtr object, int hash);

int getSize(ObjPtr object);

Bool hasPtrs(ObjPtr object);
Bool hasWords(ObjPtr object);
Bool hasBytes(ObjPtr object);

void *body(ObjPtr object);

ObjPtr getPtr(ObjPtr object, int index);
Word getWord(ObjPtr object, int index);
Byte getByte(ObjPtr object, int index);

void setPtr(ObjPtr object, int index, ObjPtr value);
void setWord(ObjPtr object, int index, Word value);
void setByte(ObjPtr object, int index, Byte value);

void initMemory(char *imageFileName);
void exitMemory(char *imageFileName);


#endif /* _MEMORY_H_ */
