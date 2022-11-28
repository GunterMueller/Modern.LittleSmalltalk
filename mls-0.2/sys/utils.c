/*
 * utils.c -- utility functions
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "utils.h"
#include "ui.h"


void *allocate(unsigned int size) {
  void *p;

  p = malloc(size);
  if (p == NULL) {
    sysError("out of memory");
  }
  return p;
}


void release(void *p) {
  if (p == NULL) {
    sysError("NULL pointer detected in release");
  }
  free(p);
}


int hash(char *s, int n) {
  unsigned int h, g;

  h = 0;
  while (n--) {
    h = (h << 4) + *s++;
    g = h & 0xF0000000;
    if (g != 0) {
      h ^= g >> 24;
      h ^= g;
    }
  }
  return h & ((1 << 30) - 1);
}
