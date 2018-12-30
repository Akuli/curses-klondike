#ifndef MISC_H
#define MISC_H

void fatal_error(char *msg);

// fatal_error() will call this before it does anything else
extern void (*onerrorexit)(void);

#endif  // MISC_H
