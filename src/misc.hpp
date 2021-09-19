#ifndef MISC_H
#define MISC_H

// doesn't return
void fatal_error(const char *msg);

// fatal_error() will call this before it does anything else
extern void (*onerrorexit)(void);

#endif  // MISC_H
