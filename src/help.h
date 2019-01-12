#ifndef HELP_H
#define HELP_H

#include <curses.h>

struct HelpKey {
	const char *key;
	const char *desc;
};

extern const struct HelpKey help_keys[];

// screen must be erased after calling, but not before
void help_show(WINDOW *win, const char *argv0);

#endif  // HELP_H
