/*
 * objects.c -- objects with known structure
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "utils.h"
#include "machine.h"
#include "objects.h"
#include "memory.h"


ObjPtr newShortInteger(int value) {
  return value | IS_SHORTINT;
}


int getShortInteger(ObjPtr object) {
  return object & IS_NEGATIVE ?
         object : object & ~IS_SHORTINT;
}


ObjPtr newFloat(double value) {
  ObjPtr object;

  object = createObject(machine.Float, sizeof(double), false, false);
  *(double *)body(object) = value;
  return object;
}


double getFloat(ObjPtr object) {
  return *(double *)body(object);
}


ObjPtr newCharacter(Byte value) {
  return machine.character[value];
}


Byte getCharacter(ObjPtr object) {
  return *(Byte *)body(object);
}


ObjPtr newString(char *string) {
  int n;
  ObjPtr object;

  n = strlen(string);
  object = createObject(machine.String, n, false, false);
  memcpy(body(object), string, n);
  return object;
}


static ObjPtr lookupSymbol(char *string, Bool createNew) {
  int n;
  int h;
  ObjPtr hashTable;
  int size, bucket;
  ObjPtr link;
  ObjPtr symbol;

  n = strlen(string);
  h = hash(string, n);
  hashTable = getPtr(machine.Smalltalk, HASHTABLE_IN_DICTIONARY);
  size = getSize(hashTable);
  bucket = h % size;
  link = getPtr(hashTable, bucket);
  while (link != machine.nil) {
    symbol = getPtr(link, KEY_IN_LINK);
    if (h == getHash(symbol) && n == getSize(symbol)) {
      /* hash and size are equal - must check the characters */
      if (memcmp(string, body(symbol), n) == 0) {
        /* symbol found, return its link */
        return link;
      }
    }
    link = getPtr(link, NEXT_IN_LINK);
  }
  /* symbol not found */
  if (!createNew) {
    /* creation of a new symbol is not desired; indicate failure */
    return machine.nil;
  }
  /* here it is requested to create a new symbol */
  /* ATTENTION: the order of the following events is crucial! */
  link = createObject(machine.Link, SIZE_OF_LINK, true, false);
  /* ATTENTION: local pointers (except link) are now invalid! */
  hashTable = getPtr(machine.Smalltalk, HASHTABLE_IN_DICTIONARY);
  setPtr(link, NEXT_IN_LINK, getPtr(hashTable, bucket));
  setPtr(hashTable, bucket, link);
  symbol = createObject(machine.Symbol, n, false, false);
  /* ATTENTION: local pointers (except symbol) are now invalid! */
  setHash(symbol, h);
  memcpy(body(symbol), string, n);
  hashTable = getPtr(machine.Smalltalk, HASHTABLE_IN_DICTIONARY);
  link = getPtr(hashTable, bucket);
  setPtr(link, KEY_IN_LINK, symbol);
  /* finally return the new link */
  return link;
}


ObjPtr newSymbol(char *string) {
  ObjPtr link;

  link = lookupSymbol(string, true);
  return getPtr(link, KEY_IN_LINK);
}


ObjPtr lookupGlobal(char *string) {
  return lookupSymbol(string, false);
}
