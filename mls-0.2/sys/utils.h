/*
 * utils.h -- utility functions
 */


#ifndef _UTILS_H_
#define _UTILS_H_


void *allocate(unsigned int size);
void release(void *p);
int hash(char *s, int n);


#endif /* _UTILS_H_ */
