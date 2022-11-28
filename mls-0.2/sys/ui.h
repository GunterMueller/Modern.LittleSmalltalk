/*
 * ui.h -- user interface
 */


#ifndef _UI_H_
#define _UI_H_


extern Bool compilationOK;


void sysError(char *fmt, ...);
void sysWarning(char *fmt, ...);
void compError(char *fmt, ...);
void compWarning(char *fmt, ...);


#endif /* _UI_H_ */
