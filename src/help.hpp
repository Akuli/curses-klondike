#ifndef HELP_H
#define HELP_H

#include <cursesw.h>
#include <string>
#include <vector>

struct HelpKey {
	std::string key;
	std::string desc;
};

// screen must be erased after calling, but not before
void help_show(WINDOW *win, std::vector<HelpKey> hkeys, const char *argv0, bool color);

#endif  // HELP_H
