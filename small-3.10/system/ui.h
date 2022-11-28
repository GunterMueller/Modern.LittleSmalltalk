/*
 * ui.h -- user interface
 */

extern noreturn sysError(STR X STR);
extern noreturn sysWarn(STR X STR);
extern noreturn compilError(STR X STR X STR);
extern noreturn compilWarn(STR X STR X STR);
extern noreturn dspMethod(STR X STR);
extern noreturn givepause(NOARGS);
extern object sysPrimitive(INT X OBJP);
