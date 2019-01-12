#ifndef MISC_H
#define MISC_H

void fatal_error(char *msg);

// fatal_error() will call this before it does anything else
extern void (*onerrorexit)(void);

// all c projects seem to have something like this :D
#define min(a, b) ((a)<(b) ? (a) : (b))
#define max(a, b) ((a)>(b) ? (a) : (b))

#endif  // MISC_H
