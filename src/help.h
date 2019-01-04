#ifndef HELP_H
#define HELP_H

#include <curses.h>

struct Help {
	char *key;
	char *desc;
};

extern struct Help help[];

// screen must be erased after calling, but not before
void help_show(WINDOW *win);

#endif  // HELP_H
