#ifndef HELP_H
#define HELP_H

#include <curses.h>

struct HelpKey {
	char *key;
	char *desc;
};

extern struct HelpKey help_keys[];

// screen must be erased after calling, but not before
void help_show(WINDOW *win, char *argv0);

#endif  // HELP_H
