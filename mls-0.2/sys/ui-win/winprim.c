/*
 * winprim.c -- windowing user interface primitives
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../common.h"
#include "../ui.h"


Bool compilationOK;


void sysError(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf("Error: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
  exit(1);
}


void sysWarning(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf("Warning: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
}


void compError(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf("Compiler error: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
  compilationOK = false;
}


void compWarning(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf("Compiler warning: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
}
